#ifndef ASSET_FLASHER_H
#define ASSET_FLASHER_H

/*
 * One-time programmer for the external Winbond W25Q64 over USART2. Lets you
 * push the TouchGFX asset blob onto the soldered flash with no extra hardware:
 * call AssetFlasher_RunAtBoot() before starting the TouchGFX process (after
 * SPI3, W25Q64 and USART2 are initialized). With valid assets it enters the
 * programmer only when explicitly requested with the USER button. Otherwise it
 * opens a bounded knock window (short by default, longer when USER is held) and
 * then boots regardless, so a missing/invalid package never strands the board.
 *
 * Wire protocol (host -> device), all multi-byte fields little-endian:
 *   knock: "ADHTPROG" (may be retried)
 *   device: 'H' | JEDEC ID (3B)
 *   header: "ADHTHEAD" | data_length (4B) | crc32 (4B), sent once
 *   device: 'A', then 'S' | current (2B) | total (2B) during erase, then 'R'
 *   data is streamed in <=256B chunks, each ACKed 'K' | next_offset (4B)
 *   device: 'V' while validating, then 'D' and an automatic system reset
 *   errors: 'E' | reason (1B)
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns immediately when assets are valid and programming was not requested.
 * Otherwise waits for a host knock for a bounded window, runs one programming
 * session if a host attaches (which resets the MCU on success), then returns so
 * the caller always proceeds to boot. */
void AssetFlasher_RunAtBoot(bool assetsValid, bool programmingRequested);

#ifdef __cplusplus
}
#endif

#endif /* ASSET_FLASHER_H */
