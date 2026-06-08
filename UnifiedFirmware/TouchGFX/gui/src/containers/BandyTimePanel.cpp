#include <gui/containers/BandyTimePanel.hpp>
#include <touchgfx/Unicode.hpp>

BandyTimePanel::BandyTimePanel()
{

}

void BandyTimePanel::initialize()
{
    BandyTimePanelBase::initialize();
    setRemainingSeconds(0);
}

void BandyTimePanel::setRemainingSeconds(uint16_t seconds)
{
    const uint16_t minutes = static_cast<uint16_t>(seconds / 60U);
    const uint16_t remaining = static_cast<uint16_t>(seconds % 60U);

    timeRemainingValueBuffer[0] = static_cast<touchgfx::Unicode::UnicodeChar>('0' + ((minutes / 10U) % 10U));
    timeRemainingValueBuffer[1] = static_cast<touchgfx::Unicode::UnicodeChar>('0' + (minutes % 10U));
    timeRemainingValueBuffer[2] = static_cast<touchgfx::Unicode::UnicodeChar>(':');
    timeRemainingValueBuffer[3] = static_cast<touchgfx::Unicode::UnicodeChar>('0' + (remaining / 10U));
    timeRemainingValueBuffer[4] = static_cast<touchgfx::Unicode::UnicodeChar>('0' + (remaining % 10U));
    timeRemainingValueBuffer[5] = 0;

    timeRemainingValue.invalidate();
}
