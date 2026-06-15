#include "asset_flasher.h"

#include "main.h"
#include "w25q64.h"
#include "asset_manifest.h"
#include "asset_flash_layout.h"

#include <string.h>

extern UART_HandleTypeDef huart2;

#define FLASHER_KNOCK             "ADHTPROG"
#define FLASHER_KNOCK_LEN         8U
#define FLASHER_HEADER_MAGIC      "ADHTHEAD"
#define FLASHER_HEADER_MAGIC_LEN  8U
#define FLASHER_HEADER_TAIL_LEN   8U
#define FLASHER_IO_TIMEOUT_MS     5000U
#define FLASHER_RESET_DELAY_MS    100U

/* Bounded knock windows: the board must always proceed to boot the UI. A short
 * window lets a host attach right after reset; holding USER asks for a longer,
 * deliberate flashing window. Never wait forever (that bricks to a black screen
 * because the display is initialised only after this returns). */
#define FLASHER_KNOCK_WINDOW_MS       3000U
#define FLASHER_KNOCK_WINDOW_LONG_MS  30000U

#define FLASHER_REPLY_HELLO       'H'
#define FLASHER_REPLY_ACCEPTED    'A'
#define FLASHER_REPLY_PROGRESS    'S'
#define FLASHER_REPLY_READY       'R'
#define FLASHER_REPLY_CHUNK_OK    'K'
#define FLASHER_REPLY_VERIFYING   'V'
#define FLASHER_REPLY_DONE_OK     'D'
#define FLASHER_REPLY_ERROR       'E'

#define FLASHER_ERROR_FLASH_ID    'I'
#define FLASHER_ERROR_HEADER      'H'
#define FLASHER_ERROR_TIMEOUT     'T'
#define FLASHER_ERROR_ERASE       'E'
#define FLASHER_ERROR_PROGRAM     'P'
#define FLASHER_ERROR_CRC         'C'
#define FLASHER_ERROR_MANIFEST    'M'
#define FLASHER_ERROR_VERIFY      'V'

static uint8_t pageBuffer[ASSET_FLASH_PAGE_SIZE];

static uint32_t ReadU32Le(const uint8_t *data)
{
  return ((uint32_t)data[0]) |
         ((uint32_t)data[1] << 8) |
         ((uint32_t)data[2] << 16) |
         ((uint32_t)data[3] << 24);
}

static void WriteU16Le(uint8_t *data, uint16_t value)
{
  data[0] = (uint8_t)value;
  data[1] = (uint8_t)(value >> 8);
}

static void WriteU32Le(uint8_t *data, uint32_t value)
{
  data[0] = (uint8_t)value;
  data[1] = (uint8_t)(value >> 8);
  data[2] = (uint8_t)(value >> 16);
  data[3] = (uint8_t)(value >> 24);
}

static bool Tx(const uint8_t *data, uint16_t length)
{
  return HAL_UART_Transmit(&huart2, (uint8_t *)data, length,
                           FLASHER_IO_TIMEOUT_MS) == HAL_OK;
}

static void Reply(uint8_t code)
{
  (void)Tx(&code, 1U);
}

static void ReplyError(uint8_t reason)
{
  uint8_t reply[2] = {FLASHER_REPLY_ERROR, reason};
  (void)Tx(reply, sizeof(reply));
}

static void ReplyHello(void)
{
  uint8_t reply[4] = {FLASHER_REPLY_HELLO, 0U, 0U, 0U};

  W25Q64_GetJedecId(&reply[1]);
  (void)Tx(reply, sizeof(reply));
}

static void ReplyProgress(uint16_t current, uint16_t total)
{
  uint8_t reply[5] = {FLASHER_REPLY_PROGRESS, 0U, 0U, 0U, 0U};

  WriteU16Le(&reply[1], current);
  WriteU16Le(&reply[3], total);
  (void)Tx(reply, sizeof(reply));
}

static void ReplyChunkOk(uint32_t nextOffset)
{
  uint8_t reply[5] = {FLASHER_REPLY_CHUNK_OK, 0U, 0U, 0U, 0U};

  WriteU32Le(&reply[1], nextOffset);
  (void)Tx(reply, sizeof(reply));
}

static bool RxExact(uint8_t *buffer, uint16_t length, uint32_t timeoutMs)
{
  return HAL_UART_Receive(&huart2, buffer, length, timeoutMs) == HAL_OK;
}

/* Scan instead of reading a fixed block so repeated knocks or stale UART bytes
 * cannot be mistaken for the package header. */
static bool WaitForSequence(const uint8_t *sequence, uint16_t length,
                            uint32_t timeoutMs)
{
  uint16_t matched = 0U;
  uint32_t start = HAL_GetTick();

  while ((timeoutMs == HAL_MAX_DELAY) ||
         ((HAL_GetTick() - start) < timeoutMs))
  {
    uint8_t byte;
    uint32_t receiveTimeout = 50U;

    if (timeoutMs != HAL_MAX_DELAY)
    {
      uint32_t elapsed = HAL_GetTick() - start;
      uint32_t remaining = (elapsed < timeoutMs) ? (timeoutMs - elapsed) : 0U;
      if (remaining == 0U)
      {
        break;
      }
      if (receiveTimeout > remaining)
      {
        receiveTimeout = remaining;
      }
    }

    if (HAL_UART_Receive(&huart2, &byte, 1U, receiveTimeout) != HAL_OK)
    {
      /* A receive error here is almost always an RX overrun: the host streams
       * knock bytes faster than this polling loop consumes them. Left pending,
       * the overrun flag can suppress the next UART transmit, so the host never
       * sees our HELLO and only catches the later timeout error. Clear it so the
       * scan (and the following replies) stay healthy. */
      __HAL_UART_CLEAR_OREFLAG(&huart2);
      continue;
    }

    if (byte == sequence[matched])
    {
      matched++;
      if (matched == length)
      {
        return true;
      }
    }
    else
    {
      matched = (byte == sequence[0]) ? 1U : 0U;
    }
  }

  return false;
}

