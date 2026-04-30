#ifndef BANDYTARGETPANEL_HPP
#define BANDYTARGETPANEL_HPP

#include <gui_generated/containers/BandyTargetPanelBase.hpp>
#include <touchgfx/Callback.hpp>
#include <stdint.h>

class BandyTargetPanel : public BandyTargetPanelBase
{
public:
    BandyTargetPanel();
    virtual ~BandyTargetPanel() {}

    virtual void initialize();
    virtual void decreaseTargetClicked();
    virtual void increaseTargetClicked();

    void setTargetMbar(int32_t targetMbar);
    void setDecreaseCallback(touchgfx::GenericCallback<>& callback);
    void setIncreaseCallback(touchgfx::GenericCallback<>& callback);
protected:
    touchgfx::GenericCallback<>* decreaseCallback;
    touchgfx::GenericCallback<>* increaseCallback;
};

#endif // BANDYTARGETPANEL_HPP
