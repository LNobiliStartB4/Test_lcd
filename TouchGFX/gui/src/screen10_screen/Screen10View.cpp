#include <gui/screen10_screen/Screen10View.hpp>

Screen10View::Screen10View()
{
}

void Screen10View::setupScreen()
{
    Screen10ViewBase::setupScreen();
}

void Screen10View::tearDownScreen()
{
    Screen10ViewBase::tearDownScreen();
}

void Screen10View::backClicked()
{
    application().gotoProductSelectScreenNoTransition();
}

void Screen10View::languageClicked()
{
    application().gotoLanguageScreenNoTransition();
}

void Screen10View::brightnessClicked()
{
    application().gotoBrightnessScreenNoTransition();
}
