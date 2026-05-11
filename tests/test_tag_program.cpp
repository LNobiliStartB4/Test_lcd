/* Logic specification test for the tag-programming decision tree.
 *
 * Mirrors the branching in rfidTagRW.c around lines 602-615:
 *
 *     if (proStationP->rfidProgramTag) {
 *         tagP->examNum = TAG_PROGRAM_DEFAULT_EXAM_NUM;
 *     } else if (tagP->examNum > 0) {
 *         if (rfidUpdateTag) tagP->examNum--;
 *         if (rfidEraseTag)  tagP->examNum = 0;
 *     } else {
 *         return TAG_EXPIRED;
 *     }
 *
 * This file does NOT compile the firmware directly (it depends on HAL +
 * RFAL). It re-encodes the same decision so we can lock down the
 * contract: any future change in rfidTagRW.c that diverges from this
 * truth table should be reflected here too.
 */
#include <doctest/doctest.h>

#include <cstdint>

namespace
{
constexpr uint16_t TAG_PROGRAM_DEFAULT_EXAM_NUM = 10U;

enum class DecisionOutcome
{
    Continue,
    Expired
};

struct TagState
{
    uint16_t examNum;
};

struct StationFlags
{
    bool rfidUpdateTag;
    bool rfidEraseTag;
    bool rfidProgramTag;
};

/* Re-implementation of the decision branch in rfidTagRW.c. */
DecisionOutcome applyTagDecision(TagState& tag, StationFlags& flags)
{
    if (flags.rfidProgramTag)
    {
        tag.examNum = TAG_PROGRAM_DEFAULT_EXAM_NUM;
        return DecisionOutcome::Continue;
    }
    if (tag.examNum > 0U)
    {
        if (flags.rfidUpdateTag)
        {
            tag.examNum--;
        }
        if (flags.rfidEraseTag)
        {
            tag.examNum = 0U;
        }
        return DecisionOutcome::Continue;
    }
    return DecisionOutcome::Expired;
}

/* Mirrors the post-write flag-reset block in rfidTagRW.c. */
void applyPostWriteReset(StationFlags& flags)
{
    if (flags.rfidEraseTag)
    {
        flags.rfidEraseTag = false;
    }
    if (flags.rfidProgramTag)
    {
        flags.rfidProgramTag = false;
    }
}
}

TEST_CASE("rfidProgramTag overrides expired tag and resets examNum to default")
{
    TagState tag{0U};
    StationFlags flags{false, false, true};

    auto outcome = applyTagDecision(tag, flags);

    CHECK(outcome == DecisionOutcome::Continue);
    CHECK(tag.examNum == TAG_PROGRAM_DEFAULT_EXAM_NUM);
}

TEST_CASE("rfidProgramTag wins over rfidEraseTag")
{
    TagState tag{3U};
    StationFlags flags{true, true, true};

    auto outcome = applyTagDecision(tag, flags);

    CHECK(outcome == DecisionOutcome::Continue);
    CHECK(tag.examNum == TAG_PROGRAM_DEFAULT_EXAM_NUM);
}

TEST_CASE("Normal read with rfidUpdateTag decrements examNum")
{
    TagState tag{5U};
    StationFlags flags{true, false, false};

    auto outcome = applyTagDecision(tag, flags);

    CHECK(outcome == DecisionOutcome::Continue);
    CHECK(tag.examNum == 4U);
}

TEST_CASE("Normal read without rfidUpdateTag leaves examNum unchanged")
{
    TagState tag{5U};
    StationFlags flags{false, false, false};

    auto outcome = applyTagDecision(tag, flags);

    CHECK(outcome == DecisionOutcome::Continue);
    CHECK(tag.examNum == 5U);
}

TEST_CASE("rfidEraseTag sets examNum to 0 even if update would decrement")
{
    TagState tag{5U};
    StationFlags flags{true, true, false};

    auto outcome = applyTagDecision(tag, flags);

    CHECK(outcome == DecisionOutcome::Continue);
    CHECK(tag.examNum == 0U);
}

TEST_CASE("Expired tag without program flag returns Expired")
{
    TagState tag{0U};
    StationFlags flags{true, false, false};

    auto outcome = applyTagDecision(tag, flags);

    CHECK(outcome == DecisionOutcome::Expired);
    CHECK(tag.examNum == 0U);
}

TEST_CASE("Post-write resets erase and program flags but not update")
{
    StationFlags flags{true, true, true};
    applyPostWriteReset(flags);

    CHECK(flags.rfidUpdateTag == true);
    CHECK(flags.rfidEraseTag == false);
    CHECK(flags.rfidProgramTag == false);
}

TEST_CASE("TAG_PROGRAM_DEFAULT_EXAM_NUM constant matches firmware contract")
{
    /* If you bump TAG_PROGRAM_DEFAULT_EXAM_NUM in rfidTagRW.h, update this
       assertion too — the constant is a public contract used by serial
       command TAGRESET. */
    CHECK(TAG_PROGRAM_DEFAULT_EXAM_NUM == 10U);
    CHECK(TAG_PROGRAM_DEFAULT_EXAM_NUM > 0U);
    CHECK(TAG_PROGRAM_DEFAULT_EXAM_NUM <= 5000U); /* MAX_NUMBER_OF_EXAMINATION */
}
