#ifndef ASSET_FLASHER_H
#define ASSET_FLASHER_H

/*
 * One-time programmer for the external Winbond W25Q64 over USART2. Lets you
 * push the TouchGFX asset blob onto the soldered flash with no extra hardware:
 * call AssetFlasher_RunAtBoot() before starting the TouchGFX process (after
 * SPI3, W25Q64 and USART2 are initialized). With valid assets it enters the
 * programmer only when explicitly requested with the USER button. Missing,
 * corrupt or firmware-incompatible assets keep the board in recovery so
 * TouchGFX can never render data using stale offsets.
 *
 * Wire protocol (host -> device), all multi-byte fields little-endian:
 *   knock: "ADHTPROG" (may be retried)
 *   device: 'H' | JEDEC ID (3B)
 *   header: "ADHTHEAD" | data_length (4B) | crc32 (4B) |
 *           package_id (16B), sent once
 *   device: 'A', then 'S' | current (2B) | total (2B) during erase, then 'R'
 *   data is streamed in <=256B chunks, each ACKed 'K' | next_offset (4B)
 *   device: 'V' while validating, then 'D' and an automatic system reset
 *   errors: 'E' | reason (1B)
 *
 * Read-only diagnostic:
 *   knock: "ADHTDIAG"
 *   device: 'Q' | "DGOK" | version (1B) | mode_count (1B), followed by one
 *   fixed "DREC" record for SPI mode 0 and one for mode 3. No erase/program
 *   command is issued by this path. A diagnostic knock also aborts a stale
 *   asset stream before any further page is programmed.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns only when assets are valid and programming was not requested.
 * Recovery sessions reset the MCU on success; failed sessions wait for a new
 * host knock without starting TouchGFX. */
void AssetFlasher_RunAtBoot(bool assetsValid, bool programmingRequested);

/* UART-only diagnostic loop: no W25Q64, no TouchGFX, no bridge. Use this from
 * a dedicated bring-up build to isolate USART2/ST-LINK VCP issues. */
void AssetFlasher_RunUartLinkDiagnosticsOnly(void);

#ifdef __cplusplus
}
#endif

#endif /* ASSET_FLASHER_H */
