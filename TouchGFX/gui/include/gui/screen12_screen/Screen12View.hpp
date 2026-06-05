#ifndef SCREEN12VIEW_HPP
#define SCREEN12VIEW_HPP

#include <gui_generated/screen12_screen/Screen12ViewBase.hpp>
#include <gui/screen12_screen/Screen12Presenter.hpp>

class Screen12View : public Screen12ViewBase
{
public:
    Screen12View();
    virtual ~Screen12View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void backClicked();
    virtual void englishClicked();
    virtual void italianClicked();

private:
    void applyLanguage(UiLanguage language);
    void refreshLanguageSelection(UiLanguage language);
};

#endif // SCREEN12VIEW_HPP
