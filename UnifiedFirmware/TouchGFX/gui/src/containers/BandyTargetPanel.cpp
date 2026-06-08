#include <gui/containers/BandyTargetPanel.hpp>
#include <touchgfx/Unicode.hpp>

namespace
{
const int16_t kSetTargetTouchStartX = 150;
const int16_t kSetTargetTouchEndX = 320;
}

BandyTargetPanel::BandyTargetPanel()
    : openCallback(0)
{

}

void BandyTargetPanel::initialize()
{
    BandyTargetPanelBase::initialize();
    setTouchable(true);
    targetOpenButton.setTouchable(false);
    setTargetMbar(0);
}

void BandyTargetPanel::handleClickEvent(const touchgfx::ClickEvent& event)
{
    if (event.getType() == touchgfx::ClickEvent::RELEASED &&
        event.getX() >= kSetTargetTouchStartX &&
        event.getX() < kSetTargetTouchEndX)
    {
        openSetpointClicked();
    }
}

void BandyTargetPanel::openSetpointClicked()
{
    if ((openCallback != 0) && openCallback->isValid())
    {
        openCallback->execute();
    }
}

void BandyTargetPanel::setTargetMbar(int32_t targetMbar)
{
    touchgfx::Unicode::snprintf(targetValueBuffer, TARGETVALUE_SIZE, "%d", static_cast<int>(targetMbar));
    targetValue.invalidate();
}

void BandyTargetPanel::setOpenCallback(touchgfx::GenericCallback<>& callback)
{
    openCallback = &callback;
}
