#ifndef BANDYTARGETPANEL_HPP
#define BANDYTARGETPANEL_HPP

#include <gui_generated/containers/BandyTargetPanelBase.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <stdint.h>

class BandyTargetPanel : public BandyTargetPanelBase
{
public:
    BandyTargetPanel();
    virtual ~BandyTargetPanel() {}

    virtual void initialize();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void openSetpointClicked();

    void setTargetMbar(int32_t targetMbar);
    void setOpenCallback(touchgfx::GenericCallback<>& callback);
protected:
    touchgfx::GenericCallback<>* openCallback;
};

#endif // BANDYTARGETPANEL_HPP
