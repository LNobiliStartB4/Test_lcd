#ifndef BANDYSTARTPANEL_HPP
#define BANDYSTARTPANEL_HPP

#include <gui_generated/containers/BandyStartPanelBase.hpp>
#include <touchgfx/Callback.hpp>

class BandyStartPanel : public BandyStartPanelBase
{
public:
    BandyStartPanel();
    virtual ~BandyStartPanel() {}

    virtual void initialize();
    virtual void startClicked();

    void setStartCallback(touchgfx::GenericCallback<>& callback);
    void setRunning(bool running);
protected:
    void applyVisualState();

    touchgfx::GenericCallback<>* startCallback;
    bool runningState;
};

#endif // BANDYSTARTPANEL_HPP
