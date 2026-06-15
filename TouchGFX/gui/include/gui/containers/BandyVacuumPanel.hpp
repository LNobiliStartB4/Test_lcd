#ifndef BANDYVACUUMPANEL_HPP
#define BANDYVACUUMPANEL_HPP

#include <gui_generated/containers/BandyVacuumPanelBase.hpp>
#include <stdint.h>

class BandyVacuumPanel : public BandyVacuumPanelBase
{
public:
    BandyVacuumPanel();
    virtual ~BandyVacuumPanel() {}

    virtual void initialize();

    void setVacuumMbar(int32_t vacuumMbar);
};

#endif // BANDYVACUUMPANEL_HPP
