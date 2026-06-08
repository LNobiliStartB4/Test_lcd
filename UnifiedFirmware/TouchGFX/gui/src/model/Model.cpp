#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include "bandy_session_store.h"
#include "product_runtime_adapter.h"
#include <string.h>
#ifndef WIN32
#include "display_backlight.h"
#endif

namespace
{
const int32_t kDefaultTargetMbar = 490;
const int32_t kDefaultHemorflowTargetMbar = 150;
const int32_t kMinTargetMbar = 290;
const int32_t kMaxTargetMbar = 490;
const int32_t kMinVacuumMbar = 0;
const int32_t kMaxVacuumMbar = 500;
const int32_t kTargetStepMbar = 10;
const uint8_t kTickDividerLimit = 6; // 60 Hz / 6 = 10 Hz UI updates
const uint8_t kMinBrightnessPercent = 10U;
const uint8_t kMaxBrightnessPercent = 100U;

void copyAdminString(char* destination, uint32_t destinationSize, const char* source)
{
    if ((destination == 0) || (destinationSize == 0U))
    {
        return;
    }

    destination[0] = 0;
    if (source == 0)
    {
        return;
    }

    uint32_t index = 0U;
    while ((source[index] != 0) && (index + 1U < destinationSize))
    {
        destination[index] = source[index];
        ++index;
    }
    destination[index] = 0;
}
}

Model::Model()
    : modelListener(0),
      bandyState(),
      hemorflowState(),
      uiLanguage(UiLanguageEnglish),
      displayBrightnessPercent(kMaxBrightnessPercent),
      tickDivider(0),
      bandyInitialized(false),
      hemorflowInitialized(false),
      bridgeSnapshotValid(false),
      bridgeVacuumState(0),
      bridgeActiveProduct(static_cast<uint8_t>(ActiveProductNone)),
      bridgeFault(0),
      bridgeRfidApproved(0),
      bridgeBandyState(0),
      bridgeDurationMinutes(15),
      bridgeRemainingSeconds(0),
      bridgePauseRemainingSeconds(0),
      bridgePausesUsed(0),
      bridgePausesMax(3),
      bridgeTargetMbar(kDefaultTargetMbar),
      bridgePressureMbar(0),
      targetCommandPending(false),
      pendingTargetMbar(kDefaultTargetMbar),
      adminAccess(),
      adminDiagnostics(),
      adminUptimeTicks100ms(0U)
{
    adminDiagnostics.framAvailable = true;
    adminDiagnostics.framSizeBytes = 32768U;
    adminDiagnostics.winbondAvailable = true;
    adminDiagnostics.winbondSizeBytes = 0x01000000UL;
}

void Model::setDisplayBrightnessPercent(uint8_t percent)
{
    if (percent < kMinBrightnessPercent)
    {
        percent = kMinBrightnessPercent;
    }
    else if (percent > kMaxBrightnessPercent)
    {
        percent = kMaxBrightnessPercent;
    }

    displayBrightnessPercent = percent;

#ifndef WIN32
    DisplayBacklight_SetPercent(displayBrightnessPercent);
#endif
}

void Model::tick()
{
    if (++tickDivider < kTickDividerLimit)
    {
        return;
    }

    tickDivider = 0;
    adminAccess.tick100ms();
    ++adminUptimeTicks100ms;
    updateBridgeSnapshot();
    updateAdminDiagnostics();

    if (hemorflowInitialized)
    {
        updateHemorflowFromInput();
    }

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

    ProductRuntimeAdapter_StartBandy();
}

void Model::stopBandyDemo()
{
    ProductRuntimeAdapter_PauseBandy();
}

void Model::resumeBandyDemo()
{
    ProductRuntimeAdapter_ResumeBandy();
}

void Model::endBandyDemo()
{
    ProductRuntimeAdapter_EndBandy();
}

void Model::startRfidScan()
{
    ProductRuntimeAdapter_StartRfidScan();
}

