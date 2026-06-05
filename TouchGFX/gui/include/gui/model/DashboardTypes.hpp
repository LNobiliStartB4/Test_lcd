#ifndef DASHBOARDTYPES_HPP
#define DASHBOARDTYPES_HPP

#include <stdint.h>

enum ActiveProduct
{
    ActiveProductNone = 0,
    ActiveProductBandy = 1,
    ActiveProductHemorflow = 2
};

enum UiLanguage
{
    UiLanguageEnglish = 0,
    UiLanguageItalian = 1
};

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

struct HemorflowState
{
    HemorflowState()
        : currentPressureMbar(0),
          targetMbar(150),
          running(false)
    {
    }

    int32_t currentPressureMbar;
    int32_t targetMbar;
    bool running;
};

#endif // DASHBOARDTYPES_HPP
