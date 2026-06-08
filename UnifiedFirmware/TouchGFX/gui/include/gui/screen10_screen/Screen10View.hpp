#ifndef SCREEN10VIEW_HPP
#define SCREEN10VIEW_HPP

#include <gui_generated/screen10_screen/Screen10ViewBase.hpp>
#include <gui/screen10_screen/Screen10Presenter.hpp>
#include <touchgfx/Callback.hpp>

class Screen10View : public Screen10ViewBase
{
public:
    Screen10View();
    virtual ~Screen10View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void backClicked();
    virtual void previousPageClicked();
    virtual void nextPageClicked();

private:
    static const uint8_t kRowsPerPage = 3;

    void showPage(uint8_t pageIndex);
    void activateSlot(uint8_t slotIndex);
    void row0Clicked();
    void row1Clicked();
    void row2Clicked();

    uint8_t currentPage;
    touchgfx::Callback<Screen10View> row0Callback;
    touchgfx::Callback<Screen10View> row1Callback;
    touchgfx::Callback<Screen10View> row2Callback;
};

#endif // SCREEN10VIEW_HPP
