#include <gui/screen7_screen/Screen7View.hpp>
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

Screen7View::Screen7View()
{

}

void Screen7View::setupScreen()
{
    Screen7ViewBase::setupScreen();

    if (presenter != 0)
    {
        applyBandyState(presenter->getBandyState());
    }
}

void Screen7View::tearDownScreen()
{
    Screen7ViewBase::tearDownScreen();
}

void Screen7View::cancelClicked()
{
    application().gotoPauseScreenNoTransition();
}

void Screen7View::confirmClicked()
{
    if (presenter != 0)
    {
        presenter->endDemo();
    }
}

void Screen7View::applyBandyState(const BandyState& state)
{
    updateTimeValue(state.remainingSeconds);

    if (state.sessionState == BandySessionRunning)
    {
        application().gotoBandyScreenNoTransition();
    }
    else if (state.sessionState == BandySessionWaitRfid)
    {
        application().gotoRfidWaitScreenNoTransition();
    }
}

void Screen7View::updateTimeValue(uint16_t seconds)
{
    formatSeconds(endVisitValueBuffer, ENDVISITVALUE_SIZE, seconds);
    endVisitValue.invalidate();
}
