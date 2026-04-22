#ifndef SCREEN2PRESENTER_HPP
#define SCREEN2PRESENTER_HPP

#include <gui/model/DashboardTypes.hpp>
#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen2View;

class Screen2Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen2Presenter(Screen2View& v);

    virtual void activate();
    virtual void deactivate();
    virtual void dashboardStateUpdated(const DashboardState& state);

    DashboardState getDashboardState() const;
    void decreaseTarget();
    void increaseTarget();
    void toggleSuction();
    void confirmRelease();

    virtual ~Screen2Presenter()
    {
    }

private:
    Screen2Presenter();
    Screen2View& view;
};

#endif // SCREEN2PRESENTER_HPP
