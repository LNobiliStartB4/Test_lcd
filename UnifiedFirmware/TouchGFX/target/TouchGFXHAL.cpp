/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : TouchGFXHAL.cpp
  ******************************************************************************
  * This file was created by TouchGFX Generator 4.26.0. This file is only
  * generated once.
  ******************************************************************************
  */
/* USER CODE END Header */

#include <TouchGFXHAL.hpp>
#include <touchgfx/widgets/canvas/CWRVectorRenderer.hpp>
#include <touchgfx/widgets/canvas/PainterRGB565.hpp>

/* USER CODE BEGIN TouchGFXHAL.cpp */

using namespace touchgfx;

namespace touchgfx
{
VectorRenderer* VectorRenderer::getInstance()
{
    static CWRVectorRendererRGB565 renderer;
    return &renderer;
}
}

void TouchGFXHAL::initialize()
{
    // The generated partial-framebuffer allocator is the single framebuffer owner.
    TouchGFXGeneratedHAL::initialize();
}

uint16_t* TouchGFXHAL::getTFTFrameBuffer() const
{
    return TouchGFXGeneratedHAL::getTFTFrameBuffer();
}

void TouchGFXHAL::setTFTFrameBuffer(uint16_t* address)
{
    TouchGFXGeneratedHAL::setTFTFrameBuffer(address);
}

void TouchGFXHAL::flushFrameBuffer()
{
    TouchGFXGeneratedHAL::flushFrameBuffer();
}

void TouchGFXHAL::flushFrameBuffer(const touchgfx::Rect& rect)
{
    TouchGFXGeneratedHAL::flushFrameBuffer(rect);
}

bool TouchGFXHAL::blockCopy(void* RESTRICT dest, const void* RESTRICT src, uint32_t numBytes)
{
    return TouchGFXGeneratedHAL::blockCopy(dest, src, numBytes);
}

void TouchGFXHAL::configureInterrupts()
{
    TouchGFXGeneratedHAL::configureInterrupts();
}

void TouchGFXHAL::enableInterrupts()
{
    TouchGFXGeneratedHAL::enableInterrupts();
}

void TouchGFXHAL::disableInterrupts()
{
    TouchGFXGeneratedHAL::disableInterrupts();
}

void TouchGFXHAL::enableLCDControllerInterrupt()
{
    TouchGFXGeneratedHAL::enableLCDControllerInterrupt();
}

bool TouchGFXHAL::beginFrame()
{
    return TouchGFXGeneratedHAL::beginFrame();
}

void TouchGFXHAL::endFrame()
{
    TouchGFXGeneratedHAL::endFrame();
}

/* USER CODE END TouchGFXHAL.cpp */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
