#include "adc_reader.h"

#include "main.h"

extern ADC_HandleTypeDef hadc1;

bool Adc1_ReadSingleChannel(uint32_t channel, uint32_t samplingTime, uint16_t *adcCounts)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  sConfig.Channel = channel;
  sConfig.Rank = 1;
  sConfig.SamplingTime = samplingTime;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    return false;
  }
  if (HAL_ADC_Start(&hadc1) != HAL_OK)
  {
    return false;
  }
  if (HAL_ADC_PollForConversion(&hadc1, 1U) != HAL_OK)
  {
    (void)HAL_ADC_Stop(&hadc1);
    return false;
  }

  *adcCounts = (uint16_t)HAL_ADC_GetValue(&hadc1);
  (void)HAL_ADC_Stop(&hadc1);
  return true;
}
