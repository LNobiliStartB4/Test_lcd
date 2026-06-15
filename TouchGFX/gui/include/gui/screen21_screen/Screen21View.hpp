#ifndef SCREEN21VIEW_HPP
#define SCREEN21VIEW_HPP

#include <gui_generated/screen21_screen/Screen21ViewBase.hpp>
#include <gui/screen21_screen/Screen21Presenter.hpp>

class Screen21View : public Screen21ViewBase
{
public:
    Screen21View();
    virtual ~Screen21View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void backClicked();
    virtual void retryClicked();
    void updatePretest(const PneumaticPretestStatus& status);
protected:
    void applyStatus(const PneumaticPretestStatus& status);
    bool isRetryAvailable(const PneumaticPretestStatus& status) const;
    void setReleaseDotsActive(bool active);
    void updateDots(uint8_t activeIndex);
    void hideReleaseDots();

private:
    bool transitionRequested;
    bool exitRequested;
    bool releaseDotsActive;
    uint16_t releaseDotTicks;
};

#endif // SCREEN21VIEW_HPP
