#ifndef TOUCHFEEDBACKMODEL_HPP
#define TOUCHFEEDBACKMODEL_HPP

#include <stdint.h>

// Framework-free logic for the touch feedback "ripple" pointer.
//
//  - TouchFeedbackState records the latest touch event. It exposes an
//    "activity sequence" that bumps on a press and on each real finger move.
//  - TouchFeedbackAnimator turns that into a position + alpha. The pointer is
//    transient: any new activity shows it at full alpha; with no new activity
//    it fades out quickly. So a finger held still (e.g. a tap that is about to
//    change screen) does NOT stay fixed, while an active drag keeps it visible
//    and following the finger.
//
// The widget (gui widget) only forwards ticks to the animator and maps the
// result onto a Drawable; it owns no logic.
namespace touchfeedback
{
struct TouchSample
{
    int16_t x;
    int16_t y;
    bool pressed;
    uint32_t activitySequence; // bumps on press and on each real move
};

class TouchFeedbackState
{
public:
    // Minimum finger travel (Manhattan, px) to count as a move. Filters out the
    // capacitive touch jitter on the real device so a held-still finger does not
    // keep the pointer alive.
    static const int16_t kMoveThreshold = 5;

    TouchFeedbackState()
        : x(0), y(0), pressed(false), activitySequence(0U)
    {
    }

    static TouchFeedbackState& instance()
    {
        static TouchFeedbackState state;
        return state;
    }

    void recordPress(int16_t px, int16_t py)
    {
        x = px;
        y = py;
        pressed = true;
        ++activitySequence;
    }

    void recordMove(int16_t px, int16_t py)
    {
        // Only a real move (beyond the jitter threshold) counts as activity; a
        // held-still finger, even with capacitive jitter, does not.
        if (!pressed)
        {
            return;
        }
        const int16_t dx = static_cast<int16_t>(px - x);
        const int16_t dy = static_cast<int16_t>(py - y);
        const int32_t distance = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
        if (distance >= kMoveThreshold)
        {
            x = px;
            y = py;
            ++activitySequence;
        }
    }

    void recordRelease()
    {
        pressed = false;
    }

    TouchSample sample() const
    {
        TouchSample s;
        s.x = x;
        s.y = y;
        s.pressed = pressed;
        s.activitySequence = activitySequence;
        return s;
    }

private:
    int16_t x;
    int16_t y;
    bool pressed;
    uint32_t activitySequence;
};

class TouchFeedbackAnimator
{
public:
    // Widget alpha shown on activity (the dot asset carries the soft
    // translucency). Short fade (~80 ms at the 60 Hz tick rate).
    static const uint8_t kVisibleAlpha = 255U;
    static const uint8_t kFadeDurationTicks = 5U;

    TouchFeedbackAnimator()
        : phase(Hidden), x(0), y(0), alpha(0U), fadeRemaining(0U), lastActivity(0U)
    {
    }

    // Force hidden (e.g. on a screen change) and acknowledge current activity so
    // the in-progress touch does not re-show the dot on the next screen.
    void reset(const TouchSample& s)
    {
        phase = Hidden;
        alpha = 0U;
        fadeRemaining = 0U;
        lastActivity = s.activitySequence;
    }

    // Call once per frame with the most recent touch sample.
    void update(const TouchSample& s)
    {
        if (s.activitySequence != lastActivity)
        {
            // New activity (press or move): show at the point, full alpha.
            lastActivity = s.activitySequence;
            phase = Visible;
            x = s.x;
            y = s.y;
            alpha = kVisibleAlpha;
            fadeRemaining = kFadeDurationTicks;
            return;
        }

        // No new activity this frame: fade out.
        if (phase != Hidden)
        {
            phase = Fading;
            if (fadeRemaining > 0U)
            {
                --fadeRemaining;
            }
            alpha = static_cast<uint8_t>((static_cast<uint32_t>(kVisibleAlpha) * fadeRemaining) /
                                         kFadeDurationTicks);
            if (fadeRemaining == 0U)
            {
                phase = Hidden;
                alpha = 0U;
            }
        }
    }

    bool isVisible() const
    {
        return phase != Hidden;
    }

    int16_t getX() const
    {
        return x;
    }

    int16_t getY() const
    {
        return y;
    }

    uint8_t getAlpha() const
    {
        return alpha;
    }

private:
    enum Phase
    {
        Hidden,
        Visible,
        Fading
    };

    Phase phase;
    int16_t x;
    int16_t y;
    uint8_t alpha;
    uint8_t fadeRemaining;
    uint32_t lastActivity;
};
} // namespace touchfeedback

#endif // TOUCHFEEDBACKMODEL_HPP
