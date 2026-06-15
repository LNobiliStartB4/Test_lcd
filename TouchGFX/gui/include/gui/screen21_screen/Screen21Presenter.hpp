#ifndef SCREEN21PRESENTER_HPP
#define SCREEN21PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen21View;

class Screen21Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen21Presenter(Screen21View& v);

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
    bool retryPretest();
    void resetPretest();

    virtual ~Screen21Presenter() {}

private:
    Screen21Presenter();

    Screen21View& view;
};

#endif // SCREEN21PRESENTER_HPP
