#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

extern "C" {
#include "asset_flash_layout.h"
}

// A page program may not cross a 256-byte page boundary on the W25Q64.
TEST_CASE("page chunk clamps to the page boundary and to remaining")
{
    const uint32_t P = ASSET_FLASH_PAGE_SIZE; // 256

    // Page-aligned: limited by remaining, then by a full page.
    CHECK(asset_flash_page_chunk(0, 256, P) == 256u);
    CHECK(asset_flash_page_chunk(0, 300, P) == 256u); // clamp to one page
    CHECK(asset_flash_page_chunk(0, 10, P) == 10u);

    // Mid-page start: limited to the bytes left in the current page.
    CHECK(asset_flash_page_chunk(100, 300, P) == 156u); // 256 - 100
    CHECK(asset_flash_page_chunk(250, 3, P) == 3u);      // fits in page
    CHECK(asset_flash_page_chunk(255, 5, P) == 1u);      // only byte 255 left
    CHECK(asset_flash_page_chunk(256, 10, P) == 10u);    // next page aligned

    // Nothing left to write.
    CHECK(asset_flash_page_chunk(0, 0, P) == 0u);
    CHECK(asset_flash_page_chunk(123, 0, P) == 0u);
}

TEST_CASE("sectors needed is a ceil division")
{
    const uint32_t S = ASSET_FLASH_SECTOR_SIZE; // 4096

    CHECK(asset_flash_sectors_needed(0, S) == 0u);
    CHECK(asset_flash_sectors_needed(1, S) == 1u);
    CHECK(asset_flash_sectors_needed(S, S) == 1u);
    CHECK(asset_flash_sectors_needed(S + 1, S) == 2u);
    CHECK(asset_flash_sectors_needed(2 * S, S) == 2u);
    CHECK(asset_flash_sectors_needed(2 * S - 1, S) == 2u);
}
