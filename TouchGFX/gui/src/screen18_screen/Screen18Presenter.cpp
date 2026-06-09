#include <gui/screen18_screen/Screen18View.hpp>
#include <gui/screen18_screen/Screen18Presenter.hpp>

Screen18Presenter::Screen18Presenter(Screen18View& v)
    : view(v)
{

}

void Screen18Presenter::activate()
{
    if (model != 0)
    {
        model->initializeBandyDemo();
        view.applyBandyState(model->getBandyState());
    }
}

void Screen18Presenter::deactivate()
{

}

void Screen18Presenter::bandyStateUpdated(const BandyState& state)
{
    view.applyBandyState(state);
}

BandyState Screen18Presenter::getBandyState() const
{
    return (model != 0) ? model->getBandyState() : BandyState();
}

void Screen18Presenter::setTarget(int32_t targetMbar)
{
    if (model != 0)
    {
        model->setBandyTarget(targetMbar);
    }
}
