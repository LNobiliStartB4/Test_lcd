#include "display_bridge_rx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISPLAY_BRIDGE_RX_FRAME_MAX 96U
#define DISPLAY_BRIDGE_RX_BANDY_PREFIX "BANDY,"
#define DISPLAY_BRIDGE_RX_BANDY_PREFIX_LEN 6U
#define DISPLAY_BRIDGE_RX_HEMO_PREFIX "HEMO,"
#define DISPLAY_BRIDGE_RX_HEMO_PREFIX_LEN 5U
#define DISPLAY_BRIDGE_ACTIVE_PRODUCT_NONE 0U
#define DISPLAY_BRIDGE_ACTIVE_PRODUCT_HEMORFLOW 2U
#define DISPLAY_BRIDGE_TX_TIMEOUT_MS 50U
#define DISPLAY_BRIDGE_DEFAULT_DURATION_MINUTES 15U
#define DISPLAY_BRIDGE_DEFAULT_TARGET_MBAR 490
#define DISPLAY_BRIDGE_TARGET_MIN_MBAR 290
#define DISPLAY_BRIDGE_TARGET_MAX_MBAR 490
#define DISPLAY_BRIDGE_USE_DEVICE_SIMULATION 1U
#define DISPLAY_BRIDGE_SIM_RFID_DELAY_MS 2000U
#define DISPLAY_BRIDGE_SIM_CYCLE_SECONDS 30U
#define DISPLAY_BRIDGE_SIM_PRESSURE_RAMP_MBAR_PER_SEC 60
#define DISPLAY_BRIDGE_SIM_PAUSE_SECONDS 5U
#define DISPLAY_BRIDGE_ACTIVE_PRODUCT_BANDY 1U
#define DISPLAY_BRIDGE_BANDY_STATE_WAIT_RFID 0U
#define DISPLAY_BRIDGE_BANDY_STATE_AUTHORIZED 1U
#define DISPLAY_BRIDGE_BANDY_STATE_RUNNING 2U
#define DISPLAY_BRIDGE_BANDY_STATE_PAUSED 3U

#if !DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
static UART_HandleTypeDef *displayBridgeUart;
static uint8_t displayBridgeRxByte;
static char displayBridgeFrame[DISPLAY_BRIDGE_RX_FRAME_MAX];
static uint8_t displayBridgeFrameLength;
static volatile uint8_t displayBridgeVacuumState;
static volatile uint8_t displayBridgeActiveProduct;
static volatile uint8_t displayBridgeFault;
static volatile uint8_t displayBridgeRfidApproved;
static volatile uint8_t displayBridgeBandyState;
static volatile uint16_t displayBridgeDurationMinutes;
static volatile uint16_t displayBridgeRemainingSeconds;
static volatile uint16_t displayBridgePauseRemainingSeconds;
static volatile uint8_t displayBridgePausesUsed;
static volatile uint8_t displayBridgePausesMax;
static volatile int32_t displayBridgeTargetMbar;
static volatile int32_t displayBridgePressureMbar;
static volatile bool displayBridgeSnapshotValid;
#endif

#if DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
static bool simInitialized;
static bool simRfidScanActive;
static uint32_t simRfidScanStartMs;
static uint32_t simLastPressureUpdateMs;
static uint32_t simLastCountdownUpdateMs;
static uint8_t simBandyState;
static uint16_t simRemainingSeconds;
static uint16_t simPauseRemainingSeconds;
static int32_t simTargetMbar;
static int32_t simPressureMbar;
static int32_t simReportedPressureMbar;

static uint32_t DisplayBridgeRx_ElapsedMs(uint32_t now, uint32_t start)
{
  return (uint32_t)(now - start);
}

static int32_t DisplayBridgeRx_ClampTarget(int32_t targetMbar)
{
  if (targetMbar < DISPLAY_BRIDGE_TARGET_MIN_MBAR)
  {
    return DISPLAY_BRIDGE_TARGET_MIN_MBAR;
  }

  if (targetMbar > DISPLAY_BRIDGE_TARGET_MAX_MBAR)
  {
    return DISPLAY_BRIDGE_TARGET_MAX_MBAR;
  }

  return targetMbar;
}

