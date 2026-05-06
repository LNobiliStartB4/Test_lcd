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
    void increaseBandyTarget();
    void decreaseBandyTarget();
    bool isVacuumCycleRunning() const;
    bool isRfidApproved() const;

    BandyState getBandyState() const
    {
        return bandyState;
    }

private:
    void notifyBandyState();
    void updateBridgeSnapshot();
    void updateBandyFromInput();
    void updateBandyDerivedState();
    void updateCountdown();
    void updateSimulatedVacuum();
    int32_t clampTarget(int32_t requestedTarget) const;
    bool hasBandyStateChanged(const BandyState& previousState) const;

    ModelListener* modelListener;
    BandyState bandyState;
    uint8_t tickDivider;
    uint8_t countdownDivider;
    bool bandyInitialized;
    bool countdownActive;
    bool bridgeSnapshotValid;
    uint8_t bridgeVacuumState;
    uint8_t bridgeFault;
    uint8_t bridgeRfidApproved;
    int32_t bridgePressureMbar;
};

#endif // MODEL_HPP
