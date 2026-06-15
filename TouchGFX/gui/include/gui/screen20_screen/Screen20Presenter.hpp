#ifndef SCREEN20PRESENTER_HPP
#define SCREEN20PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen20View;

class Screen20Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen20Presenter(Screen20View& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();
    virtual void pneumaticPretestUpdated(const PneumaticPretestStatus& status);
    PneumaticPretestStatus getStatus() const;
    void cancelPretest();
    void resetPretest();

    virtual ~Screen20Presenter() {}

private:
    Screen20Presenter();

    Screen20View& view;
};

#endif // SCREEN20PRESENTER_HPP
