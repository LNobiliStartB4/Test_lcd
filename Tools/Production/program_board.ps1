param(
    [Parameter(Mandatory = $true)][string]$InternalHex,
    [Parameter(Mandatory = $true)][string]$AssetsBin,
    [Parameter(Mandatory = $true)][string]$ManifestBin,
    [Parameter(Mandatory = $true)][string]$ExternalLoader,
    [string]$ProgrammerCli = "C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.300.202508131133\tools\bin\STM32_Programmer_CLI.exe"
)

$ErrorActionPreference = "Stop"
$externalBase = "0x90000000"
$manifestAddress = "0x907FF000"
$connection = @("-c", "port=SWD", "mode=UR", "reset=HWrst")

foreach ($path in @($ProgrammerCli, $InternalHex, $AssetsBin, $ManifestBin, $ExternalLoader)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing production input: $path"
    }
}

Write-Host "Programming STM32 internal Flash..."
& $ProgrammerCli @connection -d $InternalHex -v
if ($LASTEXITCODE) { exit $LASTEXITCODE }

Write-Host "Programming external EN25QH64A assets..."
& $ProgrammerCli @connection -el $ExternalLoader -d $AssetsBin $externalBase -v
if ($LASTEXITCODE) { exit $LASTEXITCODE }

Write-Host "Programming external asset manifest..."
& $ProgrammerCli @connection -el $ExternalLoader -d $ManifestBin $manifestAddress -v
if ($LASTEXITCODE) { exit $LASTEXITCODE }

Write-Host "Resetting the assembled board..."
& $ProgrammerCli @connection -rst
if ($LASTEXITCODE) { exit $LASTEXITCODE }

Write-Host "Production programming and verification completed."
