#include "w25q64.h"

#include "main.h"
#include "asset_manifest.h"
#include "asset_flash_layout.h"
#include "flash_selftest_pattern.h"

#include <stddef.h>
#include <string.h>

#define W25Q64_CMD_READ          0x03U
#define W25Q64_CMD_JEDEC_ID      0x9FU
#define W25Q64_CMD_DEVICE_ID     0x90U
#define W25Q64_CMD_RELEASE_POWER_DOWN 0xABU
#define W25Q64_CMD_READ_STATUS2  0x35U
#define W25Q64_CMD_READ_STATUS3  0x15U
#define W25Q64_CMD_READ_SFDP     0x5AU
#define W25Q64_CMD_ENABLE_RESET  0x66U
#define W25Q64_CMD_RESET_DEVICE  0x99U
#define W25Q64_CMD_WRITE_ENABLE  0x06U
#define W25Q64_CMD_READ_STATUS1  0x05U
#define W25Q64_CMD_PAGE_PROGRAM  0x02U
#define W25Q64_CMD_SECTOR_ERASE  0x20U  /* 4 KB sector erase */
#define W25Q64_CMD_CHIP_ERASE    0xC7U
#define W25Q64_STATUS_BUSY       0x01U  /* WIP bit in status register 1 */
#define W25Q64_MANUFACTURER_ID   0xEFU
#define W25Q64_MEMORY_TYPE       0x40U
#define W25Q64_CAPACITY_ID       0x17U  /* W25Q64 = 8 MB (0x18 would be the 16 MB W25Q128) */
#define W25Q64_TIMEOUT_MS        100U
#define W25Q64_ERASE_TIMEOUT_MS  60000U /* chip erase can take tens of seconds */
#define W25Q64_CRC_BUFFER_SIZE   256U

extern SPI_HandleTypeDef hspi3;

static volatile bool dmaReadActive;
static bool flashReady;
static uint8_t jedecId[3];

/* The post-link asset packager replaces this exact section in the programming
 * ELF/HEX. If an unpatched ELF is loaded, data_length remains zero and boot
 * safely enters asset recovery instead of rendering from incompatible offsets. */
const asset_manifest_t g_expected_asset_package
    __attribute__((used, section("AssetExpectedSection"))) =
{
  ASSET_MANIFEST_MAGIC,
  ASSET_MANIFEST_VERSION,
  0U,
  0U,
  {0U}
};

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

static bool W25Q64_SetSpiMode(uint8_t spiMode)
{
  uint32_t modeBits;

  if (spiMode == 0U)
  {
    modeBits = SPI_POLARITY_LOW | SPI_PHASE_1EDGE;
    hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  }
  else if (spiMode == 3U)
  {
    modeBits = SPI_POLARITY_HIGH | SPI_PHASE_2EDGE;
    hspi3.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi3.Init.CLKPhase = SPI_PHASE_2EDGE;
  }
  else
  {
    return false;
  }

  W25Q64_WaitForDmaRead();
  W25Q64_Deselect();
  (void)HAL_SPI_Abort(&hspi3);
  __HAL_SPI_DISABLE(&hspi3);
  MODIFY_REG(hspi3.Instance->CR1, SPI_CR1_CPOL | SPI_CR1_CPHA, modeBits);
  __HAL_SPI_ENABLE(&hspi3);
  HAL_Delay(1U);
  return true;
}

static bool W25Q64_CommandRead(const uint8_t *command, uint16_t commandLength,
                               uint8_t *data, uint16_t dataLength)
{
  HAL_StatusTypeDef status;

  if ((command == NULL) || (commandLength == 0U) ||
      ((dataLength > 0U) && (data == NULL)))
  {
    return false;
  }

  W25Q64_Select();
  status = HAL_SPI_Transmit(&hspi3, (uint8_t *)command, commandLength,
                            W25Q64_TIMEOUT_MS);
  if ((status == HAL_OK) && (dataLength > 0U))
  {
    status = HAL_SPI_Receive(&hspi3, data, dataLength, W25Q64_TIMEOUT_MS);
  }
  W25Q64_Deselect();
  return status == HAL_OK;
}

static bool W25Q64_SendCommand(uint8_t command)
{
  return W25Q64_CommandRead(&command, 1U, NULL, 0U);
}

static bool W25Q64_ResetDevice(void)
{
  W25Q64_Deselect();
  if (!W25Q64_SendCommand(W25Q64_CMD_ENABLE_RESET) ||
      !W25Q64_SendCommand(W25Q64_CMD_RESET_DEVICE))
  {
    return false;
  }
  HAL_Delay(1U);
  return true;
}

