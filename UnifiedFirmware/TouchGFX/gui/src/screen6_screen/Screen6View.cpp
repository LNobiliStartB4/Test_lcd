#include <gui/screen6_screen/Screen6View.hpp>
#include <touchgfx/Unicode.hpp>

namespace
{
void formatSeconds(touchgfx::Unicode::UnicodeChar* buffer, uint16_t bufferSize, uint16_t seconds)
{
    const uint16_t minutes = static_cast<uint16_t>(seconds / 60U);
    const uint16_t remainder = static_cast<uint16_t>(seconds % 60U);
    touchgfx::Unicode::snprintf(buffer, bufferSize, "%02d:%02d", static_cast<int>(minutes), static_cast<int>(remainder));
}
}

Screen6View::Screen6View()
{

}

void Screen6View::setupScreen()
{
    Screen6ViewBase::setupScreen();

    if (presenter != 0)
    {
        applyBandyState(presenter->getBandyState());
    }
}

void Screen6View::tearDownScreen()
{
    Screen6ViewBase::tearDownScreen();
}

void Screen6View::resumeClicked()
{
    if (presenter != 0)
    {
        presenter->resumeDemo();
    }
}

void Screen6View::endClicked()
{
    application().gotoEndConfirmScreenNoTransition();
}

void Screen6View::applyBandyState(const BandyState& state)
{
    updateTimeValues(state);

    if (state.sessionState == BandySessionRunning)
    {
        application().gotoBandyScreenNoTransition();
    }
    else if (state.sessionState == BandySessionWaitRfid)
    {
        application().gotoRfidWaitScreenNoTransition();
    }
}

void Screen6View::updateTimeValues(const BandyState& state)
{
    formatSeconds(pauseRemainingValueBuffer, PAUSEREMAININGVALUE_SIZE, state.pauseRemainingSeconds);
    pauseRemainingValue.invalidate();

    formatSeconds(pauseVisitValueBuffer, PAUSEVISITVALUE_SIZE, state.remainingSeconds);
    pauseVisitValue.invalidate();
}
