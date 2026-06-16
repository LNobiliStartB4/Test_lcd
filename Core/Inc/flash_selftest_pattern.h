#ifndef FLASH_SELFTEST_PATTERN_H
#define FLASH_SELFTEST_PATTERN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Deterministic, stateless pseudo-random byte for a given absolute byte index.
 *
 * Used by the external-flash self-test to write and then read back a known
 * pattern without any host/serial involvement. Being a pure function of the
 * index, any flash offset can be written and verified independently — no
 * running state is shared between the write pass and the read-back pass.
 */
uint8_t flash_selftest_pattern_byte(uint32_t index);

/*
 * Fill buffer[0..length) with the pattern for absolute indices
 * [startIndex, startIndex + length). A NULL buffer is a safe no-op.
 */
void flash_selftest_pattern_fill(uint8_t *buffer, uint32_t startIndex,
                                 uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_SELFTEST_PATTERN_H */