static bool W25Q64_ReadJedecId(uint8_t id[3])
{
  uint8_t command = W25Q64_CMD_JEDEC_ID;
  return W25Q64_CommandRead(&command, 1U, id, 3U);
}

static bool W25Q64_ReleasePowerDown(uint8_t *deviceId)
{
  uint8_t command[4] = {W25Q64_CMD_RELEASE_POWER_DOWN, 0U, 0U, 0U};

  if (!W25Q64_CommandRead(command, sizeof(command), deviceId, 1U))
  {
    return false;
  }
  HAL_Delay(1U);
  return true;
}

static bool W25Q64_IdIsExpected(const uint8_t id[3])
{
  return (id[0] == W25Q64_MANUFACTURER_ID) &&
         (id[1] == W25Q64_MEMORY_TYPE) &&
         (id[2] == W25Q64_CAPACITY_ID);
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
  uint8_t releaseId = 0U;

  dmaReadActive = false;
  flashReady = false;
  jedecId[0] = 0U;
  jedecId[1] = 0U;
  jedecId[2] = 0U;
  W25Q64_Deselect();

  if (W25Q64_SetSpiMode(0U) &&
      W25Q64_ResetDevice() &&
      W25Q64_ReleasePowerDown(&releaseId) &&
      W25Q64_ReadJedecId(jedecId))
  {
    flashReady = W25Q64_IdIsExpected(jedecId);
  }
  return flashReady;
}

bool W25Q64_IsReady(void)
{
  return flashReady;
}

void W25Q64_GetJedecId(uint8_t outId[3])
{
  if (outId != NULL)
  {
    outId[0] = jedecId[0];
    outId[1] = jedecId[1];
    outId[2] = jedecId[2];
  }
}

bool W25Q64_ProbeStable(uint8_t sampleCount)
{
  uint8_t sample[3] = {0U};
  uint8_t releaseId = 0U;
  uint8_t index;

  flashReady = false;
  if ((sampleCount == 0U) || !W25Q64_SetSpiMode(0U) ||
      !W25Q64_ResetDevice() ||
      !W25Q64_ReleasePowerDown(&releaseId))
  {
    return false;
  }

  for (index = 0U; index < sampleCount; index++)
  {
    if (!W25Q64_ReadJedecId(sample) || !W25Q64_IdIsExpected(sample))
    {
      memcpy(jedecId, sample, sizeof(jedecId));
      return false;
    }
  }

  memcpy(jedecId, sample, sizeof(jedecId));
  flashReady = true;
  return true;
}

bool W25Q64_RunDiagnostic(uint8_t spiMode,
                          w25q64_diagnostic_report_t *report)
{
  uint8_t command;
  uint8_t commandWithAddress[5];
  uint8_t index;
  bool ok = true;

  if (report == NULL)
  {
    return false;
  }

  memset(report, 0, sizeof(*report));
  report->spi_mode = spiMode;
  if (!W25Q64_SetSpiMode(spiMode) || !W25Q64_ResetDevice() ||
      !W25Q64_ReleasePowerDown(&report->release_device_id))
  {
    ok = false;
  }

  for (index = 0U; index < W25Q64_DIAGNOSTIC_SAMPLE_COUNT; index++)
  {
    if (!W25Q64_ReadJedecId(report->jedec_samples[index]))
    {
      ok = false;
    }
    else if (W25Q64_IdIsExpected(report->jedec_samples[index]))
    {
      report->stable_count++;
    }
  }

  commandWithAddress[0] = W25Q64_CMD_DEVICE_ID;
  commandWithAddress[1] = 0U;
  commandWithAddress[2] = 0U;
  commandWithAddress[3] = 0U;
  if (!W25Q64_CommandRead(commandWithAddress, 4U,
                          report->manufacturer_device_id, 2U))
  {
    ok = false;
  }

  command = W25Q64_CMD_READ_STATUS1;
  if (!W25Q64_CommandRead(&command, 1U, &report->status_registers[0], 1U))
  {
    ok = false;
  }
  command = W25Q64_CMD_READ_STATUS2;
  if (!W25Q64_CommandRead(&command, 1U, &report->status_registers[1], 1U))
  {
    ok = false;
  }
  command = W25Q64_CMD_READ_STATUS3;
  if (!W25Q64_CommandRead(&command, 1U, &report->status_registers[2], 1U))
  {
    ok = false;
  }

  commandWithAddress[0] = W25Q64_CMD_READ_SFDP;
  commandWithAddress[1] = 0U;
  commandWithAddress[2] = 0U;
  commandWithAddress[3] = 0U;
  commandWithAddress[4] = 0U;
  if (!W25Q64_CommandRead(commandWithAddress, sizeof(commandWithAddress),
                          report->sfdp_signature,
                          sizeof(report->sfdp_signature)))
  {
    ok = false;
  }

  if (spiMode != 0U)
  {
    (void)W25Q64_SetSpiMode(0U);
  }
  return ok;
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
      !asset_manifest_header_ok(&manifest, W25Q64_ASSET_MANIFEST_OFFSET) ||
      !asset_manifest_header_ok(&g_expected_asset_package,
                                W25Q64_ASSET_MANIFEST_OFFSET) ||
      !asset_manifest_matches_expected(&manifest, &g_expected_asset_package))
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

