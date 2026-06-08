#ifndef PRODUCT_RUNTIME_ADAPTER_H
#define PRODUCT_RUNTIME_ADAPTER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  bool valid;
  uint8_t vacuumState;
  uint8_t activeProduct;
  uint8_t fault;
  uint8_t rfidApproved;
  uint8_t bandyState;
  uint16_t durationMinutes;
  uint16_t remainingSeconds;
  uint16_t pauseRemainingSeconds;
  uint8_t pausesUsed;
  uint8_t pausesMax;
  int32_t targetMbar;
  int32_t pressureMbar;
} product_runtime_snapshot_t;

typedef struct
{
  char deviceName[24];
  char firmwareVersion[16];
  bool pressureAvailable;
  bool pressureDetailsAvailable;
  bool pressureValid;
  int32_t relativePressureMbar;
  int32_t rawRelativePressureMbar;
  int32_t zeroOffsetMbar;
  uint16_t ambientRawAdc;
  uint16_t chamberRawAdc;
  uint16_t ambientAbsMbar;
  uint16_t chamberAbsMbar;
  int32_t targetMbar;
  uint8_t pumpDutyPercent;
  uint8_t pressureState;
  uint8_t pressureFault;
  bool framAvailable;
  bool framPresent;
  uint32_t framSizeBytes;
  bool sessionRecordValid;
  char framId[16];
  bool winbondAvailable;
  bool winbondPresent;
  uint32_t winbondSizeBytes;
  bool assetPackageValid;
  char winbondId[16];
} product_runtime_admin_snapshot_t;

bool ProductRuntimeAdapter_GetSnapshot(product_runtime_snapshot_t *snapshot);
bool ProductRuntimeAdapter_GetAdminDiagnostics(product_runtime_admin_snapshot_t *snapshot,
                                               bool refreshMemory);
void ProductRuntimeAdapter_StartRfidScan(void);
void ProductRuntimeAdapter_StopRfidScan(void);
void ProductRuntimeAdapter_StartBandy(void);
void ProductRuntimeAdapter_PauseBandy(void);
void ProductRuntimeAdapter_ResumeBandy(void);
void ProductRuntimeAdapter_EndBandy(void);
void ProductRuntimeAdapter_StartHemorflow(void);
bool ProductRuntimeAdapter_SetBandyTarget(int32_t targetMbar);

#ifdef __cplusplus
}
#endif

#endif /* PRODUCT_RUNTIME_ADAPTER_H */
