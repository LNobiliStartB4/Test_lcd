#include <gui/screen1_screen/Screen1View.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/TypedText.hpp>
#include <texts/TextKeysAndLanguages.hpp>

namespace
{
touchgfx::colortype rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    return touchgfx::Color::getColorFromRGB(red, green, blue);
}
}

Screen1View::Screen1View()
    : touchActive(false)
{
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();
    touchActive = false;
    applyTouchState();
}

void Screen1View::tearDownScreen()
{
    touchActive = false;
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::touchTestClicked()
{
    touchActive = !touchActive;
    applyTouchState();
}

void Screen1View::applyTouchState()
{
    if (touchActive)
    {
        touchBackground.setColor(rgb(10, 42, 32));
        touchStatusPanel.setColor(rgb(16, 63, 51));
        touchStatusPanel.setBorderColor(rgb(86, 177, 150));
        touchStatusValue.setTypedText(touchgfx::TypedText(T_TEXT_TOUCHSTATUSON));
        touchStatusValue.setColor(rgb(205, 241, 217));
        touchTestButton.setBoxWithBorderColors(rgb(22, 91, 72), rgb(34, 123, 97), rgb(102, 198, 156), rgb(154, 231, 193));
    }
    else
    {
        touchBackground.setColor(rgb(7, 18, 35));
        touchStatusPanel.setColor(rgb(12, 31, 56));
        touchStatusPanel.setBorderColor(rgb(60, 101, 148));
        touchStatusValue.setTypedText(touchgfx::TypedText(T_TEXT_TOUCHSTATUSOFF));
        touchStatusValue.setColor(rgb(240, 185, 96));
        touchTestButton.setBoxWithBorderColors(rgb(19, 61, 92), rgb(34, 96, 136), rgb(92, 157, 205), rgb(152, 205, 243));
    }

    touchBackground.invalidate();
    touchStatusPanel.invalidate();
    touchStatusValue.invalidate();
    touchTestButton.invalidate();
}
