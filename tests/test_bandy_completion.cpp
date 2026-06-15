#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <gui/model/BandyCompletion.hpp>

TEST_CASE("natural completion is running to wait RFID at zero")
{
    BandyState previous;
    previous.sessionState = BandySessionRunning;
    previous.remainingSeconds = 1U;

    BandyState current;
    current.sessionState = BandySessionWaitRfid;
    current.remainingSeconds = 0U;

    CHECK(isNaturalBandyCompletion(previous, current));
}

TEST_CASE("paused and authorized transitions are not natural completion")
{
    BandyState previous;
    previous.sessionState = BandySessionPaused;
    previous.remainingSeconds = 1U;

    BandyState current;
    current.sessionState = BandySessionWaitRfid;
    current.remainingSeconds = 0U;

    CHECK_FALSE(isNaturalBandyCompletion(previous, current));

    previous.sessionState = BandySessionAuthorized;
    CHECK_FALSE(isNaturalBandyCompletion(previous, current));
}

TEST_CASE("running transition with remaining time is not completion")
{
    BandyState previous;
    previous.sessionState = BandySessionRunning;

    BandyState current;
    current.sessionState = BandySessionWaitRfid;
    current.remainingSeconds = 12U;

    CHECK_FALSE(isNaturalBandyCompletion(previous, current));
}
