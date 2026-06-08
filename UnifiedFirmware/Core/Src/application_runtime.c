#include "application_runtime.h"

#include "actuator_manager.h"
#include "app_config.h"
#include "application_context.h"
#include "application_feedback.h"
#include "bandy_leak_guard.h"
#include "bandy_pause_guard.h"
#include "discharge_controller.h"
#include "logger.h"
#include "main.h"
#include "platform.h"
#include "pressure_sensor.h"
#include "pump_driver.h"
#include "rfidTagRW.h"
#include "serial_comm.h"
#include "thd_protocol_utils.h"

#define APPLICATION_VACUUM_STATE_IDLE    0U
#define APPLICATION_VACUUM_STATE_RUNNING 1U
#define APPLICATION_BANDY_PAUSE_SECONDS  30U
#define APPLICATION_BANDY_MAX_PAUSES     3U
#define APPLICATION_BANDY_LEAK_TIMEOUT_MS 10000U

extern UART_HandleTypeDef huart1;

static application_bandy_state_t bandyState;
static application_active_product_t activeProduct;
static bandy_leak_guard_t bandyLeakGuard;
static bandy_pause_guard_t bandyPauseGuard;
static uint16_t bandyDurationMinutes;
static uint16_t bandyRemainingSeconds;
static uint16_t bandyPauseRemainingSeconds;
static uint32_t bandyLastRunTickMs;
static uint32_t bandyLastPauseTickMs;

static void ApplicationRuntime_InitRfidSubsystem(void);
static void ApplicationRuntime_ReinitializeRfidSubsystem(void);
static void ApplicationRuntime_FatalRfidInitLoop(void);
static void ApplicationRuntime_InitPumpControl(void);
static void ApplicationRuntime_InitBandySession(void);
static void ApplicationRuntime_ProcessBandySession(void);
static void ApplicationRuntime_ProcessSystemRequests(void);
static void ApplicationRuntime_ProcessActuators(void);
static void ApplicationRuntime_ProcessRfidScan(void);
static void ApplicationRuntime_HandleApprovedTag(void);
static void ApplicationRuntime_HandleRfidError(void);
static void ApplicationRuntime_HandleBandyLeakTimeout(void);
static void ApplicationRuntime_LoadBandySession(uint16_t durationMinutes);
static void ApplicationRuntime_StartVacuumHardware(void);
static void ApplicationRuntime_StopVacuumCycleInternal(bool restartRfidScan,
                                                       bool releaseToAtmosphere);
static void ApplicationRuntime_ResetBandySession(bool restartRfidScan);

