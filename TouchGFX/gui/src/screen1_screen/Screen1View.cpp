#include <gui/screen1_screen/Screen1View.hpp>

namespace
{
const uint8_t kSplashDurationTicks = 90; // About 1.5 s at 60 Hz
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

    if (++splashTicks >= kSplashDurationTicks)
    {
        transitionRequested = true;
        application().gotoProductSelectScreenNoTransition();
    }
}
