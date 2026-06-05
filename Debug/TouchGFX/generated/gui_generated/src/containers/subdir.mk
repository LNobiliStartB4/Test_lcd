################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/generated/gui_generated/src/containers/BandyStartPanelBase.cpp \
../TouchGFX/generated/gui_generated/src/containers/BandyTargetPanelBase.cpp \
../TouchGFX/generated/gui_generated/src/containers/BandyTimePanelBase.cpp \
../TouchGFX/generated/gui_generated/src/containers/BandyVacuumPanelBase.cpp 

OBJS += \
./TouchGFX/generated/gui_generated/src/containers/BandyStartPanelBase.o \
./TouchGFX/generated/gui_generated/src/containers/BandyTargetPanelBase.o \
./TouchGFX/generated/gui_generated/src/containers/BandyTimePanelBase.o \
./TouchGFX/generated/gui_generated/src/containers/BandyVacuumPanelBase.o 

CPP_DEPS += \
./TouchGFX/generated/gui_generated/src/containers/BandyStartPanelBase.d \
./TouchGFX/generated/gui_generated/src/containers/BandyTargetPanelBase.d \
./TouchGFX/generated/gui_generated/src/containers/BandyTimePanelBase.d \
./TouchGFX/generated/gui_generated/src/containers/BandyVacuumPanelBase.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/generated/gui_generated/src/containers/%.o TouchGFX/generated/gui_generated/src/containers/%.su TouchGFX/generated/gui_generated/src/containers/%.cyclo: ../TouchGFX/generated/gui_generated/src/containers/%.cpp TouchGFX/generated/gui_generated/src/containers/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -Os -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -femit-class-debug-always -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-generated-2f-gui_generated-2f-src-2f-containers

clean-TouchGFX-2f-generated-2f-gui_generated-2f-src-2f-containers:
	-$(RM) ./TouchGFX/generated/gui_generated/src/containers/BandyStartPanelBase.cyclo ./TouchGFX/generated/gui_generated/src/containers/BandyStartPanelBase.d ./TouchGFX/generated/gui_generated/src/containers/BandyStartPanelBase.o ./TouchGFX/generated/gui_generated/src/containers/BandyStartPanelBase.su ./TouchGFX/generated/gui_generated/src/containers/BandyTargetPanelBase.cyclo ./TouchGFX/generated/gui_generated/src/containers/BandyTargetPanelBase.d ./TouchGFX/generated/gui_generated/src/containers/BandyTargetPanelBase.o ./TouchGFX/generated/gui_generated/src/containers/BandyTargetPanelBase.su ./TouchGFX/generated/gui_generated/src/containers/BandyTimePanelBase.cyclo ./TouchGFX/generated/gui_generated/src/containers/BandyTimePanelBase.d ./TouchGFX/generated/gui_generated/src/containers/BandyTimePanelBase.o ./TouchGFX/generated/gui_generated/src/containers/BandyTimePanelBase.su ./TouchGFX/generated/gui_generated/src/containers/BandyVacuumPanelBase.cyclo ./TouchGFX/generated/gui_generated/src/containers/BandyVacuumPanelBase.d ./TouchGFX/generated/gui_generated/src/containers/BandyVacuumPanelBase.o ./TouchGFX/generated/gui_generated/src/containers/BandyVacuumPanelBase.su

.PHONY: clean-TouchGFX-2f-generated-2f-gui_generated-2f-src-2f-containers

