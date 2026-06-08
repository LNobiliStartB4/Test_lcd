#ifndef ADC_READER_H
#define ADC_READER_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

bool Adc1_ReadSingleChannel(uint32_t channel, uint32_t samplingTime, uint16_t *adcCounts);

#ifdef __cplusplus
}
#endif

#endif