static int32_t DisplayBridgeRx_ClampPressure(int32_t pressureMbar)
{
  if (pressureMbar < 0)
  {
    return 0;
  }

  if (pressureMbar > 500)
  {
    return 500;
  }

  return pressureMbar;
}

static int32_t DisplayBridgeRx_GetPressureOscillation(uint32_t now)
{
  static const int8_t kOscillationPattern[] = { 0, 2, 3, 1, 0, -2, -3, -1 };
  const uint32_t phase = (now / 350U) % (sizeof(kOscillationPattern) / sizeof(kOscillationPattern[0]));
  return kOscillationPattern[phase];
}

static void DisplayBridgeRx_ResetSimSession(void)
{
  const uint32_t now = HAL_GetTick();

  simRfidScanActive = false;
  simRfidScanStartMs = now;
  simLastPressureUpdateMs = now;
  simLastCountdownUpdateMs = now;
  simBandyState = DISPLAY_BRIDGE_BANDY_STATE_WAIT_RFID;
  simRemainingSeconds = 0U;
  simPauseRemainingSeconds = 0U;
  simTargetMbar = DISPLAY_BRIDGE_DEFAULT_TARGET_MBAR;
  simPressureMbar = 0;
  simReportedPressureMbar = 0;
}

static void DisplayBridgeRx_InitSimulation(void)
{
  simInitialized = true;
  DisplayBridgeRx_ResetSimSession();
}

static void DisplayBridgeRx_EnsureSimulationInitialized(void)
{
  if (!simInitialized)
  {
    DisplayBridgeRx_InitSimulation();
  }
}

static void DisplayBridgeRx_UpdateSimulatedPressure(uint32_t now)
{
  uint32_t elapsedMs;
  int32_t stepMbar;

  if ((simBandyState != DISPLAY_BRIDGE_BANDY_STATE_RUNNING) &&
      (simBandyState != DISPLAY_BRIDGE_BANDY_STATE_PAUSED))
  {
    simReportedPressureMbar = simPressureMbar;
    simLastPressureUpdateMs = now;
    return;
  }

  if (simBandyState == DISPLAY_BRIDGE_BANDY_STATE_PAUSED)
  {
    simReportedPressureMbar = DisplayBridgeRx_ClampPressure(simPressureMbar);
    simLastPressureUpdateMs = now;
    return;
  }

  elapsedMs = DisplayBridgeRx_ElapsedMs(now, simLastPressureUpdateMs);
  simLastPressureUpdateMs = now;

  stepMbar = (DISPLAY_BRIDGE_SIM_PRESSURE_RAMP_MBAR_PER_SEC * (int32_t)elapsedMs) / 1000;
  if ((stepMbar <= 0) && (elapsedMs > 0U))
  {
    stepMbar = 1;
  }

  if (simPressureMbar < simTargetMbar)
  {
    simPressureMbar += stepMbar;
    if (simPressureMbar > simTargetMbar)
    {
      simPressureMbar = simTargetMbar;
    }
  }
  else if (simPressureMbar > simTargetMbar)
  {
    simPressureMbar -= stepMbar;
    if (simPressureMbar < simTargetMbar)
    {
      simPressureMbar = simTargetMbar;
    }
  }

  if (simPressureMbar == simTargetMbar)
  {
    simReportedPressureMbar = DisplayBridgeRx_ClampPressure(simPressureMbar + DisplayBridgeRx_GetPressureOscillation(now));
  }
  else
  {
    simReportedPressureMbar = DisplayBridgeRx_ClampPressure(simPressureMbar);
  }
}

