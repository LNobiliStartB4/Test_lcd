#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/model/Model.hpp>

Screen2Presenter::Screen2Presenter(Screen2View& v)
    : view(v)
{
}

void Screen2Presenter::activate()
{
}

void Screen2Presenter::deactivate()
{
}

bool Screen2Presenter::startPneumaticPretest(ActiveProduct product)
{
    return model->startPneumaticPretest(product);
}
