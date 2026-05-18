#include <gui/screen2_screen/Screen2View.hpp>
#include <touchgfx/Application.hpp>

Screen2View::Screen2View()
{
}

void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();
    touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));
}

void Screen2View::tearDownScreen()
{
    Screen2ViewBase::tearDownScreen();
}

void Screen2View::bandySelected()
{
    application().gotoRfidWaitScreenNoTransition();
}

void Screen2View::hemorflowSelected()
{
    application().gotoHemorflowWaitScreenNoTransition();
}
