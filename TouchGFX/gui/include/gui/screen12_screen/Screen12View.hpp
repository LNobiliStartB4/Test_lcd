#ifndef SCREEN12VIEW_HPP
#define SCREEN12VIEW_HPP

#include <gui_generated/screen12_screen/Screen12ViewBase.hpp>
#include <gui/screen12_screen/Screen12Presenter.hpp>
#include <stdint.h>

class Screen12View : public Screen12ViewBase
{
public:
    Screen12View();
    virtual ~Screen12View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void backClicked();
    virtual void row0Clicked();
    virtual void row1Clicked();
    virtual void row2Clicked();
    virtual void row3Clicked();
    virtual void previousPageClicked();
    virtual void nextPageClicked();

private:
    void showPage(uint8_t pageIndex);
    void selectSlot(uint8_t slot);
    void applyLanguage(UiLanguage language);

    uint8_t currentPage;
    UiLanguage currentLanguage;
};

#endif // SCREEN12VIEW_HPP
