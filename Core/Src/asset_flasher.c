#include "asset_flasher.h"

#include "main.h"
#include "w25q64.h"
#include "asset_manifest.h"
#include "asset_flash_layout.h"

#include <string.h>

extern UART_HandleTypeDef huart2;

#define FLASHER_KNOCK             "ADHTPROG"
#define FLASHER_KNOCK_LEN         8U
#define FLASHER_DIAGNOSTIC_KNOCK  "ADHTDIAG"
#define FLASHER_UART_TEST_KNOCK   "ADHTUART"
#define FLASHER_UART_TEST_BYTES   65536U
#define FLASHER_DIAGNOSTIC_VERSION 1U
#define FLASHER_DIAGNOSTIC_MODE_COUNT 2U
#define FLASHER_DIAGNOSTIC_HEADER_MAGIC "DGOK"
#define FLASHER_DIAGNOSTIC_RECORD_MAGIC "DREC"
#define FLASHER_HEADER_MAGIC      "ADHTHEAD"
#define FLASHER_HEADER_MAGIC_LEN  8U
#define FLASHER_HEADER_TAIL_LEN   (8U + ASSET_PACKAGE_ID_SIZE)
#define FLASHER_IO_TIMEOUT_MS     5000U
#define FLASHER_RESET_DELAY_MS    100U

#define FLASHER_REPLY_HELLO       'H'
#define FLASHER_REPLY_DIAGNOSTIC  'Q'
#define FLASHER_REPLY_UART_TEST   'U'
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
extern const asset_manifest_t g_expected_asset_package;

typedef enum
{
  FLASHER_COMMAND_PROGRAM,
  FLASHER_COMMAND_DIAGNOSTIC,
  FLASHER_COMMAND_UART_TEST
} flasher_command_t;

typedef enum
{
  FLASHER_TRANSFER_FAILED,
  FLASHER_TRANSFER_OK,
  FLASHER_TRANSFER_DIAGNOSTIC_REQUEST
} flasher_transfer_result_t;

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

static void FlushUartRx(void)
{
  uint8_t byte;
  uint16_t guard = 0U;

  __HAL_UART_CLEAR_OREFLAG(&huart2);
  while (guard < 512U)
  {
    if (HAL_UART_Receive(&huart2, &byte, 1U, 0U) != HAL_OK)
    {
      __HAL_UART_CLEAR_OREFLAG(&huart2);
      break;
    }
    guard++;
  }
}

/* Scan instead of reading a fixed block so repeated knocks or stale UART bytes
 * cannot be mistaken for the package header. */
static flasher_transfer_result_t WaitForSequenceOrDiagnostic(
    const uint8_t *sequence, uint16_t length, uint32_t timeoutMs)
{
  const uint8_t *diagnostic =
      (const uint8_t *)FLASHER_DIAGNOSTIC_KNOCK;
  uint16_t matched = 0U;
  uint16_t diagnosticMatched = 0U;
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
        return FLASHER_TRANSFER_OK;
      }
    }
    else
    {
      matched = (byte == sequence[0]) ? 1U : 0U;
    }

    if (byte == diagnostic[diagnosticMatched])
    {
      diagnosticMatched++;
      if (diagnosticMatched == FLASHER_KNOCK_LEN)
      {
        return FLASHER_TRANSFER_DIAGNOSTIC_REQUEST;
      }
    }
    else
    {
      diagnosticMatched = (byte == diagnostic[0]) ? 1U : 0U;
    }
  }

  return FLASHER_TRANSFER_FAILED;
}

static flasher_command_t WaitForRecoveryCommand(void)
{
  const uint8_t *program =
      (const uint8_t *)FLASHER_KNOCK;
  const uint8_t *diagnostic =
      (const uint8_t *)FLASHER_DIAGNOSTIC_KNOCK;
  const uint8_t *uartTest =
      (const uint8_t *)FLASHER_UART_TEST_KNOCK;
  uint16_t programMatched = 0U;
  uint16_t diagnosticMatched = 0U;
  uint16_t uartMatched = 0U;

  for (;;)
  {
    uint8_t byte;
    if (HAL_UART_Receive(&huart2, &byte, 1U, 50U) != HAL_OK)
    {
      __HAL_UART_CLEAR_OREFLAG(&huart2);
      continue;
    }

    if (byte == program[programMatched])
    {
      programMatched++;
      if (programMatched == FLASHER_KNOCK_LEN)
      {
        return FLASHER_COMMAND_PROGRAM;
      }
    }
    else
    {
      programMatched = (byte == program[0]) ? 1U : 0U;
    }

    if (byte == diagnostic[diagnosticMatched])
    {
      diagnosticMatched++;
      if (diagnosticMatched == FLASHER_KNOCK_LEN)
      {
        return FLASHER_COMMAND_DIAGNOSTIC;
      }
    }
    else
    {
      diagnosticMatched = (byte == diagnostic[0]) ? 1U : 0U;
    }

    if (byte == uartTest[uartMatched])
    {
      uartMatched++;
      if (uartMatched == FLASHER_KNOCK_LEN)
      {
        return FLASHER_COMMAND_UART_TEST;
      }
    }
    else
    {
      uartMatched = (byte == uartTest[0]) ? 1U : 0U;
    }
  }
}

