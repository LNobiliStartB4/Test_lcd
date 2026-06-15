#ifndef ASSET_FLASHER_H
#define ASSET_FLASHER_H

/*
 * One-time programmer for the external Winbond W25Q64 over USART2. Lets you
 * push the TouchGFX asset blob onto the soldered flash with no extra hardware:
 * call AssetFlasher_RunIfRequested() early in main() (after MX_SPI3_Init and
 * W25Q64_Init); it listens briefly for a "knock" from the host tool
 * (tools/flash_assets.py). If the knock arrives it erases, programs and
 * validates the flash, then resets; otherwise it returns and the app boots
 * normally.
 *
 * Wire protocol (host -> device), all multi-byte fields little-endian:
 *   header: "ADHTPROG" (8B) | data_length (4B) | crc32 (4B)
 *   device replies 'R' (ready, erase done) | 'E' (rejected)
 *   then data_length bytes streamed in <=256B chunks, each ACKed 'K' (or 'E')
 *   finally device writes the manifest, validates, replies 'D' (ok) | 'F' (fail)
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns true if a flashing session ran (regardless of outcome), false if no
 * knock arrived within knockTimeoutMs and the app should boot normally. */
bool AssetFlasher_RunIfRequested(uint32_t knockTimeoutMs);

#ifdef __cplusplus
}
#endif

#endif /* ASSET_FLASHER_H */
