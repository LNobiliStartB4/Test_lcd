#include "discharge_controller.h"

#include "actuator_manager.h"

#define DISCHARGE_ATMOSPHERE_THRESHOLD_MBAR 10
#define DISCHARGE_STABLE_TIME_MS 500U
#define DISCHARGE_SAFETY_TIMEOUT_MS 5000U

static bool releasing;
static bool atmosphereStable;
static uint32_t releaseStartMs;
static uint32_t atmosphereStartMs;

void DischargeController_Init(void)
{
  releasing = false;
  atmosphereStable = false;
  releaseStartMs = 0U;
  atmosphereStartMs = 0U;
  ActuatorManager_SetValve3Enabled(false);
}

void DischargeController_Close(void)
{
  releasing = false;
  atmosphereStable = false;
  ActuatorManager_SetValve3Enabled(false);
}

void DischargeController_StartRelease(uint32_t nowMs)
{
  releasing = true;
  atmosphereStable = false;
  releaseStartMs = nowMs;
  atmosphereStartMs = nowMs;
  ActuatorManager_SetValve3Enabled(true);
}

void DischargeController_Process(uint32_t nowMs, bool pressureValid, int32_t pressureMbar)
{
  if (!releasing)
  {
    return;
  }

  if (pressureValid && (pressureMbar <= DISCHARGE_ATMOSPHERE_THRESHOLD_MBAR))
  {
    if (!atmosphereStable)
    {
      atmosphereStable = true;
      atmosphereStartMs = nowMs;
    }
    else if ((uint32_t)(nowMs - atmosphereStartMs) >= DISCHARGE_STABLE_TIME_MS)
    {
      DischargeController_Close();
      return;
    }
  }
  else
  {
    atmosphereStable = false;
  }

  if ((uint32_t)(nowMs - releaseStartMs) >= DISCHARGE_SAFETY_TIMEOUT_MS)
  {
    DischargeController_Close();
  }
}

bool DischargeController_IsReleasing(void)
{
  return releasing;
}
