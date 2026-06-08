#ifndef SCREEN6VIEW_HPP
#define SCREEN6VIEW_HPP

#include <gui/model/DashboardTypes.hpp>
#include <gui_generated/screen6_screen/Screen6ViewBase.hpp>
#include <gui/screen6_screen/Screen6Presenter.hpp>

class Screen6View : public Screen6ViewBase
{
public:
    Screen6View();
    virtual ~Screen6View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void resumeClicked();
    virtual void endClicked();

    void applyBandyState(const BandyState& state);
protected:
    void updateTimeValues(const BandyState& state);
};

#endif // SCREEN6VIEW_HPP
