#include <gui/containers/BandyTargetPanel.hpp>
#include <touchgfx/Unicode.hpp>

BandyTargetPanel::BandyTargetPanel()
    : decreaseCallback(0),
      increaseCallback(0)
{

}

void BandyTargetPanel::initialize()
{
    BandyTargetPanelBase::initialize();
    setTargetMbar(0);
}

void BandyTargetPanel::decreaseTargetClicked()
{
    if ((decreaseCallback != 0) && decreaseCallback->isValid())
    {
        decreaseCallback->execute();
    }
}

void BandyTargetPanel::increaseTargetClicked()
{
    if ((increaseCallback != 0) && increaseCallback->isValid())
    {
        increaseCallback->execute();
    }
}

void BandyTargetPanel::setTargetMbar(int32_t targetMbar)
{
    touchgfx::Unicode::snprintf(targetValueBuffer, TARGETVALUE_SIZE, "%d", static_cast<int>(targetMbar));
    targetValue.invalidate();
}

void BandyTargetPanel::setDecreaseCallback(touchgfx::GenericCallback<>& callback)
{
    decreaseCallback = &callback;
}

void BandyTargetPanel::setIncreaseCallback(touchgfx::GenericCallback<>& callback)
{
    increaseCallback = &callback;
}
