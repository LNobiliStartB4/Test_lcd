#include <gui/screen6_screen/Screen6View.hpp>
#include <gui/screen6_screen/Screen6Presenter.hpp>

Screen6Presenter::Screen6Presenter(Screen6View& v)
    : view(v)
{

}

void Screen6Presenter::activate()
{
    if (model != 0)
    {
        model->initializeBandyDemo();
        view.applyBandyState(model->getBandyState());
    }
}

void Screen6Presenter::deactivate()
{

}

void Screen6Presenter::bandyStateUpdated(const BandyState& state)
{
    view.applyBandyState(state);
}

BandyState Screen6Presenter::getBandyState() const
{
    return (model != 0) ? model->getBandyState() : BandyState();
}

void Screen6Presenter::resumeDemo()
{
    if (model != 0)
    {
        model->resumeBandyDemo();
    }
}

void Screen6Presenter::endDemo()
{
    if (model != 0)
    {
        model->endBandyDemo();
    }
}
