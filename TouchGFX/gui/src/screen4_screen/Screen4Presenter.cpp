#include <gui/screen4_screen/Screen4View.hpp>
#include <gui/screen4_screen/Screen4Presenter.hpp>

Screen4Presenter::Screen4Presenter(Screen4View& v)
    : view(v)
{

}

void Screen4Presenter::activate()
{
    if (model != 0)
    {
        model->initializeBandyDemo();
        model->startRfidScan();
    }
}

void Screen4Presenter::deactivate()
{
    if (model != 0)
    {
        model->stopRfidScan();
    }
}

bool Screen4Presenter::canOpenBandyScreen() const
{
    return (model != 0) && model->canOpenBandyScreen();
}
