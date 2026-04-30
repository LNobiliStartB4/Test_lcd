#ifndef SCREEN3VIEW_HPP
#define SCREEN3VIEW_HPP

#include <gui/model/DashboardTypes.hpp>
#include <gui_generated/screen3_screen/Screen3ViewBase.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>
#include <touchgfx/Callback.hpp>
#include <stdint.h>

class Screen3View : public Screen3ViewBase
{
public:
    Screen3View();
    virtual ~Screen3View() {}

    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    void decreaseTargetClicked();
    void increaseTargetClicked();
    void startClicked();

    void applyBandyState(const BandyState& state);

private:
    BandyState latestState;
    uint8_t fullRedrawTicks;
    touchgfx::Callback<Screen3View> decreaseTargetCallback;
    touchgfx::Callback<Screen3View> increaseTargetCallback;
    touchgfx::Callback<Screen3View> startCallback;
    bool returnToWaitRequested;
};

#endif // SCREEN3VIEW_HPP
