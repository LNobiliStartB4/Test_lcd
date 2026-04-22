#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

namespace
{
const int32_t kDefaultTargetMbar = 200;
const int32_t kMinTargetMbar = 120;
const int32_t kMaxTargetMbar = 260;
const uint8_t kTickDividerLimit = 6; // 60 Hz / 6 = 10 Hz dashboard updates
const uint8_t kLeakThresholdTicks = 25;
const uint8_t kVerifyFeedbackTicks = 12;

const int8_t kPressureWavePattern[] = { 0, 1, 2, 3, 2, 1, 0, -1, -2, -3, -2, -1 };

int32_t absoluteValue(int32_t value)
{
    return (value >= 0) ? value : -value;
}
}

Model::Model()
    : modelListener(0),
      dashboardState(),
      tickDivider(0),
      waveIndex(0),
      leakTicks(0),
      releaseFeedbackTicks(0),
      dashboardInitialized(false)
{
}

void Model::tick()
{
    if (!dashboardInitialized)
    {
        return;
    }

    if (++tickDivider < kTickDividerLimit)
    {
        return;
    }

    tickDivider = 0;

    DashboardState previousState = dashboardState;

    updateSimulationStep();
    updateDerivedState();

    if (hasStateChanged(previousState))
    {
        notifyDashboardState();
    }
}

void Model::initializeDashboard()
{
    dashboardState = DashboardState();
    dashboardState.targetPressureMbar = kDefaultTargetMbar;
    dashboardInitialized = true;
    tickDivider = 0;
    waveIndex = 0;
    leakTicks = 0;
    releaseFeedbackTicks = 0;

    updateDerivedState();
    notifyDashboardState();
}

void Model::setSuctionEnabled(bool enabled)
{
    if (!dashboardInitialized)
    {
        initializeDashboard();
    }

    if (dashboardState.suctionEnabled == enabled)
    {
        return;
    }

    dashboardState.suctionEnabled = enabled;
    dashboardState.warningCode = WarningCodeNone;
    leakTicks = 0;

    if (enabled)
    {
        releaseFeedbackTicks = 0;
    }
    else if (releaseFeedbackTicks == 0)
    {
        dashboardState.procedureStep = ProcedureStepLoadRing;
    }

    updateDerivedState();
    notifyDashboardState();
}

void Model::adjustTargetPressure(int32_t deltaMbar)
{
    if (!dashboardInitialized)
    {
        initializeDashboard();
    }

    DashboardState previousState = dashboardState;
    dashboardState.targetPressureMbar = clampTargetPressure(dashboardState.targetPressureMbar + deltaMbar);
    updateDerivedState();

    if (hasStateChanged(previousState))
    {
        notifyDashboardState();
    }
}

void Model::markBandReleased()
{
    if (!dashboardInitialized)
    {
        initializeDashboard();
    }

    if (dashboardState.warningCode != WarningCodeNone || dashboardState.vacuumState != VacuumStateTissueReady)
    {
        return;
    }

    dashboardState.ligatureCount++;
    dashboardState.suctionEnabled = false;
    dashboardState.warningCode = WarningCodeNone;
    dashboardState.procedureStep = ProcedureStepVerify;

    releaseFeedbackTicks = kVerifyFeedbackTicks;
    leakTicks = 0;

    updateDerivedState();
    notifyDashboardState();
}

void Model::resetProcedure()
{
    initializeDashboard();
}

void Model::notifyDashboardState()
{
    if (modelListener != 0)
    {
        modelListener->dashboardStateUpdated(dashboardState);
    }
}

