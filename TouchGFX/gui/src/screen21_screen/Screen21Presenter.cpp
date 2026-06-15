#include <gui/screen21_screen/Screen21View.hpp>
#include <gui/screen21_screen/Screen21Presenter.hpp>
#include <gui/model/Model.hpp>

Screen21Presenter::Screen21Presenter(Screen21View& v)
    : view(v)
{

}

void Screen21Presenter::activate()
{

}

void Screen21Presenter::deactivate()
{

}

void Screen21Presenter::pneumaticPretestUpdated(const PneumaticPretestStatus& status)
{
    view.updatePretest(status);
}

PneumaticPretestStatus Screen21Presenter::getStatus() const
{
    return model->getPneumaticPretestStatus();
}

bool Screen21Presenter::retryPretest()
{
    return model->retryPneumaticPretest();
}

void Screen21Presenter::resetPretest()
{
    model->resetPneumaticPretest();
}
