#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include "display_bridge_rx.h"

namespace
{
const int32_t kDefaultTargetMbar = 450;
const int32_t kMinTargetMbar = 0;
const int32_t kMaxTargetMbar = 500;
const int32_t kTargetStepMbar = 10;
const uint8_t kTickDividerLimit = 6; // 60 Hz / 6 = 10 Hz UI updates
const uint8_t kCountdownDividerLimit = 10; // 10 Hz / 10 = 1 Hz countdown
const uint16_t kCountdownStartSeconds = 60;
const int32_t kSimulationStepMbar = 8;
}

Model::Model()
    : modelListener(0),
      bandyState(),
      tickDivider(0),
      countdownDivider(0),
      bandyInitialized(false),
      countdownActive(false),
      bridgeSnapshotValid(false),
      bridgeVacuumState(0),
      bridgeFault(0),
      bridgeRfidApproved(0),
      bridgePressureMbar(0)
{
}

void Model::tick()
{
    if (++tickDivider < kTickDividerLimit)
    {
        return;
    }

    tickDivider = 0;
    updateBridgeSnapshot();

    if (!bandyInitialized)
    {
        return;
    }

    const BandyState previousState = bandyState;
    updateCountdown();
    updateBandyFromInput();
    updateBandyDerivedState();

    if (hasBandyStateChanged(previousState))
    {
        notifyBandyState();
    }
}

void Model::initializeBandyDemo()
{
    if (bandyInitialized)
    {
        return;
    }

    bandyState = BandyState();
    bandyState.targetVacuumMbar = kDefaultTargetMbar;
    bandyInitialized = true;
    tickDivider = 0;
    countdownDivider = 0;
    countdownActive = false;

    updateBandyDerivedState();
    notifyBandyState();
}

void Model::startBandyDemo()
{
    if (!bandyInitialized)
    {
        initializeBandyDemo();
    }

    if (isVacuumCycleRunning())
    {
        return;
    }

    const BandyState previousState = bandyState;
    bandyState.remainingSeconds = kCountdownStartSeconds;
    bandyState.running = true;
    bandyState.targetReached = false;
    countdownDivider = 0;
    countdownActive = true;

    (void)DisplayBridgeRx_SendVacuumStartCommand();

    updateBandyDerivedState();
    if (hasBandyStateChanged(previousState))
    {
        notifyBandyState();
    }
}

void Model::stopBandyDemo()
{
    if (!isVacuumCycleRunning() && !countdownActive && (bandyState.remainingSeconds == 0U))
    {
        return;
    }

    const BandyState previousState = bandyState;
    countdownActive = false;
    countdownDivider = 0;
    bandyState.remainingSeconds = 0;
    bandyState.running = false;
    bandyState.targetReached = false;

    (void)DisplayBridgeRx_SendVacuumStopCommand();

    updateBandyDerivedState();
    if (hasBandyStateChanged(previousState))
    {
        notifyBandyState();
    }
}

void Model::increaseBandyTarget()
{
    if (!bandyInitialized)
    {
        initializeBandyDemo();
    }

    const BandyState previousState = bandyState;
    bandyState.targetVacuumMbar = clampTarget(bandyState.targetVacuumMbar + kTargetStepMbar);
    if (bandyState.currentVacuumMbar < bandyState.targetVacuumMbar)
    {
        bandyState.targetReached = false;
    }

    updateBandyDerivedState();
    if (hasBandyStateChanged(previousState))
    {
        notifyBandyState();
    }
}

void Model::decreaseBandyTarget()
{
    if (!bandyInitialized)
    {
        initializeBandyDemo();
    }

    const BandyState previousState = bandyState;
    bandyState.targetVacuumMbar = clampTarget(bandyState.targetVacuumMbar - kTargetStepMbar);
    if (bandyState.currentVacuumMbar > bandyState.targetVacuumMbar)
    {
        bandyState.targetReached = false;
    }

    updateBandyDerivedState();
    if (hasBandyStateChanged(previousState))
    {
        notifyBandyState();
    }
}

