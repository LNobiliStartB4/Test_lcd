/**
  ******************************************************************************
  * File Name          : TouchGFXGeneratedDataReader.hpp
  ******************************************************************************
  * This file mirrors the TouchGFX Generator 4.26 DataReader output.
  ******************************************************************************
  */
#ifndef TOUCHGFXGENERATEDDATAREADER_HPP
#define TOUCHGFXGENERATEDDATAREADER_HPP

#include <touchgfx/hal/FlashDataReader.hpp>

class TouchGFXGeneratedDataReader : public touchgfx::FlashDataReader
{
public:
    TouchGFXGeneratedDataReader();
    virtual ~TouchGFXGeneratedDataReader()
    {
    }

    virtual bool addressIsAddressable(const void* address);
    virtual void copyData(const void* src, void* dst, uint32_t bytes);
    virtual void startFlashLineRead(const void* src, uint32_t bytes);
    virtual const uint8_t* waitFlashReadComplete();

private:
    uint8_t buffer1[1920];
    uint8_t buffer2[1920];
    bool readToBuffer1;
};

#endif
