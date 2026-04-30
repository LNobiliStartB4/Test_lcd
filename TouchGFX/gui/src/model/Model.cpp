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
}

Model::Model()
    : modelListener(0),
      bandyState(),
      tickDivider(0),
      bandyInitialized(false),
      bridgeSnapshotValid(false),
      bridgeVacuumState(0),
      bridgeFault(0),
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

    updateBandyDerivedState();
    notifyBandyState();
}

void Model::startBandyDemo()
{
    if (!bandyInitialized)
    {
        initializeBandyDemo();
    }

    if (bandyState.running)
    {
        return;
    }

    bandyState.running = true;
    bandyState.targetReached = false;
    updateBandyDerivedState();
    notifyBandyState();
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
    return bridgeSnapshotValid && (bridgeVacuumState == 1U);
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
    bridgePressureMbar = snapshot.pressureMbar;
}

void Model::updateBandyFromInput()
{
    if (!bridgeSnapshotValid)
    {
        return;
    }

    bandyState.currentVacuumMbar = clampTarget(bridgePressureMbar);
    bandyState.running = (bridgeVacuumState == 1U);
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

bool Model::hasBandyStateChanged(const BandyState& previousState) const
{
    return previousState.currentVacuumMbar != bandyState.currentVacuumMbar ||
           previousState.targetVacuumMbar != bandyState.targetVacuumMbar ||
           previousState.running != bandyState.running ||
           previousState.targetReached != bandyState.targetReached ||
           previousState.vacuumState != bandyState.vacuumState;
}
