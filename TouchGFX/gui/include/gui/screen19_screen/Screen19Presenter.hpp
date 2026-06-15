#ifndef SCREEN19PRESENTER_HPP
#define SCREEN19PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen19View;

class Screen19Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen19Presenter(Screen19View& v);

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

    virtual ~Screen19Presenter() {}

private:
    Screen19Presenter();

    Screen19View& view;
};

#endif // SCREEN19PRESENTER_HPP
