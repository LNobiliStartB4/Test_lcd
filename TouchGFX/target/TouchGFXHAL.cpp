/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : TouchGFXHAL.cpp
  ******************************************************************************
  * This file was created by TouchGFX Generator 4.26.0. This file is only
  * generated once! Delete this file from your project and re-generate code
  * using STM32CubeMX or change this file manually to update it.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#include <TouchGFXHAL.hpp>

/* USER CODE BEGIN TouchGFXHAL.cpp */
/* These includes must stay INSIDE the USER CODE block: TouchGFX "Generate"
 * overwrites everything above this marker and would otherwise strip them. */
#include <platform/driver/lcd/LCD16bpp.hpp>
#include <touchgfx/hal/OSWrappers.hpp>
#include <touchgfx/widgets/canvas/CWRVectorRenderer.hpp>
#include <touchgfx/widgets/canvas/PainterRGB565.hpp>
#include "RVA15MD_DisplayDriver.h"

using namespace touchgfx;

namespace touchgfx
{
VectorRenderer* VectorRenderer::getInstance()
{
    static CWRVectorRendererRGB565 renderer;
    return &renderer;
}
}

namespace
{
void kickTransferFromTask();
void startNextTransfer();

template <uint32_t blockSize, uint32_t blockCount, uint32_t bytesPerPixel>
class DmaManyBlockAllocator : public touchgfx::FrameBufferAllocator
{
public:
    DmaManyBlockAllocator()
        : sendingBlock(-1),
          drawingBlock(-1)
    {
        for (uint32_t i = 0; i < blockCount; i++)
        {
            state[i] = EMPTY;
        }
    }

    virtual uint16_t allocateBlock(const uint16_t x, const uint16_t y, const uint16_t width, const uint16_t height, uint8_t** block)
    {
        int index = findNextBlockAfter(drawingBlock, EMPTY);

        while (index < 0)
        {
            kickTransferFromTask();
            touchgfx::OSWrappers::taskYield();
            index = findNextBlockAfter(drawingBlock, EMPTY);
        }

        drawingBlock = index;
        state[index] = ALLOCATED;

        const uint32_t stride = static_cast<uint32_t>(width) * bytesPerPixel;
        const uint16_t lines = (stride == 0U) ? 0U : static_cast<uint16_t>(blockSize / stride);

        *block = reinterpret_cast<uint8_t*>(&memory[index][0]);
        blockRect[index].x = x;
        blockRect[index].y = y;
        blockRect[index].width = width;
        blockRect[index].height = (height < lines) ? height : lines;

        return blockRect[index].height;
    }

    virtual void markBlockReadyForTransfer()
    {
        if (drawingBlock >= 0 && state[drawingBlock] == ALLOCATED)
        {
            state[drawingBlock] = DRAWN;
        }
    }

    virtual bool hasBlockReadyForTransfer()
    {
        return findNextBlockWithState(DRAWN) >= 0;
    }

    virtual const uint8_t* getBlockForTransfer(Rect& rect)
    {
        int index = findNextBlockAfter(sendingBlock, DRAWN);

        if (index < 0)
        {
            return 0;
        }

        sendingBlock = index;
        rect = blockRect[index];
        state[index] = SENDING;

        return reinterpret_cast<const uint8_t*>(&memory[index][0]);
    }

    virtual const Rect& peekBlockForTransfer()
    {
        int index = findNextBlockAfter(sendingBlock, DRAWN);
        return (index >= 0) ? blockRect[index] : emptyRect;
    }

    virtual bool hasEmptyBlock()
    {
        return findNextBlockWithState(EMPTY) >= 0;
    }

    virtual void freeBlockAfterTransfer()
    {
        if (sendingBlock >= 0 && state[sendingBlock] == SENDING)
        {
            state[sendingBlock] = EMPTY;
        }
    }

private:
    int findNextBlockWithState(BlockState desiredState) const
    {
        for (uint32_t i = 0; i < blockCount; i++)
        {
            if (state[i] == desiredState)
            {
                return static_cast<int>(i);
            }
        }

        return -1;
    }

    int findNextBlockAfter(int startIndex, BlockState desiredState) const
    {
        for (uint32_t offset = 1; offset <= blockCount; offset++)
        {
            int index = (startIndex + static_cast<int>(offset)) % static_cast<int>(blockCount);

            if (state[index] == desiredState)
            {
                return index;
            }
        }

        return -1;
    }

