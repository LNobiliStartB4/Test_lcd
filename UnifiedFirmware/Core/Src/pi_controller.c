#include "pi_controller.h"

void PIController_Init(pi_controller_t *controller)
{
  if (controller == 0)
  {
    return;
  }

  controller->integrator = 0.0f;
  controller->prevError = 0.0f;
  controller->out = 0.0f;
}

void PIController_SetIntegrator(pi_controller_t *controller, float integrator)
{
  if (controller == 0)
  {
    return;
  }

  controller->integrator = integrator;

  if (controller->integrator > controller->limMaxInt)
  {
    controller->integrator = controller->limMaxInt;
  }
  else if (controller->integrator < controller->limMinInt)
  {
    controller->integrator = controller->limMinInt;
  }
}

float PIController_Update(pi_controller_t *controller, float setpoint, float measurement)
{
  float error;
  float proportional;

  if (controller == 0)
  {
    return 0.0f;
  }

  error = setpoint - measurement;
  proportional = controller->Kp * error;

  controller->integrator += 0.5f * controller->Ki * controller->T * (error + controller->prevError);
  if (controller->integrator > controller->limMaxInt)
  {
    controller->integrator = controller->limMaxInt;
  }
  else if (controller->integrator < controller->limMinInt)
  {
    controller->integrator = controller->limMinInt;
  }

  controller->out = proportional + controller->integrator;
  if (controller->out > controller->limMax)
  {
    controller->out = controller->limMax;
  }
  else if (controller->out < controller->limMin)
  {
    controller->out = controller->limMin;
  }

  controller->prevError = error;

  return controller->out;
}
