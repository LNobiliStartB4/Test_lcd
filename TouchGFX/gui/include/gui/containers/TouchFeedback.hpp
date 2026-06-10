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
    // Touch-point offset for the 28x28 container (kept at the same ratio used
    // for the previous 40x40 size).
    static const int16_t kHalfSize = 7;
};

#endif // TOUCHFEEDBACK_HPP
