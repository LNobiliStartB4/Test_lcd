#include "display_bridge_rx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISPLAY_BRIDGE_RX_FRAME_MAX 64U
#define DISPLAY_BRIDGE_RX_PREFIX "BANDY,"
#define DISPLAY_BRIDGE_RX_PREFIX_LEN 6U
#define DISPLAY_BRIDGE_TX_TIMEOUT_MS 50U
#define DISPLAY_BRIDGE_DEFAULT_DURATION_MINUTES 15U
#define DISPLAY_BRIDGE_DEFAULT_TARGET_MBAR 490
#define DISPLAY_BRIDGE_TARGET_MIN_MBAR 290
#define DISPLAY_BRIDGE_TARGET_MAX_MBAR 490

static UART_HandleTypeDef *displayBridgeUart;
static uint8_t displayBridgeRxByte;
static char displayBridgeFrame[DISPLAY_BRIDGE_RX_FRAME_MAX];
static uint8_t displayBridgeFrameLength;
static volatile uint8_t displayBridgeVacuumState;
static volatile uint8_t displayBridgeFault;
static volatile uint8_t displayBridgeRfidApproved;
static volatile uint8_t displayBridgeBandyState;
static volatile uint16_t displayBridgeDurationMinutes;
static volatile uint16_t displayBridgeRemainingSeconds;
static volatile uint16_t displayBridgePauseRemainingSeconds;
static volatile int32_t displayBridgeTargetMbar;
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
  long rfidApproved = 0;
  long bandyState = 0;
  long durationMinutes = DISPLAY_BRIDGE_DEFAULT_DURATION_MINUTES;
  long remainingSeconds = 0;
  long pauseRemainingSeconds = 0;
  long targetMbar = DISPLAY_BRIDGE_DEFAULT_TARGET_MBAR;
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
  (void)DisplayBridgeRx_ParseLongField("R=", &rfidApproved);
  (void)DisplayBridgeRx_ParseLongField("B=", &bandyState);
  (void)DisplayBridgeRx_ParseLongField("M=", &durationMinutes);
  (void)DisplayBridgeRx_ParseLongField("E=", &remainingSeconds);
  (void)DisplayBridgeRx_ParseLongField("Q=", &pauseRemainingSeconds);
  (void)DisplayBridgeRx_ParseLongField("T=", &targetMbar);

  displayBridgeVacuumState = (uint8_t)vacuumState;
  displayBridgeFault = (uint8_t)fault;
  displayBridgeRfidApproved = (uint8_t)rfidApproved;
  displayBridgeBandyState = (uint8_t)bandyState;
  displayBridgeDurationMinutes = (uint16_t)durationMinutes;
  displayBridgeRemainingSeconds = (uint16_t)remainingSeconds;
  displayBridgePauseRemainingSeconds = (uint16_t)pauseRemainingSeconds;
  displayBridgeTargetMbar = (int32_t)targetMbar;
  displayBridgePressureMbar = (int32_t)pressure;
  displayBridgeSnapshotValid = true;
}

static bool DisplayBridgeRx_SendCommandFrame(const char *frame)
{
  if ((displayBridgeUart == NULL) || (frame == NULL))
  {
    return false;
  }

  return HAL_UART_Transmit(displayBridgeUart,
                           (uint8_t *)frame,
                           (uint16_t)strlen(frame),
                           DISPLAY_BRIDGE_TX_TIMEOUT_MS) == HAL_OK;
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
  displayBridgeRfidApproved = 0U;
  displayBridgeBandyState = 0U;
  displayBridgeDurationMinutes = DISPLAY_BRIDGE_DEFAULT_DURATION_MINUTES;
  displayBridgeRemainingSeconds = 0U;
  displayBridgePauseRemainingSeconds = 0U;
  displayBridgeTargetMbar = DISPLAY_BRIDGE_DEFAULT_TARGET_MBAR;
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
  uint8_t rfidApproved;
  uint8_t bandyState;
  uint16_t durationMinutes;
  uint16_t remainingSeconds;
  uint16_t pauseRemainingSeconds;
  int32_t targetMbar;
  int32_t pressure;

  if (snapshot == NULL)
  {
    return false;
  }

  __disable_irq();
  valid = displayBridgeSnapshotValid;
  vacuumState = displayBridgeVacuumState;
  fault = displayBridgeFault;
  rfidApproved = displayBridgeRfidApproved;
  bandyState = displayBridgeBandyState;
  durationMinutes = displayBridgeDurationMinutes;
  remainingSeconds = displayBridgeRemainingSeconds;
  pauseRemainingSeconds = displayBridgePauseRemainingSeconds;
  targetMbar = displayBridgeTargetMbar;
  pressure = displayBridgePressureMbar;
  __enable_irq();

  snapshot->vacuumState = vacuumState;
  snapshot->fault = fault;
  snapshot->rfidApproved = rfidApproved;
  snapshot->bandyState = bandyState;
  snapshot->durationMinutes = durationMinutes;
  snapshot->remainingSeconds = remainingSeconds;
  snapshot->pauseRemainingSeconds = pauseRemainingSeconds;
  snapshot->targetMbar = targetMbar;
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

bool DisplayBridgeRx_SendVacuumStartCommand(void)
{
  return DisplayBridgeRx_SendCommandFrame("CMD,VAC1\n");
}

bool DisplayBridgeRx_SendVacuumPauseCommand(void)
{
  return DisplayBridgeRx_SendCommandFrame("CMD,VACPAUSE\n");
}

bool DisplayBridgeRx_SendVacuumResumeCommand(void)
{
  return DisplayBridgeRx_SendCommandFrame("CMD,VACRESUME\n");
}

bool DisplayBridgeRx_SendVacuumEndCommand(void)
{
  return DisplayBridgeRx_SendCommandFrame("CMD,VACEND\n");
}

bool DisplayBridgeRx_SendVacuumStopCommand(void)
{
  return DisplayBridgeRx_SendCommandFrame("CMD,VACEND\n");
}

bool DisplayBridgeRx_SendBandyTargetCommand(int32_t targetMbar)
{
  char frame[16];

  if ((targetMbar < DISPLAY_BRIDGE_TARGET_MIN_MBAR) ||
      (targetMbar > DISPLAY_BRIDGE_TARGET_MAX_MBAR))
  {
    return false;
  }

  (void)snprintf(frame, sizeof(frame), "CMD,VACT%03ld\n", (long)targetMbar);
  return DisplayBridgeRx_SendCommandFrame(frame);
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
