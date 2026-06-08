#include <gui/screen11_screen/Screen11View.hpp>
#include <gui/screen11_screen/Screen11Presenter.hpp>

Screen11Presenter::Screen11Presenter(Screen11View& v)
    : view(v)
{

}

void Screen11Presenter::activate()
{

}

void Screen11Presenter::deactivate()
{

}

uint8_t Screen11Presenter::getBrightnessPercent() const
{
    return (model != 0) ? model->getDisplayBrightnessPercent() : 100U;
}

void Screen11Presenter::setBrightnessPercent(uint8_t percent)
{
    if (model != 0)
    {
        model->setDisplayBrightnessPercent(percent);
    }
}
