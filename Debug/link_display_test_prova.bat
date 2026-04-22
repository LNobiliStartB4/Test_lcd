@echo off
setlocal
set "PATH=C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712\tools\bin;%PATH%"
if exist link_display_test_prova.ok del /f /q link_display_test_prova.ok
"C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712\tools\bin\arm-none-eabi-g++.exe" -o "Display_test_prova.elf" @objects.list -l:libtouchgfx-float-abi-hard.a -mcpu=cortex-m4 -T"C:\TouchGFXProjects\Display_test_prova\STM32F401RETX_FLASH.ld" --specs=nosys.specs -Wl,-Map="Display_test_prova.map" -Wl,--gc-sections -static -L"C:\TouchGFXProjects\Display_test_prova\Middlewares\ST\touchgfx\lib\core\cortex_m4f\gcc" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group
if errorlevel 1 exit /b %ERRORLEVEL%
type nul > link_display_test_prova.ok
exit /b 0
