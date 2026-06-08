#include "pump_driver.h"

#include "adc_reader.h"
#include "main.h"
#include "pressure_sensor.h"
#include "actuator_manager.h"
#define PUMP_CONTROL_PERIOD_MS              1U
#define PUMP_DUTY_MAX_PERCENT               100U
#define PUMP_DUTY_MIN_PERCENT               0U
#define PUMP_DUTY_LIMIT_PERCENT             67U

/* STEP ESPRESSI IN COUNT TIMER, NON IN PERCENTUALE */
#define PUMP_DUTY_RAMP_STEP_COUNTS          2U
#define PUMP_CURRENT_LIMIT_STEP_COUNTS      10U

#define PUMP_CURRENT_FILTER_SAMPLES         4U
#define PUMP_CURRENT_ADC_REF_MV             3300U
#define PUMP_CURRENT_ADC_MAX_COUNTS         4095U
#define PUMP_CURRENT_SAFETY_LIMIT_MA        700U
#define PUMP_CURRENT_SHUNT_MILLIOHM         25U
#define PUMP_CURRENT_SENSE_GAIN             200U

#define PUMP_PRESSURE_TARGET_MIN_MBAR         290
#define PUMP_BANDY_USER_PRESSURE_TARGET_MBAR  490
#define PUMP_BANDY_PRESSURE_CONTROL_MARGIN_MBAR 30
#define PUMP_BANDY_PRESSURE_RESTART_MARGIN_MBAR 50
#define PUMP_CURRENT_MAX_MEASURABLE_MA \
  ((PUMP_CURRENT_ADC_REF_MV * 1000U) / (PUMP_CURRENT_SENSE_GAIN * PUMP_CURRENT_SHUNT_MILLIOHM))

extern TIM_HandleTypeDef htim3;

typedef enum
{
  PUMP_CTRL_STATE_IDLE = 0,
  PUMP_CTRL_STATE_RAMP_DUTY,
  PUMP_CTRL_STATE_HOLD_DUTY,
  PUMP_CTRL_STATE_VACUUM
}
  pump_control_state_t;

static bool pumpEnabled;
static uint16_t pumpDuty;                 /* compare timer */
static uint8_t pumpRequestedDutyPercent;  /* target richiesto in percentuale */

static uint16_t pumpCurrentRawAdc;
static uint16_t pumpCurrentmA;
static uint16_t pumpFilteredCurrentmA;
static uint16_t pumpCurrentSamples[PUMP_CURRENT_FILTER_SAMPLES];
static uint32_t pumpCurrentSampleSum;
static uint8_t pumpCurrentSampleIndex;
static uint8_t pumpCurrentSampleCount;
static uint32_t lastControlTickMs;
static pump_control_state_t pumpControlState;
static uint16_t targetCompare;
static uint16_t dutyLimitCompare;
static int32_t pumpUserPressureTargetMbar;
static int32_t pumpPressureControlTargetMbar;
static int32_t pumpPressureRestartTargetMbar;
static bool pumpVacuumHoldOffActive;

static int32_t PumpDriver_UserTargetToControlTargetMbar(int32_t userTargetMbar)
{
  return userTargetMbar - (int32_t)PUMP_BANDY_PRESSURE_CONTROL_MARGIN_MBAR;
}

static int32_t PumpDriver_UserTargetToRestartTargetMbar(int32_t userTargetMbar)
{
  return userTargetMbar - (int32_t)PUMP_BANDY_PRESSURE_RESTART_MARGIN_MBAR;
}

static uint16_t PumpDriver_DutyToCompare(uint8_t dutyPercent)
{
  uint32_t periodCounts = __HAL_TIM_GET_AUTORELOAD(&htim3) + 1U;
  return (uint16_t)(((uint32_t)dutyPercent * periodCounts) / PUMP_DUTY_MAX_PERCENT);
}

static uint8_t PumpDriver_ClampDutyPercent(uint8_t dutyPercent)
{
  if (dutyPercent > PUMP_DUTY_LIMIT_PERCENT)
  {
    return PUMP_DUTY_LIMIT_PERCENT;
  }

  if (dutyPercent < PUMP_DUTY_MIN_PERCENT)
  {
    return PUMP_DUTY_MIN_PERCENT;
  }

  return dutyPercent;
}

