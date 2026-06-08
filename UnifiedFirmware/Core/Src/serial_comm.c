#include "serial_comm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actuator_manager.h"
#include "application_runtime.h"
#include "main.h"
#include "pressure_sensor.h"
#include "pump_driver.h"
#include "rfidTagRW.h"
#include "thd_auth.h"
#include "thd_protocol_utils.h"

#define SERIAL_BUF_LENGTH 127U
#define SERIAL_MAX_COMMAND_LENGTH 64U
#define SERIAL_END_OF_LINE '\r'
#define SERIAL_ACK_STRING "ACK\r"
#define SERIAL_NACK_STRING "NACK\r"
#define SERIAL_WIP_STRING "WIP\r"
#define SERIAL_TIMEOUT_MS 1000U

#define SERIAL_LED_COMM_LEV_POS 3U
#define SERIAL_LED_COMM_LEV_DIG_NUM 2U
#define SERIAL_LED_MAX_LEV 15U
#define SERIAL_DUTY_COMM_TARGET_DIG_NUM 2U
#define SERIAL_DUTY_MAX_PERCENT 50U
#define SERIAL_VACT_COMM_TARGET_DIG_NUM 3U
#define SERIAL_FIRM_COMM_LEN 4U

#define SERIAL_TAG_INFO_WITH_EOL
/* Factory/dev-only commands. Comment out for production firmware:
 *  SERIAL_USE_ERASE    — destroys a tag (examNum=0). Used only in factory.
 *  SERIAL_USE_TAGRESET — reprograms a tag with a fresh examNum. Used
 *                        only in factory.
 * In production these MUST be undefined: commenting out the #define
 * makes both commands return NACK, removing the ability to revive or
 * burn tags via the USB serial interface.
 */
#define SERIAL_USE_ERASE
#define SERIAL_USE_TAGRESET

typedef enum
{
  SERIAL_COMMAND_SCAN1,
  SERIAL_COMMAND_SCAN0,
  SERIAL_COMMAND_TAG,
  SERIAL_COMMAND_BEP,
  SERIAL_COMMAND_BOP,
  SERIAL_COMMAND_ERASE,
  SERIAL_COMMAND_CAM1,
  SERIAL_COMMAND_CAM0,
  SERIAL_COMMAND_VER,
  SERIAL_COMMAND_LED,
  SERIAL_COMMAND_DUTY,
  SERIAL_COMMAND_FIRM,
  SERIAL_COMMAND_KA,
  SERIAL_COMMAND_DEV,
  SERIAL_COMMAND_SN,
  SERIAL_COMMAND_RST,
  SERIAL_COMMAND_VAL1ON,
  SERIAL_COMMAND_VAL1OFF,
  SERIAL_COMMAND_VAL2ON,
  SERIAL_COMMAND_VAL2OFF,
  SERIAL_COMMAND_VAL_STATUS,
  SERIAL_COMMAND_VAC1,
  SERIAL_COMMAND_VAC0,
  SERIAL_COMMAND_VACPAUSE,
  SERIAL_COMMAND_VACRESUME,
  SERIAL_COMMAND_VACEND,
  SERIAL_COMMAND_VACT,
  SERIAL_COMMAND_VAC_STATUS,
  SERIAL_COMMAND_TAGRESET,
  SERIAL_COMMAND_SET0,
  SERIAL_COMMAND_SET1,
  SERIAL_COMMAND_STOP,
  SERIAL_COMMAND_TOKEN,
  SERIAL_COMMAND_START,
  SERIAL_COMMAND_VLV21,
  SERIAL_COMMAND_VLV20,
  SERIAL_COMMAND_VLV10,
  SERIAL_COMMAND_VLV11,
  SERIAL_COMMAND_NONE
} serial_command_t;

typedef enum
{
  SERIAL_STATE_INIT,
  SERIAL_STATE_LISTEN,
  SERIAL_STATE_PARSE_COMMAND,
  SERIAL_STATE_SEND_ACK,
  SERIAL_STATE_SEND_NACK,
  SERIAL_STATE_SEND_ANSWER,
  SERIAL_STATE_TIME_OUT,
  SERIAL_STATE_SEND_WIP
} serial_state_t;

typedef struct
{
  serial_state_t state;
  volatile uint32_t timer;
  bool receivedByte;
  uint8_t buffer[SERIAL_BUF_LENGTH];
  uint8_t receivedBytes;
  serial_command_t lastCommand;
  char *lastCommandString;
} serial_comm_t;

