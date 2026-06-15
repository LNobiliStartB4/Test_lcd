#include <gui/screen19_screen/Screen19View.hpp>

Screen19View::Screen19View()
{

}

void Screen19View::setupScreen()
{
    Screen19ViewBase::setupScreen();
}

void Screen19View::tearDownScreen()
{
    Screen19ViewBase::tearDownScreen();
}

void Screen19View::continueClicked()
{
    application().gotoProductSelectScreenNoTransition();
}
