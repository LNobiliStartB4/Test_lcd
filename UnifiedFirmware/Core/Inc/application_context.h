#ifndef APPLICATION_CONTEXT_H
#define APPLICATION_CONTEXT_H

#include <stdbool.h>
#include <stdint.h>

#include "rfidTagRW.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TRACKING_STRING_DELIMITER ";"
#define TRACKING_STRING_LEN 16
#define TRACKING_STRING_END_SIZE 1
#define TRACKING_STRING_NUM 3
#define DELIMITER_NUM (TRACKING_STRING_NUM - 1)

#define LINKED_TRACKING_STR_LEN (TRACKING_STRING_NUM * TRACKING_STRING_LEN + DELIMITER_NUM)
#define LINKED_TRACKING_STR_LEN_MIN DELIMITER_NUM

typedef struct
{
  char td_boardSn[TRACKING_STRING_LEN];
  char td_camSn[TRACKING_STRING_LEN];
  char td_boxSn[TRACKING_STRING_LEN];
} TRACKING_DATA;

void ApplicationContext_Init(void);
void ApplicationContext_InitTrackingData(void);

PRO_STATION *ApplicationContext_GetProStation(void);
TRACKING_DATA *ApplicationContext_GetTrackingData(void);
bool *ApplicationContext_GetSystemResetRequested(void);
bool ApplicationContext_ConsumeSystemResetRequested(void);
const char *ApplicationContext_GetFwVersionString(void);
const char *ApplicationContext_GetDeviceString(void);
uint16_t ApplicationContext_GetApplicationFirm(void);
uint8_t ApplicationContext_GetDefaultWriteRetryCounter(void);

#ifdef __cplusplus
}
#endif

#endif
