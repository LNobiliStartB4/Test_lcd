#ifndef SCREEN18VIEW_HPP
#define SCREEN18VIEW_HPP

#include <gui/model/DashboardTypes.hpp>
#include <gui_generated/screen18_screen/Screen18ViewBase.hpp>
#include <gui/screen18_screen/Screen18Presenter.hpp>
#include <stdint.h>

class Screen18View : public Screen18ViewBase
{
public:
    Screen18View();
    virtual ~Screen18View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void cancelClicked();
    virtual void digit0Clicked();
    virtual void digit1Clicked();
    virtual void digit2Clicked();
    virtual void digit3Clicked();
    virtual void digit4Clicked();
    virtual void digit5Clicked();
    virtual void digit6Clicked();
    virtual void digit7Clicked();
    virtual void digit8Clicked();
    virtual void digit9Clicked();
    virtual void deleteClicked();
    virtual void applyClicked();

    void applyBandyState(const BandyState& state);

private:
    void appendDigit(uint8_t digit);
    void updateValueDisplay();
    void initializeEntry(int32_t targetMbar);

    BandyState latestState;
    uint16_t enteredValue;
    uint8_t entryLength;
    bool replaceOnNextDigit;
    bool invalidEntry;
    bool entryInitialized;
    bool screenTransitionRequested;
};

#endif // SCREEN18VIEW_HPP
