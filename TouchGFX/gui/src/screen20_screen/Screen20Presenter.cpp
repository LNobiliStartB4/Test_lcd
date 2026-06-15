#include <gui/screen20_screen/Screen20View.hpp>
#include <gui/screen20_screen/Screen20Presenter.hpp>
#include <gui/model/Model.hpp>

Screen20Presenter::Screen20Presenter(Screen20View& v)
    : view(v)
{

}

void Screen20Presenter::activate()
{

}

void Screen20Presenter::deactivate()
{

}

void Screen20Presenter::pneumaticPretestUpdated(const PneumaticPretestStatus& status)
{
    view.updatePretest(status);
}

PneumaticPretestStatus Screen20Presenter::getStatus() const
{
    return model->getPneumaticPretestStatus();
}

void Screen20Presenter::cancelPretest()
{
    model->cancelPneumaticPretest();
}

void Screen20Presenter::resetPretest()
{
    model->resetPneumaticPretest();
}
