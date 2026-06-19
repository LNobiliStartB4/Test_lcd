#include "display_bridge_rx.h"

#include <cstring>

namespace
{
display_bridge_snapshot_t g_snapshot = {};
int g_sendVacuumStart = 0;
int g_sendVacuumPause = 0;
int g_sendVacuumResume = 0;
int g_sendVacuumEnd = 0;
int g_sendBandyTarget = 0;
int32_t g_lastBandyTarget = 0;
int g_sendRfidScanStart = 0;
int g_sendRfidScanStop = 0;
}

extern "C" {

bool DisplayBridgeRx_GetLatestSnapshot(display_bridge_snapshot_t *snapshot)
{
    if (snapshot == nullptr)
    {
        return false;
    }
    std::memcpy(snapshot, &g_snapshot, sizeof(*snapshot));
    return g_snapshot.valid;
}

bool DisplayBridgeRx_GetLatestPressureMbar(int32_t *pressureMbar)
{
    if ((pressureMbar == nullptr) || !g_snapshot.valid)
    {
        return false;
    }
    *pressureMbar = g_snapshot.pressureMbar;
    return true;
}

bool DisplayBridgeRx_SendVacuumStartCommand(void)        { g_sendVacuumStart++; return true; }
bool DisplayBridgeRx_SendVacuumPauseCommand(void)        { g_sendVacuumPause++; return true; }
bool DisplayBridgeRx_SendVacuumResumeCommand(void)       { g_sendVacuumResume++; return true; }
bool DisplayBridgeRx_SendVacuumEndCommand(void)          { g_sendVacuumEnd++; return true; }
bool DisplayBridgeRx_SendVacuumStopCommand(void)         { return true; }
bool DisplayBridgeRx_SendBandyTargetCommand(int32_t t)   { g_sendBandyTarget++; g_lastBandyTarget = t; return true; }
bool DisplayBridgeRx_SendRfidScanStartCommand(void)      { g_sendRfidScanStart++; return true; }
bool DisplayBridgeRx_SendRfidScanStopCommand(void)       { g_sendRfidScanStop++; return true; }

void TestStub_Reset(void)
{
    std::memset(&g_snapshot, 0, sizeof(g_snapshot));
    g_sendVacuumStart = 0;
    g_sendVacuumPause = 0;
    g_sendVacuumResume = 0;
    g_sendVacuumEnd = 0;
    g_sendBandyTarget = 0;
    g_lastBandyTarget = 0;
    g_sendRfidScanStart = 0;
    g_sendRfidScanStop = 0;
}

void TestStub_SetSnapshot(const display_bridge_snapshot_t *snapshot)
{
    if (snapshot != nullptr)
    {
        g_snapshot = *snapshot;
    }
}

int TestStub_GetSendCount_VacuumStart(void)        { return g_sendVacuumStart; }
int TestStub_GetSendCount_VacuumPause(void)        { return g_sendVacuumPause; }
int TestStub_GetSendCount_VacuumResume(void)       { return g_sendVacuumResume; }
int TestStub_GetSendCount_VacuumEnd(void)          { return g_sendVacuumEnd; }
int TestStub_GetSendCount_BandyTarget(void)        { return g_sendBandyTarget; }
int32_t TestStub_GetLastBandyTarget(void)          { return g_lastBandyTarget; }
int TestStub_GetSendCount_RfidScanStart(void)      { return g_sendRfidScanStart; }
int TestStub_GetSendCount_RfidScanStop(void)       { return g_sendRfidScanStop; }

}
