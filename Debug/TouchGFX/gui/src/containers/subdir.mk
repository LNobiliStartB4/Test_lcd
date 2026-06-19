################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/gui/src/containers/BandyStartPanel.cpp \
../TouchGFX/gui/src/containers/BandyTargetPanel.cpp \
../TouchGFX/gui/src/containers/BandyTimePanel.cpp \
../TouchGFX/gui/src/containers/BandyVacuumPanel.cpp 

OBJS += \
./TouchGFX/gui/src/containers/BandyStartPanel.o \
./TouchGFX/gui/src/containers/BandyTargetPanel.o \
./TouchGFX/gui/src/containers/BandyTimePanel.o \
./TouchGFX/gui/src/containers/BandyVacuumPanel.o 

CPP_DEPS += \
./TouchGFX/gui/src/containers/BandyStartPanel.d \
./TouchGFX/gui/src/containers/BandyTargetPanel.d \
./TouchGFX/gui/src/containers/BandyTimePanel.d \
./TouchGFX/gui/src/containers/BandyVacuumPanel.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/gui/src/containers/%.o TouchGFX/gui/src/containers/%.su TouchGFX/gui/src/containers/%.cyclo: ../TouchGFX/gui/src/containers/%.cpp TouchGFX/gui/src/containers/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -femit-class-debug-always -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-gui-2f-src-2f-containers

clean-TouchGFX-2f-gui-2f-src-2f-containers:
	-$(RM) ./TouchGFX/gui/src/containers/BandyStartPanel.cyclo ./TouchGFX/gui/src/containers/BandyStartPanel.d ./TouchGFX/gui/src/containers/BandyStartPanel.o ./TouchGFX/gui/src/containers/BandyStartPanel.su ./TouchGFX/gui/src/containers/BandyTargetPanel.cyclo ./TouchGFX/gui/src/containers/BandyTargetPanel.d ./TouchGFX/gui/src/containers/BandyTargetPanel.o ./TouchGFX/gui/src/containers/BandyTargetPanel.su ./TouchGFX/gui/src/containers/BandyTimePanel.cyclo ./TouchGFX/gui/src/containers/BandyTimePanel.d ./TouchGFX/gui/src/containers/BandyTimePanel.o ./TouchGFX/gui/src/containers/BandyTimePanel.su ./TouchGFX/gui/src/containers/BandyVacuumPanel.cyclo ./TouchGFX/gui/src/containers/BandyVacuumPanel.d ./TouchGFX/gui/src/containers/BandyVacuumPanel.o ./TouchGFX/gui/src/containers/BandyVacuumPanel.su

.PHONY: clean-TouchGFX-2f-gui-2f-src-2f-containers

