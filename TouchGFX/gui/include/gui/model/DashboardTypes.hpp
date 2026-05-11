#ifndef DASHBOARDTYPES_HPP
#define DASHBOARDTYPES_HPP

#include <stdint.h>

enum BandyVacuumState
{
    BandyVacuumStateReady = 0,
    BandyVacuumStatePulling,
    BandyVacuumStateTarget
};

enum BandySessionState
{
    BandySessionWaitRfid = 0,
    BandySessionAuthorized = 1,
    BandySessionRunning = 2,
    BandySessionPaused = 3
};

struct BandyState
{
    BandyState()
        : currentVacuumMbar(0),
          targetVacuumMbar(450),
          remainingSeconds(0),
          pauseRemainingSeconds(0),
          running(false),
          targetReached(false),
          vacuumState(BandyVacuumStateReady),
          sessionState(BandySessionWaitRfid)
    {
    }

    int32_t currentVacuumMbar;
    int32_t targetVacuumMbar;
    uint16_t remainingSeconds;
    uint16_t pauseRemainingSeconds;
    bool running;
    bool targetReached;
    BandyVacuumState vacuumState;
    BandySessionState sessionState;
};

#endif // DASHBOARDTYPES_HPP
