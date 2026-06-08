#ifndef VALVE_CONTROLLER_H
#define VALVE_CONTROLLER_H

#include "valve_driver.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  const valve_t *valve;
  uint32_t peakMs;
  uint32_t stateEntryTimeMs;
  bool activationRequested;
  uint8_t holdDutyPercent;
  uint8_t state;
} valve_controller_t;

void ValveController_Init(valve_controller_t *controller,
                          const valve_t *valve,
                          uint32_t peakMs,
                          uint8_t holdDutyPercent);
void ValveController_Process(valve_controller_t *controller);
void ValveController_SetActivationRequest(valve_controller_t *controller,
                                          bool enabled);

#endif
