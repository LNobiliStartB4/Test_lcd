#include <gui/screen11_screen/Screen11View.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/Unicode.hpp>

namespace
{
constexpr int16_t kTrackX = 72;
constexpr int16_t kTrackY = 198;
constexpr int16_t kTrackWidth = 336;
constexpr int16_t kTrackHeight = 8;

constexpr int16_t kKnobWidth = 30;
constexpr int16_t kKnobHeight = 30;

constexpr int16_t kTouchX = 40;
constexpr int16_t kTouchY = 164;
constexpr int16_t kTouchWidth = 400;
constexpr int16_t kTouchHeight = 78;

constexpr uint8_t kMinBrightness = 10;
constexpr uint8_t kMaxBrightness = 100;
}

Screen11View::Screen11View()
{
}

void Screen11View::setupScreen()
{
    Screen11ViewBase::setupScreen();

    // Keep the generated slider as a Designer placeholder, but avoid its bitmap
    // redraw path on hardware. A simple box-based control is more reliable here.
    brightnessSlider.setVisible(false);
    brightnessSlider.setTouchable(false);

    brightnessTrackBackground.setPosition(kTrackX, kTrackY, kTrackWidth, kTrackHeight);
    brightnessTrackBackground.setColor(touchgfx::Color::getColorFromRGB(58, 61, 56));
    add(brightnessTrackBackground);

    brightnessTrackFill.setPosition(kTrackX, kTrackY, kTrackWidth, kTrackHeight);
    brightnessTrackFill.setColor(touchgfx::Color::getColorFromRGB(240, 170, 0));
    add(brightnessTrackFill);

    brightnessKnob.setPosition(kTrackX + kTrackWidth - (kKnobWidth / 2), kTrackY - 11, kKnobWidth, kKnobHeight);
    brightnessKnob.setColor(touchgfx::Color::getColorFromRGB(240, 170, 0));
    add(brightnessKnob);

    const uint8_t percent = (presenter != 0) ? presenter->getBrightnessPercent() : 100U;
    brightnessSlider.setValue(percent);
    updateBrightnessValue(percent);
    updateBrightnessBar(percent);
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
    updateBrightnessBar(percent);
}

void Screen11View::updateBrightnessValue(uint8_t percent)
{
    touchgfx::Unicode::snprintf(brightnessValueBuffer, BRIGHTNESSVALUE_SIZE, "%u", percent);
    brightnessValue.invalidate();
}

void Screen11View::handleClickEvent(const touchgfx::ClickEvent& event)
{
    Screen11ViewBase::handleClickEvent(event);

    if ((event.getType() == touchgfx::ClickEvent::PRESSED) &&
        isBrightnessTouchArea(event.getX(), event.getY()))
    {
        updateBrightnessFromX(event.getX());
    }
}

void Screen11View::handleDragEvent(const touchgfx::DragEvent& event)
{
    Screen11ViewBase::handleDragEvent(event);

    if (isBrightnessTouchArea(event.getNewX(), event.getNewY()))
    {
        updateBrightnessFromX(event.getNewX());
    }
}

void Screen11View::updateBrightnessFromX(int16_t x)
{
    if (x < kTrackX)
    {
        x = kTrackX;
    }
    else if (x > (kTrackX + kTrackWidth))
    {
        x = kTrackX + kTrackWidth;
    }

    const int32_t numerator = static_cast<int32_t>(x - kTrackX) * (kMaxBrightness - kMinBrightness);
    const uint8_t percent = static_cast<uint8_t>(kMinBrightness + ((numerator + (kTrackWidth / 2)) / kTrackWidth));

    brightnessSlider.setValue(percent);
    brightnessChanged(percent);
}

void Screen11View::updateBrightnessBar(uint8_t percent)
{
    if (percent < kMinBrightness)
    {
        percent = kMinBrightness;
    }
    else if (percent > kMaxBrightness)
    {
        percent = kMaxBrightness;
    }

    const int32_t numerator = static_cast<int32_t>(percent - kMinBrightness) * kTrackWidth;
    const int16_t fillWidth = static_cast<int16_t>((numerator + ((kMaxBrightness - kMinBrightness) / 2)) /
                                                   (kMaxBrightness - kMinBrightness));
    const int16_t knobCenterX = static_cast<int16_t>(kTrackX + fillWidth);

    brightnessTrackFill.setWidth(fillWidth > 0 ? fillWidth : 1);
    brightnessTrackFill.invalidate();

    brightnessKnob.moveTo(static_cast<int16_t>(knobCenterX - (kKnobWidth / 2)),
                          static_cast<int16_t>(kTrackY - 11));
}

bool Screen11View::isBrightnessTouchArea(int16_t x, int16_t y) const
{
    return (x >= kTouchX) &&
           (x < (kTouchX + kTouchWidth)) &&
           (y >= kTouchY) &&
           (y < (kTouchY + kTouchHeight));
}
