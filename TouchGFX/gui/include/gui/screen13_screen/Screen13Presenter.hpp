#ifndef SCREEN13PRESENTER_HPP
#define SCREEN13PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <gui/model/DashboardTypes.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen13View;

class Screen13Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen13Presenter(Screen13View& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~Screen13Presenter() {}

    AdminAuthResult authenticate(uint16_t pin);
    bool isAuthenticated() const;
    void logout();
    uint8_t getLockoutSeconds() const;

private:
    Screen13Presenter();

    Screen13View& view;
};

#endif // SCREEN13PRESENTER_HPP