bool W25Q64_SelfTest(uint32_t volumeBytes, w25q64_selftest_result_t *result)
{
  uint8_t buffer[256];
  uint32_t offset;
  uint32_t sectors;
  uint32_t i;

  if (result == NULL)
  {
    return false;
  }
  result->bytes_tested = 0U;
  result->errors = 0U;
  result->first_bad_offset = 0U;
  result->expected_byte = 0U;
  result->got_byte = 0U;
  result->ok = false;

  if (!flashReady || (volumeBytes == 0U) ||
      (volumeBytes > W25Q64_ASSET_MANIFEST_OFFSET))
  {
    return false;
  }

  /* Erase every 4 KB sector covering the test region. */
  sectors = asset_flash_sectors_needed(volumeBytes, ASSET_FLASH_SECTOR_SIZE);
  for (i = 0U; i < sectors; i++)
  {
    if (!W25Q64_EraseSector(i * ASSET_FLASH_SECTOR_SIZE))
    {
      return false;
    }
  }

  /* Write pass: page-aligned chunks of the deterministic pattern. */
  for (offset = 0U; offset < volumeBytes; offset += (uint32_t)sizeof(buffer))
  {
    uint32_t chunk = volumeBytes - offset;
    if (chunk > sizeof(buffer))
    {
      chunk = sizeof(buffer);
    }
    flash_selftest_pattern_fill(buffer, offset, chunk);
    if (!W25Q64_Write(offset, buffer, chunk))
    {
      return false;
    }
  }

  /* Read-back + verify pass against the same deterministic pattern. */
  for (offset = 0U; offset < volumeBytes; offset += (uint32_t)sizeof(buffer))
  {
    uint32_t chunk = volumeBytes - offset;
    if (chunk > sizeof(buffer))
    {
      chunk = sizeof(buffer);
    }
    if (!W25Q64_Read(offset, buffer, chunk))
    {
      return false;
    }
    for (i = 0U; i < chunk; i++)
    {
      uint8_t expected = flash_selftest_pattern_byte(offset + i);
      if (buffer[i] != expected)
      {
        if (result->errors == 0U)
        {
          result->first_bad_offset = offset + i;
          result->expected_byte = expected;
          result->got_byte = buffer[i];
        }
        result->errors++;
      }
    }
  }

  result->bytes_tested = volumeBytes;
  result->ok = (result->errors == 0U);
  return result->ok;
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
  uint32_t start = HAL_GetTick();

  while (dmaReadActive)
  {
    /* Safety net: if the DMA completion is never signalled (mis-wired IRQ,
     * unclocked flash, bus error) abort instead of hanging the UI forever. The
     * timeout is generous versus a real bitmap transfer at ~21 MHz. */
    if ((HAL_GetTick() - start) > W25Q64_TIMEOUT_MS)
    {
      (void)HAL_SPI_Abort(&hspi3);
      W25Q64_Deselect();
      dmaReadActive = false;
      break;
    }
  }
}

void W25Q64_DmaCompleteCallback(void)
{
  W25Q64_Deselect();
  dmaReadActive = false;
}

/* --- Programming path (UART asset flasher) ----------------------------- */

static bool W25Q64_WriteEnable(void)
{
  uint8_t cmd = W25Q64_CMD_WRITE_ENABLE;
  HAL_StatusTypeDef status;

  W25Q64_Select();
  status = HAL_SPI_Transmit(&hspi3, &cmd, 1U, W25Q64_TIMEOUT_MS);
  W25Q64_Deselect();
  return status == HAL_OK;
}

