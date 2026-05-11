#ifndef MODEL_HPP
#define MODEL_HPP

#include <stdint.h>
#include <gui/model/DashboardTypes.hpp>

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    void initializeBandyDemo();
    void startBandyDemo();
    void stopBandyDemo();
    void resumeBandyDemo();
    void endBandyDemo();
    void increaseBandyTarget();
    void decreaseBandyTarget();
    bool isVacuumCycleRunning() const;
    bool isRfidApproved() const;
    bool canOpenBandyScreen() const;
    bool canOpenPauseScreen() const;

    BandyState getBandyState() const
    {
        return bandyState;
    }

private:
    void notifyBandyState();
    void updateBridgeSnapshot();
    void updateBandyFromInput();
    void updateBandyDerivedState();
    void updateSimulatedVacuum();
    void requestBandyTarget(int32_t targetMbar);
    int32_t clampTarget(int32_t requestedTarget) const;
    int32_t clampVacuum(int32_t measuredVacuum) const;
    bool hasBandyStateChanged(const BandyState& previousState) const;

    ModelListener* modelListener;
    BandyState bandyState;
    uint8_t tickDivider;
    bool bandyInitialized;
    bool bridgeSnapshotValid;
    uint8_t bridgeVacuumState;
    uint8_t bridgeFault;
    uint8_t bridgeRfidApproved;
    uint8_t bridgeBandyState;
    uint16_t bridgeDurationMinutes;
    uint16_t bridgeRemainingSeconds;
    uint16_t bridgePauseRemainingSeconds;
    int32_t bridgeTargetMbar;
    int32_t bridgePressureMbar;
    bool targetCommandPending;
    int32_t pendingTargetMbar;
};

#endif // MODEL_HPP
