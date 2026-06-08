#include "pressure_control.h"

#include "actuator_manager.h"
#include "main.h"
#include "pi_controller.h"
#include "pressure_sensor.h"
#include "pump_driver.h"

#define PRESSURE_CONTROL_PERIOD_MS 100U
#define PRESSURE_CONTROL_DEADBAND_MBAR 20
#define PRESSURE_CONTROL_MAX_DUTY_PERCENT 50U
#define PRESSURE_CONTROL_INVALID_TIMEOUT_MS 200U
#define PRESSURE_CONTROL_SATURATION_TIMEOUT_MS 45000U
#define PRESSURE_CONTROL_PI_KP 0.07f
#define PRESSURE_CONTROL_PI_KI 0.02f
#define PRESSURE_CONTROL_PI_SAMPLE_TIME_S 0.100f

static pressure_control_state_t pressureControlState;
static pressure_control_fault_t pressureControlFault;
static int32_t pressureControlTargetRelativeMbar;
static uint8_t pressureControlCommandDutyPercent;
static uint32_t pressureControlLastProcessTickMs;
static uint32_t pressureControlInvalidStartTickMs;
static uint32_t pressureControlMaxDutyStartTickMs;
static bool pressureControlTargetReached;
static pi_controller_t pressureControlPi;

static uint8_t PressureControl_ClampDutyPercent(uint8_t dutyPercent);
static uint8_t PressureControl_FloatToDutyPercent(float dutyPercent);
static void PressureControl_ApplyDutyPercent(uint8_t dutyPercent);
static void PressureControl_ConfigurePi(void);
static void PressureControl_EnterFault(pressure_control_fault_t fault);
static void PressureControl_StopPump(void);
static void PressureControl_SetVacuumValves(void);
static void PressureControl_SetReleaseValves(void);

void PressureControl_Init(void)
{
  pressureControlState = PRESSURE_CONTROL_STATE_IDLE;
  pressureControlFault = PRESSURE_CONTROL_FAULT_NONE;
  pressureControlTargetRelativeMbar = PRESSURE_CONTROL_DEFAULT_TARGET_RELATIVE_MBAR;
  pressureControlCommandDutyPercent = 0U;
  pressureControlLastProcessTickMs = 0U;
  pressureControlInvalidStartTickMs = 0U;
  pressureControlMaxDutyStartTickMs = 0U;
  pressureControlTargetReached = false;
  PressureControl_ConfigurePi();
}

void PressureControl_Process(void)
{
  uint32_t nowMs;
  int32_t pressureRelativeMbar;
  int32_t targetLowerMbar;
  int32_t targetUpperMbar;
  float controllerOutputDuty;
  uint8_t nextDutyPercent;

  if (pressureControlState != PRESSURE_CONTROL_STATE_RUNNING)
  {
    return;
  }

  nowMs = HAL_GetTick();
  if (!PressureSensor_IsValid())
  {
    if (pressureControlInvalidStartTickMs == 0U)
    {
      pressureControlInvalidStartTickMs = nowMs;
    }

    if ((nowMs - pressureControlInvalidStartTickMs) >= PRESSURE_CONTROL_INVALID_TIMEOUT_MS)
    {
      PressureControl_EnterFault(PRESSURE_CONTROL_FAULT_PRESSURE_INVALID);
    }

    return;
  }

  pressureControlInvalidStartTickMs = 0U;
  pressureRelativeMbar = PressureSensor_GetRelativeMbar();
  targetLowerMbar = pressureControlTargetRelativeMbar - PRESSURE_CONTROL_DEADBAND_MBAR;
  targetUpperMbar = pressureControlTargetRelativeMbar + PRESSURE_CONTROL_DEADBAND_MBAR;

  pressureControlTargetReached =
      ((pressureRelativeMbar >= targetLowerMbar) && (pressureRelativeMbar <= targetUpperMbar));

  if ((nowMs - pressureControlLastProcessTickMs) < PRESSURE_CONTROL_PERIOD_MS)
  {
    return;
  }

  pressureControlLastProcessTickMs = nowMs;
  controllerOutputDuty = PIController_Update(&pressureControlPi,
                                             (float)pressureControlTargetRelativeMbar,
                                             (float)pressureRelativeMbar);
  nextDutyPercent = PressureControl_FloatToDutyPercent(controllerOutputDuty);

  if (nextDutyPercent != pressureControlCommandDutyPercent)
  {
    PressureControl_ApplyDutyPercent(nextDutyPercent);
  }

  if ((pressureControlCommandDutyPercent >= PRESSURE_CONTROL_MAX_DUTY_PERCENT) && !pressureControlTargetReached)
  {
    if (pressureControlMaxDutyStartTickMs == 0U)
    {
      pressureControlMaxDutyStartTickMs = nowMs;
    }
    else if ((nowMs - pressureControlMaxDutyStartTickMs) >= PRESSURE_CONTROL_SATURATION_TIMEOUT_MS)
    {
      PressureControl_EnterFault(PRESSURE_CONTROL_FAULT_SATURATION_TIMEOUT);
    }
  }
  else
  {
    pressureControlMaxDutyStartTickMs = 0U;
  }
}

