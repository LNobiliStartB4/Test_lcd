#ifndef SCREEN3VIEW_HPP
#define SCREEN3VIEW_HPP

#include <gui/model/DashboardTypes.hpp>
#include <gui_generated/screen3_screen/Screen3ViewBase.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>

class Screen3View : public Screen3ViewBase
{
public:
    Screen3View();
    virtual ~Screen3View() {}

    virtual void setupScreen();
    virtual void tearDownScreen();
    void openSetpointClicked();
    void startClicked();

    void applyBandyState(const BandyState& state);

private:
    void updateTargetDisplay(int32_t targetMbar);
    void updateStartControl(BandySessionState sessionState);

    BandyState latestState;
    bool screenTransitionRequested;
};

#endif // SCREEN3VIEW_HPP
