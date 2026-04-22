#ifndef DASHBOARDTYPES_HPP
#define DASHBOARDTYPES_HPP

#include <stdint.h>

enum VacuumState
{
    VacuumStateReady = 0,
    VacuumStatePulling,
    VacuumStateTissueReady,
    VacuumStateWarning
};

enum WarningCode
{
    WarningCodeNone = 0,
    WarningCodeLeak,
    WarningCodeSensorMissing
};

enum ProcedureStep
{
    ProcedureStepLoadRing = 0,
    ProcedureStepPosition,
    ProcedureStepAspirate,
    ProcedureStepRelease,
    ProcedureStepVerify
};

struct DashboardState
{
    DashboardState()
        : currentPressureMbar(0),
          targetPressureMbar(200),
          stabilityPercent(0),
          ligatureCount(0),
          suctionEnabled(false),
          procedureStep(ProcedureStepLoadRing),
          vacuumState(VacuumStateReady),
          warningCode(WarningCodeNone)
    {
    }

    int32_t currentPressureMbar;
    int32_t targetPressureMbar;
    uint8_t stabilityPercent;
    uint16_t ligatureCount;
    bool suctionEnabled;
    ProcedureStep procedureStep;
    VacuumState vacuumState;
    WarningCode warningCode;
};

#endif // DASHBOARDTYPES_HPP
