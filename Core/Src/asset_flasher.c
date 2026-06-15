#include "asset_flasher.h"

#include "main.h"
#include "w25q64.h"
#include "asset_manifest.h"
#include "asset_flash_layout.h"

#include <string.h>

extern UART_HandleTypeDef huart2;

#define FLASHER_KNOCK            "ADHTPROG"
#define FLASHER_KNOCK_LEN        8U
#define FLASHER_HEADER_TAIL_LEN  8U   /* data_length(4) + crc32(4) */
#define FLASHER_IO_TIMEOUT_MS    5000U

#define FLASHER_REPLY_READY      'R'
#define FLASHER_REPLY_CHUNK_OK   'K'
#define FLASHER_REPLY_ERROR      'E'
#define FLASHER_REPLY_DONE_OK    'D'
#define FLASHER_REPLY_DONE_FAIL  'F'

static uint8_t pageBuffer[ASSET_FLASH_PAGE_SIZE];

static uint32_t ReadU32Le(const uint8_t *d)
{
  return ((uint32_t)d[0]) | ((uint32_t)d[1] << 8) |
         ((uint32_t)d[2] << 16) | ((uint32_t)d[3] << 24);
}

static void WriteU32Le(uint8_t *d, uint32_t v)
{
  d[0] = (uint8_t)v;
  d[1] = (uint8_t)(v >> 8);
  d[2] = (uint8_t)(v >> 16);
  d[3] = (uint8_t)(v >> 24);
}

static void Reply(uint8_t code)
{
  (void)HAL_UART_Transmit(&huart2, &code, 1U, FLASHER_IO_TIMEOUT_MS);
}

static bool RxExact(uint8_t *buf, uint16_t len, uint32_t timeoutMs)
{
  return HAL_UART_Receive(&huart2, buf, len, timeoutMs) == HAL_OK;
}

/* Erase the data sectors plus the manifest sector. */
static bool EraseForLength(uint32_t dataLength)
{
  uint32_t sectors = asset_flash_sectors_needed(dataLength, ASSET_FLASH_SECTOR_SIZE);
  uint32_t i;

  for (i = 0U; i < sectors; i++)
  {
    if (!W25Q64_EraseSector(i * ASSET_FLASH_SECTOR_SIZE))
    {
      return false;
    }
  }

  return W25Q64_EraseSector(W25Q64_ASSET_MANIFEST_OFFSET);
}

static bool WriteManifest(uint32_t dataLength, uint32_t crc)
{
  uint8_t manifest[ASSET_MANIFEST_SIZE];

  memset(manifest, 0, sizeof(manifest));
  WriteU32Le(&manifest[0], ASSET_MANIFEST_MAGIC);
  WriteU32Le(&manifest[4], ASSET_MANIFEST_VERSION);
  WriteU32Le(&manifest[8], dataLength);
  WriteU32Le(&manifest[12], crc);

  return W25Q64_Write(W25Q64_ASSET_MANIFEST_OFFSET, manifest, sizeof(manifest));
}

/* Stream the data body: program it page-by-page and accumulate its CRC.
 * Returns true and the running CRC if every chunk was received and programmed. */
static bool ReceiveAndProgram(uint32_t dataLength, uint32_t *outCrc)
{
  uint32_t offset = 0U;
  uint32_t crc = ASSET_CRC32_INIT;

  while (offset < dataLength)
  {
    uint32_t chunk = dataLength - offset;
    if (chunk > sizeof(pageBuffer))
    {
      chunk = sizeof(pageBuffer);
    }

    if (!RxExact(pageBuffer, (uint16_t)chunk, FLASHER_IO_TIMEOUT_MS) ||
        !W25Q64_Write(offset, pageBuffer, chunk))
    {
      Reply(FLASHER_REPLY_ERROR);
      return false;
    }

    crc = asset_crc32_update(crc, pageBuffer, chunk);
    offset += chunk;
    Reply(FLASHER_REPLY_CHUNK_OK);
  }

  *outCrc = asset_crc32_final(crc);
  return true;
}

bool AssetFlasher_RunIfRequested(uint32_t knockTimeoutMs)
{
  uint8_t knock[FLASHER_KNOCK_LEN];
  uint8_t tail[FLASHER_HEADER_TAIL_LEN];
  uint32_t dataLength;
  uint32_t expectedCrc;
  uint32_t actualCrc;

  /* Wait briefly for the host knock; absence means "boot the app". */
  if (!RxExact(knock, sizeof(knock), knockTimeoutMs) ||
      (memcmp(knock, FLASHER_KNOCK, FLASHER_KNOCK_LEN) != 0))
  {
    return false;
  }

  /* From here a session is committed; always returns true. */
  if (!RxExact(tail, sizeof(tail), FLASHER_IO_TIMEOUT_MS))
  {
    Reply(FLASHER_REPLY_ERROR);
    return true;
  }

  dataLength = ReadU32Le(&tail[0]);
  expectedCrc = ReadU32Le(&tail[4]);

  if (!W25Q64_IsReady() ||
      (dataLength == 0U) || (dataLength > W25Q64_ASSET_MANIFEST_OFFSET) ||
      !EraseForLength(dataLength))
  {
    Reply(FLASHER_REPLY_ERROR);
    return true;
  }

  Reply(FLASHER_REPLY_READY);

  if (!ReceiveAndProgram(dataLength, &actualCrc))
  {
    return true; /* ReceiveAndProgram already replied 'E' */
  }

  /* Reject a corrupted transfer before committing the manifest. */
  if ((actualCrc != expectedCrc) ||
      !WriteManifest(dataLength, expectedCrc) ||
      !W25Q64_ValidateAssetPackage())
  {
    Reply(FLASHER_REPLY_DONE_FAIL);
    return true;
  }

  Reply(FLASHER_REPLY_DONE_OK);
  return true;
}
