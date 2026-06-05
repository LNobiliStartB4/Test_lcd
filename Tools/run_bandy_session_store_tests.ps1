$ErrorActionPreference = 'Stop'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptDir
$testDir = Join-Path $projectRoot 'Tests\bandy_session_store'
$outputExe = Join-Path $testDir 'bandy_session_store_tests.exe'
$gcc = 'C:\TouchGFX\4.26.0\env\MinGW\bin\gcc.exe'

if (-not (Test-Path $gcc)) {
  throw "GCC not found: $gcc"
}

& $gcc `
  -std=c99 `
  -Wall `
  -Wextra `
  -Werror `
  -I (Join-Path $projectRoot 'Core\Inc') `
  (Join-Path $projectRoot 'Core\Src\bandy_session_store_core.c') `
  (Join-Path $testDir 'test_bandy_session_store_core.c') `
  -o $outputExe

& $outputExe