static void DisplayBridgeRx_UpdateSimulatedCountdown(uint32_t now)
{
  if (simBandyState != DISPLAY_BRIDGE_BANDY_STATE_RUNNING)
  {
    simLastCountdownUpdateMs = now;
    return;
  }

  while ((simRemainingSeconds > 0U) &&
         (DisplayBridgeRx_ElapsedMs(now, simLastCountdownUpdateMs) >= 1000U))
  {
    simRemainingSeconds--;
    simLastCountdownUpdateMs += 1000U;
  }

  if (simRemainingSeconds == 0U)
  {
    DisplayBridgeRx_ResetSimSession();
  }
}

static void DisplayBridgeRx_UpdateSimulation(void)
{
  const uint32_t now = HAL_GetTick();

  DisplayBridgeRx_EnsureSimulationInitialized();

  if (simRfidScanActive &&
      (simBandyState == DISPLAY_BRIDGE_BANDY_STATE_WAIT_RFID) &&
      (DisplayBridgeRx_ElapsedMs(now, simRfidScanStartMs) >= DISPLAY_BRIDGE_SIM_RFID_DELAY_MS))
  {
    simRfidScanActive = false;
    simBandyState = DISPLAY_BRIDGE_BANDY_STATE_AUTHORIZED;
    simPressureMbar = 0;
    simReportedPressureMbar = 0;
    simLastPressureUpdateMs = now;
    simLastCountdownUpdateMs = now;
  }

  DisplayBridgeRx_UpdateSimulatedCountdown(now);
  DisplayBridgeRx_UpdateSimulatedPressure(now);
}

static bool DisplayBridgeRx_GetSimulatedSnapshot(display_bridge_snapshot_t *snapshot)
{
  DisplayBridgeRx_UpdateSimulation();

  snapshot->vacuumState = (simBandyState == DISPLAY_BRIDGE_BANDY_STATE_RUNNING) ? 1U : 0U;
  snapshot->activeProduct = (simBandyState == DISPLAY_BRIDGE_BANDY_STATE_WAIT_RFID)
                          ? DISPLAY_BRIDGE_ACTIVE_PRODUCT_NONE
                          : DISPLAY_BRIDGE_ACTIVE_PRODUCT_BANDY;
  snapshot->fault = 0U;
  snapshot->rfidApproved = (simBandyState == DISPLAY_BRIDGE_BANDY_STATE_WAIT_RFID) ? 0U : 1U;
  snapshot->bandyState = simBandyState;
  snapshot->durationMinutes = 1U;
  snapshot->remainingSeconds = simRemainingSeconds;
  snapshot->pauseRemainingSeconds = simPauseRemainingSeconds;
  snapshot->pausesUsed = (simBandyState == DISPLAY_BRIDGE_BANDY_STATE_PAUSED) ? 1U : 0U;
  snapshot->pausesMax = 3U;
  snapshot->targetMbar = simTargetMbar;
  snapshot->pressureMbar = simReportedPressureMbar;
  snapshot->valid = true;

  return true;
}
#endif

#if !DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
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
  char *searchStart = displayBridgeFrame;

  while ((fieldStart = strstr(searchStart, fieldName)) != NULL)
  {
    if ((fieldStart == displayBridgeFrame) || (*(fieldStart - 1) == ','))
    {
      fieldStart += strlen(fieldName);
      *value = strtol(fieldStart, &endPtr, 10);
      return endPtr != fieldStart;
    }

    searchStart = fieldStart + 1;
  }

  return false;
}

