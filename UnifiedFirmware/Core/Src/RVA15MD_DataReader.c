#include "RVA15MD_DataReader.h"

#include "w25q128jv.h"

uint32_t DataReader_IsReceivingData(void)
{
  return W25Q128JV_IsDmaReadActive() ? 1U : 0U;
}

void DataReader_WaitForReceiveDone(void)
{
  W25Q128JV_WaitForDmaRead();
}

void DataReader_ReadData(uint32_t address24, uint8_t *buffer, uint32_t length)
{
  (void)W25Q128JV_Read(address24, buffer, length);
}

void DataReader_StartDMAReadData(uint32_t address24, uint8_t *buffer, uint32_t length)
{
  if (!W25Q128JV_StartDmaRead(address24, buffer, length))
  {
    (void)W25Q128JV_Read(address24, buffer, length);
  }
}

void DataReader_DMACallback(void)
{
  W25Q128JV_DmaCompleteCallback();
}
