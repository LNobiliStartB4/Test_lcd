#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/screen2_screen/Screen2View.hpp>

Screen2Presenter::Screen2Presenter(Screen2View& v)
    : view(v)
{
}

void Screen2Presenter::activate()
{
    if (model != 0)
    {
        model->initializeDashboard();
        view.applyDashboardState(model->getDashboardState());
    }
}

void Screen2Presenter::deactivate()
{
}

void Screen2Presenter::dashboardStateUpdated(const DashboardState& state)
{
    view.applyDashboardState(state);
}

DashboardState Screen2Presenter::getDashboardState() const
{
    return (model != 0) ? model->getDashboardState() : DashboardState();
}

void Screen2Presenter::decreaseTarget()
{
    if (model != 0)
    {
        model->adjustTargetPressure(-10);
    }
}

void Screen2Presenter::increaseTarget()
{
    if (model != 0)
    {
        model->adjustTargetPressure(10);
    }
}

void Screen2Presenter::toggleSuction()
{
    if (model != 0)
    {
        const DashboardState state = model->getDashboardState();
        model->setSuctionEnabled(!state.suctionEnabled);
    }
}

void Screen2Presenter::confirmRelease()
{
    if (model != 0)
    {
        model->markBandReleased();
    }
}
