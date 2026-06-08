#include "valve_controller.h"
#include "main.h"

typedef enum
{
  VALVE_CONTROLLER_STATE_OFF = 0,
  VALVE_CONTROLLER_STATE_PEAK,
  VALVE_CONTROLLER_STATE_HOLD
} ValveControllerState;

static void ValveController_Validate(valve_controller_t *controller)
{
  if ((controller == NULL) || (controller->valve == NULL))
  {
    Error_Handler();
  }
}

static void ValveController_EnterState(valve_controller_t *controller,
                                       ValveControllerState newState,
                                       uint32_t now)
{
  ValveController_Validate(controller);
  controller->state = (uint8_t)newState;
  controller->stateEntryTimeMs = now;

  switch (newState)
  {
    case VALVE_CONTROLLER_STATE_OFF:
      ValveDriver_SetDutyPercent(controller->valve, 0U);
      break;

    case VALVE_CONTROLLER_STATE_PEAK:
      ValveDriver_SetDutyPercent(controller->valve, 100U);
      break;

    case VALVE_CONTROLLER_STATE_HOLD:
      ValveDriver_SetDutyPercent(controller->valve, controller->holdDutyPercent);
      break;

    default:
      ValveDriver_SetDutyPercent(controller->valve, 0U);
      break;
  }
}

void ValveController_Init(valve_controller_t *controller,
                          const valve_t *valve,
                          uint32_t peakMs,
                          uint8_t holdDutyPercent)
{
  if (controller == NULL)
  {
    Error_Handler();
  }
  controller->valve = valve;
  controller->peakMs = peakMs;
  controller->holdDutyPercent = holdDutyPercent;
  controller->activationRequested = false;

  ValveDriver_Init(controller->valve);
  ValveController_EnterState(controller, VALVE_CONTROLLER_STATE_OFF, HAL_GetTick());
}

void ValveController_SetActivationRequest(valve_controller_t *controller, bool enabled)
{
  ValveController_Validate(controller);
  controller->activationRequested = enabled;
}

void ValveController_Process(valve_controller_t *controller)
{
  uint32_t now = HAL_GetTick();

  ValveController_Validate(controller);

  switch ((ValveControllerState)controller->state)
  {
    case VALVE_CONTROLLER_STATE_OFF:
      if (controller->activationRequested)
      {
        ValveController_EnterState(controller, VALVE_CONTROLLER_STATE_PEAK, now);
      }
      break;

    case VALVE_CONTROLLER_STATE_PEAK:
      if (!controller->activationRequested)
      {
        ValveController_EnterState(controller, VALVE_CONTROLLER_STATE_OFF, now);
      }
      else if ((uint32_t)(now - controller->stateEntryTimeMs) >= controller->peakMs)
      {
        ValveController_EnterState(controller, VALVE_CONTROLLER_STATE_HOLD, now);
      }
      break;

    case VALVE_CONTROLLER_STATE_HOLD:
      if (!controller->activationRequested)
      {
        ValveController_EnterState(controller, VALVE_CONTROLLER_STATE_OFF, now);
      }
      break;

    default:
      ValveController_EnterState(controller, VALVE_CONTROLLER_STATE_OFF, now);
      break;
  }
}
