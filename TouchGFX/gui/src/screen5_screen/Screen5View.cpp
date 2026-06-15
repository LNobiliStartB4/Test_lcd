#include <gui/screen5_screen/Screen5View.hpp>
#include <gui/model/BandyCompletion.hpp>
#include <touchgfx/Unicode.hpp>

Screen5View::Screen5View()
    : latestState(),
      screenTransitionRequested(false)
{
}

void Screen5View::setupScreen()
{
    Screen5ViewBase::setupScreen();
    screenTransitionRequested = false;

    if (presenter != 0)
    {
        applyBandyState(presenter->getBandyState());
    }
}

void Screen5View::tearDownScreen()
{
    screenTransitionRequested = false;
    Screen5ViewBase::tearDownScreen();
}

void Screen5View::backClicked()
{
    application().gotoBandyScreenNoTransition();
}

void Screen5View::decreaseTargetClicked()
{
    if (presenter != 0)
    {
        presenter->decreaseTarget();
    }
}

void Screen5View::increaseTargetClicked()
{
    if (presenter != 0)
    {
        presenter->increaseTarget();
    }
}

void Screen5View::openKeypadClicked()
{
    application().gotoSetpointKeypadScreenNoTransition();
}

void Screen5View::applyBandyState(const BandyState& state)
{
    const BandyState previousState = latestState;
    latestState = state;

    if (!screenTransitionRequested && isNaturalBandyCompletion(previousState, state))
    {
        screenTransitionRequested = true;
        application().gotoBandyCompletedScreenNoTransition();
        return;
    }

    touchgfx::Unicode::snprintf(setpointValueBuffer,
                               SETPOINTVALUE_SIZE,
                               "%d",
                               static_cast<int>(state.targetVacuumMbar));
    setpointValue.invalidate();
}
