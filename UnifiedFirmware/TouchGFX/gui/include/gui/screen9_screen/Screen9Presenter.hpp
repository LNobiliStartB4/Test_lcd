#ifndef SCREEN9PRESENTER_HPP
#define SCREEN9PRESENTER_HPP

#include <gui/model/DashboardTypes.hpp>
#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

class Screen9View;

class Screen9Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen9Presenter(Screen9View& v);
    virtual ~Screen9Presenter() {}

    virtual void activate();
    virtual void deactivate();

    HemorflowState getHemorflowState() const;
    bool shouldReturnToHemorflowWait() const;

private:
    Screen9Presenter();
    Screen9View& view;
};

#endif // SCREEN9PRESENTER_HPP
