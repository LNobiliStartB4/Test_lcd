#include "application_feedback.h"

#include <stdbool.h>

#include "application_context.h"
#include "main.h"
#include "platform.h"

#define BEP_FREQUENCY 1500U
#define BOP_FREQUENCY 300U
#define STATE_LED_BLINK_MS 500U
#define BUZZER_DURATION_MS 1000U
#define WARNING_BEEP_ON_MS         100U
#define WARNING_BEEP_GAP_MS        100U
#define WARNING_GROUP_GAP_MS       500U
#define WARNING_BEEPS_PER_GROUP    3U

extern TIM_HandleTypeDef htim2;

static void ApplicationFeedback_ProcessPowerLed(void);
static void ApplicationFeedback_ProcessStateLeds(void);
static void ApplicationFeedback_ProcessBuzzer(void);

static bool warningBuzzerActive;

void ApplicationFeedback_Process(void)
{
  ApplicationFeedback_ProcessBuzzer();
  ApplicationFeedback_ProcessStateLeds();
  ApplicationFeedback_ProcessPowerLed();
}

void ApplicationFeedback_SetWarningBuzzerActive(bool active)
{
  warningBuzzerActive = active;
}

static void ApplicationFeedback_ProcessPowerLed(void)
{
  return;
}

static void ApplicationFeedback_ProcessStateLeds(void)
{
  static uint32_t commLEDTimer = 0U;
  static bool toggleTriggered = false;
  PRO_STATION *proStation = ApplicationContext_GetProStation();

  if (proStation->rfidScanActive)
  {
    if (!toggleTriggered)
    {
      toggleTriggered = true;
      commLEDTimer = HAL_GetTick();
    }
    else if ((HAL_GetTick() - commLEDTimer) >= STATE_LED_BLINK_MS)
    {
      toggleTriggered = false;
      commLEDTimer = 0U;
      platformLedToogle(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
    }
    return;
  }

  if (proStation->rfidTagStatus == TAG_APPROVED)
  {
    platformLedOn(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
  }
  else
  {
    platformLedOff(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
  }
}

static void ApplicationFeedback_ProcessBuzzer(void)
{
  static uint32_t bepTimer = 0U;
  static bool warningBuzzerRunning = false;
  static uint32_t warningPhaseStartMs = 0U;
  static uint8_t warningBeepsDone = 0U;
  static bool warningInBeep = false;
  PRO_STATION *proStation = ApplicationContext_GetProStation();

  if (warningBuzzerActive)
  {
    uint32_t now = HAL_GetTick();

    if (!warningBuzzerRunning)
    {
      htim2.Instance->ARR = RCC_MAX_FREQUENCY / BEP_FREQUENCY;
      htim2.Instance->CCR1 = htim2.Instance->ARR / 2U;
      HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
      warningBuzzerRunning = true;
      warningPhaseStartMs = now;
      warningBeepsDone = 0U;
      warningInBeep = true;
    }
    else
    {
      uint32_t phaseDurationMs;

      if (warningInBeep)
      {
        phaseDurationMs = WARNING_BEEP_ON_MS;
      }
      else if (warningBeepsDone < WARNING_BEEPS_PER_GROUP)
      {
        phaseDurationMs = WARNING_BEEP_GAP_MS;
      }
      else
      {
        phaseDurationMs = WARNING_GROUP_GAP_MS;
      }

      if ((now - warningPhaseStartMs) >= phaseDurationMs)
      {
        warningPhaseStartMs = now;

        if (warningInBeep)
        {
          HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
          warningBeepsDone++;
          warningInBeep = false;
        }
        else
        {
          if (warningBeepsDone >= WARNING_BEEPS_PER_GROUP)
          {
            warningBeepsDone = 0U;
          }
          HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
          warningInBeep = true;
        }
      }
    }
    return;
  }

  if (warningBuzzerRunning)
  {
    warningBuzzerRunning = false;
    bepTimer = 0U;
    warningBeepsDone = 0U;
    warningInBeep = false;
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
  }

  if (proStation->bepTriggered || proStation->bopTriggered)
  {
    if (bepTimer == 0U)
    {
      bepTimer = HAL_GetTick();
      htim2.Instance->ARR = RCC_MAX_FREQUENCY / (proStation->bepTriggered ? BEP_FREQUENCY : BOP_FREQUENCY);
      htim2.Instance->CCR1 = htim2.Instance->ARR / 2U;
      HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    }
    else if ((HAL_GetTick() - bepTimer) > BUZZER_DURATION_MS)
    {
      bepTimer = 0U;
      proStation->bepTriggered = false;
      proStation->bopTriggered = false;
      HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    }
  }
}
