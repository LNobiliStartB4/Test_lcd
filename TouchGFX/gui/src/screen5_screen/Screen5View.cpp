#include <gui/screen5_screen/Screen5View.hpp>
#include <touchgfx/Unicode.hpp>

Screen5View::Screen5View()
{

}

void Screen5View::setupScreen()
{
    Screen5ViewBase::setupScreen();

    if (presenter != 0)
    {
        applyBandyState(presenter->getBandyState());
    }
}

void Screen5View::tearDownScreen()
{
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

void Screen5View::applyBandyState(const BandyState& state)
{
    touchgfx::Unicode::snprintf(setpointValueBuffer, SETPOINTVALUE_SIZE, "%d", static_cast<int>(state.targetVacuumMbar));
    setpointValue.invalidate();
}
