#include <gui/screen13_screen/Screen13View.hpp>
#include <gui/screen13_screen/Screen13Presenter.hpp>
#include <gui/model/Model.hpp>

Screen13Presenter::Screen13Presenter(Screen13View& v)
    : view(v)
{
}

void Screen13Presenter::activate()
{
}

void Screen13Presenter::deactivate()
{
}

AdminAuthResult Screen13Presenter::authenticate(uint16_t pin)
{
    return (model != 0) ? model->authenticateAdminPin(pin) : AdminAuthInvalidPin;
}

bool Screen13Presenter::isAuthenticated() const
{
    return (model != 0) && model->isAdminAuthenticated();
}

void Screen13Presenter::logout()
{
    if (model != 0)
    {
        model->logoutAdmin();
    }
}

uint8_t Screen13Presenter::getLockoutSeconds() const
{
    return (model != 0) ? model->getAdminLockoutRemainingSeconds() : 0U;
}
