#include <gui/screen1_screen/Screen1View.hpp>

namespace
{
static const uint16_t kSplashDurationTicks = 90U;
}

Screen1View::Screen1View()
    : splashTickCount(0),
      transitionRequested(false)
{
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();

    splashLogo.setVisible(true);
    splashLogo.setAlpha(255);

    splashTickCount = 0;
    transitionRequested = false;
}

void Screen1View::handleTickEvent()
{
    if (transitionRequested)
    {
        return;
    }

    if (++splashTickCount >= kSplashDurationTicks)
    {
        transitionRequested = true;
        application().gotoScreen2ScreenNoTransition();
    }
}

void Screen1View::tearDownScreen()
{
    splashTickCount = 0;
    transitionRequested = false;

    Screen1ViewBase::tearDownScreen();
}
