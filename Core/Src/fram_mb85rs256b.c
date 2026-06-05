#include "fram_mb85rs256b.h"

#define FRAM_CMD_WREN  (0x06u)
#define FRAM_CMD_READ  (0x03u)
#define FRAM_CMD_WRITE (0x02u)
#define FRAM_CMD_RDID  (0x9Fu)

static void fram_select(const fram_mb85rs256b_t *fram)
{
  HAL_GPIO_WritePin(fram->cs_port, fram->cs_pin, GPIO_PIN_RESET);
}

static void fram_deselect(const fram_mb85rs256b_t *fram)
{
  HAL_GPIO_WritePin(fram->cs_port, fram->cs_pin, GPIO_PIN_SET);
}

static fram_mb85rs256b_status_t fram_validate(const fram_mb85rs256b_t *fram)
{
  if ((fram == NULL) || (fram->spi == NULL) || (fram->cs_port == NULL))
  {
    return FRAM_MB85RS256B_ERROR_PARAM;
  }

  return FRAM_MB85RS256B_OK;
}

static fram_mb85rs256b_status_t fram_write_enable(const fram_mb85rs256b_t *fram)
{
  uint8_t command = FRAM_CMD_WREN;

  fram_select(fram);
  HAL_StatusTypeDef status = HAL_SPI_Transmit(fram->spi, &command, 1u, fram->timeout_ms);
  fram_deselect(fram);

  return (status == HAL_OK) ? FRAM_MB85RS256B_OK : FRAM_MB85RS256B_ERROR_HAL;
}

fram_mb85rs256b_status_t fram_mb85rs256b_read_id(const fram_mb85rs256b_t *fram,
                                                 uint8_t id[FRAM_MB85RS256B_ID_LENGTH])
{
  uint8_t raw[FRAM_MB85RS256B_RDID_RAW_LENGTH] = {0};

  return fram_mb85rs256b_read_id_raw(fram, raw, id);
}

fram_mb85rs256b_status_t fram_mb85rs256b_read_id_raw(const fram_mb85rs256b_t *fram,
                                                     uint8_t raw[FRAM_MB85RS256B_RDID_RAW_LENGTH],
                                                     uint8_t id[FRAM_MB85RS256B_ID_LENGTH])
{
  fram_mb85rs256b_status_t validation = fram_validate(fram);
  if ((validation != FRAM_MB85RS256B_OK) || (raw == NULL) || (id == NULL))
  {
    return FRAM_MB85RS256B_ERROR_PARAM;
  }

  uint8_t tx[FRAM_MB85RS256B_RDID_RAW_LENGTH] =
  {
    FRAM_CMD_RDID,
    0xFFu,
    0xFFu,
    0xFFu,
    0xFFu
  };

  fram_select(fram);
  HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(fram->spi,
                                                     tx,
                                                     raw,
                                                     FRAM_MB85RS256B_RDID_RAW_LENGTH,
                                                     fram->timeout_ms);
  fram_deselect(fram);

  if (status == HAL_OK)
  {
    id[0] = raw[1];
    id[1] = raw[2];
    id[2] = raw[3];
    id[3] = raw[4];
  }

  return (status == HAL_OK) ? FRAM_MB85RS256B_OK : FRAM_MB85RS256B_ERROR_HAL;
}

fram_mb85rs256b_status_t fram_mb85rs256b_read(const fram_mb85rs256b_t *fram,
                                              uint16_t address,
                                              uint8_t *data,
                                              size_t length)
{
  fram_mb85rs256b_status_t validation = fram_validate(fram);
  if ((validation != FRAM_MB85RS256B_OK) || (data == NULL))
  {
    return FRAM_MB85RS256B_ERROR_PARAM;
  }

  if (((uint32_t)address + length) > FRAM_MB85RS256B_SIZE_BYTES)
  {
    return FRAM_MB85RS256B_ERROR_PARAM;
  }

  if (length == 0u)
  {
    return FRAM_MB85RS256B_OK;
  }

  uint8_t header[3] =
  {
    FRAM_CMD_READ,
    (uint8_t)(address >> 8),
    (uint8_t)(address & 0xFFu)
  };

  fram_select(fram);
  HAL_StatusTypeDef status = HAL_SPI_Transmit(fram->spi, header, sizeof(header), fram->timeout_ms);
  if (status == HAL_OK)
  {
    status = HAL_SPI_Receive(fram->spi, data, (uint16_t)length, fram->timeout_ms);
  }
  fram_deselect(fram);

  return (status == HAL_OK) ? FRAM_MB85RS256B_OK : FRAM_MB85RS256B_ERROR_HAL;
}

fram_mb85rs256b_status_t fram_mb85rs256b_write(const fram_mb85rs256b_t *fram,
                                               uint16_t address,
                                               const uint8_t *data,
                                               size_t length)
{
  fram_mb85rs256b_status_t validation = fram_validate(fram);
  if ((validation != FRAM_MB85RS256B_OK) || (data == NULL))
  {
    return FRAM_MB85RS256B_ERROR_PARAM;
  }

  if (((uint32_t)address + length) > FRAM_MB85RS256B_SIZE_BYTES)
  {
    return FRAM_MB85RS256B_ERROR_PARAM;
  }

  if (length == 0u)
  {
    return FRAM_MB85RS256B_OK;
  }

  fram_mb85rs256b_status_t wren_status = fram_write_enable(fram);
  if (wren_status != FRAM_MB85RS256B_OK)
  {
    return wren_status;
  }

  uint8_t header[3] =
  {
    FRAM_CMD_WRITE,
    (uint8_t)(address >> 8),
    (uint8_t)(address & 0xFFu)
  };

  fram_select(fram);
  HAL_StatusTypeDef status = HAL_SPI_Transmit(fram->spi, header, sizeof(header), fram->timeout_ms);
  if (status == HAL_OK)
  {
    status = HAL_SPI_Transmit(fram->spi, (uint8_t *)data, (uint16_t)length, fram->timeout_ms);
  }
  fram_deselect(fram);

  return (status == HAL_OK) ? FRAM_MB85RS256B_OK : FRAM_MB85RS256B_ERROR_HAL;
}