    volatile BlockState state[blockCount];
    uint32_t memory[blockCount][blockSize / 4];
    Rect blockRect[blockCount];
    Rect emptyRect;
    int sendingBlock;
    int drawingBlock;
};

static const uint16_t partialFrameBufferMaxLines = 13;
static DmaManyBlockAllocator<12800, 2, 2> blockAllocator;

touchgfx::FrameBufferAllocator* currentAllocator()
{
    touchgfx::HAL* hal = touchgfx::HAL::getInstance();
    return hal ? hal->getFrameBufferAllocator() : 0;
}

void startNextTransfer()
{
    touchgfx::FrameBufferAllocator* allocator = currentAllocator();

    if (!allocator)
    {
        return;
    }

    while (!touchgfxDisplayDriverTransmitActive() && allocator->hasBlockReadyForTransfer())
    {
        touchgfx::Rect rect;
        const uint8_t* pixels = allocator->getBlockForTransfer(rect);

        if (!pixels)
        {
            return;
        }

        touchgfxDisplayDriverTransmitBlock(pixels, rect.x, rect.y, rect.width, rect.height);

        if (!touchgfxDisplayDriverTransmitActive())
        {
            allocator->freeBlockAfterTransfer();
        }
        else
        {
            return;
        }
    }
}

void kickTransferFromTask()
{
    startNextTransfer();
}

void handleTransferComplete()
{
    touchgfx::FrameBufferAllocator* allocator = currentAllocator();

    if (!allocator)
    {
        return;
    }

    allocator->freeBlockAfterTransfer();
    startNextTransfer();
}
}

extern "C" void TouchGFXHAL_TransferCompleteCallback(void)
{
    handleTransferComplete();
}

void TouchGFXHAL::initialize()
{
    TouchGFXGeneratedHAL::initialize();
    setFrameBufferAllocator(&blockAllocator);
    setMaxBlockLines(partialFrameBufferMaxLines);
}

/**
 * Gets the frame buffer address used by the TFT controller.
 *
 * @return The address of the frame buffer currently being displayed on the TFT.
 */
uint16_t* TouchGFXHAL::getTFTFrameBuffer() const
{
    // Calling parent implementation of getTFTFrameBuffer().
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.

    return TouchGFXGeneratedHAL::getTFTFrameBuffer();
}

/**
 * Sets the frame buffer address used by the TFT controller.
 *
 * @param [in] address New frame buffer address.
 */
void TouchGFXHAL::setTFTFrameBuffer(uint16_t* address)
{
    // Calling parent implementation of setTFTFrameBuffer(uint16_t* address).
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.

    TouchGFXGeneratedHAL::setTFTFrameBuffer(address);
}

/**
 * This function is called whenever the framework has performed a partial draw.
 *
 * @param rect The area of the screen that has been drawn, expressed in absolute coordinates.
 *
 * @see flushFrameBuffer().
 */
void TouchGFXHAL::flushFrameBuffer()
{
    flushFrameBuffer(touchgfx::Rect(0, 0, getDisplayWidth(), getDisplayHeight()));
}

void TouchGFXHAL::flushFrameBuffer(const touchgfx::Rect& rect)
{
    HAL::flushFrameBuffer(rect);
    frameBufferAllocator->markBlockReadyForTransfer();
    kickTransferFromTask();
}

bool TouchGFXHAL::blockCopy(void* RESTRICT dest, const void* RESTRICT src, uint32_t numBytes)
{
    return TouchGFXGeneratedHAL::blockCopy(dest, src, numBytes);
}

/**
 * Configures the interrupts relevant for TouchGFX. This primarily entails setting
 * the interrupt priorities for the DMA and LCD interrupts.
 */
void TouchGFXHAL::configureInterrupts()
{
    // Calling parent implementation of configureInterrupts().
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.

    TouchGFXGeneratedHAL::configureInterrupts();
}

/**
 * Used for enabling interrupts set in configureInterrupts()
 */
void TouchGFXHAL::enableInterrupts()
{
    // Calling parent implementation of enableInterrupts().
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.

    TouchGFXGeneratedHAL::enableInterrupts();
}

/**
 * Used for disabling interrupts set in configureInterrupts()
 */
void TouchGFXHAL::disableInterrupts()
{
    // Calling parent implementation of disableInterrupts().
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.

    TouchGFXGeneratedHAL::disableInterrupts();
}

/**
 * Configure the LCD controller to fire interrupts at VSYNC. Called automatically
 * once TouchGFX initialization has completed.
 */
void TouchGFXHAL::enableLCDControllerInterrupt()
{
    // Calling parent implementation of enableLCDControllerInterrupt().
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.

    TouchGFXGeneratedHAL::enableLCDControllerInterrupt();
}

bool TouchGFXHAL::beginFrame()
{
    return TouchGFXGeneratedHAL::beginFrame();
}

void TouchGFXHAL::endFrame()
{
    while (touchgfxDisplayDriverTransmitActive() || frameBufferAllocator->hasBlockReadyForTransfer())
    {
        kickTransferFromTask();
        touchgfx::OSWrappers::taskYield();
    }

    HAL::endFrame();
    touchgfx::OSWrappers::signalRenderingDone();
}

/* USER CODE END TouchGFXHAL.cpp */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
