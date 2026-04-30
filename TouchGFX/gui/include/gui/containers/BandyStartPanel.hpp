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
protected:
    touchgfx::GenericCallback<>* startCallback;
};

#endif // BANDYSTARTPANEL_HPP
