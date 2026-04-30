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
  int32_t pressureMbar;
  bool valid;
} display_bridge_snapshot_t;

void DisplayBridgeRx_Init(UART_HandleTypeDef *huart);
bool DisplayBridgeRx_GetLatestSnapshot(display_bridge_snapshot_t *snapshot);
bool DisplayBridgeRx_GetLatestPressureMbar(int32_t *pressureMbar);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_BRIDGE_RX_H */