static void PumpDriver_ResetCurrentFilter(void)
{
  uint8_t i;

  for (i = 0U; i < PUMP_CURRENT_FILTER_SAMPLES; i++)
  {
    pumpCurrentSamples[i] = 0U;
  }

  pumpCurrentSampleSum = 0U;
  pumpCurrentSampleIndex = 0U;
  pumpCurrentSampleCount = 0U;
  pumpCurrentRawAdc = 0U;
  pumpCurrentmA = 0U;
  pumpFilteredCurrentmA = 0U;
}

static bool PumpDriver_ReadCurrentAdc(uint16_t *adcCounts)
{
  return Adc1_ReadSingleChannel(ADC_CHANNEL_10, ADC_SAMPLETIME_3CYCLES, adcCounts);
}

static uint16_t PumpDriver_AdcToCurrentmA(uint16_t adcCounts)
{
  uint32_t pinmV;

  pinmV = ((uint32_t)adcCounts * PUMP_CURRENT_ADC_REF_MV) / PUMP_CURRENT_ADC_MAX_COUNTS;
  pinmV = (pinmV * 1000U) / (PUMP_CURRENT_SENSE_GAIN * PUMP_CURRENT_SHUNT_MILLIOHM);

  return (uint16_t)pinmV;
}

static uint16_t PumpDriver_FilterCurrentSample(uint16_t samplemA)
{
  pumpCurrentSampleSum -= pumpCurrentSamples[pumpCurrentSampleIndex];
  pumpCurrentSamples[pumpCurrentSampleIndex] = samplemA;
  pumpCurrentSampleSum += samplemA;

  if (pumpCurrentSampleCount < PUMP_CURRENT_FILTER_SAMPLES)
  {
    pumpCurrentSampleCount++;
  }

  pumpCurrentSampleIndex++;
  if (pumpCurrentSampleIndex >= PUMP_CURRENT_FILTER_SAMPLES)
  {
    pumpCurrentSampleIndex = 0U;
  }

  return (uint16_t)(pumpCurrentSampleSum / pumpCurrentSampleCount);
}

static void PumpDriver_ApplyOutput(void)
{

  if (pumpEnabled && (pumpDuty > 0U))
  {
    htim3.Instance->CCR3 = pumpDuty;
  }
  else
  {
    htim3.Instance->CCR3 = 0U;
  }

}

void PumpDriver_Init(void)
{
  pumpEnabled = false;
  pumpDuty = 0U;
  pumpRequestedDutyPercent = 0U;
  lastControlTickMs = HAL_GetTick();
  pumpControlState = PUMP_CTRL_STATE_IDLE;
  pumpVacuumHoldOffActive = false;
  pumpUserPressureTargetMbar = PUMP_BANDY_USER_PRESSURE_TARGET_MBAR;
  pumpPressureControlTargetMbar = PumpDriver_UserTargetToControlTargetMbar(pumpUserPressureTargetMbar);
  pumpPressureRestartTargetMbar = PumpDriver_UserTargetToRestartTargetMbar(pumpUserPressureTargetMbar);
  PumpDriver_ResetCurrentFilter();
  dutyLimitCompare = PumpDriver_DutyToCompare(PUMP_DUTY_LIMIT_PERCENT);
}

