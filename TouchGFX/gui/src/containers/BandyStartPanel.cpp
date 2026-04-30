#include <gui/containers/BandyStartPanel.hpp>

BandyStartPanel::BandyStartPanel()
    : startCallback(0)
{

}

void BandyStartPanel::initialize()
{
    BandyStartPanelBase::initialize();
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
