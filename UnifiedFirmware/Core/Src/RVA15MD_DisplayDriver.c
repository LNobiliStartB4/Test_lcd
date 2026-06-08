/*
 * RVA15MD_DisplayDriver.c - Adapted for ILI9488 (320x480) on STM32F401RE
 *
 * Uses SPI1 (PA5/PA6/PA7) with DMA2_Stream3_Ch3.
 * ILI9488 in SPI mode only supports 18-bit color (RGB666, 3 bytes/pixel).
 * TouchGFX renders RGB565 internally, so we convert before transmitting.
 *
 * GPIO mapping (from main.h):
 *   DISP_CS  -> PA9
 *   DISP_DC  -> PB10
 *   DISP_RST -> PA1
 */

#include "stm32f4xx_hal.h"
#include "DCS.h"
#include "main.h"
#include "RVA15MD_DisplayDriver.h"

extern SPI_HandleTypeDef hspi1;

volatile uint8_t isTransmittingBlock = 0;

enum
{
    DISPLAY_ADDRESS_MODE_LANDSCAPE = 0x28,
    DISPLAY_ADDRESS_MODE_LANDSCAPE_180 = 0xE8
};

/* RGB666 conversion buffer: 320 * 20 lines * 3 bytes = 19200 bytes */
static uint8_t rgb666_buf[19200];

/* Forward declaration - defined in TouchGFXGeneratedHAL.cpp */
extern void DisplayDriver_TransferCompleteCallback(void);

/* Stub VSync GPIO functions (no debug pin on F401) */
void setVSYNC(void)  {}
void clearVSYNC(void) {}

/* --------------------------------------------------------------------------
 * RGB565 to RGB666 conversion
 * ILI9488 SPI mode requires 18-bit color (3 bytes per pixel).
 * TouchGFX renders 16-bit RGB565 (2 bytes per pixel).
 * -------------------------------------------------------------------------- */
static void convertRGB565toRGB666(const uint16_t *src, uint8_t *dst, uint32_t pixelCount)
{
    for (uint32_t i = 0; i < pixelCount; i++)
    {
        uint16_t p = src[i];
        /* Expand 5-6-5 bits to 8-8-8, replicating MSBs into LSBs for accuracy */
        dst[i * 3 + 0] = (uint8_t)(((p >> 8) & 0xF8) | ((p >> 13) & 0x07)); /* R: 5 -> 8 */
        dst[i * 3 + 1] = (uint8_t)(((p >> 3) & 0xFC) | ((p >>  9) & 0x03)); /* G: 6 -> 8 */
        dst[i * 3 + 2] = (uint8_t)(((p << 3) & 0xF8) | ((p >>  2) & 0x07)); /* B: 5 -> 8 */
    }
}

/* --------------------------------------------------------------------------
 * Low-level SPI command/data helpers
 * -------------------------------------------------------------------------- */
static void DisplaySendCommandDataArray(uint8_t command, uint8_t *data, uint32_t size)
{
    HAL_GPIO_WritePin(DISP_CS_GPIO_Port, DISP_CS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_RESET); /* command */
    HAL_SPI_Transmit(&hspi1, &command, 1, HAL_MAX_DELAY);
    if (size > 0U && data != NULL)
    {
        HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_SET); /* data */
        HAL_SPI_Transmit(&hspi1, data, (uint16_t)size, HAL_MAX_DELAY);
    }
    HAL_GPIO_WritePin(DISP_CS_GPIO_Port, DISP_CS_Pin, GPIO_PIN_SET);
}

static void DisplaySendCommand(uint8_t command)
{
    DisplaySendCommandDataArray(command, NULL, 0);
}

