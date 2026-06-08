param(
    [string]$ElfPath = (Join-Path $PSScriptRoot '..\Release\Pressure_Stabiliser.elf'),
    [string]$ObjdumpPath = 'C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712\tools\bin\arm-none-eabi-objdump.exe',
    [int]$InternalFlashLimitBytes = 262144
)

$ErrorActionPreference = 'Stop'

$elf = (Resolve-Path $ElfPath).Path
if (-not (Test-Path $ObjdumpPath)) {
    throw "arm-none-eabi-objdump non trovato: $ObjdumpPath"
}

$sections = & $ObjdumpPath -h $elf
if ($LASTEXITCODE -ne 0) {
    throw "objdump ha restituito codice $LASTEXITCODE"
}

$parsed = for ($index = 0; $index -lt $sections.Count; $index++) {
    $line = $sections[$index]
    if ($line -match '^\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+') {
        $flags = if (($index + 1) -lt $sections.Count) { $sections[$index + 1] } else { '' }
        [pscustomobject]@{
            Name = $Matches[1]
            Size = [Convert]::ToUInt32($Matches[2], 16)
            Vma  = [Convert]::ToUInt32($Matches[3], 16)
            Lma  = [Convert]::ToUInt32($Matches[4], 16)
            Load = $flags -match '\bLOAD\b'
        }
    }
}

$flashBase = 0x08000000L
$flashEnd = 0x08080000L
$assetBase = 0x90000000L
$assetEnd = 0x91000000L
$ramBase = 0x20000000L
$ramEnd = 0x20018000L

$internal = $parsed | Where-Object {
    $_.Load -and $_.Size -gt 0 -and $_.Lma -ge $flashBase -and $_.Lma -lt $flashEnd
}
$assets = $parsed | Where-Object {
    $_.Load -and $_.Size -gt 0 -and $_.Lma -ge $assetBase -and $_.Lma -lt $assetEnd
}
$ram = $parsed | Where-Object {
    $_.Size -gt 0 -and $_.Vma -ge $ramBase -and $_.Vma -lt $ramEnd
}

$internalUsed = (($internal | ForEach-Object { [int64]$_.Lma + $_.Size } |
    Measure-Object -Maximum).Maximum) - $flashBase
$assetUsed = (($assets | ForEach-Object { [int64]$_.Lma + $_.Size } |
    Measure-Object -Maximum).Maximum) - $assetBase
$ramUsed = (($ram | ForEach-Object { [int64]$_.Vma + $_.Size } |
    Measure-Object -Maximum).Maximum) - $ramBase

$internalFree = ($flashEnd - $flashBase) - $internalUsed
$ramFree = ($ramEnd - $ramBase) - $ramUsed

Write-Host ("Internal Flash : {0:N0} bytes ({1:N1} KiB), free {2:N1}%" -f
    $internalUsed, ($internalUsed / 1KB), (100.0 * $internalFree / ($flashEnd - $flashBase)))
Write-Host ("Winbond assets : {0:N0} bytes ({1:N1} KiB)" -f
    $assetUsed, ($assetUsed / 1KB))
Write-Host ("RAM high-water : {0:N0} bytes ({1:N1} KiB), free {2:N0} bytes" -f
    $ramUsed, ($ramUsed / 1KB), $ramFree)

if ($internalUsed -gt $InternalFlashLimitBytes) {
    throw ("Flash interna oltre il limite: {0} > {1} byte" -f
        $internalUsed, $InternalFlashLimitBytes)
}

Write-Host "Memory budget OK (internal Flash <= 256 KiB)."
