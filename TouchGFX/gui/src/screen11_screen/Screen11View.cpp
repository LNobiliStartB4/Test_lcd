#include <gui/screen11_screen/Screen11View.hpp>
#include <touchgfx/Unicode.hpp>

Screen11View::Screen11View()
{
}

void Screen11View::setupScreen()
{
    Screen11ViewBase::setupScreen();

    // TouchGFX Designer 4.26 currently emits this custom slider as vertical.
    brightnessSlider.setupHorizontalSlider(16, 12, 0, 0, 336);

    const uint8_t percent = (presenter != 0) ? presenter->getBrightnessPercent() : 100U;
    brightnessSlider.setValue(percent);
    updateBrightnessValue(percent);
}

void Screen11View::tearDownScreen()
{
    Screen11ViewBase::tearDownScreen();
}

void Screen11View::backClicked()
{
    application().gotoSettingsScreenNoTransition();
}

void Screen11View::brightnessChanged(int value)
{
    const uint8_t percent = static_cast<uint8_t>(value);

    if (presenter != 0)
    {
        presenter->setBrightnessPercent(percent);
    }

    updateBrightnessValue(percent);
}

void Screen11View::updateBrightnessValue(uint8_t percent)
{
    touchgfx::Unicode::snprintf(brightnessValueBuffer, BRIGHTNESSVALUE_SIZE, "%u", percent);
    brightnessValue.invalidate();
}
