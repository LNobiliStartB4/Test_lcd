#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <gui/common/TouchFeedbackModel.hpp>

using touchfeedback::TouchFeedbackAnimator;
using touchfeedback::TouchFeedbackState;
using touchfeedback::TouchSample;

namespace
{
TouchSample press(int16_t x, int16_t y, uint32_t seq)
{
    TouchSample s;
    s.x = x;
    s.y = y;
    s.pressed = true;
    s.pressSequence = seq;
    return s;
}

TouchSample release(int16_t x, int16_t y, uint32_t seq)
{
    TouchSample s = press(x, y, seq);
    s.pressed = false;
    return s;
}
}

TEST_CASE("TouchFeedbackState records press/move/release")
{
    TouchFeedbackState state;

    const TouchSample idle = state.sample();
    CHECK(idle.pressed == false);

    state.recordPress(40, 50);
    TouchSample s = state.sample();
    CHECK(s.pressed == true);
    CHECK(s.x == 40);
    CHECK(s.y == 50);
    const uint32_t seqAfterPress = s.pressSequence;
    CHECK(seqAfterPress == idle.pressSequence + 1U);

    // Move while pressed follows the finger.
    state.recordMove(120, 200);
    s = state.sample();
    CHECK(s.x == 120);
    CHECK(s.y == 200);
    CHECK(s.pressSequence == seqAfterPress); // move does not start a new press

    // Release clears pressed but keeps the sequence.
    state.recordRelease();
    s = state.sample();
    CHECK(s.pressed == false);
    CHECK(s.pressSequence == seqAfterPress);

    // Move while released is ignored.
    state.recordMove(5, 6);
    s = state.sample();
    CHECK(s.x == 120);
    CHECK(s.y == 200);
}

TEST_CASE("Animator shows and follows while pressed")
{
    TouchFeedbackAnimator anim;
    CHECK(anim.isVisible() == false);
    CHECK(anim.getAlpha() == 0);

    anim.update(press(30, 40, 1));
    CHECK(anim.isVisible() == true);
    CHECK(anim.getAlpha() == TouchFeedbackAnimator::kVisibleAlpha);
    CHECK(anim.getX() == 30);
    CHECK(anim.getY() == 40);

    // Dragging (same press sequence, still pressed) follows the point.
    anim.update(press(33, 47, 1));
    CHECK(anim.isVisible() == true);
    CHECK(anim.getX() == 33);
    CHECK(anim.getY() == 47);
    CHECK(anim.getAlpha() == TouchFeedbackAnimator::kVisibleAlpha);
}

TEST_CASE("Animator fades out after release and disappears")
{
    TouchFeedbackAnimator anim;
    anim.update(press(100, 100, 1));
    REQUIRE(anim.isVisible());

    anim.update(release(100, 100, 1)); // first released tick: start fading
    CHECK(anim.isVisible() == true);
    uint8_t prev = anim.getAlpha();
    CHECK(prev < TouchFeedbackAnimator::kVisibleAlpha); // alpha started dropping

    // Keep ticking with no touch; alpha must monotonically decrease to 0.
    for (uint8_t i = 0; i < TouchFeedbackAnimator::kFadeDurationTicks; ++i)
    {
        anim.update(release(100, 100, 1));
        CHECK(anim.getAlpha() <= prev);
        prev = anim.getAlpha();
    }
    CHECK(anim.getAlpha() == 0);
    CHECK(anim.isVisible() == false);
}

TEST_CASE("Animator catches a quick tap (press+release within one frame)")
{
    TouchFeedbackAnimator anim;
    // New press sequence but already released by the time the animator ticks.
    anim.update(release(70, 80, 5));
    CHECK(anim.isVisible() == true); // the tap is shown for at least one frame
    CHECK(anim.getX() == 70);
    CHECK(anim.getY() == 80);

    // Then it fades away.
    for (uint8_t i = 0; i < TouchFeedbackAnimator::kFadeDurationTicks; ++i)
    {
        anim.update(release(70, 80, 5));
    }
    CHECK(anim.isVisible() == false);
}