void App_init(void)
{
  serial_comm_context_t serialCommContext;

  ApplicationContext_Init();
  serialCommContext.huart = &huart1;
  serialCommContext.proStation = ApplicationContext_GetProStation();
  serialCommContext.trackingData = ApplicationContext_GetTrackingData();
  serialCommContext.systemResetRequested = ApplicationContext_GetSystemResetRequested();
  serialCommContext.fwVersion = ApplicationContext_GetFwVersionString();
  serialCommContext.deviceString = ApplicationContext_GetDeviceString();
  SerialComm_Init(&serialCommContext);
  logUsartInit(&huart1);
  platformLedOn(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
  HAL_GPIO_WritePin(CAM_DISABLE_GPIO_Port, CAM_DISABLE_Pin, GPIO_PIN_SET);
  ApplicationRuntime_InitRfidSubsystem();
  ApplicationRuntime_InitPumpControl();
  ApplicationRuntime_InitBandySession();
}

void App_process(void)
{

  SerialComm_Process();
  ApplicationRuntime_ProcessSystemRequests();
  ApplicationFeedback_Process();
  ApplicationRuntime_ProcessActuators();
  ApplicationRuntime_ProcessBandySession();
  ApplicationRuntime_ProcessRfidScan();
}

static void ApplicationRuntime_ProcessActuators(void)
{
  PressureSensor_Process();
  DischargeController_Process(HAL_GetTick(),
                              PressureSensor_IsValid(),
                              PressureSensor_GetRelativeMbar());
  ActuatorManager_Process();
}

void ApplicationRuntime_StartRfidScan(void)
{
  PRO_STATION *proStation = ApplicationContext_GetProStation();

  if (ApplicationRuntime_IsVacuumCycleActive())
  {
    ApplicationRuntime_StopVacuumCycleInternal(false, true);
  }

  ApplicationRuntime_InitBandySession();
  proStation->rfidUpdateTag = false;
  proStation->rfidScanActive = true;
  proStation->rfidUpdateTag = true;
  proStation->rfidTagStatus = TAG_SEARCHING;
  proStation->rfidTagRWRetVal = TAG_RW_WIP;
  proStation->writeRetryCounter = ApplicationContext_GetDefaultWriteRetryCounter();
}

void ApplicationRuntime_StopRfidScan(void)
{
  PRO_STATION *proStation = ApplicationContext_GetProStation();

  proStation->rfidUpdateTag = false;
  proStation->rfidScanActive = false;
}

void ApplicationRuntime_StartVacuumCycle(void)
{
  uint32_t now;

  if (ApplicationRuntime_IsVacuumCycleActive())
  {
    return;
  }

  if ((bandyState != APPLICATION_BANDY_STATE_AUTHORIZED) &&
      (bandyState != APPLICATION_BANDY_STATE_PAUSED))
  {
    ApplicationRuntime_LoadBandySession(PRO_RFID_TAG_DEFAULT_DURATION_MINUTES);
  }

  if (bandyRemainingSeconds == 0U)
  {
    ApplicationRuntime_LoadBandySession(bandyDurationMinutes);
  }

  ApplicationRuntime_StopRfidScan();
  ApplicationFeedback_SetWarningBuzzerActive(false);
  now = HAL_GetTick();
  ApplicationRuntime_StartVacuumHardware();
  activeProduct = APPLICATION_ACTIVE_PRODUCT_BANDY;
  bandyState = APPLICATION_BANDY_STATE_RUNNING;
  bandyPauseRemainingSeconds = 0U;
  bandyLastRunTickMs = now;
  BandyLeakGuard_Start(&bandyLeakGuard, now);
}

static void ApplicationRuntime_StartVacuumHardware(void)
{
  DischargeController_Close();
  ActuatorManager_SetValve1Enabled(true);
  ActuatorManager_SetValve2Enabled(true);
  PressureSensor_StartTest();
  PumpDriver_SetTargetDutyPercent(0U);
  PumpDriver_SetEnabled(false);
  PumpDriver_SetEnabled(true);
  PumpDriver_StartVacuumTest();
}

void ApplicationRuntime_StopVacuumCycle(void)
{
  ApplicationRuntime_EndVacuumSession();
}

void ApplicationRuntime_PauseVacuumCycle(void)
{
  if (bandyState != APPLICATION_BANDY_STATE_RUNNING)
  {
    return;
  }

  if (!BandyPauseGuard_RecordAcceptedPause(&bandyPauseGuard, APPLICATION_BANDY_MAX_PAUSES))
  {
    ApplicationRuntime_EndVacuumSession();
    return;
  }

  ApplicationRuntime_StopVacuumCycleInternal(false, false);
  bandyState = APPLICATION_BANDY_STATE_PAUSED;
  bandyPauseRemainingSeconds = APPLICATION_BANDY_PAUSE_SECONDS;
  bandyLastPauseTickMs = HAL_GetTick();
  ApplicationFeedback_SetWarningBuzzerActive(false);
}

void ApplicationRuntime_ResumeVacuumCycle(void)
{
  uint32_t now;

  if ((bandyState != APPLICATION_BANDY_STATE_PAUSED) || (bandyRemainingSeconds == 0U))
  {
    return;
  }

  now = HAL_GetTick();
  ApplicationRuntime_StartVacuumHardware();
  bandyState = APPLICATION_BANDY_STATE_RUNNING;
  bandyPauseRemainingSeconds = 0U;
  bandyLastRunTickMs = now;
  if (!BandyLeakGuard_HasReachedTarget(&bandyLeakGuard))
  {
    BandyLeakGuard_Start(&bandyLeakGuard, now);
  }
  ApplicationFeedback_SetWarningBuzzerActive(false);
}

void ApplicationRuntime_EndVacuumSession(void)
{
  ApplicationFeedback_SetWarningBuzzerActive(false);
  ApplicationRuntime_StopRfidScan();
  ApplicationRuntime_ResetBandySession(false);
}

bool ApplicationRuntime_SetBandyTargetRelativeMbar(int32_t targetMbar)
{
  return PumpDriver_SetPressureTargetMbar(targetMbar);
}

static void ApplicationRuntime_StopVacuumCycleInternal(bool restartRfidScan,
                                                       bool releaseToAtmosphere)
{
  PressureSensor_StopTest();
  PumpDriver_SetTargetDutyPercent(0U);
  PumpDriver_SetEnabled(false);
  ActuatorManager_SetValve1Enabled(true);
  ActuatorManager_SetValve2Enabled(true);
  if (releaseToAtmosphere)
  {
    DischargeController_StartRelease(HAL_GetTick());
  }

  if (restartRfidScan)
  {
    ApplicationRuntime_StartRfidScan();
  }
}

bool ApplicationRuntime_IsVacuumCycleActive(void)
{
  return PumpDriver_IsVacuumTestActive();
}

uint8_t ApplicationRuntime_GetVacuumCycleState(void)
{
  return ApplicationRuntime_IsVacuumCycleActive() ? APPLICATION_VACUUM_STATE_RUNNING : APPLICATION_VACUUM_STATE_IDLE;
}

uint8_t ApplicationRuntime_GetVacuumCycleFault(void)
{
#if APPLICATION_BANDY_LEAK_GUARD_ENABLED
  return (uint8_t)BandyLeakGuard_GetFault(&bandyLeakGuard);
#else
  return 0U;
#endif
}

int32_t ApplicationRuntime_GetVacuumCycleTargetRelativeMbar(void)
{
  return PumpDriver_GetUserPressureTargetMbar();
}

void ApplicationRuntime_StartHemorflowCycle(void)
{
  thd_protocol_pressure_profile_t hemorflowProfile;

  if (ApplicationRuntime_IsVacuumCycleActive())
  {
    return;
  }

  if (!ThdProtocol_GetPressureProfile(THD_PROTOCOL_ACTIVE_PRODUCT_HEMORFLOW, &hemorflowProfile))
  {
    return;
  }

  if (!PumpDriver_SetPressureProfileMbar(hemorflowProfile.userTargetMbar,
                                         hemorflowProfile.controlTargetMbar,
                                         hemorflowProfile.restartTargetMbar))
  {
    return;
  }

  ApplicationRuntime_StopRfidScan();
  BandyLeakGuard_Reset(&bandyLeakGuard);
  ApplicationFeedback_SetWarningBuzzerActive(false);
  ApplicationRuntime_StartVacuumHardware();
  activeProduct = APPLICATION_ACTIVE_PRODUCT_HEMORFLOW;
}

int32_t ApplicationRuntime_GetVacuumCycleControlTargetRelativeMbar(void)
{
  return PumpDriver_GetPressureControlTargetMbar();
}

int32_t ApplicationRuntime_GetBandyTargetMinRelativeMbar(void)
{
  return PumpDriver_GetPressureTargetMinMbar();
}

int32_t ApplicationRuntime_GetBandyTargetMaxRelativeMbar(void)
{
  return PumpDriver_GetPressureTargetMaxMbar();
}

uint8_t ApplicationRuntime_GetVacuumCycleCommandDutyPercent(void)
{
  if (!ApplicationRuntime_IsVacuumCycleActive())
  {
    return 0U;
  }

  return PumpDriver_GetDutyLimitPercent();
}

application_bandy_state_t ApplicationRuntime_GetBandyState(void)
{
  return bandyState;
}

application_active_product_t ApplicationRuntime_GetActiveProduct(void)
{
  return activeProduct;
}

uint16_t ApplicationRuntime_GetBandyDurationMinutes(void)
{
  return bandyDurationMinutes;
}

uint16_t ApplicationRuntime_GetBandyRemainingSeconds(void)
{
  return bandyRemainingSeconds;
}

uint16_t ApplicationRuntime_GetBandyPauseRemainingSeconds(void)
{
  return bandyPauseRemainingSeconds;
}

uint8_t ApplicationRuntime_GetBandyPauseCount(void)
{
  return BandyPauseGuard_GetCount(&bandyPauseGuard);
}

uint8_t ApplicationRuntime_GetBandyMaxPauseCount(void)
{
  return APPLICATION_BANDY_MAX_PAUSES;
}

uint16_t ApplicationRuntime_GetTagRemainingExams(void)
{
  PRO_STATION *proStation = ApplicationContext_GetProStation();
  return proStation->rfidTag.examNum;
}

void ApplicationRuntime_SetManualPumpDutyPercent(uint8_t targetDutyPercent)
{
  if (bandyState != APPLICATION_BANDY_STATE_WAIT_RFID)
  {
    ApplicationRuntime_ResetBandySession(false);
  }
  else if (ApplicationRuntime_IsVacuumCycleActive())
  {
    ApplicationRuntime_StopVacuumCycleInternal(false, true);
  }

  ActuatorManager_SetValve1Enabled(true);
  ActuatorManager_SetValve2Enabled(true);

  if (targetDutyPercent == 0U)
  {
    PressureSensor_StopTest();
    PumpDriver_SetTargetDutyPercent(0U);
    PumpDriver_SetEnabled(false);
    return;
  }

  PumpDriver_SetTargetDutyPercent(targetDutyPercent);
  PumpDriver_SetEnabled(false);
  PumpDriver_SetEnabled(true);
}

static void ApplicationRuntime_InitRfidSubsystem(void)
{
  if (!demoIni())
  {
    ApplicationRuntime_FatalRfidInitLoop();
  }

  platformLog("Initialization succeeded..\r\n");
  for (int i = 0; i < 6; i++)
  {
    platformLedToogle(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
    platformLedToogle(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
    platformLedToogle(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
    platformLedToogle(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
    platformDelay(200);
  }

  platformLedOff(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
  platformLedOff(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
  platformLedOff(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
  platformLedOff(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
}

static void ApplicationRuntime_ReinitializeRfidSubsystem(void)
{
  if (!demoIni())
  {
    ApplicationRuntime_FatalRfidInitLoop();
  }
}

static void ApplicationRuntime_FatalRfidInitLoop(void)
{
  platformLog("Initialization failed..\r\n");
  while (1)
  {
    platformLedToogle(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
    platformLedToogle(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
    platformLedToogle(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
    platformLedToogle(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
    platformDelay(100);
  }
}

static void ApplicationRuntime_InitPumpControl(void)
{
  ActuatorManager_Init();
  DischargeController_Init();
  PressureSensor_Init();
  PumpDriver_Init();
}

static void ApplicationRuntime_InitBandySession(void)
{
  PRO_STATION *proStation = ApplicationContext_GetProStation();

  bandyState = APPLICATION_BANDY_STATE_WAIT_RFID;
  activeProduct = APPLICATION_ACTIVE_PRODUCT_NONE;
  proStation->rfidScanActive = false;
  proStation->rfidUpdateTag = false;
  proStation->rfidTagStatus = TAG_NOT_APPROVED;
  bandyDurationMinutes = PRO_RFID_TAG_DEFAULT_DURATION_MINUTES;
  bandyRemainingSeconds = 0U;
  bandyPauseRemainingSeconds = 0U;
  bandyLastRunTickMs = HAL_GetTick();
  bandyLastPauseTickMs = bandyLastRunTickMs;
  BandyLeakGuard_Reset(&bandyLeakGuard);
  BandyPauseGuard_Reset(&bandyPauseGuard);
  (void)PumpDriver_SetPressureTargetMbar(PumpDriver_GetPressureTargetMaxMbar());
  ApplicationFeedback_SetWarningBuzzerActive(false);
}

static void ApplicationRuntime_LoadBandySession(uint16_t durationMinutes)
{
  if ((durationMinutes < PRO_RFID_TAG_MIN_DURATION_MINUTES) ||
      (durationMinutes > PRO_RFID_TAG_MAX_DURATION_MINUTES))
  {
    durationMinutes = PRO_RFID_TAG_DEFAULT_DURATION_MINUTES;
  }

  bandyDurationMinutes = durationMinutes;
  bandyRemainingSeconds = (uint16_t)(durationMinutes * 60U);
  bandyPauseRemainingSeconds = 0U;
  bandyLastRunTickMs = HAL_GetTick();
  bandyLastPauseTickMs = bandyLastRunTickMs;
  BandyPauseGuard_Reset(&bandyPauseGuard);
}

static void ApplicationRuntime_ProcessBandySession(void)
{
  uint32_t now = HAL_GetTick();

  if (bandyState == APPLICATION_BANDY_STATE_RUNNING)
  {
#if APPLICATION_BANDY_LEAK_GUARD_ENABLED
    bool targetWasReached;
#endif

    ApplicationFeedback_SetWarningBuzzerActive(false);

#if APPLICATION_BANDY_LEAK_GUARD_ENABLED
    targetWasReached = BandyLeakGuard_HasReachedTarget(&bandyLeakGuard);
    BandyLeakGuard_Update(&bandyLeakGuard,
                          now,
                          PressureSensor_GetRelativeMbar(),
                          ApplicationRuntime_GetVacuumCycleControlTargetRelativeMbar(),
                          APPLICATION_BANDY_LEAK_TIMEOUT_MS);

    if (BandyLeakGuard_GetFault(&bandyLeakGuard) == BANDY_LEAK_GUARD_FAULT_TIMEOUT)
    {
      ApplicationRuntime_HandleBandyLeakTimeout();
      return;
    }

    if (!targetWasReached && BandyLeakGuard_HasReachedTarget(&bandyLeakGuard))
    {
      bandyLastRunTickMs = now;
    }

    if (!BandyLeakGuard_ShouldCountDown(&bandyLeakGuard))
    {
      return;
    }
#endif

    while ((bandyRemainingSeconds > 0U) && ((now - bandyLastRunTickMs) >= 1000U))
    {
      bandyRemainingSeconds--;
      bandyLastRunTickMs += 1000U;
    }

    if (bandyRemainingSeconds == 0U)
    {
      ApplicationRuntime_ResetBandySession(false);
    }
  }
  else if (bandyState == APPLICATION_BANDY_STATE_PAUSED)
  {
    while ((bandyPauseRemainingSeconds > 0U) && ((now - bandyLastPauseTickMs) >= 1000U))
    {
      bandyPauseRemainingSeconds--;
      bandyLastPauseTickMs += 1000U;
    }

    if ((bandyPauseRemainingSeconds > 0U) && (bandyPauseRemainingSeconds <= 5U))
    {
      ApplicationFeedback_SetWarningBuzzerActive(true);
    }

    if (bandyPauseRemainingSeconds == 0U)
    {
      ApplicationFeedback_SetWarningBuzzerActive(false);
      ApplicationRuntime_ResumeVacuumCycle();
    }
  }
  else
  {
    ApplicationFeedback_SetWarningBuzzerActive(false);
  }
}

static void ApplicationRuntime_HandleBandyLeakTimeout(void)
{
  ApplicationRuntime_StopVacuumCycleInternal(false, true);
  activeProduct = APPLICATION_ACTIVE_PRODUCT_NONE;
  bandyState = APPLICATION_BANDY_STATE_AUTHORIZED;
  bandyPauseRemainingSeconds = 0U;
  bandyLastPauseTickMs = HAL_GetTick();
  ApplicationFeedback_SetWarningBuzzerActive(false);
}

static void ApplicationRuntime_ProcessSystemRequests(void)
{
  if (ApplicationContext_ConsumeSystemResetRequested())
  {
    HAL_NVIC_SystemReset();
  }
}

static void ApplicationRuntime_ProcessRfidScan(void)
{
  PRO_STATION *proStation = ApplicationContext_GetProStation();

  if (!proStation->rfidScanActive)
  {
    return;
  }

  proStation->rfidTagRWRetVal = demoCycle(proStation);
  if (proStation->rfidTagRWRetVal == TAG_RW_WIP)
  {
    return;
  }

  if (proStation->rfidTagRWRetVal == TAG_OK)
  {
    ApplicationRuntime_HandleApprovedTag();
  }
  else
  {
    ApplicationRuntime_HandleRfidError();
  }

  proStation->lastRfidTagRWRetVal = proStation->rfidTagRWRetVal;
}

static void ApplicationRuntime_HandleApprovedTag(void)
{
  PRO_STATION *proStation = ApplicationContext_GetProStation();

  proStation->rfidTagStatus = TAG_APPROVED;
  proStation->rfidScanActive = false;
  proStation->bepTriggered = true;
  ApplicationRuntime_LoadBandySession(proStation->rfidTag.durationMinutes);
  bandyState = APPLICATION_BANDY_STATE_AUTHORIZED;

  ApplicationRuntime_ReinitializeRfidSubsystem();

  platformLog("\r\n");
  platformLog("\n UID: \t %s\r\n", hex2Str(proStation->rfidTagUid, RFAL_NFCV_UID_LEN));
  platformLog(" Numero rimanente di esami: \t %d\r\n", proStation->rfidTag.examNum);
  platformLog(" Durata esame : \t %d minuti \r\n", proStation->rfidTag.durationMinutes);
  platformLog(" Firma: \t %s\r\n", hex2Str((uint8_t*)&proStation->rfidTag.firm, 2));
  platformLog("\r\n");
}

static void ApplicationRuntime_HandleRfidError(void)
{
  PRO_STATION *proStation = ApplicationContext_GetProStation();

  if (((proStation->rfidTagRWRetVal == TAG_ERR_WRITE) || (proStation->rfidTagRWRetVal == TAG_ERR_READ)) &&
      (proStation->writeRetryCounter > 0U))
  {
    proStation->writeRetryCounter--;
  }
  else
  {
    proStation->rfidScanActive = false;
    proStation->rfidTagStatus = TAG_NOT_APPROVED;
    proStation->bopTriggered = true;
  }
}

static void ApplicationRuntime_ResetBandySession(bool restartRfidScan)
{
  ApplicationRuntime_StopVacuumCycleInternal(false, true);
  ApplicationRuntime_InitBandySession();

  if (restartRfidScan)
  {
    ApplicationRuntime_StartRfidScan();
  }
}
