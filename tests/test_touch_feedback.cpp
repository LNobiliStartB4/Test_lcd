#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <gui/common/TouchFeedbackModel.hpp>

using touchfeedback::TouchFeedbackAnimator;
using touchfeedback::TouchFeedbackState;
using touchfeedback::TouchSample;

namespace
{
TouchSample sample(int16_t x, int16_t y, bool pressed, uint32_t activity)
{
    TouchSample s;
    s.x = x;
    s.y = y;
    s.pressed = pressed;
    s.activitySequence = activity;
    return s;
}

// Run the animator with no new activity for kFadeDurationTicks frames.
void coast(TouchFeedbackAnimator& anim, const TouchSample& still)
{
    for (uint8_t i = 0; i < TouchFeedbackAnimator::kFadeDurationTicks; ++i)
    {
        anim.update(still);
    }
}
}

TEST_CASE("TouchFeedbackState bumps activity on press and real moves only")
{
    TouchFeedbackState state;
    const uint32_t a0 = state.sample().activitySequence;

    state.recordPress(40, 50);
    TouchSample s = state.sample();
    CHECK(s.pressed == true);
    CHECK(s.x == 40);
    CHECK(s.y == 50);
    const uint32_t a1 = s.activitySequence;
    CHECK(a1 == a0 + 1U);

    // A real move bumps activity and follows.
    state.recordMove(120, 200);
    s = state.sample();
    CHECK(s.x == 120);
    CHECK(s.y == 200);
    CHECK(s.activitySequence == a1 + 1U);

    // A move to the same point does NOT count as activity.
    const uint32_t a2 = state.sample().activitySequence;
    state.recordMove(120, 200);
    CHECK(state.sample().activitySequence == a2);

    // Release does not bump activity; a move while released is ignored.
    state.recordRelease();
    s = state.sample();
    CHECK(s.pressed == false);
    CHECK(s.activitySequence == a2);
    state.recordMove(5, 6);
    s = state.sample();
    CHECK(s.x == 120);
    CHECK(s.activitySequence == a2);
}

TEST_CASE("Animator shows on activity and follows movement")
{
    TouchFeedbackAnimator anim;
    CHECK(anim.isVisible() == false);

    anim.update(sample(30, 40, true, 1));
    CHECK(anim.isVisible() == true);
    CHECK(anim.getAlpha() == TouchFeedbackAnimator::kVisibleAlpha);
    CHECK(anim.getX() == 30);
    CHECK(anim.getY() == 40);

    // New activity (a move) re-energizes and follows.
    anim.update(sample(33, 47, true, 2));
    CHECK(anim.isVisible() == true);
    CHECK(anim.getAlpha() == TouchFeedbackAnimator::kVisibleAlpha);
    CHECK(anim.getX() == 33);
    CHECK(anim.getY() == 47);
}

TEST_CASE("A held-still finger fades out instead of staying fixed")
{
    TouchFeedbackAnimator anim;
    anim.update(sample(100, 100, true, 1));
    REQUIRE(anim.isVisible());

    // Same activity sequence (finger pressed but not moving): must fade away,
    // not stay fixed. This is the case that lingered before a screen change.
    uint8_t prev = TouchFeedbackAnimator::kVisibleAlpha;
    for (uint8_t i = 0; i < TouchFeedbackAnimator::kFadeDurationTicks; ++i)
    {
        anim.update(sample(100, 100, true, 1));
        CHECK(anim.getAlpha() <= prev);
        prev = anim.getAlpha();
    }
    CHECK(anim.getAlpha() == 0);
    CHECK(anim.isVisible() == false);
}

TEST_CASE("Fade-out is short")
{
    CHECK(TouchFeedbackAnimator::kFadeDurationTicks <= 6);
}

TEST_CASE("reset() hides and prevents carry-over to the next screen")
{
    TouchFeedbackAnimator anim;
    anim.update(sample(50, 50, true, 3));
    REQUIRE(anim.isVisible());

    // Screen change: reset acknowledges current activity.
    anim.reset(sample(50, 50, true, 3));
    CHECK(anim.isVisible() == false);

    // Same activity (finger still down, not moving) must NOT re-show.
    anim.update(sample(50, 50, true, 3));
    CHECK(anim.isVisible() == false);
    coast(anim, sample(50, 50, true, 3));
    CHECK(anim.isVisible() == false);

    // A genuinely new touch activity shows it again.
    anim.update(sample(10, 20, true, 4));
    CHECK(anim.isVisible() == true);
}
