#include "application_context.h"

#include <stdlib.h>
#include <string.h>

#include "main.h"

#define FW_VERSION_STRING "01.03\r"

#define MINICAM "5B5B\r"
#define RECTOCAM "5C5C\r"
#define EAUS "C5C5\r"
#define SURGERY "5B00\r"
#define GYNOCAM "5D5D\r"
#define BANDY "EEEE\r"

#define APPLICATION BANDY
#define APPLICATION_FIRM_STRING APPLICATION
#define APPLICATION_FIRM strtoul(APPLICATION_FIRM_STRING, NULL, 16)

#define MAX_WRITE_RETRY_COUNTER 3U

#define BOARD_SN_SIZE 16U
#define CAM_SN_SIZE 16U
#define BOX_SN_SIZE 16U

#define BOARD_SN_INIT "BRD00000"
#define CAM_SN_INIT "CAM00000"
#define BOX_SN_INIT "BOX00000"

static PRO_STATION proStation;
static bool systemResetRequested;
static TRACKING_DATA trackingData;

void ApplicationContext_Init(void)
{
  proStation.ledLightIntlevel = 0U;
  proStation.rfidScanActive = false;
  proStation.rfidUpdateTag = false;
  proStation.rfidTagStatus = TAG_NOT_APPROVED;
  proStation.rfidEraseTag = false;
  proStation.rfidProgramTag = false;
  proStation.rfidTag.examNum = 1U;
  proStation.rfidTag.durationMinutes = PRO_RFID_TAG_DEFAULT_DURATION_MINUTES;
  proStation.rfidTag.firm = ApplicationContext_GetApplicationFirm();
  proStation.rfidTag.a_var = 0;
  proStation.rfidTag.b_var = 0;
  proStation.rfidTag.c_var = 0;
  proStation.rfidTag.d_var = 0;
  proStation.rfidTag.pad1 = 1U;
  proStation.rfidTag.pad2 = 2U;
  proStation.rfidTag.pad3 = 3U;
  proStation.rfidTag.pad4 = 4U;
  proStation.rfidTag.pad5 = 5U;
  proStation.rfidTag.pad6 = 6U;
  proStation.rfidTag.pad7 = 7U;
  proStation.rfidTag.pad8 = 8U;
  proStation.rfidTag.proRfidCRC = 9U;
  proStation.bepTriggered = false;
  proStation.bopTriggered = false;
  proStation.writeRetryCounter = MAX_WRITE_RETRY_COUNTER;
  proStation.rfidTagRWRetVal = TAG_RW_WIP;
  proStation.lastRfidTagRWRetVal = TAG_RW_WIP;
  proStation.firmToCheck = ApplicationContext_GetApplicationFirm();
  proStation.keepAliveTimer = HAL_GetTick();

  ApplicationContext_InitTrackingData();
  systemResetRequested = false;
}

void ApplicationContext_InitTrackingData(void)
{
  strcpy(&trackingData.td_boardSn[0], BOARD_SN_INIT);
  strcpy(&trackingData.td_camSn[0], CAM_SN_INIT);
  strcpy(&trackingData.td_boxSn[0], BOX_SN_INIT);
}

PRO_STATION *ApplicationContext_GetProStation(void)
{
  return &proStation;
}

TRACKING_DATA *ApplicationContext_GetTrackingData(void)
{
  return &trackingData;
}

bool *ApplicationContext_GetSystemResetRequested(void)
{
  return &systemResetRequested;
}

bool ApplicationContext_ConsumeSystemResetRequested(void)
{
  bool requested = systemResetRequested;

  systemResetRequested = false;
  return requested;
}

const char *ApplicationContext_GetFwVersionString(void)
{
  return FW_VERSION_STRING;
}

const char *ApplicationContext_GetDeviceString(void)
{
  return APPLICATION_FIRM_STRING;
}

uint16_t ApplicationContext_GetApplicationFirm(void)
{
  return (uint16_t)APPLICATION_FIRM;
}

uint8_t ApplicationContext_GetDefaultWriteRetryCounter(void)
{
  return MAX_WRITE_RETRY_COUNTER;
}
