#ifndef SCREEN19VIEW_HPP
#define SCREEN19VIEW_HPP

#include <gui_generated/screen19_screen/Screen19ViewBase.hpp>
#include <gui/screen19_screen/Screen19Presenter.hpp>

class Screen19View : public Screen19ViewBase
{
public:
    Screen19View();
    virtual ~Screen19View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void continueClicked();
protected:
};

#endif // SCREEN19VIEW_HPP
