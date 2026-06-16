param(
    [string]$ToolchainBin = "C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712\tools\bin",
    [string]$ProjectRoot = (Resolve-Path "$PSScriptRoot\..\..\..")
)

$ErrorActionPreference = "Stop"
$gcc = Join-Path $ToolchainBin "arm-none-eabi-gcc.exe"
$objcopy = Join-Path $ToolchainBin "arm-none-eabi-objcopy.exe"
$cmsisDevice = Join-Path $ProjectRoot "Drivers\CMSIS\Device\ST\STM32F4xx\Include"
$cmsisCore = Join-Path $ProjectRoot "Drivers\CMSIS\Include"
$output = Join-Path $PSScriptRoot "EN25QH64A_STM32F401RE.stldr"

& $gcc -c "$PSScriptRoot\Loader_Src.c" -o "$PSScriptRoot\Loader_Src.o" `
    -mcpu=cortex-m4 -mthumb -Os -ffunction-sections -fdata-sections `
    -DSTM32F401xE "-I$cmsisDevice" "-I$cmsisCore"
if ($LASTEXITCODE) { exit $LASTEXITCODE }

& $gcc -c "$PSScriptRoot\Dev_Inf.c" -o "$PSScriptRoot\Dev_Inf.o" `
    -mcpu=cortex-m4 -mthumb -Os -ffunction-sections -fdata-sections
if ($LASTEXITCODE) { exit $LASTEXITCODE }

& $gcc -nostdlib -mcpu=cortex-m4 -mthumb `
    "-T$PSScriptRoot\loader.ld" "-Wl,--gc-sections" `
    "$PSScriptRoot\Loader_Src.o" "$PSScriptRoot\Dev_Inf.o" -o $output
if ($LASTEXITCODE) { exit $LASTEXITCODE }

& $objcopy --strip-debug $output
if ($LASTEXITCODE) { exit $LASTEXITCODE }

Write-Host "External loader generated: $output"