static bool W25Q64_WaitWhileBusy(uint32_t timeoutMs)
{
  uint8_t cmd = W25Q64_CMD_READ_STATUS1;
  uint8_t statusReg = W25Q64_STATUS_BUSY;
  uint32_t start = HAL_GetTick();

  do
  {
    W25Q64_Select();
    if ((HAL_SPI_Transmit(&hspi3, &cmd, 1U, W25Q64_TIMEOUT_MS) != HAL_OK) ||
        (HAL_SPI_Receive(&hspi3, &statusReg, 1U, W25Q64_TIMEOUT_MS) != HAL_OK))
    {
      W25Q64_Deselect();
      return false;
    }
    W25Q64_Deselect();

    if ((statusReg & W25Q64_STATUS_BUSY) == 0U)
    {
      return true;
    }
  } while ((HAL_GetTick() - start) < timeoutMs);

  return false;
}

bool W25Q64_EraseSector(uint32_t address)
{
  uint8_t header[4];
  HAL_StatusTypeDef status;

  address = W25Q64_NormalizeAddress(address);
  if (!flashReady || (address >= W25Q64_SIZE_BYTES) || !W25Q64_WriteEnable())
  {
    return false;
  }

  header[0] = W25Q64_CMD_SECTOR_ERASE;
  header[1] = (uint8_t)(address >> 16);
  header[2] = (uint8_t)(address >> 8);
  header[3] = (uint8_t)address;

  W25Q64_Select();
  status = HAL_SPI_Transmit(&hspi3, header, sizeof(header), W25Q64_TIMEOUT_MS);
  W25Q64_Deselect();
  if (status != HAL_OK)
  {
    return false;
  }

  return W25Q64_WaitWhileBusy(W25Q64_ERASE_TIMEOUT_MS);
}

bool W25Q64_EraseChip(void)
{
  uint8_t cmd = W25Q64_CMD_CHIP_ERASE;
  HAL_StatusTypeDef status;

  if (!flashReady || !W25Q64_WriteEnable())
  {
    return false;
  }

  W25Q64_Select();
  status = HAL_SPI_Transmit(&hspi3, &cmd, 1U, W25Q64_TIMEOUT_MS);
  W25Q64_Deselect();
  if (status != HAL_OK)
  {
    return false;
  }

  return W25Q64_WaitWhileBusy(W25Q64_ERASE_TIMEOUT_MS);
}

bool W25Q64_ProgramPage(uint32_t address, const uint8_t *data, uint32_t length)
{
  uint8_t header[4];
  HAL_StatusTypeDef status;

  address = W25Q64_NormalizeAddress(address);
  if (!flashReady || (data == NULL) || (length == 0U) ||
      (length > ASSET_FLASH_PAGE_SIZE) ||
      (address >= W25Q64_SIZE_BYTES) ||
      (length > (W25Q64_SIZE_BYTES - address)) ||
      /* must not cross a page boundary */
      (asset_flash_page_chunk(address, length, ASSET_FLASH_PAGE_SIZE) != length))
  {
    return false;
  }

  if (!W25Q64_WriteEnable())
  {
    return false;
  }

  header[0] = W25Q64_CMD_PAGE_PROGRAM;
  header[1] = (uint8_t)(address >> 16);
  header[2] = (uint8_t)(address >> 8);
  header[3] = (uint8_t)address;

  W25Q64_Select();
  status = HAL_SPI_Transmit(&hspi3, header, sizeof(header), W25Q64_TIMEOUT_MS);
  if (status == HAL_OK)
  {
    /* data is const; HAL takes a non-const buffer but does not modify it. */
    status = HAL_SPI_Transmit(&hspi3, (uint8_t *)data, (uint16_t)length, W25Q64_TIMEOUT_MS);
  }
  W25Q64_Deselect();
  if (status != HAL_OK)
  {
    return false;
  }

  return W25Q64_WaitWhileBusy(W25Q64_TIMEOUT_MS);
}

bool W25Q64_Write(uint32_t address, const uint8_t *data, uint32_t length)
{
  uint32_t offset = 0U;

  address = W25Q64_NormalizeAddress(address);
  if (!flashReady || (data == NULL) || (length == 0U) ||
      (address >= W25Q64_SIZE_BYTES) ||
      (length > (W25Q64_SIZE_BYTES - address)))
  {
    return false;
  }

  while (offset < length)
  {
    uint32_t chunk = asset_flash_page_chunk(address + offset, length - offset,
                                            ASSET_FLASH_PAGE_SIZE);
    if (!W25Q64_ProgramPage(address + offset, &data[offset], chunk))
    {
      return false;
    }
    offset += chunk;
  }

  return true;
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
