################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/RVA15MD_DataReader.c \
../Core/Src/RVA15MD_DisplayDriver.c \
../Core/Src/bandy_session_store.c \
../Core/Src/bandy_session_store_core.c \
../Core/Src/display_backlight.c \
../Core/Src/display_bridge_rx.c \
../Core/Src/fram_mb85rs256b.c \
../Core/Src/main.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c 

C_DEPS += \
./Core/Src/RVA15MD_DataReader.d \
./Core/Src/RVA15MD_DisplayDriver.d \
./Core/Src/bandy_session_store.d \
./Core/Src/bandy_session_store_core.d \
./Core/Src/display_backlight.d \
./Core/Src/display_bridge_rx.d \
./Core/Src/fram_mb85rs256b.d \
./Core/Src/main.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d 

OBJS += \
./Core/Src/RVA15MD_DataReader.o \
./Core/Src/RVA15MD_DisplayDriver.o \
./Core/Src/bandy_session_store.o \
./Core/Src/bandy_session_store_core.o \
./Core/Src/display_backlight.o \
./Core/Src/display_bridge_rx.o \
./Core/Src/fram_mb85rs256b.o \
./Core/Src/main.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/RVA15MD_DataReader.cyclo ./Core/Src/RVA15MD_DataReader.d ./Core/Src/RVA15MD_DataReader.o ./Core/Src/RVA15MD_DataReader.su ./Core/Src/RVA15MD_DisplayDriver.cyclo ./Core/Src/RVA15MD_DisplayDriver.d ./Core/Src/RVA15MD_DisplayDriver.o ./Core/Src/RVA15MD_DisplayDriver.su ./Core/Src/bandy_session_store.cyclo ./Core/Src/bandy_session_store.d ./Core/Src/bandy_session_store.o ./Core/Src/bandy_session_store.su ./Core/Src/bandy_session_store_core.cyclo ./Core/Src/bandy_session_store_core.d ./Core/Src/bandy_session_store_core.o ./Core/Src/bandy_session_store_core.su ./Core/Src/display_backlight.cyclo ./Core/Src/display_backlight.d ./Core/Src/display_backlight.o ./Core/Src/display_backlight.su ./Core/Src/display_bridge_rx.cyclo ./Core/Src/display_bridge_rx.d ./Core/Src/display_bridge_rx.o ./Core/Src/display_bridge_rx.su ./Core/Src/fram_mb85rs256b.cyclo ./Core/Src/fram_mb85rs256b.d ./Core/Src/fram_mb85rs256b.o ./Core/Src/fram_mb85rs256b.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su

.PHONY: clean-Core-2f-Src