static void DisplaySendCommandData(uint8_t command, uint8_t data)
{
    DisplaySendCommandDataArray(command, &data, 1);
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */
void DisplayDriver_DisplayOn(void)
{
    DisplaySendCommand(DCS_SET_DISPLAY_ON);
    HAL_Delay(10);
}

void Display_OFF(void)
{
    DisplaySendCommand(DCS_SET_DISPLAY_OFF);
    HAL_Delay(10);
}

void Display_Set_Area(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    static uint16_t old_x0 = 0xFFFF;
    static uint16_t old_x1 = 0xFFFF;
    static uint16_t old_y0 = 0xFFFF;
    static uint16_t old_y1 = 0xFFFF;
    uint8_t arguments[4];

    if (x0 != old_x0 || x1 != old_x1)
    {
        arguments[0] = (uint8_t)(x0 >> 8);
        arguments[1] = (uint8_t)(x0 & 0xFF);
        arguments[2] = (uint8_t)(x1 >> 8);
        arguments[3] = (uint8_t)(x1 & 0xFF);
        DisplaySendCommandDataArray(DCS_SET_COLUMN_ADDRESS, arguments, 4);
        old_x0 = x0;
        old_x1 = x1;
    }

    if (y0 != old_y0 || y1 != old_y1)
    {
        arguments[0] = (uint8_t)(y0 >> 8);
        arguments[1] = (uint8_t)(y0 & 0xFF);
        arguments[2] = (uint8_t)(y1 >> 8);
        arguments[3] = (uint8_t)(y1 & 0xFF);
        DisplaySendCommandDataArray(DCS_SET_PAGE_ADDRESS, arguments, 4);
        old_y0 = y0;
        old_y1 = y1;
    }
}

void DisplayDriver_DisplayInit(void)
{
    uint8_t arguments[16];

    /* Exit sleep mode FIRST */
    DisplaySendCommand(DCS_EXIT_SLEEP_MODE);
    HAL_Delay(120);

    /* Pixel format: 18-bit (RGB666) for SPI interface */
    DisplaySendCommandData(DCS_SET_PIXEL_FORMAT, 0x66);

    /* Memory access control: keep TouchGFX landscape, optionally rotate panel 180 degrees. */
#if DISPLAY_ROTATE_180
    DisplaySendCommandData(DCS_SET_ADDRESS_MODE, DISPLAY_ADDRESS_MODE_LANDSCAPE_180);
#else
    DisplaySendCommandData(DCS_SET_ADDRESS_MODE, DISPLAY_ADDRESS_MODE_LANDSCAPE);
#endif

    /* ILI9488 requires invert mode ON for correct colors */
    DisplaySendCommand(DCS_ENTER_INVERT_MODE);

    /* Interface mode control */
    DisplaySendCommandData(ILI9488_INTERFACE_MODE, 0x00);

    /* Frame rate control: 60Hz */
    arguments[0] = 0xA0; /* DIVA = clock division ratio */
    arguments[1] = 0x11; /* RTNA = frame rate */
    DisplaySendCommandDataArray(ILI9488_FRAME_RATE, arguments, 2);

    /* Display function control */
    arguments[0] = 0x02;
    arguments[1] = 0x02;
    arguments[2] = 0x3B;
    DisplaySendCommandDataArray(ILI9488_DISPLAY_FUNC, arguments, 3);

    /* Power control 1 */
    arguments[0] = 0x15;
    arguments[1] = 0x17;
    DisplaySendCommandDataArray(ILI9488_POWER_CTRL1, arguments, 2);

    /* Power control 2 */
    DisplaySendCommandData(ILI9488_POWER_CTRL2, 0x41);

    /* VCOM control */
    arguments[0] = 0x00;
    arguments[1] = 0x12;
    arguments[2] = 0x80;
    DisplaySendCommandDataArray(ILI9488_VCOM_CTRL, arguments, 3);

    /* Image function: disable 24-bit data bus */
    DisplaySendCommandData(ILI9488_IMAGE_FUNC, 0x00);

    /* Adjust control 3 */
    arguments[0] = 0xA9;
    arguments[1] = 0x51;
    arguments[2] = 0x2C;
    arguments[3] = 0x82;
    DisplaySendCommandDataArray(ILI9488_ADJUST_CTRL3, arguments, 4);

    /* Positive gamma correction */
    arguments[0]  = 0x00; arguments[1]  = 0x03; arguments[2]  = 0x09;
    arguments[3]  = 0x08; arguments[4]  = 0x16; arguments[5]  = 0x0A;
    arguments[6]  = 0x3F; arguments[7]  = 0x78; arguments[8]  = 0x4C;
    arguments[9]  = 0x09; arguments[10] = 0x0A; arguments[11] = 0x08;
    arguments[12] = 0x16; arguments[13] = 0x1A; arguments[14] = 0x0F;
    DisplaySendCommandDataArray(ILI9488_POSITIVE_GAMMA, arguments, 15);

    /* Negative gamma correction */
    arguments[0]  = 0x00; arguments[1]  = 0x16; arguments[2]  = 0x19;
    arguments[3]  = 0x03; arguments[4]  = 0x0F; arguments[5]  = 0x05;
    arguments[6]  = 0x32; arguments[7]  = 0x45; arguments[8]  = 0x46;
    arguments[9]  = 0x04; arguments[10] = 0x0E; arguments[11] = 0x0D;
    arguments[12] = 0x35; arguments[13] = 0x37; arguments[14] = 0x0F;
    DisplaySendCommandDataArray(ILI9488_NEGATIVE_GAMMA, arguments, 15);
}

void DisplayDriver_DisplayReset(void)
{
    HAL_GPIO_WritePin(DISP_RST_GPIO_Port, DISP_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(DISP_RST_GPIO_Port, DISP_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(120);
}

void DisplayDriver_Init(void)
{
    /* SPI1 is already initialized by MX_SPI1_Init().
     * Nothing extra needed — DMA is configured in HAL_SPI_MspInit. */
}

int touchgfxDisplayDriverTransmitActive(void)
{
    return (int)isTransmittingBlock;
}

void touchgfxDisplayDriverTransmitBlock(const uint8_t *pixels, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint32_t pixelCount = (uint32_t)w * (uint32_t)h;

    isTransmittingBlock = 1;

    Display_Set_Area(x, y, x + w - 1, y + h - 1);

    /* Convert RGB565 → RGB666 (3 bytes/pixel for ILI9488 SPI mode) */
    convertRGB565toRGB666((const uint16_t *)pixels, rgb666_buf, pixelCount);

    /* Send RAMWR command (polling — only 1 byte) */
    HAL_GPIO_WritePin(DISP_CS_GPIO_Port, DISP_CS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_RESET); /* command */
    uint8_t cmd = DCS_WRITE_MEMORY_START;
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_SET);   /* data */

    /* DMA transfer — 8-bit mode, size in bytes */
    HAL_SPI_Transmit_DMA(&hspi1, rgb666_buf, (uint16_t)(pixelCount * 3));
    /* Transfer completion handled in HAL_SPI_TxCpltCallback */
}

/**
  * @brief SPI TX DMA transfer complete callback.
  *        Called from IRQ context when DMA finishes sending pixel data.
  */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        HAL_GPIO_WritePin(DISP_CS_GPIO_Port, DISP_CS_Pin, GPIO_PIN_SET);
        isTransmittingBlock = 0;
        DisplayDriver_TransferCompleteCallback();
    }
}

void DisplayDriver_DMACallback(void)
{
    /* Not used — DMA completion handled by HAL_SPI_TxCpltCallback */
}

int touchgfxDisplayDriverShouldTransferBlock(uint16_t bottom)
{
    (void)bottom;
    /* No tearing effect pin — always allow transfer */
    return 1;
}
