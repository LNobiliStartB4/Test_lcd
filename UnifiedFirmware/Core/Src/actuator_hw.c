#include "actuator_hw.h"

extern TIM_HandleTypeDef htim3;

const valve_t valve1 = { .timer = &htim3, .channel = TIM_CHANNEL_1, .complementaryOutput = false };
const valve_t valve2 = { .timer = &htim3, .channel = TIM_CHANNEL_2, .complementaryOutput = false };
const valve_t valve3 = { .timer = &htim3, .channel = TIM_CHANNEL_4, .complementaryOutput = false };
