#ifndef DISPLAY_BACKLIGHT_H
#define DISPLAY_BACKLIGHT_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void DisplayBacklight_Init(TIM_HandleTypeDef* timer, uint32_t channel);
void DisplayBacklight_SetPercent(uint8_t percent);
uint8_t DisplayBacklight_GetPercent(void);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_BACKLIGHT_H */
