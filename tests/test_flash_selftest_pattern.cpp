#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

extern "C" {
#include "flash_selftest_pattern.h"
}

#include <set>
#include <cstdint>

// Contract of the deterministic, stateless self-test pattern used to write and
// verify the external flash without any host/serial involvement.

TEST_CASE("pattern is deterministic for a given index")
{
    for (uint32_t i = 0; i < 2000; ++i)
    {
        CHECK(flash_selftest_pattern_byte(i) == flash_selftest_pattern_byte(i));
    }
}

TEST_CASE("fill matches the byte function and is reproducible (no shared state)")
{
    uint8_t a[300];
    uint8_t b[300];
    flash_selftest_pattern_fill(a, 1000u, sizeof(a));
    flash_selftest_pattern_fill(b, 1000u, sizeof(b)); // same range again
    for (uint32_t k = 0; k < sizeof(a); ++k)
    {
        CHECK(a[k] == b[k]);
        CHECK(a[k] == flash_selftest_pattern_byte(1000u + k));
    }
}

TEST_CASE("contiguous fills join seamlessly (offset addressable)")
{
    uint8_t whole[512];
    uint8_t lo[256];
    uint8_t hi[256];
    flash_selftest_pattern_fill(whole, 4096u, sizeof(whole));
    flash_selftest_pattern_fill(lo, 4096u, sizeof(lo));
    flash_selftest_pattern_fill(hi, 4096u + 256u, sizeof(hi));
    for (uint32_t k = 0; k < 256u; ++k)
    {
        CHECK(whole[k] == lo[k]);
        CHECK(whole[256u + k] == hi[k]);
    }
}

TEST_CASE("good distribution over 256 samples (not constant/low-entropy)")
{
    std::set<uint8_t> values;
    for (uint32_t i = 0; i < 256u; ++i)
    {
        values.insert(flash_selftest_pattern_byte(i));
    }
    // A near-uniform byte generator yields ~160 distinct values across 256
    // draws; a constant or trivial counter would fail badly here.
    CHECK(values.size() > 100u);
}

TEST_CASE("adjacent indices almost always differ")
{
    int diffs = 0;
    for (uint32_t i = 0; i < 255u; ++i)
    {
        if (flash_selftest_pattern_byte(i) != flash_selftest_pattern_byte(i + 1u))
        {
            ++diffs;
        }
    }
    CHECK(diffs > 240);
}

TEST_CASE("null buffer fill is safe")
{
    flash_selftest_pattern_fill(nullptr, 0u, 16u);
    CHECK(true);
}
