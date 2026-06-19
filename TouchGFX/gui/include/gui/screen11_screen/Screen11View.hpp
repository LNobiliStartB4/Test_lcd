#ifndef SCREEN11VIEW_HPP
#define SCREEN11VIEW_HPP

#include <stdint.h>
#include <gui_generated/screen11_screen/Screen11ViewBase.hpp>
#include <gui/screen11_screen/Screen11Presenter.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/events/DragEvent.hpp>
#include <touchgfx/widgets/Box.hpp>

class Screen11View : public Screen11ViewBase
{
public:
    Screen11View();
    virtual ~Screen11View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void backClicked();
    virtual void brightnessChanged(int value);
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleDragEvent(const touchgfx::DragEvent& event);

private:
    void updateBrightnessValue(uint8_t percent);
    void updateBrightnessFromX(int16_t x);
    void updateBrightnessBar(uint8_t percent);
    bool isBrightnessTouchArea(int16_t x, int16_t y) const;

    touchgfx::Box brightnessTrackBackground;
    touchgfx::Box brightnessTrackFill;
    touchgfx::Box brightnessKnob;
};

#endif // SCREEN11VIEW_HPP
