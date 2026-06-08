#include <gui/screen5_screen/Screen5View.hpp>
#include <gui/screen5_screen/Screen5Presenter.hpp>

Screen5Presenter::Screen5Presenter(Screen5View& v)
    : view(v)
{

}

void Screen5Presenter::activate()
{
    if (model != 0)
    {
        model->initializeBandyDemo();
        view.applyBandyState(model->getBandyState());
    }
}

void Screen5Presenter::deactivate()
{

}

void Screen5Presenter::bandyStateUpdated(const BandyState& state)
{
    view.applyBandyState(state);
}

BandyState Screen5Presenter::getBandyState() const
{
    return (model != 0) ? model->getBandyState() : BandyState();
}

void Screen5Presenter::increaseTarget()
{
    if (model != 0)
    {
        model->increaseBandyTarget();
    }
}

void Screen5Presenter::decreaseTarget()
{
    if (model != 0)
    {
        model->decreaseBandyTarget();
    }
}
