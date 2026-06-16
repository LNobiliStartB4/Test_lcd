#include "flash_selftest_pattern.h"

#include <stddef.h>

uint8_t flash_selftest_pattern_byte(uint32_t index)
{
  /* Stateless avalanche hash of the index (MurmurHash3 finalizer style):
   * a one-bit change in the index flips ~half the output bits, so the byte
   * stream is near-uniform and strongly position-dependent — good at exposing
   * dropped/shifted/stuck bytes on the SPI link, yet fully reproducible. */
  uint32_t x = index + 0x9E3779B9U; /* offset so index 0 is not trivial */

  x ^= x >> 16;
  x *= 0x85EBCA6BU;
  x ^= x >> 13;
  x *= 0xC2B2AE35U;
  x ^= x >> 16;

  return (uint8_t)(x & 0xFFU);
}

void flash_selftest_pattern_fill(uint8_t *buffer, uint32_t startIndex,
                                 uint32_t length)
{
  uint32_t i;

  if (buffer == NULL)
  {
    return;
  }

  for (i = 0U; i < length; i++)
  {
    buffer[i] = flash_selftest_pattern_byte(startIndex + i);
  }
}
