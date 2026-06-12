#include "asset_manifest.h"

#include <stddef.h>

static uint32_t read_u32_le(const uint8_t* d)
{
    return ((uint32_t)d[0]) |
           ((uint32_t)d[1] << 8) |
           ((uint32_t)d[2] << 16) |
           ((uint32_t)d[3] << 24);
}

uint32_t asset_crc32_update(uint32_t crc, const uint8_t* data, uint32_t length)
{
    uint32_t i;

    if (data == NULL)
    {
        return crc;
    }

    for (i = 0U; i < length; i++)
    {
        uint8_t bit;
        crc ^= data[i];
        for (bit = 0U; bit < 8U; bit++)
        {
            crc = ((crc & 1U) != 0U) ? ((crc >> 1) ^ 0xEDB88320UL) : (crc >> 1);
        }
    }

    return crc;
}

uint32_t asset_crc32_final(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFUL;
}

bool asset_manifest_parse(const uint8_t* raw, asset_manifest_t* out)
{
    if ((raw == NULL) || (out == NULL))
    {
        return false;
    }

    out->magic = read_u32_le(&raw[0]);
    out->version = read_u32_le(&raw[4]);
    out->data_length = read_u32_le(&raw[8]);
    out->expected_crc = read_u32_le(&raw[12]);
    return true;
}

bool asset_manifest_header_ok(const asset_manifest_t* manifest, uint32_t max_data_length)
{
    if (manifest == NULL)
    {
        return false;
    }
    if (manifest->magic != ASSET_MANIFEST_MAGIC)
    {
        return false;
    }
    if (manifest->version != ASSET_MANIFEST_VERSION)
    {
        return false;
    }
    if ((manifest->data_length == 0U) || (manifest->data_length > max_data_length))
    {
        return false;
    }
    return true;
}
