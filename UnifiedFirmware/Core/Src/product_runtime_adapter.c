#include "product_runtime_adapter.h"

#include "application_context.h"
#include "application_runtime.h"
#include "bandy_session_store.h"
#include "fram_mb85rs256b.h"
#include "pressure_sensor.h"
#include "w25q128jv.h"

#include <string.h>

static product_runtime_admin_snapshot_t adminMemoryCache;

static void ProductRuntimeAdapter_CopyTrimmedAscii(char *destination,
                                                   size_t destinationSize,
                                                   const char *source)
{
  size_t index = 0U;

  if ((destination == NULL) || (destinationSize == 0U))
  {
    return;
  }

  destination[0] = '\0';
  if (source == NULL)
  {
    return;
  }

  while ((source[index] != '\0') &&
         (source[index] != '\r') &&
         (source[index] != '\n') &&
         (index + 1U < destinationSize))
  {
    destination[index] = source[index];
    ++index;
  }
  destination[index] = '\0';
}

bool ProductRuntimeAdapter_GetSnapshot(product_runtime_snapshot_t *snapshot)
{
  application_bandy_state_t bandyState;

  if (snapshot == NULL)
  {
    return false;
  }

  memset(snapshot, 0, sizeof(*snapshot));
  bandyState = ApplicationRuntime_GetBandyState();

  snapshot->valid = true;
  snapshot->vacuumState = ApplicationRuntime_GetVacuumCycleState();
  snapshot->activeProduct = (uint8_t)ApplicationRuntime_GetActiveProduct();
  snapshot->fault = ApplicationRuntime_GetVacuumCycleFault();
  snapshot->rfidApproved =
      (bandyState == APPLICATION_BANDY_STATE_AUTHORIZED) ||
      (bandyState == APPLICATION_BANDY_STATE_RUNNING) ||
      (bandyState == APPLICATION_BANDY_STATE_PAUSED);
  snapshot->bandyState = (uint8_t)bandyState;
  snapshot->durationMinutes = ApplicationRuntime_GetBandyDurationMinutes();
  snapshot->remainingSeconds = ApplicationRuntime_GetBandyRemainingSeconds();
  snapshot->pauseRemainingSeconds = ApplicationRuntime_GetBandyPauseRemainingSeconds();
  snapshot->pausesUsed = ApplicationRuntime_GetBandyPauseCount();
  snapshot->pausesMax = ApplicationRuntime_GetBandyMaxPauseCount();
  snapshot->targetMbar = ApplicationRuntime_GetVacuumCycleTargetRelativeMbar();
  snapshot->pressureMbar = PressureSensor_GetRelativeMbar();
  return true;
}

