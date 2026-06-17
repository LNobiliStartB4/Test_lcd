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

/*
 * Build-time switch for the external asset flash (W25Q64 on SPI3).
 *   1 (default): images may live in ExtFlashSection and are fetched from the
 *                W25Q64 at runtime; the boot-time asset flasher/validation runs.
 *   0          : the external flash is ignored at boot (no SPI3 probe, no asset
 *                validation/recovery) and the DataReader bridge becomes a no-op,
 *                so the firmware is a single internal-only HEX. In this mode
 *                EVERY drawn image MUST be internal (IntFlashSection) — set
 *                application.config image section to IntFlashSection so no asset
 *                ends up in ExtFlashSection. Unused functions are dropped by
 *                --gc-sections.
 */
#ifndef USE_EXTERNAL_FLASH
#define USE_EXTERNAL_FLASH 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define W25Q64_SIZE_BYTES            0x00800000UL  /* 8 MB */
#define W25Q64_VIRTUAL_BASE          0x90000000UL
#define W25Q64_ASSET_MANIFEST_OFFSET 0x007FF000UL  /* 8 MB - 4 KB */
#define W25Q64_DIAGNOSTIC_SAMPLE_COUNT 32U

typedef struct
{
  uint8_t spi_mode;
  uint8_t stable_count;
  uint8_t jedec_samples[W25Q64_DIAGNOSTIC_SAMPLE_COUNT][3];
  uint8_t manufacturer_device_id[2];
  uint8_t release_device_id;
  uint8_t status_registers[3];
  uint8_t sfdp_signature[4];
} w25q64_diagnostic_report_t;

bool W25Q64_Init(void);                 /* JEDEC ID check (EF 40 17); true if present */
bool W25Q64_IsReady(void);
void W25Q64_GetJedecId(uint8_t outId[3]);
bool W25Q64_ProbeStable(uint8_t sampleCount);
bool W25Q64_RunDiagnostic(uint8_t spiMode,
                          w25q64_diagnostic_report_t* report);
bool W25Q64_ValidateAssetPackage(void); /* CRC32-validates the asset blob via the manifest */

/* --- Read-only/serial-free self-test (bring-up diagnostics) -------------- */
typedef struct
{
  uint32_t bytes_tested;
  uint32_t errors;
  uint32_t first_bad_offset;
  uint8_t  expected_byte;
  uint8_t  got_byte;
  bool     ok;
} w25q64_selftest_result_t;

/* Erase + write a deterministic pattern over [0, volumeBytes), read it back and
 * verify byte-by-byte. No host/serial involvement — isolates the SPI/flash link
 * from the UART path. Fills *result; returns result->ok (errors == 0). */
bool W25Q64_SelfTest(uint32_t volumeBytes, w25q64_selftest_result_t* result);
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