/* UART-only link test: stream a counter pattern (byte[i] == i & 0xFF)
 * device->host so the host can measure drops/corruption on the serial path
 * alone, with no SPI/flash/erase involved. */
static void RunUartTestSession(void)
{
  uint8_t buffer[256];
  uint32_t i;
  uint32_t sent;

  for (i = 0U; i < sizeof(buffer); i++)
  {
    buffer[i] = (uint8_t)i; /* 0..255; chunks start at 256-byte boundaries */
  }

  Reply(FLASHER_REPLY_UART_TEST);
  for (sent = 0U; sent < FLASHER_UART_TEST_BYTES; sent += (uint32_t)sizeof(buffer))
  {
    (void)Tx(buffer, sizeof(buffer));
  }
}

static void ReplyDiagnosticRecord(const w25q64_diagnostic_report_t *report)
{
  uint8_t header[2] = {report->spi_mode, report->stable_count};

  (void)Tx((const uint8_t *)FLASHER_DIAGNOSTIC_RECORD_MAGIC, 4U);
  (void)Tx(header, sizeof(header));
  (void)Tx(&report->jedec_samples[0][0],
           W25Q64_DIAGNOSTIC_SAMPLE_COUNT * 3U);
  (void)Tx(report->manufacturer_device_id,
           sizeof(report->manufacturer_device_id));
  (void)Tx(&report->release_device_id, 1U);
  (void)Tx(report->status_registers, sizeof(report->status_registers));
  (void)Tx(report->sfdp_signature, sizeof(report->sfdp_signature));
}

static void RunDiagnosticSession(void)
{
  uint8_t header[3] =
  {
    FLASHER_REPLY_DIAGNOSTIC,
    FLASHER_DIAGNOSTIC_VERSION,
    FLASHER_DIAGNOSTIC_MODE_COUNT
  };
  w25q64_diagnostic_report_t report;

  (void)Tx(&header[0], 1U);
  (void)Tx((const uint8_t *)FLASHER_DIAGNOSTIC_HEADER_MAGIC, 4U);
  (void)Tx(&header[1], 2U);
  (void)W25Q64_RunDiagnostic(0U, &report);
  ReplyDiagnosticRecord(&report);
  (void)W25Q64_RunDiagnostic(3U, &report);
  ReplyDiagnosticRecord(&report);
  (void)W25Q64_Init();
  FlushUartRx();
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

static bool WriteManifest(uint32_t dataLength, uint32_t crc,
                          const uint8_t packageId[ASSET_PACKAGE_ID_SIZE])
{
  uint8_t manifest[ASSET_MANIFEST_SIZE];

  memset(manifest, 0, sizeof(manifest));
  WriteU32Le(&manifest[0], ASSET_MANIFEST_MAGIC);
  WriteU32Le(&manifest[4], ASSET_MANIFEST_VERSION);
  WriteU32Le(&manifest[8], dataLength);
  WriteU32Le(&manifest[12], crc);
  memcpy(&manifest[16], packageId, ASSET_PACKAGE_ID_SIZE);

  return W25Q64_Write(W25Q64_ASSET_MANIFEST_OFFSET, manifest,
                      sizeof(manifest));
}

static flasher_transfer_result_t ReceiveAndProgram(uint32_t dataLength,
                                                   uint32_t *outCrc)
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

    /* Receive the whole chunk in ONE tight HAL call. Reading byte-by-byte with
     * per-byte work (the previous diagnostic-matching loop) is slow enough to
     * miss bytes during a 115200 burst -> RX overrun -> dropped byte. The
     * serial-free self-test proved the SPI/flash path is clean, so the glitch
     * is here. The diagnostic command is still reachable at the knock stage. */
    if (!RxExact(pageBuffer, (uint16_t)chunk, FLASHER_IO_TIMEOUT_MS))
    {
      ReplyError(FLASHER_ERROR_TIMEOUT);
      return FLASHER_TRANSFER_FAILED;
    }
    if (!W25Q64_Write(offset, pageBuffer, chunk))
    {
      ReplyError(FLASHER_ERROR_PROGRAM);
      return FLASHER_TRANSFER_FAILED;
    }

    crc = asset_crc32_update(crc, pageBuffer, chunk);
    offset += chunk;
    ReplyChunkOk(offset);
  }

  *outCrc = asset_crc32_final(crc);
  return FLASHER_TRANSFER_OK;
}

