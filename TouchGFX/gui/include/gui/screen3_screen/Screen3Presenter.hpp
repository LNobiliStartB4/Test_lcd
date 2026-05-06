#ifndef SCREEN3PRESENTER_HPP
#define SCREEN3PRESENTER_HPP

#include <gui/model/DashboardTypes.hpp>
#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen3View;

class Screen3Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen3Presenter(Screen3View& v);
    virtual ~Screen3Presenter() {}

    virtual void activate();
    virtual void deactivate();
    virtual void bandyStateUpdated(const BandyState& state);

    BandyState getBandyState() const;
    void startDemo();
    void stopDemo();
    void increaseTarget();
    void decreaseTarget();
    bool isVacuumCycleRunning() const;

private:
    Screen3Presenter();

    Screen3View& view;
};

#endif // SCREEN3PRESENTER_HPP
