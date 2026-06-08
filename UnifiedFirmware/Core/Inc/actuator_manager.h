#ifndef ACTUATOR_MANAGER_H
#define ACTUATOR_MANAGER_H

#include <stdbool.h>

void ActuatorManager_Init(void);
void ActuatorManager_Process(void);
void ActuatorManager_SetValve1Enabled(bool enabled);
void ActuatorManager_SetValve2Enabled(bool enabled);
void ActuatorManager_SetValve3Enabled(bool enabled);
bool ActuatorManager_IsValve1Enabled(void);
bool ActuatorManager_IsValve2Enabled(void);
bool ActuatorManager_IsValve3Enabled(void);
void ActuatorManager_SetPumpEnabled(bool enabled);

#endif