static bool EraseForLength(uint32_t dataLength)
{
  uint32_t dataSectors =
      asset_flash_sectors_needed(dataLength, ASSET_FLASH_SECTOR_SIZE);
  uint16_t total = (uint16_t)(dataSectors + 1U);
  uint32_t index;

  for (index = 0U; index < dataSectors; index++)
  {
    if (!W25Q64_EraseSector(index * ASSET_FLASH_SECTOR_SIZE))
    {
      return false;
    }
    ReplyProgress((uint16_t)(index + 1U), total);
  }

  if (!W25Q64_EraseSector(W25Q64_ASSET_MANIFEST_OFFSET))
  {
    return false;
  }
  ReplyProgress(total, total);
  return true;
}

static bool WriteManifest(uint32_t dataLength, uint32_t crc)
{
  uint8_t manifest[ASSET_MANIFEST_SIZE];

  memset(manifest, 0, sizeof(manifest));
  WriteU32Le(&manifest[0], ASSET_MANIFEST_MAGIC);
  WriteU32Le(&manifest[4], ASSET_MANIFEST_VERSION);
  WriteU32Le(&manifest[8], dataLength);
  WriteU32Le(&manifest[12], crc);

  return W25Q64_Write(W25Q64_ASSET_MANIFEST_OFFSET, manifest,
                      sizeof(manifest));
}

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

    if (!RxExact(pageBuffer, (uint16_t)chunk, FLASHER_IO_TIMEOUT_MS))
    {
      ReplyError(FLASHER_ERROR_TIMEOUT);
      return false;
    }
    if (!W25Q64_Write(offset, pageBuffer, chunk))
    {
      ReplyError(FLASHER_ERROR_PROGRAM);
      return false;
    }

    crc = asset_crc32_update(crc, pageBuffer, chunk);
    offset += chunk;
    ReplyChunkOk(offset);
  }

  *outCrc = asset_crc32_final(crc);
  return true;
}

static bool RunProgrammingSession(void)
{
  uint8_t tail[FLASHER_HEADER_TAIL_LEN];
  uint32_t dataLength;
  uint32_t expectedCrc;
  uint32_t actualCrc;

  ReplyHello();
  if (!W25Q64_IsReady())
  {
    ReplyError(FLASHER_ERROR_FLASH_ID);
    return false;
  }

  if (!WaitForSequence((const uint8_t *)FLASHER_HEADER_MAGIC,
                       FLASHER_HEADER_MAGIC_LEN, FLASHER_IO_TIMEOUT_MS) ||
      !RxExact(tail, sizeof(tail), FLASHER_IO_TIMEOUT_MS))
  {
    ReplyError(FLASHER_ERROR_TIMEOUT);
    return false;
  }

  dataLength = ReadU32Le(&tail[0]);
  expectedCrc = ReadU32Le(&tail[4]);
  if ((dataLength == 0U) || (dataLength > W25Q64_ASSET_MANIFEST_OFFSET))
  {
    ReplyError(FLASHER_ERROR_HEADER);
    return false;
  }

  Reply(FLASHER_REPLY_ACCEPTED);
  if (!EraseForLength(dataLength))
  {
    ReplyError(FLASHER_ERROR_ERASE);
    return false;
  }

  Reply(FLASHER_REPLY_READY);
  if (!ReceiveAndProgram(dataLength, &actualCrc))
  {
    return false;
  }

  if (actualCrc != expectedCrc)
  {
    ReplyError(FLASHER_ERROR_CRC);
    return false;
  }
  if (!WriteManifest(dataLength, expectedCrc))
  {
    ReplyError(FLASHER_ERROR_MANIFEST);
    return false;
  }

  Reply(FLASHER_REPLY_VERIFYING);
  if (!W25Q64_ValidateAssetPackage())
  {
    ReplyError(FLASHER_ERROR_VERIFY);
    return false;
  }

  Reply(FLASHER_REPLY_DONE_OK);
  HAL_Delay(FLASHER_RESET_DELAY_MS);
  NVIC_SystemReset();
  return true;
}

void AssetFlasher_RunAtBoot(bool assetsValid, bool programmingRequested)
{
  /* The board must ALWAYS reach the display init that runs after this call.
   * A bounded knock window replaces the previous infinite recovery loop, which
   * stranded the device on a black screen whenever the external flash held no
   * valid asset package. */
  uint32_t windowMs = programmingRequested ? FLASHER_KNOCK_WINDOW_LONG_MS
                                           : FLASHER_KNOCK_WINDOW_MS;

  if (assetsValid && !programmingRequested)
  {
    return;
  }

  if (WaitForSequence((const uint8_t *)FLASHER_KNOCK,
                      FLASHER_KNOCK_LEN, windowMs))
  {
    /* Host attached: run one session. On success the device resets inside
     * RunProgrammingSession; on failure we fall through and boot so a bad
     * attempt can never strand the board. */
    (void)RunProgrammingSession();
  }

  /* No knock within the window (or a failed session): boot the UI. With an
   * invalid/empty package the images render from whatever is in flash; the
   * manifest check is integrity-only and does not gate rendering. */
}
