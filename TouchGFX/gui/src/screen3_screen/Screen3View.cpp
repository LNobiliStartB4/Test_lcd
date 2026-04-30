#include <gui/screen3_screen/Screen3View.hpp>
#include <touchgfx/Application.hpp>

namespace
{
const uint8_t kInitialFullRedrawTicks = 4;
}

Screen3View::Screen3View()
    : latestState(),
      fullRedrawTicks(0),
      decreaseTargetCallback(this, &Screen3View::decreaseTargetClicked),
      increaseTargetCallback(this, &Screen3View::increaseTargetClicked),
      startCallback(this, &Screen3View::startClicked),
      returnToWaitRequested(false)
{
}

void Screen3View::setupScreen()
{
    Screen3ViewBase::setupScreen();
    targetPanel.setDecreaseCallback(decreaseTargetCallback);
    targetPanel.setIncreaseCallback(increaseTargetCallback);
    startPanel.setStartCallback(startCallback);

    fullRedrawTicks = kInitialFullRedrawTicks;
    returnToWaitRequested = false;
    touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));

    if (presenter != 0)
    {
        applyBandyState(presenter->getBandyState());
    }
}

void Screen3View::tearDownScreen()
{
    returnToWaitRequested = false;
    Screen3ViewBase::tearDownScreen();
}

void Screen3View::handleTickEvent()
{
    if (fullRedrawTicks > 0U)
    {
        touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));
        fullRedrawTicks--;
    }

    if (!returnToWaitRequested && (presenter != 0) && !presenter->isVacuumCycleRunning())
    {
        returnToWaitRequested = true;
        application().gotoRfidWaitScreenNoTransition();
    }
}

void Screen3View::decreaseTargetClicked()
{
    if (presenter != 0)
    {
        presenter->decreaseTarget();
    }
}

void Screen3View::increaseTargetClicked()
{
    if (presenter != 0)
    {
        presenter->increaseTarget();
    }
}

void Screen3View::startClicked()
{
    if (presenter != 0)
    {
        presenter->startDemo();
    }
}

void Screen3View::applyBandyState(const BandyState& state)
{
    latestState = state;

    vacuumPanel.setVacuumMbar(state.currentVacuumMbar);
    targetPanel.setTargetMbar(state.targetVacuumMbar);
}
