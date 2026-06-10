#ifndef TOUCHFEEDBACK_HPP
#define TOUCHFEEDBACK_HPP

#include <gui_generated/containers/TouchFeedbackBase.hpp>
#include <gui/common/TouchFeedbackController.hpp>

// On-screen touch feedback pointer (the "ripple" dot).
//
// One instance lives on every screen (added topmost in the TouchGFX Designer).
// It registers itself as the active widget with TouchFeedbackController; the
// controller drives its position/alpha each frame. The widget is non-touchable
// so it never intercepts touches meant for the UI underneath.
class TouchFeedback : public TouchFeedbackBase, public touchfeedback::ITouchFeedbackWidget
{
public:
    TouchFeedback();
    virtual ~TouchFeedback();

    virtual void initialize();

    // touchfeedback::ITouchFeedbackWidget
    virtual void applyFeedback(bool visible, int16_t x, int16_t y, uint8_t alpha);

private:
    // Half of the 48x48 container, so the dot is centered on the touch point.
    static const int16_t kHalfSize = 24;
};

#endif // TOUCHFEEDBACK_HPP
