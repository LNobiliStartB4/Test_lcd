#include <gui/screen4_screen/Screen4View.hpp>
#include <touchgfx/Application.hpp>

Screen4View::Screen4View()
    : transitionRequested(false),
      waitingPulseAlpha(255),
      waitingPulseStep(-8),
      waitingPulseTickDivider(0)
{

}

void Screen4View::setupScreen()
{
    Screen4ViewBase::setupScreen();
    transitionRequested = false;
    waitingPulseAlpha = 255;
    waitingPulseStep = -8;
    waitingPulseTickDivider = 0;
    rfidStatus.setAlpha(waitingPulseAlpha);
    touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));
}

void Screen4View::tearDownScreen()
{
    transitionRequested = false;
    rfidStatus.setAlpha(255);
    Screen4ViewBase::tearDownScreen();
}

void Screen4View::backClicked()
{
    transitionRequested = true;
    application().gotoProductSelectScreenNoTransition();
}

void Screen4View::handleTickEvent()
{
    if (++waitingPulseTickDivider >= 2)
    {
        waitingPulseTickDivider = 0;

        int16_t nextAlpha = static_cast<int16_t>(waitingPulseAlpha) + waitingPulseStep;
        if (nextAlpha <= 96)
        {
            nextAlpha = 96;
            waitingPulseStep = 8;
        }
        else if (nextAlpha >= 255)
        {
            nextAlpha = 255;
            waitingPulseStep = -8;
        }

        waitingPulseAlpha = static_cast<uint8_t>(nextAlpha);
        rfidStatus.setAlpha(waitingPulseAlpha);
        rfidStatus.invalidate();
    }

    if (!transitionRequested && (presenter != 0) && presenter->canOpenBandyScreen())
    {
        transitionRequested = true;
        application().gotoBandyScreenNoTransition();
    }
}