static flasher_transfer_result_t RunProgrammingSession(void)
{
  uint8_t tail[FLASHER_HEADER_TAIL_LEN];
  uint32_t dataLength;
  uint32_t expectedCrc;
  uint32_t actualCrc;
  asset_manifest_t receivedPackage;
  flasher_transfer_result_t waitResult;
  flasher_transfer_result_t programResult;

  /* Never erase on a lucky single read from a marginal SPI connection. */
  (void)W25Q64_ProbeStable(W25Q64_DIAGNOSTIC_SAMPLE_COUNT);

  ReplyHello();
  if (!W25Q64_IsReady())
  {
    ReplyError(FLASHER_ERROR_FLASH_ID);
    return FLASHER_TRANSFER_FAILED;
  }

  waitResult = WaitForSequenceOrDiagnostic(
      (const uint8_t *)FLASHER_HEADER_MAGIC,
      FLASHER_HEADER_MAGIC_LEN, FLASHER_IO_TIMEOUT_MS);
  if (waitResult == FLASHER_TRANSFER_DIAGNOSTIC_REQUEST)
  {
    return FLASHER_TRANSFER_DIAGNOSTIC_REQUEST;
  }
  if ((waitResult != FLASHER_TRANSFER_OK) ||
      !RxExact(tail, sizeof(tail), FLASHER_IO_TIMEOUT_MS))
  {
    ReplyError(FLASHER_ERROR_TIMEOUT);
    return FLASHER_TRANSFER_FAILED;
  }

  dataLength = ReadU32Le(&tail[0]);
  expectedCrc = ReadU32Le(&tail[4]);
  receivedPackage.magic = ASSET_MANIFEST_MAGIC;
  receivedPackage.version = ASSET_MANIFEST_VERSION;
  receivedPackage.data_length = dataLength;
  receivedPackage.expected_crc = expectedCrc;
  memcpy(receivedPackage.package_id, &tail[8], ASSET_PACKAGE_ID_SIZE);

  if (!asset_manifest_header_ok(&receivedPackage,
                                W25Q64_ASSET_MANIFEST_OFFSET) ||
      !asset_manifest_matches_expected(&receivedPackage,
                                       &g_expected_asset_package))
  {
    ReplyError(FLASHER_ERROR_HEADER);
    return FLASHER_TRANSFER_FAILED;
  }

  Reply(FLASHER_REPLY_ACCEPTED);
  if (!EraseForLength(dataLength))
  {
    ReplyError(FLASHER_ERROR_ERASE);
    return FLASHER_TRANSFER_FAILED;
  }

  Reply(FLASHER_REPLY_READY);
  programResult = ReceiveAndProgram(dataLength, &actualCrc);
  if (programResult != FLASHER_TRANSFER_OK)
  {
    return programResult;
  }

  if (actualCrc != expectedCrc)
  {
    ReplyError(FLASHER_ERROR_CRC);
    return FLASHER_TRANSFER_FAILED;
  }
  if (!WriteManifest(dataLength, expectedCrc, receivedPackage.package_id))
  {
    ReplyError(FLASHER_ERROR_MANIFEST);
    return FLASHER_TRANSFER_FAILED;
  }

  Reply(FLASHER_REPLY_VERIFYING);
  if (!W25Q64_ValidateAssetPackage())
  {
    ReplyError(FLASHER_ERROR_VERIFY);
    return FLASHER_TRANSFER_FAILED;
  }

  Reply(FLASHER_REPLY_DONE_OK);
  HAL_Delay(FLASHER_RESET_DELAY_MS);
  NVIC_SystemReset();
  return FLASHER_TRANSFER_OK;
}

void AssetFlasher_RunAtBoot(bool assetsValid, bool programmingRequested)
{
  if (assetsValid && !programmingRequested)
  {
    return;
  }

  /* Invalid or deliberately reprogrammed assets must never reach TouchGFX.
   * Stay in recovery until one complete package matching this MCU firmware has
   * been written and verified. Development uses USART2; production will use
   * the SWD external loader and therefore boots directly with valid assets. */
  for (;;)
  {
    flasher_command_t command = WaitForRecoveryCommand();
    if (command == FLASHER_COMMAND_DIAGNOSTIC)
    {
      RunDiagnosticSession();
    }
    else if (command == FLASHER_COMMAND_UART_TEST)
    {
      RunUartTestSession();
    }
    else
    {
      flasher_transfer_result_t result = RunProgrammingSession();
      if (result == FLASHER_TRANSFER_DIAGNOSTIC_REQUEST)
      {
        RunDiagnosticSession();
      }
      else if (result == FLASHER_TRANSFER_FAILED)
      {
        FlushUartRx();
      }
    }
  }
}
