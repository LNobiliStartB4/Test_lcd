#include <gui/screen8_screen/Screen8View.hpp>
#include <touchgfx/Application.hpp>

Screen8View::Screen8View()
    : transitionRequested(false)
{
}

void Screen8View::setupScreen()
{
    Screen8ViewBase::setupScreen();
    transitionRequested = false;

    presenter->initializeHemorflowMonitor();
    touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));
}

void Screen8View::tearDownScreen()
{
    transitionRequested = false;
    Screen8ViewBase::tearDownScreen();
}

void Screen8View::handleTickEvent()
{
    if (!transitionRequested && presenter->canOpenHemorflowMonitor())
    {
        transitionRequested = true;
        application().gotoHemorflowScreenNoTransition();
    }
}

void Screen8View::backClicked()
{
    transitionRequested = true;
    application().gotoProductSelectScreenNoTransition();
}
