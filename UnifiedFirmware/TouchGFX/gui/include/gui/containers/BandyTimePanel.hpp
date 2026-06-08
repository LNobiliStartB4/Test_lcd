#ifndef BANDYTIMEPANEL_HPP
#define BANDYTIMEPANEL_HPP

#include <gui_generated/containers/BandyTimePanelBase.hpp>
#include <stdint.h>

class BandyTimePanel : public BandyTimePanelBase
{
public:
    BandyTimePanel();
    virtual ~BandyTimePanel() {}

    virtual void initialize();
    void setRemainingSeconds(uint16_t seconds);
protected:
};

#endif // BANDYTIMEPANEL_HPP