bool ProductRuntimeAdapter_GetAdminDiagnostics(product_runtime_admin_snapshot_t *snapshot,
                                               bool refreshMemory)
{
  bandy_session_store_record_t record;
  bandy_session_store_status_t framStatus;

  if (snapshot == NULL)
  {
    return false;
  }

  if (refreshMemory)
  {
    memset(&record, 0, sizeof(record));
    framStatus = BandySessionStore_Read(&record);

    adminMemoryCache.framAvailable = true;
    adminMemoryCache.framPresent =
        (framStatus == BANDY_SESSION_STORE_OK) ||
        (framStatus == BANDY_SESSION_STORE_EMPTY) ||
        (framStatus == BANDY_SESSION_STORE_INVALID);
    adminMemoryCache.framSizeBytes = FRAM_MB85RS256B_SIZE_BYTES;
    adminMemoryCache.sessionRecordValid =
        (framStatus == BANDY_SESSION_STORE_OK) && record.valid;
    ProductRuntimeAdapter_CopyTrimmedAscii(
        adminMemoryCache.framId,
        sizeof(adminMemoryCache.framId),
        adminMemoryCache.framPresent ? "MB85RS256B" : "N/A");

    adminMemoryCache.winbondAvailable = true;
    adminMemoryCache.winbondPresent = W25Q128JV_IsReady();
    adminMemoryCache.winbondSizeBytes = W25Q128JV_SIZE_BYTES;
    adminMemoryCache.assetPackageValid =
        adminMemoryCache.winbondPresent && W25Q128JV_ValidateAssetPackage();
    ProductRuntimeAdapter_CopyTrimmedAscii(
        adminMemoryCache.winbondId,
        sizeof(adminMemoryCache.winbondId),
        adminMemoryCache.winbondPresent ? "EF 40 18" : "N/A");
  }

  memset(snapshot, 0, sizeof(*snapshot));
  ProductRuntimeAdapter_CopyTrimmedAscii(snapshot->deviceName,
                                         sizeof(snapshot->deviceName),
                                         ApplicationContext_GetDeviceString());
  ProductRuntimeAdapter_CopyTrimmedAscii(snapshot->firmwareVersion,
                                         sizeof(snapshot->firmwareVersion),
                                         ApplicationContext_GetFwVersionString());

  snapshot->pressureAvailable = true;
  snapshot->pressureDetailsAvailable = true;
  snapshot->pressureValid = PressureSensor_IsValid();
  snapshot->relativePressureMbar = PressureSensor_GetRelativeMbar();
  snapshot->rawRelativePressureMbar = PressureSensor_GetRawRelativeMbar();
  snapshot->zeroOffsetMbar = PressureSensor_GetZeroOffsetMbar();
  snapshot->ambientRawAdc = PressureSensor_GetAmbientRawAdc();
  snapshot->chamberRawAdc = PressureSensor_GetChamberRawAdc();
  snapshot->ambientAbsMbar = PressureSensor_GetAmbientAbsMbar();
  snapshot->chamberAbsMbar = PressureSensor_GetChamberAbsMbar();
  snapshot->targetMbar = ApplicationRuntime_GetVacuumCycleTargetRelativeMbar();
  snapshot->pumpDutyPercent = ApplicationRuntime_GetVacuumCycleCommandDutyPercent();
  snapshot->pressureState = ApplicationRuntime_GetVacuumCycleState();
  snapshot->pressureFault = ApplicationRuntime_GetVacuumCycleFault();

  snapshot->framAvailable = adminMemoryCache.framAvailable;
  snapshot->framPresent = adminMemoryCache.framPresent;
  snapshot->framSizeBytes = adminMemoryCache.framSizeBytes;
  snapshot->sessionRecordValid = adminMemoryCache.sessionRecordValid;
  memcpy(snapshot->framId, adminMemoryCache.framId, sizeof(snapshot->framId));
  snapshot->winbondAvailable = adminMemoryCache.winbondAvailable;
  snapshot->winbondPresent = adminMemoryCache.winbondPresent;
  snapshot->winbondSizeBytes = adminMemoryCache.winbondSizeBytes;
  snapshot->assetPackageValid = adminMemoryCache.assetPackageValid;
  memcpy(snapshot->winbondId, adminMemoryCache.winbondId, sizeof(snapshot->winbondId));
  return true;
}

void ProductRuntimeAdapter_StartRfidScan(void)
{
  ApplicationRuntime_StartRfidScan();
}

void ProductRuntimeAdapter_StopRfidScan(void)
{
  ApplicationRuntime_StopRfidScan();
}

void ProductRuntimeAdapter_StartBandy(void)
{
  ApplicationRuntime_StartVacuumCycle();
}

void ProductRuntimeAdapter_PauseBandy(void)
{
  ApplicationRuntime_PauseVacuumCycle();
}

void ProductRuntimeAdapter_ResumeBandy(void)
{
  ApplicationRuntime_ResumeVacuumCycle();
}

void ProductRuntimeAdapter_EndBandy(void)
{
  ApplicationRuntime_EndVacuumSession();
}

void ProductRuntimeAdapter_StartHemorflow(void)
{
  ApplicationRuntime_StartHemorflowCycle();
}

bool ProductRuntimeAdapter_SetBandyTarget(int32_t targetMbar)
{
  return ApplicationRuntime_SetBandyTargetRelativeMbar(targetMbar);
}