static serial_comm_context_t serialContext;
static serial_comm_t serialComm;
static serial_comm_t *serialP = &serialComm;

static const char *serialCommandList[SERIAL_COMMAND_NONE] =
{
  "SCAN1", "SCAN0", "TAG", "BEP", "BOP", "ERASE", "CAM1", "CAM0", "VER", "LED",
  "DUTY", "FIRM", "KA", "DEV", "SN", "RST", "VAL1ON", "VAL1OFF", "VAL2ON", "VAL2OFF", "VAL?",
  "VAC1", "VAC0", "VACPAUSE", "VACRESUME", "VACEND", "VACT", "VAC?", "TAGRESET",
  "SET0", "SET1", "STOP", "TOKEN", "START", "VLV21", "VLV20", "VLV10", "VLV11"
};

static thd_auth_context_t serialThdAuth;
static thd_protocol_bank_t serialThdSelectedBank;

static uint16_t SerialComm_ThdGenerateToken(void)
{
  /* Mix di HAL_GetTick() per evitare token banali. Non crypto-grade, ma il
   * segreto non sta nel token, sta nella chiave 0xB782. */
  uint32_t t = HAL_GetTick();
  return (uint16_t)((t ^ (t << 7) ^ (t >> 3)) ^ 0xACE1U);
}

static void SerialComm_ResetAndArm(void);
static void SerialComm_HexToStr(unsigned char *dest, unsigned char *source, size_t dataLen);
static void SerialComm_SetCamPowerDisabled(bool disabled);
static bool SerialComm_AppendString(const char *text);
static void SerialComm_SendThdAck(void);
static void SerialComm_SendThdNack(void);
static void SerialComm_SendThdToken(uint16_t token);

void SerialComm_Init(const serial_comm_context_t *context)
{

  serialContext = *context;

  for (uint8_t i = 0U; i < SERIAL_BUF_LENGTH; i++)
  {
    serialP->buffer[i] = 0U;
  }

  serialP->receivedByte = false;
  serialP->lastCommand = SERIAL_COMMAND_NONE;
  serialP->lastCommandString = NULL;
  serialP->receivedBytes = 0U;
  serialP->timer = 0U;
  serialP->state = SERIAL_STATE_INIT;
  serialThdSelectedBank = THD_PROTOCOL_BANK_NONE;
  ThdAuth_Init(&serialThdAuth);
}

static bool SerialComm_AppendString(const char *text)
{
  size_t textLength;

  if (text == NULL)
  {
    return false;
  }

  textLength = strlen(text);
  if (textLength > ((size_t)SERIAL_BUF_LENGTH - serialP->receivedBytes))
  {
    return false;
  }

  memcpy(&serialP->buffer[serialP->receivedBytes], (const uint8_t *)text, textLength);
  serialP->receivedBytes += (uint8_t)textLength;
  return true;
}

static void SerialComm_SendThdAck(void)
{
  if (SerialComm_AppendString(SERIAL_ACK_STRING))
  {
    serialP->state = SERIAL_STATE_SEND_ANSWER;
  }
  else
  {
    serialP->receivedBytes = 0U;
    serialP->state = SERIAL_STATE_SEND_NACK;
  }
}

static void SerialComm_SendThdNack(void)
{
  serialP->receivedBytes = 0U;
  if (SerialComm_AppendString(SERIAL_NACK_STRING))
  {
    serialP->state = SERIAL_STATE_SEND_ANSWER;
  }
  else
  {
    serialP->state = SERIAL_STATE_SEND_NACK;
  }
}

static void SerialComm_SendThdToken(uint16_t token)
{
  int written;
  size_t remainingLength = SERIAL_BUF_LENGTH - serialP->receivedBytes;

  written = snprintf((char *)&serialP->buffer[serialP->receivedBytes],
                     remainingLength,
                     "%04X\r%s",
                     (unsigned int)token,
                     SERIAL_ACK_STRING);
  if ((written > 0) && ((size_t)written < remainingLength))
  {
    serialP->receivedBytes += (uint8_t)written;
    serialP->state = SERIAL_STATE_SEND_ANSWER;
  }
  else
  {
    SerialComm_SendThdNack();
  }
}

