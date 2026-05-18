#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

#include "display_bridge_rx.h"

namespace
{
display_bridge_snapshot_t makeSnapshot(uint8_t bandyState,
                                       int32_t pressureMbar = 0,
                                       int32_t targetMbar = 490)
{
    display_bridge_snapshot_t s = {};
    s.valid = true;
    s.bandyState = bandyState;
    s.durationMinutes = 15;
    s.remainingSeconds = 900;
    s.pauseRemainingSeconds = 0;
    s.pressureMbar = pressureMbar;
    s.targetMbar = targetMbar;
    return s;
}

/* Tick divider in Model is 6 — call tick() at least 6 times to push a
 * snapshot update through to the Model state. */
void pumpModel(Model &m, int times = 8)
{
    for (int i = 0; i < times; ++i)
    {
        m.tick();
    }
}
}

display_bridge_snapshot_t makeHemorflowSnapshot(uint8_t activeProduct,
                                                uint8_t vacuumState,
                                                int32_t pressureMbar,
                                                int32_t targetMbar)
{
    display_bridge_snapshot_t s = {};
    s.valid = true;
    s.activeProduct = activeProduct;
    s.vacuumState = vacuumState;
    s.pressureMbar = pressureMbar;
    s.targetMbar = targetMbar;
    return s;
}

TEST_CASE("Defaults after construction and initializeBandyDemo")
{
    TestStub_Reset();
    Model m;
    m.initializeBandyDemo();

    const BandyState st = m.getBandyState();
    CHECK(st.targetVacuumMbar == 490);
    CHECK(st.remainingSeconds == 0);
    CHECK(st.running == false);
    CHECK(st.sessionState == BandySessionWaitRfid);
}

TEST_CASE("decreaseBandyTarget clamps to MIN 290 mbar")
{
    TestStub_Reset();
    Model m;
    m.initializeBandyDemo();

    for (int i = 0; i < 50; ++i)
    {
        m.decreaseBandyTarget();
    }

    CHECK(m.getBandyState().targetVacuumMbar == 290);
    CHECK(TestStub_GetLastBandyTarget() == 290);
}

TEST_CASE("increaseBandyTarget cannot go above MAX 490 mbar")
{
    TestStub_Reset();
    Model m;
    m.initializeBandyDemo();

    for (int i = 0; i < 5; ++i)
    {
        m.increaseBandyTarget();
    }

    CHECK(m.getBandyState().targetVacuumMbar == 490);
    CHECK(TestStub_GetSendCount_BandyTarget() == 0);
}

TEST_CASE("Lifecycle commands map to bridge sends")
{
    TestStub_Reset();
    Model m;

    m.startBandyDemo();
    CHECK(TestStub_GetSendCount_VacuumStart() == 1);

    m.stopBandyDemo();
    CHECK(TestStub_GetSendCount_VacuumPause() == 1);

    m.resumeBandyDemo();
    CHECK(TestStub_GetSendCount_VacuumResume() == 1);

    m.endBandyDemo();
    CHECK(TestStub_GetSendCount_VacuumEnd() == 1);

    m.startRfidScan();
    CHECK(TestStub_GetSendCount_RfidScanStart() == 1);

    m.stopRfidScan();
    CHECK(TestStub_GetSendCount_RfidScanStop() == 1);
}

TEST_CASE("canOpenBandyScreen only on AUTHORIZED or RUNNING")
{
    TestStub_Reset();
    Model m;
    m.initializeBandyDemo();

    display_bridge_snapshot_t s = makeSnapshot(BandySessionWaitRfid);
    TestStub_SetSnapshot(&s);
    pumpModel(m);
    CHECK_FALSE(m.canOpenBandyScreen());

    s = makeSnapshot(BandySessionAuthorized);
    TestStub_SetSnapshot(&s);
    pumpModel(m);
    CHECK(m.canOpenBandyScreen());

    s = makeSnapshot(BandySessionRunning);
    TestStub_SetSnapshot(&s);
    pumpModel(m);
    CHECK(m.canOpenBandyScreen());

    s = makeSnapshot(BandySessionPaused);
    TestStub_SetSnapshot(&s);
    pumpModel(m);
    CHECK_FALSE(m.canOpenBandyScreen());
}

TEST_CASE("canOpenPauseScreen only on PAUSED")
{
    TestStub_Reset();
    Model m;
    m.initializeBandyDemo();

    display_bridge_snapshot_t s = makeSnapshot(BandySessionPaused);
    TestStub_SetSnapshot(&s);
    pumpModel(m);
    CHECK(m.canOpenPauseScreen());

    s = makeSnapshot(BandySessionRunning);
    TestStub_SetSnapshot(&s);
    pumpModel(m);
    CHECK_FALSE(m.canOpenPauseScreen());
}



TEST_CASE("Hemorflow monitor opens only when active and running")
{
    TestStub_Reset();
    Model m;
    m.initializeHemorflowMonitor();

    display_bridge_snapshot_t s = makeHemorflowSnapshot(0, 0, 0, 150);
    TestStub_SetSnapshot(&s);
    pumpModel(m);
    CHECK_FALSE(m.canOpenHemorflowMonitor());

    s = makeHemorflowSnapshot(2, 1, 82, 150);
    TestStub_SetSnapshot(&s);
    pumpModel(m);
    CHECK(m.canOpenHemorflowMonitor());
}

TEST_CASE("Hemorflow snapshot updates pressure target and running state")
{
    TestStub_Reset();
    Model m;
    m.initializeHemorflowMonitor();

    display_bridge_snapshot_t s = makeHemorflowSnapshot(2, 1, 123, 150);
    TestStub_SetSnapshot(&s);
    pumpModel(m);

    const HemorflowState st = m.getHemorflowState();
    CHECK(st.currentPressureMbar == 123);
    CHECK(st.targetMbar == 150);
    CHECK(st.running == true);
}

TEST_CASE("Hemorflow STOP returns to wait screen")
{
    TestStub_Reset();
    Model m;
    m.initializeHemorflowMonitor();

    display_bridge_snapshot_t s = makeHemorflowSnapshot(2, 1, 100, 150);
    TestStub_SetSnapshot(&s);
    pumpModel(m);
    CHECK_FALSE(m.shouldReturnToHemorflowWait());

    s = makeHemorflowSnapshot(0, 0, 5, 150);
    TestStub_SetSnapshot(&s);
    pumpModel(m);
    CHECK(m.shouldReturnToHemorflowWait());
}
