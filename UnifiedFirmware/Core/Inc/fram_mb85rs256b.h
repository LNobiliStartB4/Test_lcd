#ifndef FRAM_MB85RS256B_H
#define FRAM_MB85RS256B_H

#include "stm32f4xx_hal.h"

#include <stddef.h>
#include <stdint.h>

#define FRAM_MB85RS256B_SIZE_BYTES (32768u)
#define FRAM_MB85RS256B_ID_LENGTH  (4u)
#define FRAM_MB85RS256B_RDID_RAW_LENGTH (5u)

typedef enum
{
  FRAM_MB85RS256B_OK = 0,
  FRAM_MB85RS256B_ERROR_PARAM,
  FRAM_MB85RS256B_ERROR_HAL
} fram_mb85rs256b_status_t;

typedef struct
{
  SPI_HandleTypeDef *spi;
  GPIO_TypeDef *cs_port;
  uint16_t cs_pin;
  uint32_t timeout_ms;
} fram_mb85rs256b_t;

fram_mb85rs256b_status_t fram_mb85rs256b_read_id(const fram_mb85rs256b_t *fram,
                                                 uint8_t id[FRAM_MB85RS256B_ID_LENGTH]);
fram_mb85rs256b_status_t fram_mb85rs256b_read_id_raw(const fram_mb85rs256b_t *fram,
                                                     uint8_t raw[FRAM_MB85RS256B_RDID_RAW_LENGTH],
                                                     uint8_t id[FRAM_MB85RS256B_ID_LENGTH]);
fram_mb85rs256b_status_t fram_mb85rs256b_read(const fram_mb85rs256b_t *fram,
                                              uint16_t address,
                                              uint8_t *data,
                                              size_t length);
fram_mb85rs256b_status_t fram_mb85rs256b_write(const fram_mb85rs256b_t *fram,
                                               uint16_t address,
                                               const uint8_t *data,
                                               size_t length);

#endif /* FRAM_MB85RS256B_H */
