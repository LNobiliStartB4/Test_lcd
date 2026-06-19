/*
 * RVA15MD_DataReader.c - Stub for STM32F401RE
 *
 * The original template used SPI2 to read from external flash.
 * This project has no external flash connected, so all functions are stubs.
 * TouchGFX fonts and bitmaps are stored in internal flash (generated .cpp files).
 */

#include "stm32f4xx_hal.h"
#include "RVA15MD_DataReader.h"

uint32_t DataReader_IsReceivingData(void)
{
    return 0;
}

void DataReader_WaitForReceiveDone(void)
{
    /* Nothing to wait for - no external flash */
}

void DataReader_ReadData(uint32_t address24, uint8_t *buffer, uint32_t length)
{
    (void)address24;
    (void)buffer;
    (void)length;
    /* No external flash - not implemented */
}

void DataReader_StartDMAReadData(uint32_t address24, uint8_t *buffer, uint32_t length)
{
    (void)address24;
    (void)buffer;
    (void)length;
    /* No external flash - not implemented */
}

void DataReader_DMACallback(void)
{
    /* No external flash - not implemented */
}
