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

    // Center the dot on the touch point; moveTo invalidates old + new areas.
    if (!isVisible())
    {
        setVisible(true);
    }
    dot.setAlpha(alpha);
    moveTo(static_cast<int16_t>(x - kHalfSize), static_cast<int16_t>(y - kHalfSize));
    invalidate();
#else
    (void)visible;
    (void)x;
    (void)y;
    (void)alpha;
#endif
}
