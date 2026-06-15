#include <gui/screen7_screen/Screen7View.hpp>
#include <gui/model/BandyCompletion.hpp>
#include <touchgfx/Application.hpp>
#include <touchgfx/Unicode.hpp>
#include <texts/TextKeysAndLanguages.hpp>

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
    : latestState(),
      transitionRequested(false),
      manualEndRequested(false)
{
}

void Screen7View::setupScreen()
{
    Screen7ViewBase::setupScreen();
    transitionRequested = false;
    manualEndRequested = false;
    subtitleText.setLinespacing(2);

    if (presenter != 0)
    {
        applyBandyState(presenter->getBandyState());
    }

    touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));
}

void Screen7View::tearDownScreen()
{
    transitionRequested = false;
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
        manualEndRequested = true;
        presenter->endDemo();
    }
}

void Screen7View::applyBandyState(const BandyState& state)
{
    const BandyState previousState = latestState;
    latestState = state;
    updateTimeValue(state.remainingSeconds);

    if (!transitionRequested && (state.sessionState == BandySessionWaitRfid))
    {
        transitionRequested = true;
        if (!manualEndRequested && isNaturalBandyCompletion(previousState, state))
        {
            application().gotoBandyCompletedScreenNoTransition();
        }
        else
        {
            application().gotoProductSelectScreenNoTransition();
        }
        return;
    }

    if (!transitionRequested && (state.sessionState == BandySessionRunning))
    {
        transitionRequested = true;
        application().gotoBandyScreenNoTransition();
    }
}

void Screen7View::updateTimeValue(uint16_t seconds)
{
    formatSeconds(timeValueBuffer, TIMEVALUE_SIZE, seconds);
    timeValue.invalidate();
}
