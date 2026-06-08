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

struct AdminDiagnosticsSnapshot
{
    AdminDiagnosticsSnapshot()
        : uptimeSeconds(0U),
          language(UiLanguageEnglish),
          brightnessPercent(100U),
          pressureAvailable(false),
          pressureDetailsAvailable(false),
          pressureValid(false),
          relativePressureMbar(0),
          rawRelativePressureMbar(0),
          zeroOffsetMbar(0),
          ambientRawAdc(0U),
          chamberRawAdc(0U),
          ambientAbsMbar(0U),
          chamberAbsMbar(0U),
          targetMbar(0),
          pumpDutyPercent(0U),
          pressureState(0U),
          pressureFault(0U),
          framAvailable(false),
          framPresent(false),
          framSizeBytes(0U),
          sessionRecordValid(false),
          winbondAvailable(false),
          winbondPresent(false),
          winbondSizeBytes(0U),
          assetPackageValid(false)
    {
        deviceName[0] = 0;
        firmwareVersion[0] = 0;
        framId[0] = 0;
        winbondId[0] = 0;
    }

    char deviceName[24];
    char firmwareVersion[16];
    uint32_t uptimeSeconds;
    UiLanguage language;
    uint8_t brightnessPercent;

    bool pressureAvailable;
    bool pressureDetailsAvailable;
    bool pressureValid;
    int32_t relativePressureMbar;
    int32_t rawRelativePressureMbar;
    int32_t zeroOffsetMbar;
    uint16_t ambientRawAdc;
    uint16_t chamberRawAdc;
    uint16_t ambientAbsMbar;
    uint16_t chamberAbsMbar;
    int32_t targetMbar;
    uint8_t pumpDutyPercent;
    uint8_t pressureState;
    uint8_t pressureFault;

    bool framAvailable;
    bool framPresent;
    uint32_t framSizeBytes;
    bool sessionRecordValid;
    char framId[16];

    bool winbondAvailable;
    bool winbondPresent;
    uint32_t winbondSizeBytes;
    bool assetPackageValid;
    char winbondId[16];
};

#endif // DASHBOARDTYPES_HPP
