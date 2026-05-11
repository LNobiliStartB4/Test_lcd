#ifndef SCREEN5VIEW_HPP
#define SCREEN5VIEW_HPP

#include <gui/model/DashboardTypes.hpp>
#include <gui_generated/screen5_screen/Screen5ViewBase.hpp>
#include <gui/screen5_screen/Screen5Presenter.hpp>
#include <stdint.h>

class Screen5View : public Screen5ViewBase
{
public:
    Screen5View();
    virtual ~Screen5View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void backClicked();
    virtual void decreaseTargetClicked();
    virtual void increaseTargetClicked();

    void applyBandyState(const BandyState& state);
};

#endif // SCREEN5VIEW_HPP
