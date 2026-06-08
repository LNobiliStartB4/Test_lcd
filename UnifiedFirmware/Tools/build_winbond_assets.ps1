param(
    [string]$ElfPath = (Join-Path $PSScriptRoot '..\Release\Pressure_Stabiliser.elf'),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\Artifacts\Winbond'),
    [string]$ObjcopyPath = 'C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712\tools\bin\arm-none-eabi-objcopy.exe'
)

$ErrorActionPreference = 'Stop'

$elf = (Resolve-Path $ElfPath).Path
if (-not (Test-Path $ObjcopyPath)) {
    throw "arm-none-eabi-objcopy non trovato: $ObjcopyPath"
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$assetPath = Join-Path $OutputDirectory 'Pressure_Stabiliser_assets.bin'
$manifestPath = Join-Path $OutputDirectory 'Pressure_Stabiliser_assets_manifest.bin'
$jsonPath = Join-Path $OutputDirectory 'Pressure_Stabiliser_assets_manifest.json'

& $ObjcopyPath -O binary `
    --only-section=ExtFlashSection `
    --only-section=FontFlashSection `
    --only-section=TextFlashSection `
    $elf $assetPath
if ($LASTEXITCODE -ne 0) {
    throw "objcopy ha restituito codice $LASTEXITCODE"
}

$assetBytes = [System.IO.File]::ReadAllBytes($assetPath)
if ($assetBytes.Length -eq 0) {
    throw 'Il pacchetto asset e vuoto.'
}

$crc = [uint64]4294967295
foreach ($value in $assetBytes) {
    $crc = ($crc -bxor [uint64]$value) -band [uint64]4294967295
    for ($bit = 0; $bit -lt 8; $bit++) {
        if (($crc -band 1) -ne 0) {
            $crc = (($crc -shr 1) -bxor [uint64]3988292384) -band [uint64]4294967295
        } else {
            $crc = ($crc -shr 1) -band [uint64]4294967295
        }
    }
}
$crc = [uint32](($crc -bxor [uint64]4294967295) -band [uint64]4294967295)

$manifest = New-Object byte[] 24
[System.BitConverter]::GetBytes([uint32]0x41444854).CopyTo($manifest, 0)
[System.BitConverter]::GetBytes([uint32]1).CopyTo($manifest, 4)
[System.BitConverter]::GetBytes([uint32]$assetBytes.Length).CopyTo($manifest, 8)
[System.BitConverter]::GetBytes([uint32]$crc).CopyTo($manifest, 12)
[System.IO.File]::WriteAllBytes($manifestPath, $manifest)

[ordered]@{
    format = 'THDA'
    version = 1
    asset_file = [System.IO.Path]::GetFileName($assetPath)
    asset_offset = '0x00000000'
    asset_length = $assetBytes.Length
    asset_crc32 = ('0x{0:X8}' -f $crc)
    manifest_file = [System.IO.Path]::GetFileName($manifestPath)
    manifest_offset = '0x00FFF000'
} | ConvertTo-Json | Set-Content -Encoding ascii $jsonPath

Write-Host ("Asset Winbond: {0} bytes, CRC32 0x{1:X8}" -f $assetBytes.Length, $crc)
Write-Host "Programmare:"
Write-Host "  $assetPath @ offset 0x00000000"
Write-Host "  $manifestPath @ offset 0x00FFF000"
