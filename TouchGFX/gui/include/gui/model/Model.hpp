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

    void initializeDashboard();
    void setSuctionEnabled(bool enabled);
    void adjustTargetPressure(int32_t deltaMbar);
    void markBandReleased();
    void resetProcedure();

    DashboardState getDashboardState() const
    {
        return dashboardState;
    }

private:
    void notifyDashboardState();
    void updateSimulationStep();
    void updateDerivedState();
    bool hasStateChanged(const DashboardState& previousState) const;
    int32_t clampTargetPressure(int32_t requestedTarget) const;

    ModelListener* modelListener;
    DashboardState dashboardState;
    uint8_t tickDivider;
    uint8_t waveIndex;
    uint8_t leakTicks;
    uint8_t releaseFeedbackTicks;
    bool dashboardInitialized;
};

#endif // MODEL_HPP
