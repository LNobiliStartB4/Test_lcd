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
    void increaseBandyTarget();
    void decreaseBandyTarget();
    bool isVacuumCycleRunning() const;

    BandyState getBandyState() const
    {
        return bandyState;
    }

private:
    void notifyBandyState();
    void updateBridgeSnapshot();
    void updateBandyFromInput();
    void updateBandyDerivedState();
    int32_t clampTarget(int32_t requestedTarget) const;
    bool hasBandyStateChanged(const BandyState& previousState) const;

    ModelListener* modelListener;
    BandyState bandyState;
    uint8_t tickDivider;
    bool bandyInitialized;
    bool bridgeSnapshotValid;
    uint8_t bridgeVacuumState;
    uint8_t bridgeFault;
    int32_t bridgePressureMbar;
};

#endif // MODEL_HPP
