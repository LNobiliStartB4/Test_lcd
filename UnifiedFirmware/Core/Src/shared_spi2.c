#include "shared_spi2.h"

#include "main.h"

extern SPI_HandleTypeDef hspi2;

static volatile shared_spi2_device_t selectedDevice;

static void SharedSpi2_SetChipSelect(shared_spi2_device_t device, GPIO_PinState state)
{
  if (device == SHARED_SPI2_DEVICE_RFID)
  {
    HAL_GPIO_WritePin(RFID_CS_GPIO_Port, RFID_CS_Pin, state);
  }
  else if (device == SHARED_SPI2_DEVICE_FRAM)
  {
    HAL_GPIO_WritePin(FRAM_CS_GPIO_Port, FRAM_CS_Pin, state);
  }
}

void SharedSpi2_Init(void)
{
  HAL_GPIO_WritePin(RFID_CS_GPIO_Port, RFID_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(FRAM_CS_GPIO_Port, FRAM_CS_Pin, GPIO_PIN_SET);
  selectedDevice = SHARED_SPI2_DEVICE_NONE;
}

bool SharedSpi2_Select(shared_spi2_device_t device, uint32_t timeoutMs)
{
  uint32_t startMs;

  if ((device != SHARED_SPI2_DEVICE_RFID) && (device != SHARED_SPI2_DEVICE_FRAM))
  {
    return false;
  }

  startMs = HAL_GetTick();
  for (;;)
  {
    bool acquired = false;

    __disable_irq();
    if (selectedDevice == SHARED_SPI2_DEVICE_NONE)
    {
      selectedDevice = device;
      acquired = true;
    }
    __enable_irq();

    if (acquired)
    {
      SharedSpi2_SetChipSelect(device, GPIO_PIN_RESET);
      return true;
    }

    if ((timeoutMs == 0U) || ((uint32_t)(HAL_GetTick() - startMs) >= timeoutMs))
    {
      return false;
    }
  }
}

void SharedSpi2_Deselect(shared_spi2_device_t device)
{
  SharedSpi2_SetChipSelect(device, GPIO_PIN_SET);

  __disable_irq();
  if (selectedDevice == device)
  {
    selectedDevice = SHARED_SPI2_DEVICE_NONE;
  }
  __enable_irq();
}

HAL_StatusTypeDef SharedSpi2_Transmit(const uint8_t *data, uint16_t length, uint32_t timeoutMs)
{
  if ((selectedDevice == SHARED_SPI2_DEVICE_NONE) || (data == NULL) || (length == 0U))
  {
    return HAL_ERROR;
  }

  return HAL_SPI_Transmit(&hspi2, (uint8_t *)data, length, timeoutMs);
}

HAL_StatusTypeDef SharedSpi2_Receive(uint8_t *data, uint16_t length, uint32_t timeoutMs)
{
  if ((selectedDevice == SHARED_SPI2_DEVICE_NONE) || (data == NULL) || (length == 0U))
  {
    return HAL_ERROR;
  }

  return HAL_SPI_Receive(&hspi2, data, length, timeoutMs);
}

HAL_StatusTypeDef SharedSpi2_TransmitReceive(const uint8_t *txData,
                                             uint8_t *rxData,
                                             uint16_t length,
                                             uint32_t timeoutMs)
{
  if ((selectedDevice == SHARED_SPI2_DEVICE_NONE) ||
      (txData == NULL) ||
      (rxData == NULL) ||
      (length == 0U))
  {
    return HAL_ERROR;
  }

  return HAL_SPI_TransmitReceive(&hspi2, (uint8_t *)txData, rxData, length, timeoutMs);
}