void PumpDriver_Process(void)
{
  uint16_t adcCounts;
  uint32_t now = HAL_GetTick();

  if (!pumpEnabled)
  {
    return;
  }

  if ((now - lastControlTickMs) < PUMP_CONTROL_PERIOD_MS)
  {
    return;
  }

  lastControlTickMs = now;

  if (!PumpDriver_ReadCurrentAdc(&adcCounts))
  {
    PumpDriver_ResetCurrentFilter();
    pumpEnabled = false;
    pumpDuty = 0U;
    pumpControlState = PUMP_CTRL_STATE_IDLE;
    pumpVacuumHoldOffActive = false;
    PumpDriver_ApplyOutput();
    return;
  }

  pumpCurrentRawAdc = adcCounts;
  pumpCurrentmA = PumpDriver_AdcToCurrentmA(adcCounts);
  pumpFilteredCurrentmA = PumpDriver_FilterCurrentSample(pumpCurrentmA);







  switch (pumpControlState)
  {
    case PUMP_CTRL_STATE_IDLE:
      pumpDuty = 0U;
      break;




    case PUMP_CTRL_STATE_RAMP_DUTY:
      if (pumpDuty < targetCompare)
      {
        uint32_t nextDuty = (uint32_t)pumpDuty + PUMP_DUTY_RAMP_STEP_COUNTS;

        if (nextDuty >= targetCompare)
        {
          pumpDuty = targetCompare;
          pumpControlState = PUMP_CTRL_STATE_HOLD_DUTY;
        }
        else
        {
          pumpDuty = (uint16_t)nextDuty;
        }
      }

      break;

    case PUMP_CTRL_STATE_HOLD_DUTY:
      break;
    case PUMP_CTRL_STATE_VACUUM:
    {
      /* Il controllo usa la misura filtrata per evitare chatter sulla soglia.
       * La pressione raw resta disponibile solo per telemetria/display. */
      int32_t pressureRelativeMbar = PressureSensor_GetRelativeMbar();

      if (pressureRelativeMbar >= pumpPressureControlTargetMbar)
      {
        pumpDuty = 0U;
        pumpVacuumHoldOffActive = true;
        ActuatorManager_SetValve2Enabled(false);
      }
      else if (pressureRelativeMbar < pumpPressureRestartTargetMbar)
      {
        pumpVacuumHoldOffActive = false;
        pumpDuty++;
        ActuatorManager_SetValve2Enabled(true);
      }
      else if (pumpVacuumHoldOffActive)
      {
        pumpDuty = 0U;
        ActuatorManager_SetValve2Enabled(false);
      }
      else
      {
        ActuatorManager_SetValve2Enabled(true);
      }
      break;
    }

    default:
      pumpDuty = 0U;
      pumpControlState = PUMP_CTRL_STATE_IDLE;
      pumpVacuumHoldOffActive = false;
      break;
  }
/*
 * Duty Protection
 */
  if (pumpDuty > dutyLimitCompare)
  {
    pumpDuty = dutyLimitCompare;
  }

  /*
   * Current Protection
   */
  if (pumpCurrentmA > PUMP_CURRENT_SAFETY_LIMIT_MA)
       {
         if (pumpDuty > PUMP_CURRENT_LIMIT_STEP_COUNTS)
         {
           pumpDuty -= PUMP_CURRENT_LIMIT_STEP_COUNTS;
         }
         else
         {
           pumpDuty = 0U;
         }
       }
  PumpDriver_ApplyOutput();
}

void PumpDriver_SetEnabled(bool enabled)
{
  if (enabled == pumpEnabled)
  {
    return;
  }

  pumpEnabled = enabled;

  if (!enabled)
  {
    pumpDuty = 0U;
    pumpControlState = PUMP_CTRL_STATE_IDLE;
    pumpVacuumHoldOffActive = false;
    PumpDriver_ResetCurrentFilter();
    PumpDriver_ApplyOutput();
    return;
  }

  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  lastControlTickMs = HAL_GetTick();
  pumpDuty = 0U;
  PumpDriver_ResetCurrentFilter();

  if (pumpRequestedDutyPercent > 0U)
  {
    pumpControlState = PUMP_CTRL_STATE_RAMP_DUTY;
  }
  else
  {
    pumpControlState = PUMP_CTRL_STATE_IDLE;
  }

  PumpDriver_ApplyOutput();
}

void PumpDriver_SetTargetDutyPercent(uint8_t targetDutyPercent)
{
  uint8_t clampedDuty;


  clampedDuty = PumpDriver_ClampDutyPercent(targetDutyPercent);

  if (clampedDuty == pumpRequestedDutyPercent)
  {
    return;
  }

  pumpRequestedDutyPercent = clampedDuty;
  targetCompare = PumpDriver_DutyToCompare(pumpRequestedDutyPercent);

  if (!pumpEnabled)
  {
    return;
  }

  if (targetCompare == 0U)
  {
    pumpDuty = 0U;
    pumpControlState = PUMP_CTRL_STATE_IDLE;
    pumpVacuumHoldOffActive = false;
    PumpDriver_ApplyOutput();
    return;
  }

  pumpVacuumHoldOffActive = false;

  if (pumpDuty >= targetCompare)
  {
    pumpDuty = targetCompare;
    pumpControlState = PUMP_CTRL_STATE_HOLD_DUTY;
    PumpDriver_ApplyOutput();
  }
  else
  {
    pumpControlState = PUMP_CTRL_STATE_RAMP_DUTY;
  }

}

