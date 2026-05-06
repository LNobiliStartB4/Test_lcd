#include <gui/containers/BandyStartPanel.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Color.hpp>
#include <texts/TextKeysAndLanguages.hpp>

namespace
{
const touchgfx::colortype kPanelBlack = touchgfx::Color::getColorFromRGB(0, 0, 0);
const touchgfx::colortype kStartFrameColor = touchgfx::Color::getColorFromRGB(245, 242, 232);
const touchgfx::colortype kStartFillColor = touchgfx::Color::getColorFromRGB(0, 0, 0);
const touchgfx::colortype kStartFillPressedColor = touchgfx::Color::getColorFromRGB(22, 22, 20);
const touchgfx::colortype kStartBorderColor = touchgfx::Color::getColorFromRGB(245, 242, 232);
const touchgfx::colortype kStartBorderPressedColor = touchgfx::Color::getColorFromRGB(255, 255, 248);

const touchgfx::colortype kStopFrameColor = touchgfx::Color::getColorFromRGB(245, 242, 232);
const touchgfx::colortype kStopFillColor = touchgfx::Color::getColorFromRGB(0, 0, 0);
const touchgfx::colortype kStopFillPressedColor = touchgfx::Color::getColorFromRGB(35, 15, 18);
const touchgfx::colortype kStopBorderColor = touchgfx::Color::getColorFromRGB(245, 242, 232);
const touchgfx::colortype kStopBorderPressedColor = touchgfx::Color::getColorFromRGB(255, 255, 248);
}

BandyStartPanel::BandyStartPanel()
    : startCallback(0),
      runningState(false)
{

}

void BandyStartPanel::initialize()
{
    BandyStartPanelBase::initialize();
    applyVisualState();
}

void BandyStartPanel::startClicked()
{
    if ((startCallback != 0) && startCallback->isValid())
    {
        startCallback->execute();
    }
}

void BandyStartPanel::setStartCallback(touchgfx::GenericCallback<>& callback)
{
    startCallback = &callback;
}

void BandyStartPanel::setRunning(bool running)
{
    if (running == runningState)
    {
        return;
    }

    runningState = running;
    startLabel.setTypedText(touchgfx::TypedText(runningState ? T_TEXT_STOP : T_TEXT_START));
    applyVisualState();
}

void BandyStartPanel::applyVisualState()
{
    if (runningState)
    {
        startFrame.setColor(kPanelBlack);
        startFrame.setBorderColor(kStopFrameColor);
        startButton.setBoxWithBorderColors(kStopFillColor,
                                           kStopFillPressedColor,
                                           kStopBorderColor,
                                           kStopBorderPressedColor);
        startIcon.setBitmap(touchgfx::Bitmap(BITMAP_START_STOP_ICON_WHITE_ID));
    }
    else
    {
        startFrame.setColor(kPanelBlack);
        startFrame.setBorderColor(kStartFrameColor);
        startButton.setBoxWithBorderColors(kStartFillColor,
                                           kStartFillPressedColor,
                                           kStartBorderColor,
                                           kStartBorderPressedColor);
        startIcon.setBitmap(touchgfx::Bitmap(BITMAP_START_PLAY_ICON_WHITE_ID));
    }

    startFrame.invalidate();
    startButton.invalidate();
    startIcon.invalidate();
    startLabel.invalidate();
}
