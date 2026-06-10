################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/generated/images/src/BitmapDatabase.cpp \
../TouchGFX/generated/images/src/SVGDatabase.cpp \
../TouchGFX/generated/images/src/image_A1.cpp \
../TouchGFX/generated/images/src/image_TouchDot.cpp \
../TouchGFX/generated/images/src/image_alert_triangle_white.cpp \
../TouchGFX/generated/images/src/image_arrow_left_white.cpp \
../TouchGFX/generated/images/src/image_brightness_slider_knob.cpp \
../TouchGFX/generated/images/src/image_brightness_slider_track_dim.cpp \
../TouchGFX/generated/images/src/image_brightness_slider_track_gold.cpp \
../TouchGFX/generated/images/src/image_pc_wait_device.cpp \
../TouchGFX/generated/images/src/image_pill_red.cpp \
../TouchGFX/generated/images/src/image_rfid_contactless.cpp \
../TouchGFX/generated/images/src/image_start_play_icon_white.cpp \
../TouchGFX/generated/images/src/image_start_stop_icon_white.cpp \
../TouchGFX/generated/images/src/image_target_button_ring_white.cpp \
../TouchGFX/generated/images/src/image_thd_corner_logo.cpp \
../TouchGFX/generated/images/src/image_time_left_icon_white.cpp \
../TouchGFX/generated/images/src/image_trash_red.cpp 

OBJS += \
./TouchGFX/generated/images/src/BitmapDatabase.o \
./TouchGFX/generated/images/src/SVGDatabase.o \
./TouchGFX/generated/images/src/image_A1.o \
./TouchGFX/generated/images/src/image_TouchDot.o \
./TouchGFX/generated/images/src/image_alert_triangle_white.o \
./TouchGFX/generated/images/src/image_arrow_left_white.o \
./TouchGFX/generated/images/src/image_brightness_slider_knob.o \
./TouchGFX/generated/images/src/image_brightness_slider_track_dim.o \
./TouchGFX/generated/images/src/image_brightness_slider_track_gold.o \
./TouchGFX/generated/images/src/image_pc_wait_device.o \
./TouchGFX/generated/images/src/image_pill_red.o \
./TouchGFX/generated/images/src/image_rfid_contactless.o \
./TouchGFX/generated/images/src/image_start_play_icon_white.o \
./TouchGFX/generated/images/src/image_start_stop_icon_white.o \
./TouchGFX/generated/images/src/image_target_button_ring_white.o \
./TouchGFX/generated/images/src/image_thd_corner_logo.o \
./TouchGFX/generated/images/src/image_time_left_icon_white.o \
./TouchGFX/generated/images/src/image_trash_red.o 

CPP_DEPS += \
./TouchGFX/generated/images/src/BitmapDatabase.d \
./TouchGFX/generated/images/src/SVGDatabase.d \
./TouchGFX/generated/images/src/image_A1.d \
./TouchGFX/generated/images/src/image_TouchDot.d \
./TouchGFX/generated/images/src/image_alert_triangle_white.d \
./TouchGFX/generated/images/src/image_arrow_left_white.d \
./TouchGFX/generated/images/src/image_brightness_slider_knob.d \
./TouchGFX/generated/images/src/image_brightness_slider_track_dim.d \
./TouchGFX/generated/images/src/image_brightness_slider_track_gold.d \
./TouchGFX/generated/images/src/image_pc_wait_device.d \
./TouchGFX/generated/images/src/image_pill_red.d \
./TouchGFX/generated/images/src/image_rfid_contactless.d \
./TouchGFX/generated/images/src/image_start_play_icon_white.d \
./TouchGFX/generated/images/src/image_start_stop_icon_white.d \
./TouchGFX/generated/images/src/image_target_button_ring_white.d \
./TouchGFX/generated/images/src/image_thd_corner_logo.d \
./TouchGFX/generated/images/src/image_time_left_icon_white.d \
./TouchGFX/generated/images/src/image_trash_red.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/generated/images/src/%.o TouchGFX/generated/images/src/%.su TouchGFX/generated/images/src/%.cyclo: ../TouchGFX/generated/images/src/%.cpp TouchGFX/generated/images/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -Os -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -femit-class-debug-always -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-generated-2f-images-2f-src

