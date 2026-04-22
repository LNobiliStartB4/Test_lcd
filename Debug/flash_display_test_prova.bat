@echo off
setlocal
"C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" -c port=SWD freq=4000 mode=UR reset=HWrst -w "C:\TouchGFXProjects\Display_test_prova\Debug\Display_test_prova.elf" -v -rst
exit /b %ERRORLEVEL%
