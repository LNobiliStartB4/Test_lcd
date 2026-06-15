#ifndef ASSET_FLASH_LAYOUT_H
#define ASSET_FLASH_LAYOUT_H

/*
 * Pure (HAL-free) arithmetic for programming the external Winbond flash:
 * page-program chunking and sector-erase counting. Kept separate from the
 * HAL driver so the boundary math is unit-testable on the host.
 *
 * W25Q64: page program is limited to one 256-byte page (a write that crosses
 * a page boundary wraps within the page on the chip); erase granularity is the
 * 4 KB sector.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ASSET_FLASH_PAGE_SIZE   256U
#define ASSET_FLASH_SECTOR_SIZE 4096U

/* Bytes that can be programmed starting at `address` without crossing the next
 * page boundary, clamped to `remaining`. Returns 0 when remaining is 0. */
uint32_t asset_flash_page_chunk(uint32_t address, uint32_t remaining, uint32_t page_size);

/* Number of `sector_size` sectors that must be erased to hold `data_length`
 * bytes starting at a sector-aligned base (ceil division). 0 -> 0. */
uint32_t asset_flash_sectors_needed(uint32_t data_length, uint32_t sector_size);

#ifdef __cplusplus
}
#endif

#endif /* ASSET_FLASH_LAYOUT_H */
