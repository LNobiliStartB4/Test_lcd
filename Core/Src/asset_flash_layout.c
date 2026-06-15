#include "asset_flash_layout.h"

uint32_t asset_flash_page_chunk(uint32_t address, uint32_t remaining, uint32_t page_size)
{
    uint32_t page_remaining;

    if ((remaining == 0U) || (page_size == 0U))
    {
        return 0U;
    }

    page_remaining = page_size - (address % page_size);
    return (remaining < page_remaining) ? remaining : page_remaining;
}

uint32_t asset_flash_sectors_needed(uint32_t data_length, uint32_t sector_size)
{
    if ((data_length == 0U) || (sector_size == 0U))
    {
        return 0U;
    }

    return (data_length + sector_size - 1U) / sector_size;
}
