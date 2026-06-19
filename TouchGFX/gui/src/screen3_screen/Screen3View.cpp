#include <gui/screen3_screen/Screen3View.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Application.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/Unicode.hpp>
#include <texts/TextKeysAndLanguages.hpp>

Screen3View::Screen3View()
    : latestState(),
      screenTransitionRequested(false)
{
}

void Screen3View::setupScreen()
{
    Screen3ViewBase::setupScreen();
    screenTransitionRequested = false;

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
        application().gotoRfidWaitScreenNoTransition();
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

    screen3StartButton.setBoxWithBorderColors(fillColor, pressedFillColor, borderColor, pressedBorderColor);
    screen3StartLabel.setTypedText(touchgfx::TypedText(running ? T_TEXT_PAUSE : T_TEXT_START));
    screen3StartIcon.setBitmap(touchgfx::Bitmap(running ? BITMAP_START_STOP_ICON_WHITE_ID : BITMAP_START_PLAY_ICON_WHITE_ID));

    screen3StartButton.invalidate();
    screen3StartLabel.invalidate();
    screen3StartIcon.invalidate();
}