bool Model::isVacuumCycleRunning() const
{
    return countdownActive || bandyState.running || (bridgeSnapshotValid && (bridgeVacuumState == 1U));
}

bool Model::isRfidApproved() const
{
  return bridgeSnapshotValid && (bridgeRfidApproved == 1U);
}

void Model::notifyBandyState()
{
    if (modelListener != 0)
    {
        modelListener->bandyStateUpdated(bandyState);
    }
}

void Model::updateBridgeSnapshot()
{
    display_bridge_snapshot_t snapshot;

    if (!DisplayBridgeRx_GetLatestSnapshot(&snapshot))
    {
        return;
    }

  bridgeSnapshotValid = snapshot.valid;
  bridgeVacuumState = snapshot.vacuumState;
  bridgeFault = snapshot.fault;
  bridgeRfidApproved = snapshot.rfidApproved;
  bridgePressureMbar = snapshot.pressureMbar;
}

void Model::updateBandyFromInput()
{
    if (bridgeSnapshotValid)
    {
        bandyState.currentVacuumMbar = clampTarget(bridgePressureMbar);
    }
    else
    {
        updateSimulatedVacuum();
    }

    bandyState.running = countdownActive || (bridgeSnapshotValid && (bridgeVacuumState == 1U));
    bandyState.targetReached =
        (bandyState.currentVacuumMbar >= (bandyState.targetVacuumMbar - 20)) &&
        (bandyState.currentVacuumMbar <= (bandyState.targetVacuumMbar + 20));
}

void Model::updateBandyDerivedState()
{
    if (!bandyState.running)
    {
        bandyState.vacuumState = BandyVacuumStateReady;
        return;
    }

    if (bandyState.targetReached)
    {
        bandyState.vacuumState = BandyVacuumStateTarget;
    }
    else
    {
        bandyState.vacuumState = BandyVacuumStatePulling;
    }
}

int32_t Model::clampTarget(int32_t requestedTarget) const
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

void Model::updateCountdown()
{
    if (!countdownActive)
    {
        return;
    }

    if (++countdownDivider < kCountdownDividerLimit)
    {
        return;
    }

    countdownDivider = 0;

    if (bandyState.remainingSeconds > 0U)
    {
        bandyState.remainingSeconds--;
    }

    if (bandyState.remainingSeconds == 0U)
    {
        countdownActive = false;
        bandyState.running = false;
        (void)DisplayBridgeRx_SendVacuumStopCommand();
    }
}

void Model::updateSimulatedVacuum()
{
    if (!countdownActive)
    {
        return;
    }

    if (bandyState.currentVacuumMbar < bandyState.targetVacuumMbar)
    {
        bandyState.currentVacuumMbar += kSimulationStepMbar;
        if (bandyState.currentVacuumMbar > bandyState.targetVacuumMbar)
        {
            bandyState.currentVacuumMbar = bandyState.targetVacuumMbar;
        }
    }
    else if (bandyState.currentVacuumMbar > bandyState.targetVacuumMbar)
    {
        bandyState.currentVacuumMbar -= kSimulationStepMbar;
        if (bandyState.currentVacuumMbar < bandyState.targetVacuumMbar)
        {
            bandyState.currentVacuumMbar = bandyState.targetVacuumMbar;
        }
    }
}

bool Model::hasBandyStateChanged(const BandyState& previousState) const
{
    return previousState.currentVacuumMbar != bandyState.currentVacuumMbar ||
           previousState.targetVacuumMbar != bandyState.targetVacuumMbar ||
           previousState.remainingSeconds != bandyState.remainingSeconds ||
           previousState.running != bandyState.running ||
           previousState.targetReached != bandyState.targetReached ||
           previousState.vacuumState != bandyState.vacuumState;
}
