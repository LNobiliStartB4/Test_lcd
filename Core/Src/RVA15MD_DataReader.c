/*
 * RVA15MD_DataReader.c
 *
 * TouchGFX FlashDataReader bridge to the external Winbond W25Q64 (SPI3).
 * The TouchGFX framework (TouchGFXDataReader) calls these C entry points to
 * fetch image assets that are linked at the virtual base 0x90000000
 * (ExtFlashSection) but physically stored in the external flash.
 */

#include "RVA15MD_DataReader.h"

#include "w25q64.h"

uint32_t DataReader_IsReceivingData(void)
{
  return W25Q64_IsDmaReadActive() ? 1U : 0U;
}

void DataReader_WaitForReceiveDone(void)
{
  W25Q64_WaitForDmaRead();
}

void DataReader_ReadData(uint32_t address24, uint8_t *buffer, uint32_t length)
{
  (void)W25Q64_Read(address24, buffer, length);
}

void DataReader_StartDMAReadData(uint32_t address24, uint8_t *buffer, uint32_t length)
{
  /* Fall back to a blocking read if DMA could not be started so the caller
   * always gets its data. */
  if (!W25Q64_StartDmaRead(address24, buffer, length))
  {
    (void)W25Q64_Read(address24, buffer, length);
  }
}

void DataReader_DMACallback(void)
{
  W25Q64_DmaCompleteCallback();
}
