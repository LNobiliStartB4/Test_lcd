#ifndef W25Q64_H
#define W25Q64_H

/*
 * Winbond W25Q64 (8 MB) SPI NOR flash driver — holds the TouchGFX image assets.
 * Driven via SPI3 (hspi3) with software CS on ASSET_FLASH_CS (PC3). DMA reads
 * use DMA1_Stream0 (SPI3_RX). Assets are linked at the virtual base 0x90000000
 * (ExtFlashSection); TouchGFX's FlashDataReader fetches them on demand.
 *
 * Ported from the UnifiedFirmware W25Q128JV driver and adapted to the 8 MB part
 * (JEDEC capacity 0x17). Manifest validation uses the host-tested asset_manifest.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define W25Q64_SIZE_BYTES            0x00800000UL  /* 8 MB */
#define W25Q64_VIRTUAL_BASE          0x90000000UL
#define W25Q64_ASSET_MANIFEST_OFFSET 0x007FF000UL  /* 8 MB - 4 KB */

bool W25Q64_Init(void);                 /* JEDEC ID check (EF 40 17); true if present */
bool W25Q64_IsReady(void);
bool W25Q64_ValidateAssetPackage(void); /* CRC32-validates the asset blob via the manifest */
bool W25Q64_Read(uint32_t address, uint8_t* data, uint32_t length);       /* blocking */
bool W25Q64_StartDmaRead(uint32_t address, uint8_t* data, uint32_t length);/* non-blocking */
bool W25Q64_IsDmaReadActive(void);
void W25Q64_WaitForDmaRead(void);
void W25Q64_DmaCompleteCallback(void);

/* --- Programming (used by the UART asset flasher; not on the read path) --- */
bool W25Q64_EraseSector(uint32_t address); /* erases the 4 KB sector containing address */
bool W25Q64_EraseChip(void);               /* full chip erase (seconds) */
/* Program up to one 256-byte page; the write must not cross a page boundary. */
bool W25Q64_ProgramPage(uint32_t address, const uint8_t* data, uint32_t length);
/* Program an arbitrary span, splitting into page-bounded ProgramPage calls.
 * The target sectors must already be erased. */
bool W25Q64_Write(uint32_t address, const uint8_t* data, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* W25Q64_H */
