#ifndef TOUCHFEEDBACKMODEL_HPP
#define TOUCHFEEDBACKMODEL_HPP

#include <stdint.h>

// Framework-free logic for the touch feedback "ripple" pointer.
//
// Split in two pieces so it can be unit tested without TouchGFX:
//  - TouchFeedbackState : records the latest touch event (written from the
//    application event hooks, read once per frame).
//  - TouchFeedbackAnimator : turns that state into a position + alpha for the
//    on-screen widget, handling "follow the finger" and the fade-out on release.
//
// The widget itself (gui widget) only forwards ticks to the animator and maps
// the result onto a Drawable; it owns no logic.
namespace touchfeedback
{
struct TouchSample
{
    int16_t x;
    int16_t y;
    bool pressed;
    uint32_t pressSequence; // incremented on every new press (catches quick taps)
};

class TouchFeedbackState
{
public:
    TouchFeedbackState()
        : x(0), y(0), pressed(false), pressSequence(0U)
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
        ++pressSequence;
    }

    void recordMove(int16_t px, int16_t py)
    {
        if (pressed)
        {
            x = px;
            y = py;
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
        s.pressSequence = pressSequence;
        return s;
    }

private:
    int16_t x;
    int16_t y;
    bool pressed;
    uint32_t pressSequence;
};

class TouchFeedbackAnimator
{
public:
    // Widget alpha shown while the finger is down (the dot asset itself carries
    // the soft translucency). 250 ms fade-out at the 60 Hz tick rate.
    static const uint8_t kVisibleAlpha = 255U;
    static const uint8_t kFadeDurationTicks = 15U;

    TouchFeedbackAnimator()
        : phase(Hidden), x(0), y(0), alpha(0U), fadeRemaining(0U), lastPressSequence(0U)
    {
    }

    // Call once per frame with the most recent touch sample.
    void update(const TouchSample& s)
    {
        const bool newPress = (s.pressSequence != lastPressSequence);
        if (newPress)
        {
            lastPressSequence = s.pressSequence;
            show(s.x, s.y);
        }

        if (s.pressed)
        {
            show(s.x, s.y);
            return;
        }

        if (phase == Visible)
        {
            phase = Fading;
            fadeRemaining = kFadeDurationTicks;
        }

        if (phase == Fading)
        {
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

    void show(int16_t px, int16_t py)
    {
        phase = Visible;
        x = px;
        y = py;
        alpha = kVisibleAlpha;
    }

    Phase phase;
    int16_t x;
    int16_t y;
    uint8_t alpha;
    uint8_t fadeRemaining;
    uint32_t lastPressSequence;
};
} // namespace touchfeedback

#endif // TOUCHFEEDBACKMODEL_HPP
