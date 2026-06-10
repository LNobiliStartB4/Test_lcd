#ifndef SCREEN13VIEW_HPP
#define SCREEN13VIEW_HPP

#include <gui_generated/screen13_screen/Screen13ViewBase.hpp>
#include <gui/screen13_screen/Screen13Presenter.hpp>
#include <gui/model/DashboardTypes.hpp>

class Screen13View : public Screen13ViewBase
{
public:
    Screen13View();
    virtual ~Screen13View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

    virtual void backClicked();
    virtual void digit0Clicked() { appendDigit(0U); }
    virtual void digit1Clicked() { appendDigit(1U); }
    virtual void digit2Clicked() { appendDigit(2U); }
    virtual void digit3Clicked() { appendDigit(3U); }
    virtual void digit4Clicked() { appendDigit(4U); }
    virtual void digit5Clicked() { appendDigit(5U); }
    virtual void digit6Clicked() { appendDigit(6U); }
    virtual void digit7Clicked() { appendDigit(7U); }
    virtual void digit8Clicked() { appendDigit(8U); }
    virtual void digit9Clicked() { appendDigit(9U); }
    virtual void deleteClicked();

private:
    void appendDigit(uint8_t digit);
    void attemptAuthenticate();
    void updatePinDisplay();
    void updateLockoutState();

    uint8_t pinDigits[4];
    uint8_t pinLength;
    bool invalidPin;
    uint8_t tickDivider;
};

#endif // SCREEN13VIEW_HPP
