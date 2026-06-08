#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H

#include <stdbool.h>

#include "application_context.h"
#include "stm32f4xx_hal.h"

typedef struct
{
  UART_HandleTypeDef *huart;
  PRO_STATION *proStation;
  TRACKING_DATA *trackingData;
  bool *systemResetRequested;
  const char *fwVersion;
  const char *deviceString;
} serial_comm_context_t;

void SerialComm_Init(const serial_comm_context_t *context);
void SerialComm_Process(void);

#endif
