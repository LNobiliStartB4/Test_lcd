#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

extern "C" {
#include "asset_manifest.h"
}

// Standard CRC-32 (poly 0xEDB88320, init 0xFFFFFFFF, final XOR) — same algorithm
// the external-flash driver uses to validate the asset package.
TEST_CASE("CRC32 matches standard vectors")
{
    CHECK(asset_crc32_final(asset_crc32_update(ASSET_CRC32_INIT,
                                               reinterpret_cast<const uint8_t*>(""), 0)) == 0x00000000u);

    const char* check = "123456789";
    CHECK(asset_crc32_final(asset_crc32_update(ASSET_CRC32_INIT,
                                               reinterpret_cast<const uint8_t*>(check), 9)) == 0xCBF43926u);
}

TEST_CASE("CRC32 is chunkable (incremental == one-shot)")
{
    const char* s = "123456789";
    uint32_t one = asset_crc32_update(ASSET_CRC32_INIT, reinterpret_cast<const uint8_t*>(s), 9);
    uint32_t inc = asset_crc32_update(ASSET_CRC32_INIT, reinterpret_cast<const uint8_t*>(s), 4);
    inc = asset_crc32_update(inc, reinterpret_cast<const uint8_t*>(s) + 4, 5);
    CHECK(asset_crc32_final(one) == asset_crc32_final(inc));
}

TEST_CASE("manifest parse reads little-endian fields")
{
    uint8_t raw[ASSET_MANIFEST_SIZE] = {0};
    raw[0] = 0x54; raw[1] = 0x48; raw[2] = 0x44; raw[3] = 0x41; // "ADHT" LE -> 0x41444854
    raw[4] = 0x02;                                              // version = 2
    raw[8] = 0x00; raw[9] = 0x01; raw[10] = 0x00; raw[11] = 0x00; // data_length = 256
    raw[12] = 0xEF; raw[13] = 0xBE; raw[14] = 0xAD; raw[15] = 0xDE; // crc = 0xDEADBEEF
    for (uint8_t i = 0; i < ASSET_PACKAGE_ID_SIZE; ++i)
    {
        raw[16 + i] = static_cast<uint8_t>(0xA0u + i);
    }

    asset_manifest_t m;
    REQUIRE(asset_manifest_parse(raw, &m) == true);
    CHECK(m.magic == ASSET_MANIFEST_MAGIC);
    CHECK(m.version == ASSET_MANIFEST_VERSION);
    CHECK(m.data_length == 256u);
    CHECK(m.expected_crc == 0xDEADBEEFu);
    for (uint8_t i = 0; i < ASSET_PACKAGE_ID_SIZE; ++i)
    {
        CHECK(m.package_id[i] == static_cast<uint8_t>(0xA0u + i));
    }
}

TEST_CASE("manifest header validation (W25Q64: max 0x7FF000)")
{
    const uint32_t kMax = 0x007FF000u;
    asset_manifest_t m;
    m.magic = ASSET_MANIFEST_MAGIC;
    m.version = ASSET_MANIFEST_VERSION;
    m.data_length = 256u;
    m.expected_crc = 0u;
    for (uint8_t i = 0; i < ASSET_PACKAGE_ID_SIZE; ++i)
    {
        m.package_id[i] = static_cast<uint8_t>(i + 1u);
    }

    CHECK(asset_manifest_header_ok(&m, kMax) == true);

    asset_manifest_t bad = m; bad.magic = 0u;
    CHECK(asset_manifest_header_ok(&bad, kMax) == false);
    bad = m; bad.version = ASSET_MANIFEST_VERSION + 1u;
    CHECK(asset_manifest_header_ok(&bad, kMax) == false);
    bad = m; bad.data_length = 0u;
    CHECK(asset_manifest_header_ok(&bad, kMax) == false);
    bad = m; bad.data_length = 0x00800000u; // > max
    CHECK(asset_manifest_header_ok(&bad, kMax) == false);
    bad = m;
    for (uint8_t i = 0; i < ASSET_PACKAGE_ID_SIZE; ++i)
    {
        bad.package_id[i] = 0u;
    }
    CHECK(asset_manifest_header_ok(&bad, kMax) == false);
}

TEST_CASE("manifest must match the package embedded in the MCU firmware")
{
    asset_manifest_t expected = {};
    expected.magic = ASSET_MANIFEST_MAGIC;
    expected.version = ASSET_MANIFEST_VERSION;
    expected.data_length = 237616u;
    expected.expected_crc = 0xBE7B78A0u;
    for (uint8_t i = 0; i < ASSET_PACKAGE_ID_SIZE; ++i)
    {
        expected.package_id[i] = static_cast<uint8_t>(0x10u + i);
    }

    asset_manifest_t actual = expected;
    CHECK(asset_manifest_matches_expected(&actual, &expected) == true);

    actual.data_length++;
    CHECK(asset_manifest_matches_expected(&actual, &expected) == false);
    actual = expected;
    actual.expected_crc ^= 1u;
    CHECK(asset_manifest_matches_expected(&actual, &expected) == false);
    actual = expected;
    actual.package_id[7] ^= 1u;
    CHECK(asset_manifest_matches_expected(&actual, &expected) == false);
}
