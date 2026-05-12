/* Host-only stub of display_bridge_rx.h for unit tests.
 * Mirrors the real header API but omits HAL types.
 */
#ifndef DISPLAY_BRIDGE_RX_H
#define DISPLAY_BRIDGE_RX_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  uint8_t vacuumState;
  uint8_t fault;
  uint8_t rfidApproved;
  uint8_t bandyState;
  uint16_t durationMinutes;
  uint16_t remainingSeconds;
  uint16_t pauseRemainingSeconds;
  int32_t targetMbar;
  int32_t pressureMbar;
  bool valid;
} display_bridge_snapshot_t;

bool DisplayBridgeRx_GetLatestSnapshot(display_bridge_snapshot_t *snapshot);
bool DisplayBridgeRx_GetLatestPressureMbar(int32_t *pressureMbar);
bool DisplayBridgeRx_SendVacuumStartCommand(void);
bool DisplayBridgeRx_SendVacuumPauseCommand(void);
bool DisplayBridgeRx_SendVacuumResumeCommand(void);
bool DisplayBridgeRx_SendVacuumEndCommand(void);
bool DisplayBridgeRx_SendVacuumStopCommand(void);
bool DisplayBridgeRx_SendBandyTargetCommand(int32_t targetMbar);
bool DisplayBridgeRx_SendRfidScanStartCommand(void);
bool DisplayBridgeRx_SendRfidScanStopCommand(void);

/* Test-only helpers (not present in production header). */
void TestStub_Reset(void);
void TestStub_SetSnapshot(const display_bridge_snapshot_t *snapshot);
int TestStub_GetSendCount_VacuumStart(void);
int TestStub_GetSendCount_VacuumPause(void);
int TestStub_GetSendCount_VacuumResume(void);
int TestStub_GetSendCount_VacuumEnd(void);
int TestStub_GetSendCount_BandyTarget(void);
int32_t TestStub_GetLastBandyTarget(void);
int TestStub_GetSendCount_RfidScanStart(void);
int TestStub_GetSendCount_RfidScanStop(void);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_BRIDGE_RX_H */
