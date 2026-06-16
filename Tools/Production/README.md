# Production programming

The final PCB is programmed only through `SWD + NRST`. UART, USER buttons and
LEDs are not part of the production flow.

## Memory map

- STM32F401RE internal Flash: `Display_test_prova_internal.hex`
- EN25QH64A asset data: `assets.bin` at `0x90000000`
- Asset manifest v2: `asset_manifest.bin` at `0x907FF000`
- External Flash size: 8 MiB
- EN25QH64A pins: SPI3 `PC10/PC11/PC12`, CS `PB1`

## Build the loader

Run:

```powershell
Tools\ExternalLoader\EN25QH64A_STM32F401\build_loader.ps1
```

The loader checks EON manufacturer ID `0x1C` and 64-Mbit capacity ID `0x17`.
The middle JEDEC memory-type byte is intentionally not fixed because it must be
confirmed on the first assembled PCB.

## Program one board

```powershell
Tools\Production\program_board.ps1 `
  -InternalHex Display_test_prova_internal.hex `
  -AssetsBin assets.bin `
  -ManifestBin asset_manifest.bin `
  -ExternalLoader Tools\ExternalLoader\EN25QH64A_STM32F401\EN25QH64A_STM32F401RE.stldr
```

CubeProgrammer verifies each download before resetting the board. The firmware
then performs its own JEDEC, length, CRC and package-ID checks at boot.

The loader must be validated electrically on the final PCB before release. In
particular, record the complete EN25QH64A JEDEC ID and verify sector erase,
unaligned writes, read-back and reset behavior.
