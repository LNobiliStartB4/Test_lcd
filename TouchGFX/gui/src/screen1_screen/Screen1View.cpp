#include <gui/screen1_screen/Screen1View.hpp>
#include <images/SVGDatabase.hpp>

namespace
{
constexpr uint8_t kSplashDurationTicks = 165;     // ~2.75 s at 60 Hz
constexpr uint8_t kDotCycleTicks       = 20;      // ~333 ms per dot
}

Screen1View::Screen1View()
    : splashTicks(0),
      transitionRequested(false)
{
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();
    splashTicks = 0;
    transitionRequested = false;
    updateDots(0);
}

void Screen1View::tearDownScreen()
{
    splashTicks = 0;
    transitionRequested = false;
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::handleTickEvent()
{
    if (transitionRequested)
    {
        return;
    }

    updateDots(static_cast<uint8_t>((splashTicks / kDotCycleTicks) % 3U));

    if (++splashTicks >= kSplashDurationTicks)
    {
        transitionRequested = true;
        application().gotoProductSelectScreenNoTransition();
    }
}

void Screen1View::updateDots(uint8_t activeIndex)
{
    splashDot1.setSVG(activeIndex == 0U ? SVG_DOT_ACTIVE_ID : SVG_DOT_DIM_ID);
    splashDot2.setSVG(activeIndex == 1U ? SVG_DOT_ACTIVE_ID : SVG_DOT_DIM_ID);
    splashDot3.setSVG(activeIndex == 2U ? SVG_DOT_ACTIVE_ID : SVG_DOT_DIM_ID);
    splashDot1.invalidate();
    splashDot2.invalidate();
    splashDot3.invalidate();
}
