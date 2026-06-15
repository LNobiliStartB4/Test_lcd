#include "w25q64.h"

#include "main.h"
#include "asset_manifest.h"

#include <stddef.h>

#define W25Q64_CMD_READ          0x03U
#define W25Q64_CMD_JEDEC_ID      0x9FU
#define W25Q64_MANUFACTURER_ID   0xEFU
#define W25Q64_MEMORY_TYPE       0x40U
#define W25Q64_CAPACITY_ID       0x17U  /* W25Q64 = 8 MB (0x18 would be the 16 MB W25Q128) */
#define W25Q64_TIMEOUT_MS        100U
#define W25Q64_CRC_BUFFER_SIZE   256U

extern SPI_HandleTypeDef hspi3;

static volatile bool dmaReadActive;
static bool flashReady;

static uint32_t W25Q64_NormalizeAddress(uint32_t address)
{
  if (address >= W25Q64_VIRTUAL_BASE)
  {
    address -= W25Q64_VIRTUAL_BASE;
  }

  return address;
}

static void W25Q64_Select(void)
{
  HAL_GPIO_WritePin(ASSET_FLASH_CS_GPIO_Port, ASSET_FLASH_CS_Pin, GPIO_PIN_RESET);
}

static void W25Q64_Deselect(void)
{
  HAL_GPIO_WritePin(ASSET_FLASH_CS_GPIO_Port, ASSET_FLASH_CS_Pin, GPIO_PIN_SET);
}

static bool W25Q64_SendReadHeader(uint32_t address)
{
  uint8_t header[4];

  address = W25Q64_NormalizeAddress(address);
  if (address >= W25Q64_SIZE_BYTES)
  {
    return false;
  }

  header[0] = W25Q64_CMD_READ;
  header[1] = (uint8_t)(address >> 16);
  header[2] = (uint8_t)(address >> 8);
  header[3] = (uint8_t)address;

  W25Q64_Select();
  if (HAL_SPI_Transmit(&hspi3, header, sizeof(header), W25Q64_TIMEOUT_MS) != HAL_OK)
  {
    W25Q64_Deselect();
    return false;
  }

  return true;
}

bool W25Q64_Init(void)
{
  uint8_t command = W25Q64_CMD_JEDEC_ID;
  uint8_t id[3] = {0U};

  dmaReadActive = false;
  flashReady = false;
  W25Q64_Deselect();

  W25Q64_Select();
  if ((HAL_SPI_Transmit(&hspi3, &command, 1U, W25Q64_TIMEOUT_MS) == HAL_OK) &&
      (HAL_SPI_Receive(&hspi3, id, sizeof(id), W25Q64_TIMEOUT_MS) == HAL_OK))
  {
    flashReady = (id[0] == W25Q64_MANUFACTURER_ID) &&
                 (id[1] == W25Q64_MEMORY_TYPE) &&
                 (id[2] == W25Q64_CAPACITY_ID);
  }
  W25Q64_Deselect();
  return flashReady;
}

bool W25Q64_IsReady(void)
{
  return flashReady;
}

bool W25Q64_ValidateAssetPackage(void)
{
  uint8_t manifestRaw[ASSET_MANIFEST_SIZE];
  uint8_t data[W25Q64_CRC_BUFFER_SIZE];
  asset_manifest_t manifest;
  uint32_t offset = 0U;
  uint32_t crc = ASSET_CRC32_INIT;

  if (!flashReady ||
      !W25Q64_Read(W25Q64_ASSET_MANIFEST_OFFSET, manifestRaw, sizeof(manifestRaw)))
  {
    return false;
  }

  if (!asset_manifest_parse(manifestRaw, &manifest) ||
      !asset_manifest_header_ok(&manifest, W25Q64_ASSET_MANIFEST_OFFSET))
  {
    return false;
  }

  while (offset < manifest.data_length)
  {
    uint32_t chunkLength = manifest.data_length - offset;
    if (chunkLength > sizeof(data))
    {
      chunkLength = sizeof(data);
    }

    if (!W25Q64_Read(offset, data, chunkLength))
    {
      return false;
    }

    crc = asset_crc32_update(crc, data, chunkLength);
    offset += chunkLength;
  }

  return asset_crc32_final(crc) == manifest.expected_crc;
}

bool W25Q64_Read(uint32_t address, uint8_t *data, uint32_t length)
{
  uint32_t normalizedAddress = W25Q64_NormalizeAddress(address);

  if ((data == NULL) ||
      (length == 0U) ||
      (length > UINT16_MAX) ||
      (normalizedAddress >= W25Q64_SIZE_BYTES) ||
      (length > (W25Q64_SIZE_BYTES - normalizedAddress)))
  {
    return false;
  }

  W25Q64_WaitForDmaRead();
  if (!W25Q64_SendReadHeader(normalizedAddress))
  {
    return false;
  }

  HAL_StatusTypeDef status =
      HAL_SPI_Receive(&hspi3, data, (uint16_t)length, W25Q64_TIMEOUT_MS);
  W25Q64_Deselect();
  return status == HAL_OK;
}

bool W25Q64_StartDmaRead(uint32_t address, uint8_t *data, uint32_t length)
{
  uint32_t normalizedAddress = W25Q64_NormalizeAddress(address);

  if ((data == NULL) ||
      (length == 0U) ||
      (length > UINT16_MAX) ||
      (normalizedAddress >= W25Q64_SIZE_BYTES) ||
      (length > (W25Q64_SIZE_BYTES - normalizedAddress)))
  {
    return false;
  }

  W25Q64_WaitForDmaRead();
  if (!W25Q64_SendReadHeader(normalizedAddress))
  {
    return false;
  }

  dmaReadActive = true;
  if (HAL_SPI_Receive_DMA(&hspi3, data, (uint16_t)length) != HAL_OK)
  {
    dmaReadActive = false;
    W25Q64_Deselect();
    return false;
  }

  return true;
}

bool W25Q64_IsDmaReadActive(void)
{
  return dmaReadActive;
}

void W25Q64_WaitForDmaRead(void)
{
  while (dmaReadActive)
  {
  }
}

void W25Q64_DmaCompleteCallback(void)
{
  W25Q64_Deselect();
  dmaReadActive = false;
}

/* SPI3 is dedicated to the asset flash; SPI1 (display) defines only its own
 * HAL_SPI_TxCpltCallback, so these RX/error completions don't collide. */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI3)
  {
    W25Q64_DmaCompleteCallback();
  }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI3)
  {
    W25Q64_DmaCompleteCallback();
  }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI3)
  {
    W25Q64_DmaCompleteCallback();
  }
}
