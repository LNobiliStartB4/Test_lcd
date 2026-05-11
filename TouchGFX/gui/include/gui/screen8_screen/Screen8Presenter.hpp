#ifndef SCREEN8PRESENTER_HPP
#define SCREEN8PRESENTER_HPP

#include <gui/model/DashboardTypes.hpp>
#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen8View;

class Screen8Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen8Presenter(Screen8View& v);

    virtual void activate();
    virtual void deactivate();
    virtual void bandyStateUpdated(const BandyState& state);

    BandyState getBandyState() const;
    void cancelEndDemo();

    virtual ~Screen8Presenter() {}

private:
    Screen8Presenter();

    Screen8View& view;
};

#endif // SCREEN8PRESENTER_HPP