void Model::updateSimulationStep()
{
    const int32_t previousPressure = dashboardState.currentPressureMbar;

    if (dashboardState.suctionEnabled)
    {
        int32_t desiredPressure = dashboardState.targetPressureMbar;
        if (dashboardState.currentPressureMbar >= (dashboardState.targetPressureMbar - 18))
        {
            desiredPressure += kPressureWavePattern[waveIndex];
            waveIndex = static_cast<uint8_t>((waveIndex + 1U) % (sizeof(kPressureWavePattern) / sizeof(kPressureWavePattern[0])));
        }

        int32_t delta = desiredPressure - dashboardState.currentPressureMbar;
        int32_t step = delta / 3;

        if (step == 0 && delta != 0)
        {
            step = (delta > 0) ? 1 : -1;
        }

        dashboardState.currentPressureMbar += step;
        if (dashboardState.currentPressureMbar < 0)
        {
            dashboardState.currentPressureMbar = 0;
        }
    }
    else if (dashboardState.currentPressureMbar > 0)
    {
        int32_t decayStep = dashboardState.currentPressureMbar / 4;
        if (decayStep < 4)
        {
            decayStep = 4;
        }

        dashboardState.currentPressureMbar -= decayStep;
        if (dashboardState.currentPressureMbar < 0)
        {
            dashboardState.currentPressureMbar = 0;
        }
    }

    if (dashboardState.suctionEnabled)
    {
        const int32_t targetPressure = dashboardState.targetPressureMbar;
        const int32_t pressureDrop = previousPressure - dashboardState.currentPressureMbar;
        const bool underTarget = dashboardState.currentPressureMbar < ((targetPressure * 8) / 10);

        if (underTarget || pressureDrop > 20)
        {
            if (leakTicks < 255)
            {
                leakTicks++;
            }
        }
        else
        {
            leakTicks = 0;
        }
    }
    else
    {
        leakTicks = 0;
    }

    if (releaseFeedbackTicks > 0)
    {
        releaseFeedbackTicks--;
    }
}

void Model::updateDerivedState()
{
    const int32_t currentPressure = dashboardState.currentPressureMbar;
    const int32_t targetPressure = dashboardState.targetPressureMbar;
    const int32_t pressureError = absoluteValue(targetPressure - currentPressure);

    if (!dashboardState.suctionEnabled)
    {
        dashboardState.stabilityPercent = 0;
    }
    else
    {
        int32_t stability = 100 - (pressureError * 3);
        if (stability < 0)
        {
            stability = 0;
        }
        else if (stability > 100)
        {
            stability = 100;
        }

        dashboardState.stabilityPercent = static_cast<uint8_t>(stability);
    }

    if (dashboardState.warningCode == WarningCodeSensorMissing)
    {
        dashboardState.vacuumState = VacuumStateWarning;
    }
    else if (dashboardState.suctionEnabled && leakTicks >= kLeakThresholdTicks)
    {
        dashboardState.warningCode = WarningCodeLeak;
        dashboardState.vacuumState = VacuumStateWarning;
    }
    else
    {
        dashboardState.warningCode = WarningCodeNone;

        if (!dashboardState.suctionEnabled)
        {
            dashboardState.vacuumState = VacuumStateReady;
        }
        else if (pressureError <= 10)
        {
            dashboardState.vacuumState = VacuumStateTissueReady;
        }
        else
        {
            dashboardState.vacuumState = VacuumStatePulling;
        }
    }

    if (releaseFeedbackTicks > 0)
    {
        dashboardState.procedureStep = ProcedureStepVerify;
    }
    else if (!dashboardState.suctionEnabled)
    {
        dashboardState.procedureStep = ProcedureStepLoadRing;
    }
    else if (currentPressure < (targetPressure / 3))
    {
        dashboardState.procedureStep = ProcedureStepPosition;
    }
    else if (dashboardState.vacuumState == VacuumStateTissueReady)
    {
        dashboardState.procedureStep = ProcedureStepRelease;
    }
    else
    {
        dashboardState.procedureStep = ProcedureStepAspirate;
    }
}

bool Model::hasStateChanged(const DashboardState& previousState) const
{
    return (dashboardState.currentPressureMbar != previousState.currentPressureMbar) ||
           (dashboardState.targetPressureMbar != previousState.targetPressureMbar) ||
           (dashboardState.stabilityPercent != previousState.stabilityPercent) ||
           (dashboardState.ligatureCount != previousState.ligatureCount) ||
           (dashboardState.suctionEnabled != previousState.suctionEnabled) ||
           (dashboardState.procedureStep != previousState.procedureStep) ||
           (dashboardState.vacuumState != previousState.vacuumState) ||
           (dashboardState.warningCode != previousState.warningCode);
}

int32_t Model::clampTargetPressure(int32_t requestedTarget) const
{
    if (requestedTarget < kMinTargetMbar)
    {
        return kMinTargetMbar;
    }

    if (requestedTarget > kMaxTargetMbar)
    {
        return kMaxTargetMbar;
    }

    return requestedTarget;
}