clean-TouchGFX-2f-generated-2f-images-2f-src:
	-$(RM) ./TouchGFX/generated/images/src/BitmapDatabase.cyclo ./TouchGFX/generated/images/src/BitmapDatabase.d ./TouchGFX/generated/images/src/BitmapDatabase.o ./TouchGFX/generated/images/src/BitmapDatabase.su ./TouchGFX/generated/images/src/SVGDatabase.cyclo ./TouchGFX/generated/images/src/SVGDatabase.d ./TouchGFX/generated/images/src/SVGDatabase.o ./TouchGFX/generated/images/src/SVGDatabase.su ./TouchGFX/generated/images/src/image_A1.cyclo ./TouchGFX/generated/images/src/image_A1.d ./TouchGFX/generated/images/src/image_A1.o ./TouchGFX/generated/images/src/image_A1.su ./TouchGFX/generated/images/src/image_TouchDot.cyclo ./TouchGFX/generated/images/src/image_TouchDot.d ./TouchGFX/generated/images/src/image_TouchDot.o ./TouchGFX/generated/images/src/image_TouchDot.su ./TouchGFX/generated/images/src/image_alert_triangle_white.cyclo ./TouchGFX/generated/images/src/image_alert_triangle_white.d ./TouchGFX/generated/images/src/image_alert_triangle_white.o ./TouchGFX/generated/images/src/image_alert_triangle_white.su ./TouchGFX/generated/images/src/image_arrow_left_white.cyclo ./TouchGFX/generated/images/src/image_arrow_left_white.d ./TouchGFX/generated/images/src/image_arrow_left_white.o ./TouchGFX/generated/images/src/image_arrow_left_white.su ./TouchGFX/generated/images/src/image_brightness_slider_knob.cyclo ./TouchGFX/generated/images/src/image_brightness_slider_knob.d ./TouchGFX/generated/images/src/image_brightness_slider_knob.o ./TouchGFX/generated/images/src/image_brightness_slider_knob.su ./TouchGFX/generated/images/src/image_brightness_slider_track_dim.cyclo ./TouchGFX/generated/images/src/image_brightness_slider_track_dim.d ./TouchGFX/generated/images/src/image_brightness_slider_track_dim.o ./TouchGFX/generated/images/src/image_brightness_slider_track_dim.su ./TouchGFX/generated/images/src/image_brightness_slider_track_gold.cyclo ./TouchGFX/generated/images/src/image_brightness_slider_track_gold.d ./TouchGFX/generated/images/src/image_brightness_slider_track_gold.o ./TouchGFX/generated/images/src/image_brightness_slider_track_gold.su ./TouchGFX/generated/images/src/image_pc_wait_device.cyclo ./TouchGFX/generated/images/src/image_pc_wait_device.d ./TouchGFX/generated/images/src/image_pc_wait_device.o ./TouchGFX/generated/images/src/image_pc_wait_device.su ./TouchGFX/generated/images/src/image_pill_red.cyclo ./TouchGFX/generated/images/src/image_pill_red.d ./TouchGFX/generated/images/src/image_pill_red.o ./TouchGFX/generated/images/src/image_pill_red.su ./TouchGFX/generated/images/src/image_rfid_contactless.cyclo ./TouchGFX/generated/images/src/image_rfid_contactless.d ./TouchGFX/generated/images/src/image_rfid_contactless.o ./TouchGFX/generated/images/src/image_rfid_contactless.su ./TouchGFX/generated/images/src/image_start_play_icon_white.cyclo ./TouchGFX/generated/images/src/image_start_play_icon_white.d ./TouchGFX/generated/images/src/image_start_play_icon_white.o ./TouchGFX/generated/images/src/image_start_play_icon_white.su ./TouchGFX/generated/images/src/image_start_stop_icon_white.cyclo ./TouchGFX/generated/images/src/image_start_stop_icon_white.d ./TouchGFX/generated/images/src/image_start_stop_icon_white.o ./TouchGFX/generated/images/src/image_start_stop_icon_white.su ./TouchGFX/generated/images/src/image_target_button_ring_white.cyclo ./TouchGFX/generated/images/src/image_target_button_ring_white.d ./TouchGFX/generated/images/src/image_target_button_ring_white.o ./TouchGFX/generated/images/src/image_target_button_ring_white.su ./TouchGFX/generated/images/src/image_thd_corner_logo.cyclo ./TouchGFX/generated/images/src/image_thd_corner_logo.d ./TouchGFX/generated/images/src/image_thd_corner_logo.o ./TouchGFX/generated/images/src/image_thd_corner_logo.su ./TouchGFX/generated/images/src/image_time_left_icon_white.cyclo ./TouchGFX/generated/images/src/image_time_left_icon_white.d ./TouchGFX/generated/images/src/image_time_left_icon_white.o ./TouchGFX/generated/images/src/image_time_left_icon_white.su ./TouchGFX/generated/images/src/image_trash_red.cyclo ./TouchGFX/generated/images/src/image_trash_red.d ./TouchGFX/generated/images/src/image_trash_red.o ./TouchGFX/generated/images/src/image_trash_red.su

.PHONY: clean-TouchGFX-2f-generated-2f-images-2f-src

