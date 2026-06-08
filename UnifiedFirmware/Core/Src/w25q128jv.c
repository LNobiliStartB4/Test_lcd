#include "w25q128jv.h"

#include "main.h"

#define W25Q128JV_CMD_READ 0x03U
#define W25Q128JV_CMD_JEDEC_ID 0x9FU
#define W25Q128JV_MANUFACTURER_ID 0xEFU
#define W25Q128JV_MEMORY_TYPE 0x40U
#define W25Q128JV_CAPACITY_ID 0x18U
#define W25Q128JV_TIMEOUT_MS 100U
#define W25Q128JV_ASSET_MAGIC 0x41444854UL
#define W25Q128JV_ASSET_FORMAT_VERSION 1UL
#define W25Q128JV_ASSET_MANIFEST_SIZE 24U
#define W25Q128JV_CRC_BUFFER_SIZE 256U

extern SPI_HandleTypeDef hspi3;

static volatile bool dmaReadActive;
static bool flashReady;

static uint32_t W25Q128JV_ReadU32Le(const uint8_t *data)
{
  return ((uint32_t)data[0]) |
         ((uint32_t)data[1] << 8) |
         ((uint32_t)data[2] << 16) |
         ((uint32_t)data[3] << 24);
}

static uint32_t W25Q128JV_UpdateCrc32(uint32_t crc, const uint8_t *data, uint32_t length)
{
  uint32_t index;

  for (index = 0U; index < length; index++)
  {
    uint8_t bit;
    crc ^= data[index];
    for (bit = 0U; bit < 8U; bit++)
    {
      crc = ((crc & 1U) != 0U) ? ((crc >> 1) ^ 0xEDB88320UL) : (crc >> 1);
    }
  }

  return crc;
}

static uint32_t W25Q128JV_NormalizeAddress(uint32_t address)
{
  if (address >= W25Q128JV_VIRTUAL_BASE)
  {
    address -= W25Q128JV_VIRTUAL_BASE;
  }

  return address;
}

static void W25Q128JV_Select(void)
{
  HAL_GPIO_WritePin(ASSET_FLASH_CS_GPIO_Port, ASSET_FLASH_CS_Pin, GPIO_PIN_RESET);
}

static void W25Q128JV_Deselect(void)
{
  HAL_GPIO_WritePin(ASSET_FLASH_CS_GPIO_Port, ASSET_FLASH_CS_Pin, GPIO_PIN_SET);
}

static bool W25Q128JV_SendReadHeader(uint32_t address)
{
  uint8_t header[4];

  address = W25Q128JV_NormalizeAddress(address);
  if (address >= W25Q128JV_SIZE_BYTES)
  {
    return false;
  }

  header[0] = W25Q128JV_CMD_READ;
  header[1] = (uint8_t)(address >> 16);
  header[2] = (uint8_t)(address >> 8);
  header[3] = (uint8_t)address;

  W25Q128JV_Select();
  if (HAL_SPI_Transmit(&hspi3, header, sizeof(header), W25Q128JV_TIMEOUT_MS) != HAL_OK)
  {
    W25Q128JV_Deselect();
    return false;
  }

  return true;
}

bool W25Q128JV_Init(void)
{
  uint8_t command = W25Q128JV_CMD_JEDEC_ID;
  uint8_t id[3] = {0U};

  dmaReadActive = false;
  flashReady = false;
  W25Q128JV_Deselect();

  W25Q128JV_Select();
  if ((HAL_SPI_Transmit(&hspi3, &command, 1U, W25Q128JV_TIMEOUT_MS) == HAL_OK) &&
      (HAL_SPI_Receive(&hspi3, id, sizeof(id), W25Q128JV_TIMEOUT_MS) == HAL_OK))
  {
    flashReady = (id[0] == W25Q128JV_MANUFACTURER_ID) &&
                 (id[1] == W25Q128JV_MEMORY_TYPE) &&
                 (id[2] == W25Q128JV_CAPACITY_ID);
  }
  W25Q128JV_Deselect();
  return flashReady;
}

