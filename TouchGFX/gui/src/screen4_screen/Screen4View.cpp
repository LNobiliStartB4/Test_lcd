#include <gui/screen4_screen/Screen4View.hpp>
#include <touchgfx/Application.hpp>

namespace
{
const uint8_t kInitialFullRedrawTicks = 4U;
}

Screen4View::Screen4View()
    : transitionRequested(false),
      fullRedrawTicks(0U)
{

}

void Screen4View::setupScreen()
{
    Screen4ViewBase::setupScreen();
    transitionRequested = false;
    fullRedrawTicks = kInitialFullRedrawTicks;
    touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));
}

void Screen4View::tearDownScreen()
{
    transitionRequested = false;
    Screen4ViewBase::tearDownScreen();
}

void Screen4View::handleTickEvent()
{
    if (fullRedrawTicks > 0U)
    {
        touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));
        fullRedrawTicks--;
    }

    if (!transitionRequested && (presenter != 0) && presenter->isRfidApproved())
    {
        transitionRequested = true;
        application().gotoBandyScreenNoTransition();
    }
}
