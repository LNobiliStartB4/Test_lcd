#include "display_bridge_rx.h"

#include <stdlib.h>
#include <string.h>

#define DISPLAY_BRIDGE_RX_FRAME_MAX 64U
#define DISPLAY_BRIDGE_RX_PREFIX "BANDY,"
#define DISPLAY_BRIDGE_RX_PREFIX_LEN 6U

static UART_HandleTypeDef *displayBridgeUart;
static uint8_t displayBridgeRxByte;
static char displayBridgeFrame[DISPLAY_BRIDGE_RX_FRAME_MAX];
static uint8_t displayBridgeFrameLength;
static volatile uint8_t displayBridgeVacuumState;
static volatile uint8_t displayBridgeFault;
static volatile int32_t displayBridgePressureMbar;
static volatile bool displayBridgeSnapshotValid;

static void DisplayBridgeRx_Rearm(void)
{
  if (displayBridgeUart != NULL)
  {
    (void)HAL_UART_Receive_IT(displayBridgeUart, &displayBridgeRxByte, 1U);
  }
}

static void DisplayBridgeRx_ResetFrame(void)
{
  displayBridgeFrameLength = 0U;
}

static bool DisplayBridgeRx_ParseLongField(const char *fieldName, long *value)
{
  char *fieldStart;
  char *endPtr;

  fieldStart = strstr(displayBridgeFrame, fieldName);
  if (fieldStart == NULL)
  {
    return false;
  }

  fieldStart += strlen(fieldName);
  *value = strtol(fieldStart, &endPtr, 10);
  return endPtr != fieldStart;
}

static void DisplayBridgeRx_ParseFrame(void)
{
  long vacuumState = 0;
  long fault = 0;
  long pressure;

  displayBridgeFrame[displayBridgeFrameLength] = '\0';

  if (strncmp(displayBridgeFrame, DISPLAY_BRIDGE_RX_PREFIX, DISPLAY_BRIDGE_RX_PREFIX_LEN) != 0)
  {
    return;
  }

  if (!DisplayBridgeRx_ParseLongField("P=", &pressure))
  {
    return;
  }

  (void)DisplayBridgeRx_ParseLongField("S=", &vacuumState);
  (void)DisplayBridgeRx_ParseLongField("F=", &fault);

  displayBridgeVacuumState = (uint8_t)vacuumState;
  displayBridgeFault = (uint8_t)fault;
  displayBridgePressureMbar = (int32_t)pressure;
  displayBridgeSnapshotValid = true;
}

static void DisplayBridgeRx_ProcessByte(uint8_t byte)
{
  if (byte == '\r')
  {
    return;
  }

  if (byte == '\n')
  {
    DisplayBridgeRx_ParseFrame();
    DisplayBridgeRx_ResetFrame();
    return;
  }

  if (displayBridgeFrameLength >= (DISPLAY_BRIDGE_RX_FRAME_MAX - 1U))
  {
    DisplayBridgeRx_ResetFrame();
    return;
  }

  displayBridgeFrame[displayBridgeFrameLength] = (char)byte;
  displayBridgeFrameLength++;
}

void DisplayBridgeRx_Init(UART_HandleTypeDef *huart)
{
  displayBridgeUart = huart;
  displayBridgeVacuumState = 0U;
  displayBridgeFault = 0U;
  displayBridgePressureMbar = 0;
  displayBridgeSnapshotValid = false;
  DisplayBridgeRx_ResetFrame();
  DisplayBridgeRx_Rearm();
}

bool DisplayBridgeRx_GetLatestSnapshot(display_bridge_snapshot_t *snapshot)
{
  bool valid;
  uint8_t vacuumState;
  uint8_t fault;
  int32_t pressure;

  if (snapshot == NULL)
  {
    return false;
  }

  __disable_irq();
  valid = displayBridgeSnapshotValid;
  vacuumState = displayBridgeVacuumState;
  fault = displayBridgeFault;
  pressure = displayBridgePressureMbar;
  __enable_irq();

  snapshot->vacuumState = vacuumState;
  snapshot->fault = fault;
  snapshot->pressureMbar = pressure;
  snapshot->valid = valid;

  return valid;
}

bool DisplayBridgeRx_GetLatestPressureMbar(int32_t *pressureMbar)
{
  display_bridge_snapshot_t snapshot;

  if (pressureMbar == NULL)
  {
    return false;
  }

  if (!DisplayBridgeRx_GetLatestSnapshot(&snapshot))
  {
    return false;
  }

  *pressureMbar = snapshot.pressureMbar;
  return true;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if ((displayBridgeUart == NULL) || (huart != displayBridgeUart))
  {
    return;
  }

  DisplayBridgeRx_ProcessByte(displayBridgeRxByte);
  DisplayBridgeRx_Rearm();
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if ((displayBridgeUart == NULL) || (huart != displayBridgeUart))
  {
    return;
  }

  HAL_UART_AbortReceive_IT(huart);
  DisplayBridgeRx_ResetFrame();
  DisplayBridgeRx_Rearm();
}
