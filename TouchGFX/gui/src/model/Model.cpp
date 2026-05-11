#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include "display_bridge_rx.h"

namespace
{
const int32_t kDefaultTargetMbar = 490;
const int32_t kMinTargetMbar = 290;
const int32_t kMaxTargetMbar = 490;
const int32_t kMinVacuumMbar = 0;
const int32_t kMaxVacuumMbar = 500;
const int32_t kTargetStepMbar = 10;
const uint8_t kTickDividerLimit = 6; // 60 Hz / 6 = 10 Hz UI updates
const int32_t kSimulationStepMbar = 8;
}

Model::Model()
    : modelListener(0),
      bandyState(),
      tickDivider(0),
      bandyInitialized(false),
      bridgeSnapshotValid(false),
      bridgeVacuumState(0),
      bridgeFault(0),
      bridgeRfidApproved(0),
      bridgeBandyState(0),
      bridgeDurationMinutes(15),
      bridgeRemainingSeconds(0),
      bridgePauseRemainingSeconds(0),
      bridgeTargetMbar(kDefaultTargetMbar),
      bridgePressureMbar(0),
      targetCommandPending(false),
      pendingTargetMbar(kDefaultTargetMbar)
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
    targetCommandPending = false;
    pendingTargetMbar = kDefaultTargetMbar;
    bandyInitialized = true;
    tickDivider = 0;

    updateBandyFromInput();
    updateBandyDerivedState();
    notifyBandyState();
}

void Model::startBandyDemo()
{
    if (!bandyInitialized)
    {
        initializeBandyDemo();
    }

    (void)DisplayBridgeRx_SendVacuumStartCommand();
}

void Model::stopBandyDemo()
{
    (void)DisplayBridgeRx_SendVacuumPauseCommand();
}

void Model::resumeBandyDemo()
{
    (void)DisplayBridgeRx_SendVacuumResumeCommand();
}

void Model::endBandyDemo()
{
    (void)DisplayBridgeRx_SendVacuumEndCommand();
}

void Model::cancelEndBandyDemo()
{
    (void)DisplayBridgeRx_SendVacuumEndCancelCommand();
}

void Model::startRfidScan()
{
    (void)DisplayBridgeRx_SendRfidScanStartCommand();
}

void Model::stopRfidScan()
{
    (void)DisplayBridgeRx_SendRfidScanStopCommand();
}

void Model::increaseBandyTarget()
{
    if (!bandyInitialized)
    {
        initializeBandyDemo();
    }

    requestBandyTarget(bandyState.targetVacuumMbar + kTargetStepMbar);
}

void Model::decreaseBandyTarget()
{
    if (!bandyInitialized)
    {
        initializeBandyDemo();
    }

    requestBandyTarget(bandyState.targetVacuumMbar - kTargetStepMbar);
}

bool Model::isVacuumCycleRunning() const
{
    return bridgeSnapshotValid && (bridgeBandyState == static_cast<uint8_t>(BandySessionRunning));
}

bool Model::isRfidApproved() const
{
    return bridgeSnapshotValid && (bridgeBandyState == static_cast<uint8_t>(BandySessionAuthorized));
}

bool Model::canOpenBandyScreen() const
{
    return bridgeSnapshotValid &&
           ((bridgeBandyState == static_cast<uint8_t>(BandySessionAuthorized)) ||
            (bridgeBandyState == static_cast<uint8_t>(BandySessionRunning)));
}

bool Model::canOpenPauseScreen() const
{
    return bridgeSnapshotValid && (bridgeBandyState == static_cast<uint8_t>(BandySessionPaused));
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
    bridgeBandyState = snapshot.bandyState;
    bridgeDurationMinutes = snapshot.durationMinutes;
    bridgeRemainingSeconds = snapshot.remainingSeconds;
    bridgePauseRemainingSeconds = snapshot.pauseRemainingSeconds;
    bridgeTargetMbar = snapshot.targetMbar;
    bridgePressureMbar = snapshot.pressureMbar;

    if (targetCommandPending && (clampTarget(bridgeTargetMbar) == pendingTargetMbar))
    {
        targetCommandPending = false;
    }
}

void Model::updateBandyFromInput()
{
    if (bridgeSnapshotValid)
    {
        const int32_t receivedTargetMbar = clampTarget(bridgeTargetMbar);

        bandyState.currentVacuumMbar = clampVacuum(bridgePressureMbar);
        bandyState.targetVacuumMbar = targetCommandPending ? pendingTargetMbar : receivedTargetMbar;
        bandyState.remainingSeconds = bridgeRemainingSeconds;
        bandyState.pauseRemainingSeconds = bridgePauseRemainingSeconds;
        bandyState.sessionState = static_cast<BandySessionState>(bridgeBandyState);
        bandyState.running = bridgeBandyState == static_cast<uint8_t>(BandySessionRunning);

        if (bandyState.sessionState == BandySessionWaitRfid)
        {
            targetCommandPending = false;
            pendingTargetMbar = kDefaultTargetMbar;
        }
    }
    else
    {
        updateSimulatedVacuum();
    }

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

void Model::requestBandyTarget(int32_t targetMbar)
{
    if (!bandyInitialized)
    {
        initializeBandyDemo();
    }

    const BandyState previousState = bandyState;
    const int32_t requestedTargetMbar = clampTarget(targetMbar);

    if (requestedTargetMbar == bandyState.targetVacuumMbar)
    {
        return;
    }

    if (!DisplayBridgeRx_SendBandyTargetCommand(requestedTargetMbar))
    {
        return;
    }

    pendingTargetMbar = requestedTargetMbar;
    targetCommandPending = true;
    bandyState.targetVacuumMbar = pendingTargetMbar;

    updateBandyDerivedState();
    if (hasBandyStateChanged(previousState))
    {
        notifyBandyState();
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

int32_t Model::clampVacuum(int32_t measuredVacuum) const
{
    if (measuredVacuum < kMinVacuumMbar)
    {
        return kMinVacuumMbar;
    }

    if (measuredVacuum > kMaxVacuumMbar)
    {
        return kMaxVacuumMbar;
    }

    return measuredVacuum;
}

void Model::updateSimulatedVacuum()
{
    if (!bandyState.running)
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
           previousState.pauseRemainingSeconds != bandyState.pauseRemainingSeconds ||
           previousState.running != bandyState.running ||
           previousState.targetReached != bandyState.targetReached ||
           previousState.vacuumState != bandyState.vacuumState ||
           previousState.sessionState != bandyState.sessionState;
}
