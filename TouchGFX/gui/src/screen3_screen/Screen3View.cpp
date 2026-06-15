#include <gui/screen3_screen/Screen3View.hpp>
#include <gui/model/BandyCompletion.hpp>
#include <images/BitmapDatabase.hpp>
#include <images/SVGDatabase.hpp>
#include <touchgfx/Application.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/Unicode.hpp>
#include <texts/TextKeysAndLanguages.hpp>

Screen3View::Screen3View()
    : latestState(),
      screenTransitionRequested(false),
      startControlInitialized(false),
      startControlRunning(false)
{
}

void Screen3View::setupScreen()
{
    Screen3ViewBase::setupScreen();
    screenTransitionRequested = false;
    startControlInitialized = false;

    touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));

    if (presenter != 0)
    {
        applyBandyState(presenter->getBandyState());
    }
}

void Screen3View::tearDownScreen()
{
    screenTransitionRequested = false;
    Screen3ViewBase::tearDownScreen();
}

void Screen3View::openSetpointClicked()
{
    application().gotoSetpointEditScreenNoTransition();
}

void Screen3View::startClicked()
{
    if (presenter != 0)
    {
        if (latestState.running)
        {
            presenter->stopDemo();
        }
        else
        {
            presenter->startDemo();
        }
    }
}

void Screen3View::applyBandyState(const BandyState& state)
{
    const BandyState previousState = latestState;
    latestState = state;

    vacuumPanel.setVacuumMbar(state.currentVacuumMbar);
    updateTargetDisplay(state.targetVacuumMbar);
    timePanel.setRemainingSeconds(state.remainingSeconds);
    updateStartControl(state.sessionState);

    if (!screenTransitionRequested && (state.sessionState == BandySessionPaused))
    {
        screenTransitionRequested = true;
        application().gotoPauseScreenNoTransition();
    }
    else if (!screenTransitionRequested && (state.sessionState == BandySessionWaitRfid))
    {
        screenTransitionRequested = true;
        if (isNaturalBandyCompletion(previousState, state))
        {
            application().gotoBandyCompletedScreenNoTransition();
        }
        else
        {
            application().gotoRfidWaitScreenNoTransition();
        }
    }
}

void Screen3View::updateTargetDisplay(int32_t targetMbar)
{
    touchgfx::Unicode::snprintf(screen3TargetValueBuffer, SCREEN3TARGETVALUE_SIZE, "%d", static_cast<int>(targetMbar));
    screen3TargetValue.invalidate();
}

void Screen3View::updateStartControl(BandySessionState sessionState)
{
    const bool running = sessionState == BandySessionRunning;

    if (startControlInitialized && (running == startControlRunning))
    {
        return;
    }

    startControlInitialized = true;
    startControlRunning = running;

    const touchgfx::colortype fillColor = touchgfx::Color::getColorFromRGB(0, 0, 0);
    const touchgfx::colortype pressedFillColor = running
                                               ? touchgfx::Color::getColorFromRGB(35, 15, 18)
                                               : touchgfx::Color::getColorFromRGB(22, 22, 20);
    const touchgfx::colortype borderColor = running
                                             ? touchgfx::Color::getColorFromRGB(218, 56, 68)
                                             : touchgfx::Color::getColorFromRGB(245, 242, 232);
    const touchgfx::colortype pressedBorderColor = running
                                                    ? touchgfx::Color::getColorFromRGB(255, 92, 104)
                                                    : touchgfx::Color::getColorFromRGB(255, 255, 248);

    screen3StartButton.setPosition(176, 240, 304, 78);
    screen3StartButton.setBoxWithBorderColors(fillColor, pressedFillColor, borderColor, pressedBorderColor);
    screen3StartIcon.setSVG(running ? SVG_START_PAUSE_GOLD_ID : SVG_START_PLAY_GOLD_ID);
    screen3StartLabel.setTypedText(touchgfx::TypedText(running ? T_TEXT_PAUSE : T_TEXT_START));

    screen3StartButton.invalidate();
    screen3StartIcon.invalidate();
    screen3StartLabel.invalidate();

}
