#ifndef ASSET_MANIFEST_H
#define ASSET_MANIFEST_H

/*
 * Pure, hardware-free helpers to validate the TouchGFX external-flash asset
 * package stored on the Winbond. Kept HAL-free so it is unit-testable on the
 * host; the W25Qxx driver uses it to CRC-check the asset blob at boot.
 *
 * Manifest layout (little-endian, 32 bytes, at the end of the flash):
 *   [0..3]  magic        = 0x41444854 ("ADHT")
 *   [4..7]  version      = 2
 *   [8..11] data_length  = number of asset bytes (from flash offset 0)
 *   [12..15]expected_crc = CRC-32 of those bytes
 *   [16..31]package_id   = first 128 bits of SHA-256(asset bytes)
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ASSET_MANIFEST_MAGIC    0x41444854UL  /* "ADHT" little-endian */
#define ASSET_MANIFEST_VERSION  2UL
#define ASSET_PACKAGE_ID_SIZE   16U
#define ASSET_MANIFEST_SIZE     32U
#define ASSET_CRC32_INIT        0xFFFFFFFFUL

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t data_length;
    uint32_t expected_crc;
    uint8_t package_id[ASSET_PACKAGE_ID_SIZE];
} asset_manifest_t;

/* CRC-32 (poly 0xEDB88320). Seed with ASSET_CRC32_INIT, accumulate over chunks,
 * then asset_crc32_final() to get the standard CRC-32 value. */
uint32_t asset_crc32_update(uint32_t crc, const uint8_t* data, uint32_t length);
uint32_t asset_crc32_final(uint32_t crc);

/* Parse the first 16 manifest bytes (LE) into out. Returns false on NULL args. */
bool asset_manifest_parse(const uint8_t* raw, asset_manifest_t* out);

/* True if magic/version are correct and data_length is in (0, max_data_length]. */
bool asset_manifest_header_ok(const asset_manifest_t* manifest, uint32_t max_data_length);

/* Exact firmware-to-assets compatibility check. */
bool asset_manifest_matches_expected(const asset_manifest_t* actual,
                                     const asset_manifest_t* expected);

#ifdef __cplusplus
}
#endif

#endif /* ASSET_MANIFEST_H */
