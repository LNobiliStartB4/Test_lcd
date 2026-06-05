################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/gui/src/screen5_screen/Screen5Presenter.cpp \
../TouchGFX/gui/src/screen5_screen/Screen5View.cpp 

OBJS += \
./TouchGFX/gui/src/screen5_screen/Screen5Presenter.o \
./TouchGFX/gui/src/screen5_screen/Screen5View.o 

CPP_DEPS += \
./TouchGFX/gui/src/screen5_screen/Screen5Presenter.d \
./TouchGFX/gui/src/screen5_screen/Screen5View.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/gui/src/screen5_screen/%.o TouchGFX/gui/src/screen5_screen/%.su TouchGFX/gui/src/screen5_screen/%.cyclo: ../TouchGFX/gui/src/screen5_screen/%.cpp TouchGFX/gui/src/screen5_screen/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -Os -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -femit-class-debug-always -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-gui-2f-src-2f-screen5_screen

clean-TouchGFX-2f-gui-2f-src-2f-screen5_screen:
	-$(RM) ./TouchGFX/gui/src/screen5_screen/Screen5Presenter.cyclo ./TouchGFX/gui/src/screen5_screen/Screen5Presenter.d ./TouchGFX/gui/src/screen5_screen/Screen5Presenter.o ./TouchGFX/gui/src/screen5_screen/Screen5Presenter.su ./TouchGFX/gui/src/screen5_screen/Screen5View.cyclo ./TouchGFX/gui/src/screen5_screen/Screen5View.d ./TouchGFX/gui/src/screen5_screen/Screen5View.o ./TouchGFX/gui/src/screen5_screen/Screen5View.su

.PHONY: clean-TouchGFX-2f-gui-2f-src-2f-screen5_screen

