#include <gui/screen8_screen/Screen8Presenter.hpp>
#include <gui/screen8_screen/Screen8View.hpp>
#include <gui/model/Model.hpp>

Screen8Presenter::Screen8Presenter(Screen8View& v)
    : view(v)
{
}

void Screen8Presenter::activate()
{
}

void Screen8Presenter::deactivate()
{
}

void Screen8Presenter::initializeHemorflowMonitor()
{
    model->initializeHemorflowMonitor();
}

bool Screen8Presenter::canOpenHemorflowMonitor() const
{
    return model->canOpenHemorflowMonitor();
}
