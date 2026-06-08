#ifndef VALVE_DRIVER_H
#define VALVE_DRIVER_H

#include "main.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
  TIM_HandleTypeDef *timer;
  uint32_t channel;
  bool complementaryOutput;
} valve_t;

void ValveDriver_Init(const valve_t *valve);
void ValveDriver_SetDutyPercent(const valve_t *valve, uint8_t dutyPercent);
void ValveDriver_Set(const valve_t *valve, bool enabled);

#endif
