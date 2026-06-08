#include "actuator_manager.h"
#include "actuator_hw.h"
#include "pump_driver.h"
#include "valve_controller.h"

static valve_controller_t valve1Controller;
static valve_controller_t valve2Controller;
static valve_controller_t valve3Controller;
static bool valve1Enabled;
static bool valve2Enabled;
static bool valve3Enabled;

void ActuatorManager_Init(void)
{
  PumpDriver_Init();
  valve1Enabled = true;
  valve2Enabled = true;
  valve3Enabled = false;
  ValveController_Init(&valve1Controller, &valve1, 200U, 50U);
  ValveController_Init(&valve2Controller, &valve2, 200U, 50U);
  ValveController_Init(&valve3Controller, &valve3, 200U, 50U);
}

void ActuatorManager_SetValve1Enabled(bool enabled) { valve1Enabled = enabled; }
void ActuatorManager_SetValve2Enabled(bool enabled) { valve2Enabled = enabled; }
void ActuatorManager_SetValve3Enabled(bool enabled) { valve3Enabled = enabled; }
bool ActuatorManager_IsValve1Enabled(void) { return valve1Enabled; }
bool ActuatorManager_IsValve2Enabled(void) { return valve2Enabled; }
bool ActuatorManager_IsValve3Enabled(void) { return valve3Enabled; }
void ActuatorManager_SetPumpEnabled(bool enabled) { PumpDriver_SetEnabled(enabled); }

void ActuatorManager_Process(void)
{
  ValveController_SetActivationRequest(&valve1Controller, valve1Enabled);
  ValveController_SetActivationRequest(&valve2Controller, valve2Enabled);
  ValveController_SetActivationRequest(&valve3Controller, valve3Enabled);
  ValveController_Process(&valve1Controller);
  ValveController_Process(&valve2Controller);
  ValveController_Process(&valve3Controller);
  PumpDriver_Process();
}
