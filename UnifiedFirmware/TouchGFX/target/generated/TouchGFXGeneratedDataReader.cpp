/**
  ******************************************************************************
  * File Name          : TouchGFXGeneratedDataReader.cpp
  ******************************************************************************
  * This file mirrors the TouchGFX Generator 4.26 DataReader output.
  ******************************************************************************
  */
#include <TouchGFXGeneratedDataReader.hpp>
#include <stm32f4xx.h>

#define MEM_BASE_ADDRESS 0x90000000UL
#define MEM_FLASH_SIZE   0x01000000UL

extern "C" void DataReader_WaitForReceiveDone();
extern "C" void DataReader_ReadData(uint32_t address24, uint8_t* buffer, uint32_t length);
extern "C" void DataReader_StartDMAReadData(uint32_t address24, uint8_t* buffer, uint32_t length);

TouchGFXGeneratedDataReader::TouchGFXGeneratedDataReader()
    : readToBuffer1(true)
{
}

bool TouchGFXGeneratedDataReader::addressIsAddressable(const void* address)
{
    const uintptr_t value = reinterpret_cast<uintptr_t>(address);
    return value < MEM_BASE_ADDRESS || value >= (MEM_BASE_ADDRESS + MEM_FLASH_SIZE);
}

void TouchGFXGeneratedDataReader::copyData(const void* src, void* dst, uint32_t bytes)
{
    if (bytes < 250U)
    {
        DataReader_ReadData(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(src)),
                            static_cast<uint8_t*>(dst),
                            bytes);
    }
    else
    {
        DataReader_StartDMAReadData(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(src)),
                                    static_cast<uint8_t*>(dst),
                                    bytes);
        DataReader_WaitForReceiveDone();
    }
}

void TouchGFXGeneratedDataReader::startFlashLineRead(const void* src, uint32_t bytes)
{
    uint8_t* const buffer = readToBuffer1 ? buffer1 : buffer2;
    DataReader_StartDMAReadData(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(src)),
                                buffer,
                                bytes);
}

const uint8_t* TouchGFXGeneratedDataReader::waitFlashReadComplete()
{
    const uint8_t* const buffer = readToBuffer1 ? buffer1 : buffer2;
    DataReader_WaitForReceiveDone();
    readToBuffer1 = !readToBuffer1;
    return buffer;
}
