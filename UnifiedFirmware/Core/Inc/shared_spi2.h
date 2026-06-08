#ifndef SHARED_SPI2_H
#define SHARED_SPI2_H

#include "stm32f4xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  SHARED_SPI2_DEVICE_NONE = 0,
  SHARED_SPI2_DEVICE_RFID,
  SHARED_SPI2_DEVICE_FRAM
} shared_spi2_device_t;

void SharedSpi2_Init(void);
bool SharedSpi2_Select(shared_spi2_device_t device, uint32_t timeoutMs);
void SharedSpi2_Deselect(shared_spi2_device_t device);
HAL_StatusTypeDef SharedSpi2_Transmit(const uint8_t *data, uint16_t length, uint32_t timeoutMs);
HAL_StatusTypeDef SharedSpi2_Receive(uint8_t *data, uint16_t length, uint32_t timeoutMs);
HAL_StatusTypeDef SharedSpi2_TransmitReceive(const uint8_t *txData,
                                             uint8_t *rxData,
                                             uint16_t length,
                                             uint32_t timeoutMs);

#ifdef __cplusplus
}
#endif

#endif /* SHARED_SPI2_H */