void Model::stopRfidScan()
{
    ProductRuntimeAdapter_StopRfidScan();
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

void Model::initializeHemorflowMonitor()
{
    hemorflowState = HemorflowState();
    hemorflowState.targetMbar = kDefaultHemorflowTargetMbar;
    hemorflowInitialized = true;
    tickDivider = 0;
    ProductRuntimeAdapter_StartHemorflow();
    updateBridgeSnapshot();
    updateHemorflowFromInput();
}

bool Model::canOpenHemorflowMonitor() const
{
    return bridgeSnapshotValid &&
           (bridgeActiveProduct == static_cast<uint8_t>(ActiveProductHemorflow)) &&
           (bridgeVacuumState != 0U);
}

bool Model::shouldReturnToHemorflowWait() const
{
    return hemorflowInitialized &&
           bridgeSnapshotValid &&
           (bridgeActiveProduct != static_cast<uint8_t>(ActiveProductHemorflow));
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
    product_runtime_snapshot_t snapshot;

    if (!ProductRuntimeAdapter_GetSnapshot(&snapshot))
    {
        return;
    }

    bridgeSnapshotValid = snapshot.valid;
    bridgeVacuumState = snapshot.vacuumState;
    bridgeActiveProduct = snapshot.activeProduct;
    bridgeFault = snapshot.fault;
    bridgeRfidApproved = snapshot.rfidApproved;
    bridgeBandyState = snapshot.bandyState;
    bridgeDurationMinutes = snapshot.durationMinutes;
    bridgeRemainingSeconds = snapshot.remainingSeconds;
    bridgePauseRemainingSeconds = snapshot.pauseRemainingSeconds;
    bridgePausesUsed = snapshot.pausesUsed;
    bridgePausesMax = snapshot.pausesMax;
    bridgeTargetMbar = snapshot.targetMbar;
    bridgePressureMbar = snapshot.pressureMbar;

    bandy_session_store_snapshot_t storeSnapshot;
    storeSnapshot.valid = snapshot.valid;
    storeSnapshot.bandy_state = snapshot.bandyState;
    storeSnapshot.remaining_seconds = snapshot.remainingSeconds;
    storeSnapshot.duration_minutes = snapshot.durationMinutes;
    storeSnapshot.pause_remaining_seconds = snapshot.pauseRemainingSeconds;
    storeSnapshot.pauses_used = snapshot.pausesUsed;
    storeSnapshot.pauses_max = snapshot.pausesMax;
    storeSnapshot.target_mbar = snapshot.targetMbar;
    (void)BandySessionStore_ProcessSnapshot(&storeSnapshot);

    if (targetCommandPending && (clampTarget(bridgeTargetMbar) == pendingTargetMbar))
    {
        targetCommandPending = false;
    }
}

void Model::updateBandyFromInput()
{
    if (!bridgeSnapshotValid)
    {
        return;
    }

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

    bandyState.targetReached =
        (bandyState.currentVacuumMbar >= (bandyState.targetVacuumMbar - 20)) &&
        (bandyState.currentVacuumMbar <= (bandyState.targetVacuumMbar + 20));
}

void Model::updateHemorflowFromInput()
{
    if (!bridgeSnapshotValid)
    {
        return;
    }

    hemorflowState.currentPressureMbar = clampVacuum(bridgePressureMbar);
    hemorflowState.targetMbar = bridgeTargetMbar;
    hemorflowState.running =
        (bridgeActiveProduct == static_cast<uint8_t>(ActiveProductHemorflow)) &&
        (bridgeVacuumState != 0U);
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

    if (!ProductRuntimeAdapter_SetBandyTarget(requestedTargetMbar))
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

void Model::updateAdminDiagnostics()
{
    product_runtime_admin_snapshot_t snapshot = {};
    if (!ProductRuntimeAdapter_GetAdminDiagnostics(&snapshot, false))
    {
        return;
    }

    adminDiagnostics.uptimeSeconds = adminUptimeTicks100ms / 10U;
    adminDiagnostics.language = uiLanguage;
    adminDiagnostics.brightnessPercent = displayBrightnessPercent;
    copyAdminString(adminDiagnostics.deviceName, sizeof(adminDiagnostics.deviceName), snapshot.deviceName);
    copyAdminString(adminDiagnostics.firmwareVersion, sizeof(adminDiagnostics.firmwareVersion), snapshot.firmwareVersion);
    adminDiagnostics.pressureAvailable = snapshot.pressureAvailable;
    adminDiagnostics.pressureDetailsAvailable = snapshot.pressureDetailsAvailable;
    adminDiagnostics.pressureValid = snapshot.pressureValid;
    adminDiagnostics.relativePressureMbar = snapshot.relativePressureMbar;
    adminDiagnostics.rawRelativePressureMbar = snapshot.rawRelativePressureMbar;
    adminDiagnostics.zeroOffsetMbar = snapshot.zeroOffsetMbar;
    adminDiagnostics.ambientRawAdc = snapshot.ambientRawAdc;
    adminDiagnostics.chamberRawAdc = snapshot.chamberRawAdc;
    adminDiagnostics.ambientAbsMbar = snapshot.ambientAbsMbar;
    adminDiagnostics.chamberAbsMbar = snapshot.chamberAbsMbar;
    adminDiagnostics.targetMbar = snapshot.targetMbar;
    adminDiagnostics.pumpDutyPercent = snapshot.pumpDutyPercent;
    adminDiagnostics.pressureState = snapshot.pressureState;
    adminDiagnostics.pressureFault = snapshot.pressureFault;
}

void Model::refreshAdminMemoryDiagnostics()
{
    product_runtime_admin_snapshot_t snapshot = {};
    if (!ProductRuntimeAdapter_GetAdminDiagnostics(&snapshot, true))
    {
        return;
    }

    adminDiagnostics.framAvailable = snapshot.framAvailable;
    adminDiagnostics.framPresent = snapshot.framPresent;
    adminDiagnostics.framSizeBytes = snapshot.framSizeBytes;
    adminDiagnostics.sessionRecordValid = snapshot.sessionRecordValid;
    copyAdminString(adminDiagnostics.framId, sizeof(adminDiagnostics.framId), snapshot.framId);
    adminDiagnostics.winbondAvailable = snapshot.winbondAvailable;
    adminDiagnostics.winbondPresent = snapshot.winbondPresent;
    adminDiagnostics.winbondSizeBytes = snapshot.winbondSizeBytes;
    adminDiagnostics.assetPackageValid = snapshot.assetPackageValid;
    copyAdminString(adminDiagnostics.winbondId, sizeof(adminDiagnostics.winbondId), snapshot.winbondId);
}
