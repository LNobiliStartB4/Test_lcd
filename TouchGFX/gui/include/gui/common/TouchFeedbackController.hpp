#ifndef TOUCHFEEDBACKCONTROLLER_HPP
#define TOUCHFEEDBACKCONTROLLER_HPP

#include <gui/common/TouchFeedbackModel.hpp>
#include <stdint.h>

// Master switch for the whole touch-feedback feature. When 0 the controller
// becomes inert (it never shows the pointer) at zero runtime cost.
#ifndef TOUCH_FEEDBACK_ENABLED
#define TOUCH_FEEDBACK_ENABLED 1
#endif

namespace touchfeedback
{
// Implemented by the on-screen widget (the TouchFeedback custom container).
// The controller pushes the animated result to it once per frame.
class ITouchFeedbackWidget
{
public:
    virtual void applyFeedback(bool visible, int16_t x, int16_t y, uint8_t alpha) = 0;

protected:
    ~ITouchFeedbackWidget() {}
};

// Glue between the framework event hooks (FrontendApplication) and the widget
// instance that lives on the active screen. Framework-free and header-only so
// it stays unit-testable; the only stateful logic is in TouchFeedbackModel.
class TouchFeedbackController
{
public:
    static TouchFeedbackController& instance()
    {
        static TouchFeedbackController controller;
        return controller;
    }

    // Called by the active screen's widget when it is constructed / destroyed.
    // Registering a new widget means a screen change: hide the pointer so a
    // fading dot does not carry over onto the new screen.
    void setWidget(ITouchFeedbackWidget* w)
    {
        widget = w;
        animator.reset(state.sample());
    }
    void clearWidget(const ITouchFeedbackWidget* w)
    {
        if (widget == w)
        {
            widget = 0;
        }
    }

    // Called from the application touch event hooks.
    void recordPress(int16_t x, int16_t y)
    {
        state.recordPress(x, y);
    }
    void recordMove(int16_t x, int16_t y)
    {
        state.recordMove(x, y);
    }
    void recordRelease()
    {
        state.recordRelease();
    }

    // Called once per frame (from FrontendApplication::handleTickEvent).
    void tick()
    {
        animator.update(state.sample());
        if (widget != 0)
        {
            widget->applyFeedback(animator.isVisible(),
                                  animator.getX(),
                                  animator.getY(),
                                  animator.getAlpha());
        }
    }

private:
    TouchFeedbackController() : widget(0) {}

    TouchFeedbackState state;
    TouchFeedbackAnimator animator;
    ITouchFeedbackWidget* widget;
};
} // namespace touchfeedback

#endif // TOUCHFEEDBACKCONTROLLER_HPP
