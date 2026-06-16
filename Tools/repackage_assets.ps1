<#
.SYNOPSIS
  Post-build asset repackaging for Display_test_prova.

  Run this AFTER every STM32CubeIDE build whenever the set of external images
  (ExtFlashSection) changes. It:
    1. extracts ExtFlashSection from the freshly built .elf  -> assets.bin
    2. computes the v2 manifest (magic/ver/len/crc32 + SHA-256 package_id)
    3. PATCHES AssetExpectedSection inside the firmware so g_expected_asset_package
       matches assets.bin  (without this the board boots into the recovery loop
       and never renders -- "black screen")
    4. emits a flashable internal HEX

  Then flash:
    - Display_test_prova_internal.hex  via ST-Link (firmware)
    - assets.bin                       via  python Tools/flash_assets.py --port COMx assets.bin

.NOTES
  Do NOT flash the firmware straight from the IDE: the IDE build leaves
  AssetExpectedSection zeroed (unpatched) -> recovery lockup. Always flash the
  *_internal.hex produced here.
#>
param(
    [string]$ElfPath = (Join-Path $PSScriptRoot '..\Debug\Display_test_prova.elf'),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..'),
    [string]$ObjcopyPath = 'C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712\tools\bin\arm-none-eabi-objcopy.exe',
    [string]$Python = 'python'
)

$ErrorActionPreference = 'Stop'

$elf = (Resolve-Path $ElfPath).Path
if (-not (Test-Path $ObjcopyPath)) { throw "arm-none-eabi-objcopy non trovato: $ObjcopyPath" }

$out          = (Resolve-Path $OutputDirectory).Path
$assetPath    = Join-Path $out 'assets.bin'
$manifestPath = Join-Path $out 'asset_manifest.bin'
$jsonPath     = Join-Path $out 'asset_manifest.json'
$patchedElf   = Join-Path $out 'Display_test_prova_internal.elf'
$hexPath      = Join-Path $out 'Display_test_prova_internal.hex'
$metadataTool = Join-Path $PSScriptRoot 'build_asset_metadata.py'

# 1) extract external image blob
& $ObjcopyPath -O binary --only-section=ExtFlashSection $elf $assetPath
if ($LASTEXITCODE -ne 0) { throw "objcopy (extract) -> $LASTEXITCODE" }
$len = (Get-Item $assetPath).Length
if ($len -eq 0) { throw 'ExtFlashSection vuota: nessuna immagine esterna?' }

# 2) build v2 manifest (crc32 + sha256 package_id)
& $Python $metadataTool $assetPath $manifestPath $jsonPath
if ($LASTEXITCODE -ne 0) { throw "build_asset_metadata.py -> $LASTEXITCODE" }

# 3) patch the embedded expected manifest, then 4) emit internal HEX.
#    ExtFlashSection lives at 0x90000000 (external flash) and is already in
#    assets.bin -- it MUST be stripped from the internal HEX, otherwise the
#    ST-Link programmer tries to write 0x90000000 and fails ("failed to download").
Copy-Item $elf $patchedElf -Force
& $ObjcopyPath --update-section AssetExpectedSection=$manifestPath $patchedElf
if ($LASTEXITCODE -ne 0) { throw "objcopy (patch) -> $LASTEXITCODE" }
& $ObjcopyPath -O ihex --remove-section=ExtFlashSection $patchedElf $hexPath
if ($LASTEXITCODE -ne 0) { throw "objcopy (hex) -> $LASTEXITCODE" }

Write-Host ""
Write-Host "OK - assets.bin = $len byte"
Write-Host "Firmware da flashare (ST-Link): $hexPath"
Write-Host "Asset da flashare (UART):       $assetPath"
Write-Host "  python Tools/flash_assets.py --port COMx assets.bin"