static void SerialComm_ResetAndArm(void)
{
  if (serialP->lastCommandString != NULL)
  {
    free(serialP->lastCommandString);
    serialP->lastCommandString = NULL;
  }

  serialP->receivedByte = false;
  serialP->receivedBytes = 0U;
  serialP->lastCommand = SERIAL_COMMAND_NONE;
  serialP->timer = HAL_GetTick();
  if ((serialContext.huart != NULL) &&
      (HAL_UART_Receive_IT(serialContext.huart, &serialP->buffer[0], 1U) == HAL_OK))
  {
    serialP->state = SERIAL_STATE_LISTEN;
  }
  else
  {
    serialP->state = SERIAL_STATE_INIT;
  }
}

void SerialComm_Process(void)
{
  uint8_t commandIndex = 0U;
  uint8_t commLen = 0U;

  switch (serialP->state)
  {
    default:
    case SERIAL_STATE_INIT:
      SerialComm_ResetAndArm();
      break;

    case SERIAL_STATE_LISTEN:
      if (serialP->receivedByte)
      {
        serialP->receivedByte = false;
        serialP->state = SERIAL_STATE_PARSE_COMMAND;
        serialContext.proStation->keepAliveTimer = HAL_GetTick();
      }
      else if ((serialP->receivedBytes > 0U) &&
               ((HAL_GetTick() - serialP->timer) > SERIAL_TIMEOUT_MS))
      {
        serialP->state = SERIAL_STATE_TIME_OUT;
      }
      break;

    case SERIAL_STATE_PARSE_COMMAND:
      free(serialP->lastCommandString);
      serialP->lastCommandString = malloc(sizeof(char) * serialP->receivedBytes);
      if (serialP->lastCommandString == NULL)
      {
        serialP->lastCommand = SERIAL_COMMAND_NONE;
        serialP->receivedBytes = 0U;
        serialP->state = SERIAL_STATE_SEND_NACK;
        break;
      }

      serialP->lastCommandString[0] = '\0';
      strncat(serialP->lastCommandString, (char *)&serialP->buffer[0], serialP->receivedBytes - 1U);

      while ((commandIndex < (uint8_t)SERIAL_COMMAND_NONE) &&
             (strcmp(serialCommandList[commandIndex], serialP->lastCommandString) != 0))
      {
        commandIndex++;
      }

      if (commandIndex < (uint8_t)SERIAL_COMMAND_NONE)
      {
        serialP->lastCommand = (serial_command_t)commandIndex;
      }
      else
      {
        serialP->lastCommand = SERIAL_COMMAND_NONE;
        if (strncmp(serialCommandList[SERIAL_COMMAND_LED], serialP->lastCommandString, SERIAL_LED_COMM_LEV_POS) == 0)
        {
          serialP->lastCommand = SERIAL_COMMAND_LED;
        }
        else if (strncmp(serialCommandList[SERIAL_COMMAND_DUTY], serialP->lastCommandString, strlen(serialCommandList[SERIAL_COMMAND_DUTY])) == 0)
        {
          serialP->lastCommand = SERIAL_COMMAND_DUTY;
        }
        else if (strncmp(serialCommandList[SERIAL_COMMAND_FIRM], serialP->lastCommandString, strlen(serialCommandList[SERIAL_COMMAND_FIRM])) == 0)
        {
          serialP->lastCommand = SERIAL_COMMAND_FIRM;
        }
        else if (strncmp(serialCommandList[SERIAL_COMMAND_SN], serialP->lastCommandString, strlen(serialCommandList[SERIAL_COMMAND_SN])) == 0)
        {
          serialP->lastCommand = SERIAL_COMMAND_SN;
        }
        else if (strncmp(serialCommandList[SERIAL_COMMAND_VACT], serialP->lastCommandString, strlen(serialCommandList[SERIAL_COMMAND_VACT])) == 0)
        {
          serialP->lastCommand = SERIAL_COMMAND_VACT;
        }
        else if (strncmp(serialCommandList[SERIAL_COMMAND_START],
                         serialP->lastCommandString,
                         strlen(serialCommandList[SERIAL_COMMAND_START])) == 0)
        {
          serialP->lastCommand = SERIAL_COMMAND_START;
        }
      }
      serialP->receivedBytes = 0U;

      switch (serialP->lastCommand)
      {
        case SERIAL_COMMAND_SCAN1:
          ApplicationRuntime_StartRfidScan();
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_SCAN0:
          ApplicationRuntime_StopRfidScan();
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_TAG:
          if (serialContext.proStation->rfidTagStatus == TAG_SEARCHING)
          {
            serialP->state = SERIAL_STATE_SEND_WIP;
            break;
          }

          SerialComm_HexToStr(&serialP->buffer[serialP->receivedBytes],
                              (uint8_t *)&serialContext.proStation->rfidTagUid,
                              RFAL_NFCV_UID_LEN);
          serialP->receivedBytes += (RFAL_NFCV_UID_LEN * 2U);
          SerialComm_HexToStr(&serialP->buffer[serialP->receivedBytes],
                              (uint8_t *)&serialContext.proStation->rfidTag.examNum,
                              sizeof(serialContext.proStation->rfidTag.examNum));
          serialP->receivedBytes += (sizeof(serialContext.proStation->rfidTag.examNum) * 2U);
          SerialComm_HexToStr(&serialP->buffer[serialP->receivedBytes],
                              (uint8_t *)&serialContext.proStation->rfidTag.durationMinutes,
                              sizeof(serialContext.proStation->rfidTag.durationMinutes));
          serialP->receivedBytes += (sizeof(serialContext.proStation->rfidTag.durationMinutes) * 2U);
          SerialComm_HexToStr(&serialP->buffer[serialP->receivedBytes],
                              (uint8_t *)&serialContext.proStation->rfidTag.firm,
                              sizeof(serialContext.proStation->rfidTag.firm));
          serialP->receivedBytes += (sizeof(serialContext.proStation->rfidTag.firm) * 2U);
          SerialComm_HexToStr(&serialP->buffer[serialP->receivedBytes],
                              (uint8_t *)&serialContext.proStation->lastRfidTagRWRetVal,
                              sizeof(serialContext.proStation->lastRfidTagRWRetVal));
          serialP->receivedBytes += (sizeof(serialContext.proStation->lastRfidTagRWRetVal) * 2U);

#ifdef SERIAL_TAG_INFO_WITH_EOL
          serialP->buffer[serialP->receivedBytes] = SERIAL_END_OF_LINE;
          serialP->receivedBytes++;
#endif

          if (serialContext.proStation->rfidTagStatus == TAG_APPROVED)
          {
            serialP->state = SERIAL_STATE_SEND_ACK;
          }
          else
          {
            serialP->state = SERIAL_STATE_SEND_NACK;
          }
          break;

        case SERIAL_COMMAND_BEP:
          serialContext.proStation->bepTriggered = true;
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_BOP:
          serialContext.proStation->bopTriggered = true;
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_ERASE:
#ifdef SERIAL_USE_ERASE
          serialContext.proStation->rfidUpdateTag = true;
          serialContext.proStation->rfidEraseTag = true;
          serialContext.proStation->rfidScanActive = true;
          serialP->state = SERIAL_STATE_SEND_ACK;
#else
          serialP->state = SERIAL_STATE_SEND_NACK;
#endif
          break;

        case SERIAL_COMMAND_CAM1:
          SerialComm_SetCamPowerDisabled(false);
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_CAM0:
          SerialComm_SetCamPowerDisabled(true);
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_LED:
        {
          uint8_t lev;
          uint8_t levLen;
          uint8_t i = 0U;
          char levC[SERIAL_LED_COMM_LEV_DIG_NUM + 1U] = {0};

          levLen = strlen(serialP->lastCommandString) - SERIAL_LED_COMM_LEV_POS;
          serialP->state = SERIAL_STATE_SEND_NACK;
          if ((levLen <= SERIAL_LED_COMM_LEV_DIG_NUM) && (levLen > 0U))
          {
            strncpy(levC, &serialP->lastCommandString[SERIAL_LED_COMM_LEV_POS], levLen);
            while ((levC[i] >= '0') && (levC[i] <= '9') && (i < levLen))
            {
              i++;
            }
            if (i >= levLen)
            {
              lev = atoi(levC);
              if (lev <= SERIAL_LED_MAX_LEV)
              {
                serialContext.proStation->ledLightIntlevel = lev;
                serialP->state = SERIAL_STATE_SEND_ACK;
              }
            }
          }
          break;
        }

        case SERIAL_COMMAND_DUTY:
        {
          uint8_t dutyTargetPercent;
          uint8_t dutyLen;
          uint8_t dutyIndex = 0U;
          char dutyC[SERIAL_DUTY_COMM_TARGET_DIG_NUM + 1U] = {0};

          commLen = strlen(serialCommandList[SERIAL_COMMAND_DUTY]);
          dutyLen = strlen(serialP->lastCommandString) - commLen;
          serialP->state = SERIAL_STATE_SEND_NACK;
          if ((dutyLen <= SERIAL_DUTY_COMM_TARGET_DIG_NUM) && (dutyLen > 0U))
          {
            strncpy(dutyC, &serialP->lastCommandString[commLen], dutyLen);
            while ((dutyC[dutyIndex] >= '0') && (dutyC[dutyIndex] <= '9') && (dutyIndex < dutyLen))
            {
              dutyIndex++;
            }
            if (dutyIndex >= dutyLen)
            {
              dutyTargetPercent = (uint8_t)atoi(dutyC);
              if (dutyTargetPercent <= SERIAL_DUTY_MAX_PERCENT)
              {
                ApplicationRuntime_SetManualPumpDutyPercent(dutyTargetPercent);
                serialP->state = SERIAL_STATE_SEND_ACK;
              }
            }
          }
          break;
        }

        case SERIAL_COMMAND_FIRM:
        {
          uint8_t firmLen;
          uint8_t j = 0U;
          char firmC[SERIAL_FIRM_COMM_LEN + 1U] = "0000";

          commLen = strlen(serialCommandList[SERIAL_COMMAND_FIRM]);
          firmLen = strlen(serialP->lastCommandString) - commLen;
          serialP->state = SERIAL_STATE_SEND_NACK;
          if (firmLen == SERIAL_FIRM_COMM_LEN)
          {
            strcpy(firmC, &serialP->lastCommandString[commLen]);
            while ((j < firmLen) &&
                   (((firmC[j] >= '0') && (firmC[j] <= '9')) || ((firmC[j] >= 'A') && (firmC[j] <= 'F'))))
            {
              j++;
            }
            if (j >= firmLen)
            {
              serialContext.proStation->firmToCheck = strtoul(firmC, NULL, 16);
              serialP->state = SERIAL_STATE_SEND_ACK;
            }
          }
          break;
        }

        case SERIAL_COMMAND_VER:
          if (SerialComm_AppendString(serialContext.fwVersion) &&
              SerialComm_AppendString("\r"))
          {
            serialP->state = SERIAL_STATE_SEND_ACK;
          }
          else
          {
            serialP->receivedBytes = 0U;
            serialP->state = SERIAL_STATE_SEND_NACK;
          }
          break;

        case SERIAL_COMMAND_KA:
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_DEV:
          if (SerialComm_AppendString(serialContext.deviceString) &&
              SerialComm_AppendString("\r"))
          {
            serialP->state = SERIAL_STATE_SEND_ACK;
          }
          else
          {
            serialP->receivedBytes = 0U;
            serialP->state = SERIAL_STATE_SEND_NACK;
          }
          break;

        case SERIAL_COMMAND_SN:
        {
          char sn[LINKED_TRACKING_STR_LEN] = "";

          commLen = strlen(serialP->lastCommandString) - strlen(serialCommandList[SERIAL_COMMAND_SN]);

          if (commLen >= LINKED_TRACKING_STR_LEN_MIN)
          {
            uint8_t delimiters = 0U;
            char *charP = sn;

            strcpy(sn, &serialP->lastCommandString[strlen(serialCommandList[SERIAL_COMMAND_SN])]);

            do
            {
              if ((charP = strstr(charP, TRACKING_STRING_DELIMITER)) != NULL)
              {
                charP++;
                delimiters++;
              }
            }
            while (charP != NULL);

            if (delimiters != (TRACKING_STRING_NUM - 1U))
            {
              serialP->state = SERIAL_STATE_SEND_NACK;
            }
            else
            {
              strcpy(serialContext.trackingData->td_boardSn, strtok(sn, TRACKING_STRING_DELIMITER));
              strcpy(serialContext.trackingData->td_camSn, strtok(NULL, TRACKING_STRING_DELIMITER));
              strcpy(serialContext.trackingData->td_boxSn, strtok(NULL, TRACKING_STRING_DELIMITER));
              serialP->state = SERIAL_STATE_SEND_ACK;
            }
          }
          else if (commLen == 0U)
          {
            char eolStr[2] = {SERIAL_END_OF_LINE, '\0'};

            strcat(sn, serialContext.trackingData->td_boardSn);
            strcat(sn, TRACKING_STRING_DELIMITER);
            strcat(sn, serialContext.trackingData->td_camSn);
            strcat(sn, TRACKING_STRING_DELIMITER);
            strcat(sn, serialContext.trackingData->td_boxSn);
            strcat(sn, eolStr);
            memcpy(&serialP->buffer[serialP->receivedBytes], (uint8_t *)sn, strlen(sn));
            serialP->receivedBytes += strlen(sn);
            serialP->state = SERIAL_STATE_SEND_ACK;
          }
          else
          {
            serialP->state = SERIAL_STATE_SEND_NACK;
          }
          break;
        }

        case SERIAL_COMMAND_RST:
          memcpy(&serialP->buffer[serialP->receivedBytes], (uint8_t *)SERIAL_ACK_STRING, strlen(SERIAL_ACK_STRING));
          serialP->receivedBytes += strlen(SERIAL_ACK_STRING);
          HAL_UART_Transmit(serialContext.huart, serialP->buffer, serialP->receivedBytes, 50U);
          *serialContext.systemResetRequested = true;
          serialP->state = SERIAL_STATE_INIT;
          break;

        case SERIAL_COMMAND_VAL1ON:
          ActuatorManager_SetValve1Enabled(true);
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_VAL1OFF:
          ActuatorManager_SetValve1Enabled(false);
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_VAL2ON:
          ActuatorManager_SetValve2Enabled(true);
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_VAL2OFF:
          ActuatorManager_SetValve2Enabled(false);
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_VAL_STATUS:
        {
          int written;
          size_t remainingLength = SERIAL_BUF_LENGTH - serialP->receivedBytes;

          written = snprintf((char *)&serialP->buffer[serialP->receivedBytes],
                             remainingLength,
                             "VAL,V1=%u,V2=%u\r",
                             ActuatorManager_IsValve1Enabled() ? 1U : 0U,
                             ActuatorManager_IsValve2Enabled() ? 1U : 0U);
          if ((written > 0) && ((size_t)written < remainingLength))
          {
            serialP->receivedBytes += (uint8_t)written;
            serialP->state = SERIAL_STATE_SEND_ACK;
          }
          else
          {
            serialP->receivedBytes = 0U;
            serialP->state = SERIAL_STATE_SEND_NACK;
          }
          break;
        }

        case SERIAL_COMMAND_VAC1:
          ApplicationRuntime_StartVacuumCycle();
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_VAC0:
          ApplicationRuntime_StopVacuumCycle();
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_VACPAUSE:
          ApplicationRuntime_PauseVacuumCycle();
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_VACRESUME:
          ApplicationRuntime_ResumeVacuumCycle();
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_VACEND:
          ApplicationRuntime_EndVacuumSession();
          serialP->state = SERIAL_STATE_SEND_ACK;
          break;

        case SERIAL_COMMAND_TAGRESET:
#ifdef SERIAL_USE_TAGRESET
          serialContext.proStation->rfidUpdateTag = true;
          serialContext.proStation->rfidProgramTag = true;
          serialContext.proStation->rfidScanActive = true;
          serialP->state = SERIAL_STATE_SEND_ACK;
#else
          serialP->state = SERIAL_STATE_SEND_NACK;
#endif
          break;

        case SERIAL_COMMAND_VACT:
        {
          uint16_t targetMbar;
          uint8_t targetLen;
          uint8_t targetIndex = 0U;
          char targetC[SERIAL_VACT_COMM_TARGET_DIG_NUM + 1U] = {0};

          commLen = strlen(serialCommandList[SERIAL_COMMAND_VACT]);
          targetLen = strlen(serialP->lastCommandString) - commLen;
          serialP->state = SERIAL_STATE_SEND_NACK;

          if (targetLen == SERIAL_VACT_COMM_TARGET_DIG_NUM)
          {
            strncpy(targetC, &serialP->lastCommandString[commLen], targetLen);
            while ((targetC[targetIndex] >= '0') && (targetC[targetIndex] <= '9') && (targetIndex < targetLen))
            {
              targetIndex++;
            }

            if (targetIndex >= targetLen)
            {
              targetMbar = (uint16_t)atoi(targetC);
              if (ApplicationRuntime_SetBandyTargetRelativeMbar((int32_t)targetMbar))
              {
                serialP->state = SERIAL_STATE_SEND_ACK;
              }
            }
          }
          break;
        }

        case SERIAL_COMMAND_VAC_STATUS:
        {
          int written;
          size_t remainingLength = SERIAL_BUF_LENGTH - serialP->receivedBytes;

          written = snprintf((char *)&serialP->buffer[serialP->receivedBytes],
                             remainingLength,
                             "VAC,S=%d,F=%d,A=%u,R=%u,B=%u,M=%u,E=%u,Q=%u,N=%u,P=%ld,T=%ld,X=%ld,C=%u,I=%u,D=%u,PU=%u,PM=%u\r",
                             (int)ApplicationRuntime_GetVacuumCycleState(),
                             (int)ApplicationRuntime_GetVacuumCycleFault(),
                             (unsigned int)ApplicationRuntime_GetActiveProduct(),
                             (serialContext.proStation->rfidTagStatus == TAG_APPROVED) ? 1U : 0U,
                             (unsigned int)ApplicationRuntime_GetBandyState(),
                             (unsigned int)ApplicationRuntime_GetBandyDurationMinutes(),
                             (unsigned int)ApplicationRuntime_GetBandyRemainingSeconds(),
                             (unsigned int)ApplicationRuntime_GetBandyPauseRemainingSeconds(),
                             (unsigned int)ApplicationRuntime_GetTagRemainingExams(),
                             (long)PressureSensor_GetRawRelativeMbar(),
                             (long)ApplicationRuntime_GetVacuumCycleTargetRelativeMbar(),
                             (long)ApplicationRuntime_GetVacuumCycleControlTargetRelativeMbar(),
                             (unsigned int)ApplicationRuntime_GetVacuumCycleCommandDutyPercent(),
                             (unsigned int)PumpDriver_GetFilteredCurrentmA(),
                             (unsigned int)PumpDriver_GetDuty(),
                             (unsigned int)ApplicationRuntime_GetBandyPauseCount(),
                             (unsigned int)ApplicationRuntime_GetBandyMaxPauseCount());
          if ((written > 0) && ((size_t)written < remainingLength))
          {
            serialP->receivedBytes += (uint8_t)written;
            serialP->state = SERIAL_STATE_SEND_ACK;
          }
          else
          {
            serialP->receivedBytes = 0U;
            serialP->state = SERIAL_STATE_SEND_NACK;
          }
          break;
        }

        case SERIAL_COMMAND_SET0:
          serialThdSelectedBank = THD_PROTOCOL_BANK_HEMORFLOW;
          SerialComm_SendThdAck();
          break;

        case SERIAL_COMMAND_SET1:
          serialThdSelectedBank = THD_PROTOCOL_BANK_BANDY;
          SerialComm_SendThdAck();
          break;

        case SERIAL_COMMAND_STOP:
          ApplicationRuntime_StopVacuumCycle();
          serialThdSelectedBank = ThdProtocol_GetStopBank();
          SerialComm_SendThdAck();
          break;

        case SERIAL_COMMAND_TOKEN:
        {
          uint16_t token = SerialComm_ThdGenerateToken();

          ThdAuth_IssueToken(&serialThdAuth, token);
          SerialComm_SendThdToken(token);
          break;
        }

        case SERIAL_COMMAND_START:
        {
          uint16_t response;
          bool authAccepted;
          thd_protocol_active_product_t startProduct;

          if (ThdProtocol_ParseCommand(serialP->lastCommandString, &response) != THD_PROTOCOL_COMMAND_START)
          {
            SerialComm_SendThdNack();
            break;
          }

          authAccepted = ThdAuth_ConsumeResponse(&serialThdAuth, response);
          startProduct = ThdProtocol_GetStartProduct(serialThdSelectedBank, authAccepted);
          if (startProduct == THD_PROTOCOL_ACTIVE_PRODUCT_HEMORFLOW)
          {
            ApplicationRuntime_StartHemorflowCycle();
            SerialComm_SendThdAck();
          }
          else
          {
            SerialComm_SendThdNack();
          }
          break;
        }

        case SERIAL_COMMAND_VLV21:
          /* THD PC VLV2 controls the outlet/pedal valve, wired as firmware VAL1. */
          ActuatorManager_SetValve1Enabled(true);
          SerialComm_SendThdAck();
          break;

        case SERIAL_COMMAND_VLV20:
          /* THD PC VLV2 controls the outlet/pedal valve, wired as firmware VAL1. */
          ActuatorManager_SetValve1Enabled(false);
          SerialComm_SendThdAck();
          break;

        case SERIAL_COMMAND_VLV10:
        case SERIAL_COMMAND_VLV11:
          SerialComm_SendThdNack();
          break;

        case SERIAL_COMMAND_NONE:
        default:
          serialP->lastCommand = SERIAL_COMMAND_NONE;
          free(serialP->lastCommandString);
          serialP->lastCommandString = NULL;
          serialP->state = SERIAL_STATE_SEND_NACK;
          break;
      }
      break;

    case SERIAL_STATE_SEND_ACK:
      if (SerialComm_AppendString(SERIAL_ACK_STRING))
      {
        serialP->state = SERIAL_STATE_SEND_ANSWER;
      }
      else
      {
        serialP->receivedBytes = 0U;
        serialP->state = SERIAL_STATE_SEND_NACK;
      }
      break;

    case SERIAL_STATE_SEND_NACK:
      serialP->receivedBytes = 0U;
      if (SerialComm_AppendString(SERIAL_NACK_STRING))
      {
        serialP->state = SERIAL_STATE_SEND_ANSWER;
      }
      else
      {
        SerialComm_ResetAndArm();
      }
      break;

    case SERIAL_STATE_SEND_WIP:
      serialP->receivedBytes = 0U;
      if (SerialComm_AppendString(SERIAL_WIP_STRING))
      {
        serialP->state = SERIAL_STATE_SEND_ANSWER;
      }
      else
      {
        SerialComm_ResetAndArm();
      }
      break;

    case SERIAL_STATE_SEND_ANSWER:
      HAL_UART_Transmit(serialContext.huart, serialP->buffer, serialP->receivedBytes, 50U);
      serialP->state = SERIAL_STATE_INIT;
      break;

    case SERIAL_STATE_TIME_OUT:
      HAL_UART_AbortReceive_IT(serialContext.huart);
      SerialComm_ResetAndArm();
      break;
  }
}

