# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Development

**Build tool: STM32CubeIDE** — there is no CLI build command. All compilation is done from within STM32CubeIDE (Debug configuration). Always verify results against the IDE build output after making changes.

**Reference template:** `C:\TouchGFXProjects\Prova_display` (Nucleo-C092RC version — same physical connector layout, some different GPIO names).

**Schematics:**
- Nucleo-F401RE: `C:\Users\Lorenzo Nobili\Downloads\schema_elettrico_F401RE.pdf`
- Nucleo-C092RC template board: `C:\Users\Lorenzo Nobili\Downloads\mb2046-c092rc-b03-schematic.pdf`

## Critical Rule — IOC Synchronization

Any hardware change in `main.c` (timers, GPIO, peripherals) **must also be applied to `Display_test_prova.ioc`**. CubeMX re-generation overwrites all generated code (`MX_*_Init`, `GPIO_Init`, etc.) but preserves `USER CODE BEGIN/END` blocks. If the `.ioc` is out of sync, the next re-generation reverts the hardware settings.

- Peripheral parameters (TIM, SPI, UART...): update the matching section in `.ioc`
- GPIO additions/removals: update `Mcu.PinN=`, `Mcu.PinsNb`, and the `PXx.*` entries in `.ioc`

## Architecture Overview

**Stack:** TouchGFX 4.26.0 · Partial Framebuffer Strategy · 320×480 RGB565 · Bare metal (no FreeRTOS)

**MCU:** STM32F401RE @ 84 MHz (HSI → PLL: PLLM=16, PLLN=336, PLLP=4)

**Display:** RVA15MD-NUC64A (Riverdi, ST7789 controller) via SPI1 @ 21 MHz (prescaler /4)

### Initialization Order (`main.c`)

1. `MX_GPIO_Init` → `MX_CRC_Init` → `MX_I2C1_Init` → `MX_SPI1_Init` → `MX_TIM11_Init` → `MX_USART2_UART_Init`
2. `DisplayDriver_DisplayReset()` → `DisplayDriver_Init()` → `DisplayDriver_DisplayInit()` → `DisplayDriver_DisplayOn()`
3. `MX_TouchGFX_Init()`
4. `HAL_TIM_Base_Start_IT(&htim11)` ← **must come after** `MX_TouchGFX_Init()` to avoid null HAL pointer
5. Main loop: `MX_TouchGFX_Process()`

### VSync (TIM11)

TIM11 fires at ~60 Hz (Prescaler=8399 → 10 kHz tick, Period=165). `HAL_TIM_PeriodElapsedCallback` in `stm32f4xx_it.c` calls `touchgfxSignalVSync()`, which drives the TouchGFX rendering pipeline.

### SPI Pixel Transfer (DMA)

SPI1 runs in 8-bit mode normally and switches to 16-bit for RGB565 pixel data.

- `touchgfxDisplayDriverTransmitBlock()` sets area, sends `RAMWR` command (polling), then calls `HAL_SPI_Transmit_DMA` in 16-bit mode (non-blocking)
- `HAL_SPI_TxCpltCallback` (called from DMA IRQ): restores 8-bit mode, raises CS, calls `DisplayDriver_TransferCompleteCallback()`
- DMA: **DMA2 Stream 3 Channel 3** for SPI1_TX; IRQ handler `DMA2_Stream3_IRQHandler` in `stm32f4xx_it.c` (USER CODE BEGIN 1)
- DMA config in `stm32f4xx_hal_msp.c` → `HAL_SPI_MspInit` (USER CODE BEGIN SPI1_MspInit 1)
- `hdma_spi1_tx` declared in `main.c` (USER CODE BEGIN PV), `extern` in `main.h` (USER CODE BEGIN EFP)

### Touch Controller

I2C1 @ 400 kHz (PB8=SCL, PB9=SDA). IC address 0x83 (read), 5-byte read: `rx_buf[2]`=X, `rx_buf[4]`=Y.
TOUCH_IRQ (PA10) triggers EXTI falling-edge → sets `volatile uint32_t newTouch = 1`.
`STM32TouchController::sampleTouch()` polls `newTouch` and reads I2C when set.

## GPIO Pin Mapping

All signals use **Morpho connectors (CN7/CN10)**, not Arduino headers.

| Signal | Pin | Port/Pin |
|---|---|---|
| DISP_RST | PA1 | CN7_30 |
| DISP_CS | PA9 | CN10_21 |
| DISP_DC (WRX) | PB10 | CN10_25 |
| Backlight | PB4 | Arduino D5 — hardcoded HIGH in USER CODE |
| TOUCH_RST | PC5 | GPIO Output |
| TOUCH_IRQ | PA10 | EXTI falling, pull-up |
| LED (breadboard) | PC8 | — |
| LED2 | PC6 | — |
| SPI1_SCK | PA5 | — |
| SPI1_MOSI | PA7 | — |
| I2C1_SCL | PB8 | — |
| I2C1_SDA | PB9 | — |

> Note: DC pin differs from C092RC template (template uses PC8; this project uses PB10 because CN10_25 maps to a different GPIO on F401RE).
## verify the build process and verify the correct compilation. 
## Key Files

| File | Purpose |
|---|---|
| `Core/Inc/DCS.h` | ST7789 DCS command definitions |
| `Core/Inc/main.h` | GPIO pin macro definitions, `extern hdma_spi1_tx` |
| `Core/Src/RVA15MD_DisplayDriver.c` | ST7789 driver: init, area set, DMA pixel transfer |
| `Core/Src/RVA15MD_DataReader.c` | Stubbed (no external flash on F401) |
| `Core/Src/stm32f4xx_it.c` | IRQ handlers: TIM11 VSync, DMA2 Stream3, EXTI touch, HAL_GPIO_EXTI_Callback |
| `Core/Src/stm32f4xx_hal_msp.c` | DMA init for SPI1_TX in `HAL_SPI_MspInit` |
| `TouchGFX/gui/src/screen1_screen/Screen1View.cpp` | User UI logic (LED toggle, slide menu) |
| `TouchGFX/target/TouchGFXHAL.cpp` | TouchGFX HAL overrides (user-editable, generated once) |
| `TouchGFX/target/generated/TouchGFXGeneratedHAL.cpp` | Generated HAL: `DisplayDriver_TransferCompleteCallback`, `touchgfxSignalVSync` |
| `TouchGFX/target/STM32TouchController.cpp` | Touch IC driver (I2C read, coordinate mapping) |
| `Display_test_prova.ioc` | CubeMX project — must stay in sync with `main.c` |
