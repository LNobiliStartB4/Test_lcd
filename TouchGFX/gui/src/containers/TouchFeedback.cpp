#include <gui/containers/TouchFeedback.hpp>

TouchFeedback::TouchFeedback()
{
    touchfeedback::TouchFeedbackController::instance().setWidget(this);
}

TouchFeedback::~TouchFeedback()
{
    touchfeedback::TouchFeedbackController::instance().clearWidget(this);
}

void TouchFeedback::initialize()
{
    TouchFeedbackBase::initialize();
    // Must never steal touches from the UI underneath, and starts hidden.
    setTouchable(false);
    dot.setTouchable(false);
    setVisible(false);
}

void TouchFeedback::applyFeedback(bool visible, int16_t x, int16_t y, uint8_t alpha)
{
#if TOUCH_FEEDBACK_ENABLED
    if (!visible)
    {
        if (isVisible())
        {
            setVisible(false);
            invalidate();
        }
        return;
    }

    // Position the dot relative to the touch point (kHalfSize offset).
    const int16_t newX = static_cast<int16_t>(x - kHalfSize);
    const int16_t newY = static_cast<int16_t>(y - kHalfSize);
    const bool moved = (newX != getX()) || (newY != getY());

    if (!isVisible())
    {
        setVisible(true);
    }
    dot.setAlpha(alpha);
    moveTo(newX, newY); // invalidates old + new areas when the position changes
    if (!moved)
    {
        // Same position (e.g. a fade frame, alpha only): moveTo did nothing, so
        // force a redraw to reflect the new alpha.
        invalidate();
    }
#else
    (void)visible;
    (void)x;
    (void)y;
    (void)alpha;
#endif
}
