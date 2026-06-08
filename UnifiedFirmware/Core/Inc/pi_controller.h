#ifndef PI_CONTROLLER_H
#define PI_CONTROLLER_H

typedef struct
{
  float Kp;
  float Ki;
  float limMin;
  float limMax;
  float limMinInt;
  float limMaxInt;
  float T;
  float integrator;
  float prevError;
  float out;
} pi_controller_t;

void PIController_Init(pi_controller_t *controller);
void PIController_SetIntegrator(pi_controller_t *controller, float integrator);
float PIController_Update(pi_controller_t *controller, float setpoint, float measurement);

#endif /* PI_CONTROLLER_H */
