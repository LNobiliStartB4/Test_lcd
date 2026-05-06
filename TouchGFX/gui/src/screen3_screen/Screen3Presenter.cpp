#include <gui/screen3_screen/Screen3Presenter.hpp>
#include <gui/screen3_screen/Screen3View.hpp>

Screen3Presenter::Screen3Presenter(Screen3View& v)
    : view(v)
{
}

void Screen3Presenter::activate()
{
    if (model != 0)
    {
        model->initializeBandyDemo();
        view.applyBandyState(model->getBandyState());
    }
}

void Screen3Presenter::deactivate()
{
}

void Screen3Presenter::bandyStateUpdated(const BandyState& state)
{
    view.applyBandyState(state);
}

BandyState Screen3Presenter::getBandyState() const
{
    return (model != 0) ? model->getBandyState() : BandyState();
}

void Screen3Presenter::startDemo()
{
    if (model != 0)
    {
        model->startBandyDemo();
    }
}

void Screen3Presenter::stopDemo()
{
    if (model != 0)
    {
        model->stopBandyDemo();
    }
}

void Screen3Presenter::increaseTarget()
{
    if (model != 0)
    {
        model->increaseBandyTarget();
    }
}

void Screen3Presenter::decreaseTarget()
{
    if (model != 0)
    {
        model->decreaseBandyTarget();
    }
}

bool Screen3Presenter::isVacuumCycleRunning() const
{
    return (model != 0) && model->isVacuumCycleRunning();
}
