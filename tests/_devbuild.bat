@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 || exit /b 1
cd /d "%~dp0build" || exit /b 1
cmake -G Ninja -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl .. || exit /b 1
cmake --build . || exit /b 1
ctest --output-on-failure || exit /b 1