bool W25Q128JV_IsReady(void)
{
  return flashReady;
}

bool W25Q128JV_ValidateAssetPackage(void)
{
  uint8_t manifest[W25Q128JV_ASSET_MANIFEST_SIZE];
  uint8_t data[W25Q128JV_CRC_BUFFER_SIZE];
  uint32_t dataLength;
  uint32_t expectedCrc;
  uint32_t offset = 0U;
  uint32_t crc = 0xFFFFFFFFUL;

  if (!flashReady ||
      !W25Q128JV_Read(W25Q128JV_ASSET_MANIFEST_OFFSET,
                      manifest,
                      sizeof(manifest)))
  {
    return false;
  }

  if ((W25Q128JV_ReadU32Le(&manifest[0]) != W25Q128JV_ASSET_MAGIC) ||
      (W25Q128JV_ReadU32Le(&manifest[4]) != W25Q128JV_ASSET_FORMAT_VERSION))
  {
    return false;
  }

  dataLength = W25Q128JV_ReadU32Le(&manifest[8]);
  expectedCrc = W25Q128JV_ReadU32Le(&manifest[12]);
  if ((dataLength == 0U) || (dataLength > W25Q128JV_ASSET_MANIFEST_OFFSET))
  {
    return false;
  }

  while (offset < dataLength)
  {
    uint32_t chunkLength = dataLength - offset;
    if (chunkLength > sizeof(data))
    {
      chunkLength = sizeof(data);
    }

    if (!W25Q128JV_Read(offset, data, chunkLength))
    {
      return false;
    }

    crc = W25Q128JV_UpdateCrc32(crc, data, chunkLength);
    offset += chunkLength;
  }

  return (crc ^ 0xFFFFFFFFUL) == expectedCrc;
}

bool W25Q128JV_Read(uint32_t address, uint8_t *data, uint32_t length)
{
  uint32_t normalizedAddress = W25Q128JV_NormalizeAddress(address);

  if ((data == NULL) ||
      (length == 0U) ||
      (length > UINT16_MAX) ||
      (normalizedAddress >= W25Q128JV_SIZE_BYTES) ||
      (length > (W25Q128JV_SIZE_BYTES - normalizedAddress)))
  {
    return false;
  }

  W25Q128JV_WaitForDmaRead();
  if (!W25Q128JV_SendReadHeader(normalizedAddress))
  {
    return false;
  }

  HAL_StatusTypeDef status =
      HAL_SPI_Receive(&hspi3, data, (uint16_t)length, W25Q128JV_TIMEOUT_MS);
  W25Q128JV_Deselect();
  return status == HAL_OK;
}

bool W25Q128JV_StartDmaRead(uint32_t address, uint8_t *data, uint32_t length)
{
  uint32_t normalizedAddress = W25Q128JV_NormalizeAddress(address);

  if ((data == NULL) ||
      (length == 0U) ||
      (length > UINT16_MAX) ||
      (normalizedAddress >= W25Q128JV_SIZE_BYTES) ||
      (length > (W25Q128JV_SIZE_BYTES - normalizedAddress)))
  {
    return false;
  }

  W25Q128JV_WaitForDmaRead();
  if (!W25Q128JV_SendReadHeader(normalizedAddress))
  {
    return false;
  }

  dmaReadActive = true;
  if (HAL_SPI_Receive_DMA(&hspi3, data, (uint16_t)length) != HAL_OK)
  {
    dmaReadActive = false;
    W25Q128JV_Deselect();
    return false;
  }

  return true;
}

bool W25Q128JV_IsDmaReadActive(void)
{
  return dmaReadActive;
}

void W25Q128JV_WaitForDmaRead(void)
{
  while (dmaReadActive)
  {
  }
}

void W25Q128JV_DmaCompleteCallback(void)
{
  W25Q128JV_Deselect();
  dmaReadActive = false;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI3)
  {
    W25Q128JV_DmaCompleteCallback();
  }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI3)
  {
    W25Q128JV_DmaCompleteCallback();
  }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI3)
  {
    W25Q128JV_DmaCompleteCallback();
  }
}
