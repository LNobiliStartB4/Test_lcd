#include <gui/screen1_screen/Screen1View.hpp>
#include <images/BitmapDatabase.hpp>

namespace
{
constexpr uint8_t kSplashDurationTicks = 120;     // ~2.0 s at 60 Hz
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
    const touchgfx::Bitmap dim(BITMAP_DOT_DIM_ID);
    const touchgfx::Bitmap active(BITMAP_DOT_ACTIVE_ID);

    splashDot1.setBitmap(activeIndex == 0U ? active : dim);
    splashDot2.setBitmap(activeIndex == 1U ? active : dim);
    splashDot3.setBitmap(activeIndex == 2U ? active : dim);
    splashDot1.invalidate();
    splashDot2.invalidate();
    splashDot3.invalidate();
}
