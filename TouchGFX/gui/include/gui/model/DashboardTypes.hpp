#ifndef DASHBOARDTYPES_HPP
#define DASHBOARDTYPES_HPP

#include <stdint.h>

enum BandyVacuumState
{
    BandyVacuumStateReady = 0,
    BandyVacuumStatePulling,
    BandyVacuumStateTarget
};

struct BandyState
{
    BandyState()
        : currentVacuumMbar(0),
          targetVacuumMbar(450),
          running(false),
          targetReached(false),
          vacuumState(BandyVacuumStateReady)
    {
    }

    int32_t currentVacuumMbar;
    int32_t targetVacuumMbar;
    bool running;
    bool targetReached;
    BandyVacuumState vacuumState;
};

#endif // DASHBOARDTYPES_HPP
