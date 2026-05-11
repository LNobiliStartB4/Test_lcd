#ifndef DISPLAY_BRIDGE_RX_H
#define DISPLAY_BRIDGE_RX_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  uint8_t vacuumState;
  uint8_t fault;
  uint8_t rfidApproved;
  uint8_t bandyState;
  uint16_t durationMinutes;
  uint16_t remainingSeconds;
  uint16_t pauseRemainingSeconds;
  int32_t targetMbar;
  int32_t pressureMbar;
  bool valid;
} display_bridge_snapshot_t;

void DisplayBridgeRx_Init(UART_HandleTypeDef *huart);
bool DisplayBridgeRx_GetLatestSnapshot(display_bridge_snapshot_t *snapshot);
bool DisplayBridgeRx_GetLatestPressureMbar(int32_t *pressureMbar);
bool DisplayBridgeRx_SendVacuumStartCommand(void);
bool DisplayBridgeRx_SendVacuumPauseCommand(void);
bool DisplayBridgeRx_SendVacuumResumeCommand(void);
bool DisplayBridgeRx_SendVacuumEndCommand(void);
bool DisplayBridgeRx_SendVacuumStopCommand(void);
bool DisplayBridgeRx_SendBandyTargetCommand(int32_t targetMbar);
bool DisplayBridgeRx_SendRfidScanStartCommand(void);
bool DisplayBridgeRx_SendRfidScanStopCommand(void);
bool DisplayBridgeRx_SendVacuumEndCancelCommand(void);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_BRIDGE_RX_H */