static void SerialComm_HexToStr(unsigned char *dest, unsigned char *source, size_t dataLen)
{
  unsigned char *pin = source;
  const char *hex = "0123456789ABCDEF";
  uint8_t i = 0U;

  if (dataLen == 0U)
  {
    return;
  }

  for (; i < dataLen - 1U; ++i)
  {
    *dest++ = hex[(*pin >> 4) & 0xFU];
    *dest++ = hex[(*pin++) & 0xFU];
  }

  *dest++ = hex[(*pin >> 4) & 0xFU];
  *dest++ = hex[(*pin) & 0xFU];
}

static void SerialComm_SetCamPowerDisabled(bool disabled)
{
  HAL_GPIO_WritePin(CAM_DISABLE_GPIO_Port,
                    CAM_DISABLE_Pin,
                    disabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if ((serialContext.huart == NULL) || (huart != serialContext.huart))
  {
    return;
  }

  if ((serialP->receivedBytes + 1U) > SERIAL_BUF_LENGTH)
  {
    SerialComm_ResetAndArm();
    return;
  }

  serialP->receivedBytes++;
  serialP->timer = HAL_GetTick();

  if ((serialP->receivedBytes == 1U) &&
      ((serialP->buffer[0] == SERIAL_END_OF_LINE) || (serialP->buffer[0] == '\n')))
  {
    serialP->receivedBytes = 0U;
    if (HAL_UART_Receive_IT(huart, &serialP->buffer[0], 1U) != HAL_OK)
    {
      serialP->state = SERIAL_STATE_INIT;
    }
    return;
  }

  if (serialP->buffer[serialP->receivedBytes - 1U] == SERIAL_END_OF_LINE)
  {
    serialP->receivedByte = true;
    return;
  }

  if (serialP->receivedBytes >= SERIAL_MAX_COMMAND_LENGTH)
  {
    SerialComm_ResetAndArm();
    return;
  }

  if (HAL_UART_Receive_IT(huart, &serialP->buffer[serialP->receivedBytes], 1U) != HAL_OK)
  {
    serialP->state = SERIAL_STATE_INIT;
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  uint32_t errorCode;

  if ((serialContext.huart == NULL) || (huart != serialContext.huart))
  {
    return;
  }

  errorCode = huart->ErrorCode;
  if (errorCode == HAL_UART_ERROR_NONE)
  {
    return;
  }

  if ((errorCode & HAL_UART_ERROR_PE) != HAL_UART_ERROR_NONE)
  {
    __HAL_UART_CLEAR_PEFLAG(huart);
  }
  if ((errorCode & HAL_UART_ERROR_FE) != HAL_UART_ERROR_NONE)
  {
    __HAL_UART_CLEAR_FEFLAG(huart);
  }
  if ((errorCode & HAL_UART_ERROR_NE) != HAL_UART_ERROR_NONE)
  {
    __HAL_UART_CLEAR_NEFLAG(huart);
  }
  if ((errorCode & HAL_UART_ERROR_ORE) != HAL_UART_ERROR_NONE)
  {
    __HAL_UART_CLEAR_OREFLAG(huart);
    HAL_UART_AbortReceive(huart);
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    SerialComm_ResetAndArm();
    return;
  }

  huart->ErrorCode = HAL_UART_ERROR_NONE;
  if (huart->RxState == HAL_UART_STATE_READY)
  {
    SerialComm_ResetAndArm();
  }
}