void PumpDriver_StartVacuumTest(void)
{
  pumpVacuumHoldOffActive = false;
  pumpControlState = PUMP_CTRL_STATE_VACUUM;
}

bool PumpDriver_SetPressureTargetMbar(int32_t targetMbar)
{
  if ((targetMbar < PUMP_PRESSURE_TARGET_MIN_MBAR) ||
      (targetMbar > PUMP_BANDY_USER_PRESSURE_TARGET_MBAR))
  {
    return false;
  }

  pumpUserPressureTargetMbar = targetMbar;
  pumpPressureControlTargetMbar = PumpDriver_UserTargetToControlTargetMbar(targetMbar);
  pumpPressureRestartTargetMbar = PumpDriver_UserTargetToRestartTargetMbar(targetMbar);
  return true;
}

bool PumpDriver_SetPressureProfileMbar(int32_t userTargetMbar, int32_t controlTargetMbar, int32_t restartTargetMbar)
{
  if ((userTargetMbar < 0) ||
      (userTargetMbar > PUMP_BANDY_USER_PRESSURE_TARGET_MBAR) ||
      (controlTargetMbar < 0) ||
      (controlTargetMbar > userTargetMbar) ||
      (restartTargetMbar < 0) ||
      (restartTargetMbar > controlTargetMbar))
  {
    return false;
  }

  pumpUserPressureTargetMbar = userTargetMbar;
  pumpPressureControlTargetMbar = controlTargetMbar;
  pumpPressureRestartTargetMbar = restartTargetMbar;
  return true;
}

bool PumpDriver_IsEnabled(void)
{
  return pumpEnabled;
}

bool PumpDriver_IsVacuumTestActive(void)
{
  return (pumpEnabled && (pumpControlState == PUMP_CTRL_STATE_VACUUM));
}

uint16_t PumpDriver_GetCurrentRawAdc(void)
{
  return pumpCurrentRawAdc;
}

uint16_t PumpDriver_GetCurrentmA(void)
{
  return pumpCurrentmA;
}

uint16_t PumpDriver_GetFilteredCurrentmA(void)
{
  return pumpFilteredCurrentmA;
}

/* Restituisce la duty in percentuale per compatibilità API */
uint8_t PumpDriver_GetDuty(void)
{
  uint32_t periodCounts = __HAL_TIM_GET_AUTORELOAD(&htim3) + 1U;
  return (periodCounts > 0U)
           ? (uint8_t)(((uint32_t)pumpDuty * PUMP_DUTY_MAX_PERCENT) / periodCounts)
           : 0U;
}

uint8_t PumpDriver_GetDutyLimitPercent(void)
{
  return PUMP_DUTY_LIMIT_PERCENT;
}

int32_t PumpDriver_GetPressureStopMbar(void)
{
  return pumpPressureControlTargetMbar;
}

int32_t PumpDriver_GetPressureRestartMbar(void)
{
  return pumpPressureRestartTargetMbar;
}

int32_t PumpDriver_GetPressureTargetMinMbar(void)
{
  return PUMP_PRESSURE_TARGET_MIN_MBAR;
}

int32_t PumpDriver_GetPressureTargetMaxMbar(void)
{
  return PUMP_BANDY_USER_PRESSURE_TARGET_MBAR;
}

int32_t PumpDriver_GetUserPressureTargetMbar(void)
{
  return pumpUserPressureTargetMbar;
}

int32_t PumpDriver_GetPressureControlTargetMbar(void)
{
  return pumpPressureControlTargetMbar;
}

uint16_t PumpDriver_GetMaxMeasurableCurrentmA(void)
{
  return PUMP_CURRENT_MAX_MEASURABLE_MA;
}
