#include <gui/screen7_screen/Screen7View.hpp>
#include <gui/screen7_screen/Screen7Presenter.hpp>

Screen7Presenter::Screen7Presenter(Screen7View& v)
    : view(v)
{

}

void Screen7Presenter::activate()
{
    if (model != 0)
    {
        model->initializeBandyDemo();
        view.applyBandyState(model->getBandyState());
    }
}

void Screen7Presenter::deactivate()
{

}

void Screen7Presenter::bandyStateUpdated(const BandyState& state)
{
    view.applyBandyState(state);
}

BandyState Screen7Presenter::getBandyState() const
{
    return (model != 0) ? model->getBandyState() : BandyState();
}

void Screen7Presenter::endDemo()
{
    if (model != 0)
    {
        model->endBandyDemo();
    }
}