bool PressureControl_Start(int32_t targetRelativeMbar)
{
  uint32_t nowMs;

  if (pressureControlState == PRESSURE_CONTROL_STATE_RUNNING)
  {
    return false;
  }

  if ((targetRelativeMbar < 0) || (targetRelativeMbar > PRESSURE_CONTROL_MAX_RELATIVE_MBAR))
  {
    return false;
  }

  if (!PressureSensor_IsValid())
  {
    return false;
  }

  PressureSensor_CaptureZero();
  if (!PressureSensor_IsValid())
  {
    return false;
  }

  nowMs = HAL_GetTick();
  pressureControlState = PRESSURE_CONTROL_STATE_RUNNING;
  pressureControlFault = PRESSURE_CONTROL_FAULT_NONE;
  pressureControlTargetRelativeMbar = targetRelativeMbar;
  pressureControlCommandDutyPercent = 0U;
  pressureControlLastProcessTickMs = nowMs;
  pressureControlInvalidStartTickMs = 0U;
  pressureControlMaxDutyStartTickMs = 0U;
  pressureControlTargetReached = false;
  PressureControl_ConfigurePi();

  PumpDriver_SetTargetDutyPercent(0U);
  PumpDriver_SetEnabled(false);
  PressureControl_SetVacuumValves();

  return true;
}

void PressureControl_Stop(void)
{
  PressureControl_StopPump();
  PressureControl_SetReleaseValves();
  pressureControlState = PRESSURE_CONTROL_STATE_IDLE;
  pressureControlFault = PRESSURE_CONTROL_FAULT_NONE;
  pressureControlTargetRelativeMbar = PRESSURE_CONTROL_DEFAULT_TARGET_RELATIVE_MBAR;
  pressureControlCommandDutyPercent = 0U;
  pressureControlLastProcessTickMs = 0U;
  pressureControlInvalidStartTickMs = 0U;
  pressureControlMaxDutyStartTickMs = 0U;
  pressureControlTargetReached = false;
  PressureControl_ConfigurePi();
}

bool PressureControl_IsActive(void)
{
  return (pressureControlState == PRESSURE_CONTROL_STATE_RUNNING);
}

pressure_control_state_t PressureControl_GetState(void)
{
  return pressureControlState;
}

pressure_control_fault_t PressureControl_GetFault(void)
{
  return pressureControlFault;
}

int32_t PressureControl_GetTargetRelativeMbar(void)
{
  return pressureControlTargetRelativeMbar;
}

uint8_t PressureControl_GetCommandDutyPercent(void)
{
  return pressureControlCommandDutyPercent;
}

bool PressureControl_IsTargetReached(void)
{
  return pressureControlTargetReached;
}

static uint8_t PressureControl_ClampDutyPercent(uint8_t dutyPercent)
{
  if (dutyPercent > PRESSURE_CONTROL_MAX_DUTY_PERCENT)
  {
    return PRESSURE_CONTROL_MAX_DUTY_PERCENT;
  }

  return dutyPercent;
}

static uint8_t PressureControl_FloatToDutyPercent(float dutyPercent)
{
  if (dutyPercent <= 0.0f)
  {
    return 0U;
  }

  if (dutyPercent >= (float)PRESSURE_CONTROL_MAX_DUTY_PERCENT)
  {
    return PRESSURE_CONTROL_MAX_DUTY_PERCENT;
  }

  return (uint8_t)(dutyPercent + 0.5f);
}

static void PressureControl_ApplyDutyPercent(uint8_t dutyPercent)
{
  pressureControlCommandDutyPercent = PressureControl_ClampDutyPercent(dutyPercent);
  PumpDriver_SetTargetDutyPercent(pressureControlCommandDutyPercent);

  if (pressureControlCommandDutyPercent == 0U)
  {
    PumpDriver_SetEnabled(false);
    return;
  }

  if (!PumpDriver_IsEnabled())
  {
    PumpDriver_SetEnabled(true);
  }
}

static void PressureControl_ConfigurePi(void)
{
  pressureControlPi.Kp = PRESSURE_CONTROL_PI_KP;
  pressureControlPi.Ki = PRESSURE_CONTROL_PI_KI;
  pressureControlPi.T = PRESSURE_CONTROL_PI_SAMPLE_TIME_S;
  pressureControlPi.limMin = 0.0f;
  pressureControlPi.limMax = (float)PRESSURE_CONTROL_MAX_DUTY_PERCENT;
  pressureControlPi.limMinInt = 0.0f;
  pressureControlPi.limMaxInt = (float)PRESSURE_CONTROL_MAX_DUTY_PERCENT;
  PIController_Init(&pressureControlPi);
}

static void PressureControl_EnterFault(pressure_control_fault_t fault)
{
  PressureControl_StopPump();
  if (fault == PRESSURE_CONTROL_FAULT_MAX_PRESSURE)
  {
    PressureControl_SetVacuumValves();
  }
  else
  {
    PressureControl_SetReleaseValves();
  }

  pressureControlState = PRESSURE_CONTROL_STATE_FAULT;
  pressureControlFault = fault;
  pressureControlCommandDutyPercent = 0U;
  pressureControlInvalidStartTickMs = 0U;
  pressureControlMaxDutyStartTickMs = 0U;
  pressureControlTargetReached = false;
  PressureControl_ConfigurePi();
}

static void PressureControl_StopPump(void)
{
  PumpDriver_SetTargetDutyPercent(0U);
  PumpDriver_SetEnabled(false);
}

static void PressureControl_SetVacuumValves(void)
{
  ActuatorManager_SetValve1Enabled(false);
  ActuatorManager_SetValve2Enabled(true);
}

static void PressureControl_SetReleaseValves(void)
{
  ActuatorManager_SetValve1Enabled(true);
  ActuatorManager_SetValve2Enabled(true);
}
