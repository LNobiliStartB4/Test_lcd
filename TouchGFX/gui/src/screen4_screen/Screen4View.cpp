#include <gui/screen4_screen/Screen4View.hpp>
#include <touchgfx/Application.hpp>

Screen4View::Screen4View()
    : transitionRequested(false)
{

}

void Screen4View::setupScreen()
{
    Screen4ViewBase::setupScreen();
    transitionRequested = false;
    touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));
}

void Screen4View::tearDownScreen()
{
    transitionRequested = false;
    Screen4ViewBase::tearDownScreen();
}

void Screen4View::backClicked()
{
    transitionRequested = true;
    application().gotoProductSelectScreenNoTransition();
}

void Screen4View::handleTickEvent()
{
    if (!transitionRequested && (presenter != 0) && presenter->canOpenBandyScreen())
    {
        transitionRequested = true;
        application().gotoBandyScreenNoTransition();
    }
}
