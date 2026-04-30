#include <gui/containers/BandyVacuumPanel.hpp>
#include <touchgfx/Unicode.hpp>

namespace
{
const int32_t kGaugeMaxMbar = 500;
const int16_t kMeterMaxWidth = 356;

int16_t meterWidthForValue(int32_t vacuumMbar)
{
    if (vacuumMbar < 0)
    {
        vacuumMbar = 0;
    }
    else if (vacuumMbar > kGaugeMaxMbar)
    {
        vacuumMbar = kGaugeMaxMbar;
    }

    return static_cast<int16_t>((vacuumMbar * kMeterMaxWidth) / kGaugeMaxMbar);
}
}

BandyVacuumPanel::BandyVacuumPanel()
{

}

void BandyVacuumPanel::initialize()
{
    BandyVacuumPanelBase::initialize();
    setVacuumMbar(0);
}

void BandyVacuumPanel::setVacuumMbar(int32_t vacuumMbar)
{
    touchgfx::Unicode::snprintf(vacuumValueBuffer, VACUUMVALUE_SIZE, "%d", static_cast<int>(vacuumMbar));

    vacuumFill.invalidate();
    vacuumFill.setWidth(meterWidthForValue(vacuumMbar));
    vacuumFill.invalidate();
    vacuumValue.invalidate();
}
