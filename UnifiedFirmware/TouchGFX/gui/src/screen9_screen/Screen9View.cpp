#include <gui/screen9_screen/Screen9View.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/Application.hpp>

Screen9View::Screen9View()
{
}

void Screen9View::setupScreen()
{
    Screen9ViewBase::setupScreen();
    applyHemorflowState();
    touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));
}

void Screen9View::tearDownScreen()
{
    Screen9ViewBase::tearDownScreen();
}

void Screen9View::handleTickEvent()
{
    if (presenter->shouldReturnToHemorflowWait())
    {
        application().gotoHemorflowWaitScreenNoTransition();
        return;
    }

    applyHemorflowState();
}

void Screen9View::applyHemorflowState()
{
    const HemorflowState state = presenter->getHemorflowState();

    hemorflowVacuumPanel.setVacuumMbar(state.currentPressureMbar);
    touchgfx::Unicode::snprintf(hemorflowTargetValueBuffer,
                                HEMORFLOWTARGETVALUE_SIZE,
                                "%d",
                                static_cast<int>(state.targetMbar));
    touchgfx::Unicode::strncpy(hemorflowStatusValueBuffer,
                               state.running ? "RUNNING" : "STOPPED",
                               HEMORFLOWSTATUSVALUE_SIZE);

    hemorflowTargetValue.invalidate();
    hemorflowStatusValue.invalidate();
}
