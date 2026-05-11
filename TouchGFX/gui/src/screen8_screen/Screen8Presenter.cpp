#include <gui/screen8_screen/Screen8View.hpp>
#include <gui/screen8_screen/Screen8Presenter.hpp>

Screen8Presenter::Screen8Presenter(Screen8View& v)
    : view(v)
{
}

void Screen8Presenter::activate()
{
    if (model != 0)
    {
        view.applyBandyState(model->getBandyState());
    }
}

void Screen8Presenter::deactivate()
{
}

void Screen8Presenter::bandyStateUpdated(const BandyState& state)
{
    view.applyBandyState(state);
}

BandyState Screen8Presenter::getBandyState() const
{
    return (model != 0) ? model->getBandyState() : BandyState();
}

void Screen8Presenter::cancelEndDemo()
{
    if (model != 0)
    {
        model->cancelEndBandyDemo();
    }
}
