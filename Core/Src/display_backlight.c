#include "display_backlight.h"

#define DISPLAY_BACKLIGHT_MIN_PERCENT 10U
#define DISPLAY_BACKLIGHT_MAX_PERCENT 100U

static TIM_HandleTypeDef* displayBacklightTimer;
static uint32_t displayBacklightChannel;
static uint8_t displayBacklightPercent = DISPLAY_BACKLIGHT_MAX_PERCENT;

static uint8_t DisplayBacklight_ClampPercent(uint8_t percent)
{
  if (percent < DISPLAY_BACKLIGHT_MIN_PERCENT)
  {
    return DISPLAY_BACKLIGHT_MIN_PERCENT;
  }

  if (percent > DISPLAY_BACKLIGHT_MAX_PERCENT)
  {
    return DISPLAY_BACKLIGHT_MAX_PERCENT;
  }

  return percent;
}

void DisplayBacklight_Init(TIM_HandleTypeDef* timer, uint32_t channel)
{
  displayBacklightTimer = timer;
  displayBacklightChannel = channel;
  displayBacklightPercent = DISPLAY_BACKLIGHT_MAX_PERCENT;

  if (displayBacklightTimer == NULL)
  {
    return;
  }

  DisplayBacklight_SetPercent(displayBacklightPercent);
  (void)HAL_TIM_PWM_Start(displayBacklightTimer, displayBacklightChannel);
}

void DisplayBacklight_SetPercent(uint8_t percent)
{
  uint32_t period;
  uint32_t pulse;

  displayBacklightPercent = DisplayBacklight_ClampPercent(percent);

  if (displayBacklightTimer == NULL)
  {
    return;
  }

  period = __HAL_TIM_GET_AUTORELOAD(displayBacklightTimer) + 1U;
  pulse = (period * displayBacklightPercent) / DISPLAY_BACKLIGHT_MAX_PERCENT;
  __HAL_TIM_SET_COMPARE(displayBacklightTimer, displayBacklightChannel, pulse);
}

uint8_t DisplayBacklight_GetPercent(void)
{
  return displayBacklightPercent;
}
