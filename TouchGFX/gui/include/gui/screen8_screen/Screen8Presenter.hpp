#ifndef SCREEN8PRESENTER_HPP
#define SCREEN8PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

class Screen8View;

class Screen8Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen8Presenter(Screen8View& v);
    virtual ~Screen8Presenter() {}

    virtual void activate();
    virtual void deactivate();

    void initializeHemorflowMonitor();
    bool canOpenHemorflowMonitor() const;

private:
    Screen8Presenter();
    Screen8View& view;
};

#endif // SCREEN8PRESENTER_HPP
