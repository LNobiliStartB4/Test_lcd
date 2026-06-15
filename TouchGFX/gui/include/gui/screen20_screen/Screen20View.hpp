#ifndef SCREEN20VIEW_HPP
#define SCREEN20VIEW_HPP

#include <gui_generated/screen20_screen/Screen20ViewBase.hpp>
#include <gui/screen20_screen/Screen20Presenter.hpp>

class Screen20View : public Screen20ViewBase
{
public:
    Screen20View();
    virtual ~Screen20View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void backClicked();
    void updatePretest(const PneumaticPretestStatus& status);
protected:
    void applyStatus(const PneumaticPretestStatus& status);
    void completeSuccessfulPretest();

private:
    bool transitionRequested;
    bool successDisplayActive;
    bool successReadyToContinue;
    uint16_t successDisplayTicks;
    ActiveProduct successfulProduct;
};

#endif // SCREEN20VIEW_HPP
