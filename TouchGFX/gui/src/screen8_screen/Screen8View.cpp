#include <gui/screen8_screen/Screen8View.hpp>
#include <touchgfx/Application.hpp>

Screen8View::Screen8View()
{
}

void Screen8View::setupScreen()
{
    Screen8ViewBase::setupScreen();

    if (presenter != 0)
    {
        applyBandyState(presenter->getBandyState());
    }

    touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));
}

void Screen8View::tearDownScreen()
{
    Screen8ViewBase::tearDownScreen();
}

void Screen8View::cancelClicked()
{
    if (presenter != 0)
    {
        presenter->cancelEndDemo();
    }
}

void Screen8View::applyBandyState(const BandyState& state)
{
    if (state.sessionState == BandySessionEnding)
    {
        return;
    }

    if (state.sessionState == BandySessionWaitRfid)
    {
        application().gotoProductSelectScreenNoTransition();
        return;
    }

    if (state.sessionState == BandySessionPaused)
    {
        application().gotoPauseScreenNoTransition();
        return;
    }

    if (state.sessionState == BandySessionRunning)
    {
        application().gotoBandyScreenNoTransition();
    }
}