static void DisplayBridgeRx_ParseFrame(void)
{
  long vacuumState = 0;
  long activeProduct = DISPLAY_BRIDGE_ACTIVE_PRODUCT_NONE;
  bool isBandyFrame;
  bool isHemoFrame;
  long fault = 0;
  long rfidApproved = 0;
  long bandyState = 0;
  long durationMinutes = DISPLAY_BRIDGE_DEFAULT_DURATION_MINUTES;
  long remainingSeconds = 0;
  long pauseRemainingSeconds = 0;
  long pausesUsed = 0;
  long pausesMax = 3;
  long targetMbar = DISPLAY_BRIDGE_DEFAULT_TARGET_MBAR;
  long pressure;

  displayBridgeFrame[displayBridgeFrameLength] = '\0';

  isBandyFrame = strncmp(displayBridgeFrame, DISPLAY_BRIDGE_RX_BANDY_PREFIX, DISPLAY_BRIDGE_RX_BANDY_PREFIX_LEN) == 0;
  isHemoFrame = strncmp(displayBridgeFrame, DISPLAY_BRIDGE_RX_HEMO_PREFIX, DISPLAY_BRIDGE_RX_HEMO_PREFIX_LEN) == 0;

  if (!isBandyFrame && !isHemoFrame)
  {
    return;
  }

  if (!DisplayBridgeRx_ParseLongField("P=", &pressure))
  {
    return;
  }

  (void)DisplayBridgeRx_ParseLongField("S=", &vacuumState);
  if (isHemoFrame)
  {
    activeProduct = (vacuumState != 0) ? DISPLAY_BRIDGE_ACTIVE_PRODUCT_HEMORFLOW : DISPLAY_BRIDGE_ACTIVE_PRODUCT_NONE;
    (void)DisplayBridgeRx_ParseLongField("A=", &activeProduct);
  }
  (void)DisplayBridgeRx_ParseLongField("F=", &fault);
  (void)DisplayBridgeRx_ParseLongField("R=", &rfidApproved);
  (void)DisplayBridgeRx_ParseLongField("B=", &bandyState);
  (void)DisplayBridgeRx_ParseLongField("M=", &durationMinutes);
  (void)DisplayBridgeRx_ParseLongField("E=", &remainingSeconds);
  (void)DisplayBridgeRx_ParseLongField("Q=", &pauseRemainingSeconds);
  (void)DisplayBridgeRx_ParseLongField("PU=", &pausesUsed);
  (void)DisplayBridgeRx_ParseLongField("PM=", &pausesMax);
  (void)DisplayBridgeRx_ParseLongField("T=", &targetMbar);

  displayBridgeVacuumState = (uint8_t)vacuumState;
  displayBridgeActiveProduct = (uint8_t)activeProduct;
  displayBridgeFault = (uint8_t)fault;
  displayBridgeRfidApproved = (uint8_t)rfidApproved;
  displayBridgeBandyState = (uint8_t)bandyState;
  displayBridgeDurationMinutes = (uint16_t)durationMinutes;
  displayBridgeRemainingSeconds = (uint16_t)remainingSeconds;
  displayBridgePauseRemainingSeconds = (uint16_t)pauseRemainingSeconds;
  displayBridgePausesUsed = (uint8_t)pausesUsed;
  displayBridgePausesMax = (uint8_t)pausesMax;
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
#endif

void DisplayBridgeRx_Init(UART_HandleTypeDef *huart)
{
#if DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
  (void)huart;
  DisplayBridgeRx_InitSimulation();
  return;
#else
  displayBridgeUart = huart;
  displayBridgeVacuumState = 0U;
  displayBridgeActiveProduct = DISPLAY_BRIDGE_ACTIVE_PRODUCT_NONE;
  displayBridgeFault = 0U;
  displayBridgeRfidApproved = 0U;
  displayBridgeBandyState = 0U;
  displayBridgeDurationMinutes = DISPLAY_BRIDGE_DEFAULT_DURATION_MINUTES;
  displayBridgeRemainingSeconds = 0U;
  displayBridgePauseRemainingSeconds = 0U;
  displayBridgePausesUsed = 0U;
  displayBridgePausesMax = 3U;
  displayBridgeTargetMbar = DISPLAY_BRIDGE_DEFAULT_TARGET_MBAR;
  displayBridgePressureMbar = 0;
  displayBridgeSnapshotValid = false;
  DisplayBridgeRx_ResetFrame();
  DisplayBridgeRx_Rearm();
#endif
}

bool DisplayBridgeRx_GetLatestSnapshot(display_bridge_snapshot_t *snapshot)
{
  if (snapshot == NULL)
  {
    return false;
  }

#if DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
  return DisplayBridgeRx_GetSimulatedSnapshot(snapshot);
#else
  bool valid;
  uint8_t vacuumState;
  uint8_t activeProduct;
  uint8_t fault;
  uint8_t rfidApproved;
  uint8_t bandyState;
  uint16_t durationMinutes;
  uint16_t remainingSeconds;
  uint16_t pauseRemainingSeconds;
  uint8_t pausesUsed;
  uint8_t pausesMax;
  int32_t targetMbar;
  int32_t pressure;

  __disable_irq();
  valid = displayBridgeSnapshotValid;
  vacuumState = displayBridgeVacuumState;
  activeProduct = displayBridgeActiveProduct;
  fault = displayBridgeFault;
  rfidApproved = displayBridgeRfidApproved;
  bandyState = displayBridgeBandyState;
  durationMinutes = displayBridgeDurationMinutes;
  remainingSeconds = displayBridgeRemainingSeconds;
  pauseRemainingSeconds = displayBridgePauseRemainingSeconds;
  pausesUsed = displayBridgePausesUsed;
  pausesMax = displayBridgePausesMax;
  targetMbar = displayBridgeTargetMbar;
  pressure = displayBridgePressureMbar;
  __enable_irq();

  snapshot->vacuumState = vacuumState;
  snapshot->activeProduct = activeProduct;
  snapshot->fault = fault;
  snapshot->rfidApproved = rfidApproved;
  snapshot->bandyState = bandyState;
  snapshot->durationMinutes = durationMinutes;
  snapshot->remainingSeconds = remainingSeconds;
  snapshot->pauseRemainingSeconds = pauseRemainingSeconds;
  snapshot->pausesUsed = pausesUsed;
  snapshot->pausesMax = pausesMax;
  snapshot->targetMbar = targetMbar;
  snapshot->pressureMbar = pressure;
  snapshot->valid = valid;

  return valid;
#endif
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
#if DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
  const uint32_t now = HAL_GetTick();

  DisplayBridgeRx_EnsureSimulationInitialized();
  if ((simBandyState == DISPLAY_BRIDGE_BANDY_STATE_AUTHORIZED) ||
      (simBandyState == DISPLAY_BRIDGE_BANDY_STATE_PAUSED))
  {
    simBandyState = DISPLAY_BRIDGE_BANDY_STATE_RUNNING;
    simRemainingSeconds = DISPLAY_BRIDGE_SIM_CYCLE_SECONDS;
    simPauseRemainingSeconds = 0U;
    simLastPressureUpdateMs = now;
    simLastCountdownUpdateMs = now;
  }
  return true;
#else
  return DisplayBridgeRx_SendCommandFrame("CMD,VAC1\n");
#endif
}

bool DisplayBridgeRx_SendVacuumPauseCommand(void)
{
#if DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
  DisplayBridgeRx_EnsureSimulationInitialized();
  if (simBandyState == DISPLAY_BRIDGE_BANDY_STATE_RUNNING)
  {
    simBandyState = DISPLAY_BRIDGE_BANDY_STATE_PAUSED;
    simPauseRemainingSeconds = DISPLAY_BRIDGE_SIM_PAUSE_SECONDS;
  }
  return true;
#else
  return DisplayBridgeRx_SendCommandFrame("CMD,VACPAUSE\n");
#endif
}

bool DisplayBridgeRx_SendVacuumResumeCommand(void)
{
#if DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
  const uint32_t now = HAL_GetTick();

  DisplayBridgeRx_EnsureSimulationInitialized();
  if (simBandyState == DISPLAY_BRIDGE_BANDY_STATE_PAUSED)
  {
    simBandyState = DISPLAY_BRIDGE_BANDY_STATE_RUNNING;
    simPauseRemainingSeconds = 0U;
    simLastPressureUpdateMs = now;
    simLastCountdownUpdateMs = now;
  }
  return true;
#else
  return DisplayBridgeRx_SendCommandFrame("CMD,VACRESUME\n");
#endif
}

bool DisplayBridgeRx_SendVacuumEndCommand(void)
{
#if DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
  DisplayBridgeRx_EnsureSimulationInitialized();
  DisplayBridgeRx_ResetSimSession();
  return true;
#else
  return DisplayBridgeRx_SendCommandFrame("CMD,VACEND\n");
#endif
}

bool DisplayBridgeRx_SendVacuumStopCommand(void)
{
#if DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
  DisplayBridgeRx_EnsureSimulationInitialized();
  DisplayBridgeRx_ResetSimSession();
  return true;
#else
  return DisplayBridgeRx_SendCommandFrame("CMD,VACEND\n");
#endif
}

bool DisplayBridgeRx_SendBandyTargetCommand(int32_t targetMbar)
{
#if DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
  DisplayBridgeRx_EnsureSimulationInitialized();
  simTargetMbar = DisplayBridgeRx_ClampTarget(targetMbar);
  return true;
#else
  char frame[16];

  if ((targetMbar < DISPLAY_BRIDGE_TARGET_MIN_MBAR) ||
      (targetMbar > DISPLAY_BRIDGE_TARGET_MAX_MBAR))
  {
    return false;
  }

  (void)snprintf(frame, sizeof(frame), "CMD,VACT%03ld\n", (long)targetMbar);
  return DisplayBridgeRx_SendCommandFrame(frame);
#endif
}

bool DisplayBridgeRx_SendRfidScanStartCommand(void)
{
#if DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
  DisplayBridgeRx_EnsureSimulationInitialized();
  if (simBandyState == DISPLAY_BRIDGE_BANDY_STATE_WAIT_RFID)
  {
    simRfidScanActive = true;
    simRfidScanStartMs = HAL_GetTick();
  }
  return true;
#else
  return DisplayBridgeRx_SendCommandFrame("CMD,SCAN1\n");
#endif
}

bool DisplayBridgeRx_SendRfidScanStopCommand(void)
{
#if DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
  DisplayBridgeRx_EnsureSimulationInitialized();
  simRfidScanActive = false;
  return true;
#else
  return DisplayBridgeRx_SendCommandFrame("CMD,SCAN0\n");
#endif
}

bool DisplayBridgeRx_SendFramStatus(int32_t status,
                                    bool recordValid,
                                    uint16_t remainingSeconds,
                                    uint16_t durationMinutes,
                                    uint8_t bandyState,
                                    uint8_t pausesUsed,
                                    uint8_t pausesMax,
                                    uint32_t sequence)
{
#if DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
  (void)status;
  (void)recordValid;
  (void)remainingSeconds;
  (void)durationMinutes;
  (void)bandyState;
  (void)pausesUsed;
  (void)pausesMax;
  (void)sequence;
  return true;
#else
  char frame[96];

  (void)snprintf(frame,
                 sizeof(frame),
                 "FRAM,ST=%ld,V=%u,E=%u,M=%u,B=%u,PU=%u,PM=%u,SEQ=%lu\n",
                 (long)status,
                 recordValid ? 1U : 0U,
                 (unsigned int)remainingSeconds,
                 (unsigned int)durationMinutes,
                 (unsigned int)bandyState,
                 (unsigned int)pausesUsed,
                 (unsigned int)pausesMax,
                 (unsigned long)sequence);
  return DisplayBridgeRx_SendCommandFrame(frame);
#endif
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
#if DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
  (void)huart;
  return;
#else
  if ((displayBridgeUart == NULL) || (huart != displayBridgeUart))
  {
    return;
  }

  DisplayBridgeRx_ProcessByte(displayBridgeRxByte);
  DisplayBridgeRx_Rearm();
#endif
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
#if DISPLAY_BRIDGE_USE_DEVICE_SIMULATION
  (void)huart;
  return;
#else
  if ((displayBridgeUart == NULL) || (huart != displayBridgeUart))
  {
    return;
  }

  HAL_UART_AbortReceive_IT(huart);
  DisplayBridgeRx_ResetFrame();
  DisplayBridgeRx_Rearm();
#endif
}
