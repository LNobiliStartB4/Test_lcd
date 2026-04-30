/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : STM32TouchController.cpp
  ******************************************************************************
  * This file was created by TouchGFX Generator 4.26.0. This file is only
  * generated once! Delete this file from your project and re-generate code
  * using STM32CubeMX or change this file manually to update it.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* USER CODE BEGIN STM32TouchController */

#include <STM32TouchController.hpp>
#include "main.h"

volatile uint32_t newTouch = 0;
extern "C" I2C_HandleTypeDef hi2c1;

namespace
{
const uint16_t kIli2130ReadAddress = 0x83;
const uint8_t kIli2130ReportId = 0x48;
const uint8_t kIli2130ReportSize = 64;
const uint8_t kIli2130ContactCountIndex = 61;
const uint8_t kIli2130MaxTouches = 10;
const uint8_t kIli2130TouchTipMask = 0x40;
const int32_t kTouchMaxX = 479;
const int32_t kTouchMaxY = 319;

bool coordinatesAreValid(int32_t x, int32_t y)
{
    return x >= 0 && x <= kTouchMaxX && y >= 0 && y <= kTouchMaxY;
}
}

void STM32TouchController::init()
{
    HAL_GPIO_WritePin(TOUCH_RST_GPIO_Port, TOUCH_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(TOUCH_RST_GPIO_Port, TOUCH_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
}

bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y)
{
    if (newTouch)
    {
        uint8_t report[kIli2130ReportSize];

        newTouch = 0;

        if (HAL_I2C_Master_Receive(&hi2c1, kIli2130ReadAddress, report, sizeof(report), 20) != HAL_OK)
        {
            return false;
        }

        if (report[0] != kIli2130ReportId || report[kIli2130ContactCountIndex] == 0U)
        {
            return false;
        }

        for (uint8_t touchIndex = 0; touchIndex < kIli2130MaxTouches; touchIndex++)
        {
            const uint8_t offset = static_cast<uint8_t>(1U + (5U * touchIndex));
            if ((report[offset] & kIli2130TouchTipMask) == 0U)
            {
                continue;
            }

            const int32_t touchX = static_cast<int32_t>((static_cast<uint16_t>(report[offset + 2]) << 8) | report[offset + 1]);
            const int32_t touchY = static_cast<int32_t>((static_cast<uint16_t>(report[offset + 4]) << 8) | report[offset + 3]);

            if (coordinatesAreValid(touchX, touchY))
            {
                x = touchX;
                y = touchY;
                return true;
            }
        }
    }

    return false;
}

/* USER CODE END STM32TouchController */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
