#include "valve_driver.h"

#include "main.h"

static bool ValveDriver_IsConnected(const valve_t *valve)
{
  return (valve != NULL) && (valve->timer != NULL);
}

static uint32_t ValveDriver_DutyPercentToCompare(const valve_t *valve, uint8_t dutyPercent)
{
  uint32_t autoreload;

  if (!ValveDriver_IsConnected(valve)) return 0U;
  autoreload = __HAL_TIM_GET_AUTORELOAD(valve->timer);
  if (dutyPercent >= 100U) return autoreload + 1U;
  return ((autoreload + 1U) * dutyPercent) / 100U;
}

void ValveDriver_Init(const valve_t *valve)
{
  if (!ValveDriver_IsConnected(valve)) return;
  ValveDriver_SetDutyPercent(valve, 0U);
  if (valve->complementaryOutput)
  {
    if (HAL_TIMEx_PWMN_Start(valve->timer, valve->channel) != HAL_OK) Error_Handler();
  }
  else
  {
    if (HAL_TIM_PWM_Start(valve->timer, valve->channel) != HAL_OK) Error_Handler();
  }
}

void ValveDriver_SetDutyPercent(const valve_t *valve, uint8_t dutyPercent)
{
  if (!ValveDriver_IsConnected(valve)) return;
  __HAL_TIM_SET_COMPARE(valve->timer, valve->channel, ValveDriver_DutyPercentToCompare(valve, dutyPercent));
}

void ValveDriver_Set(const valve_t *valve, bool enabled)
{
  ValveDriver_SetDutyPercent(valve, enabled ? 100U : 0U);
}
