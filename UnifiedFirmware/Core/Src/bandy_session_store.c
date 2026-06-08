#include "bandy_session_store.h"

#include "fram_mb85rs256b.h"
#include "main.h"

#define BANDY_SESSION_STORE_ADDRESS (0x0100u)
#define BANDY_SESSION_STORE_TIMEOUT_MS (50u)

extern SPI_HandleTypeDef hspi2;

static bool storeInitialized;
static bool storeHasActiveRecord;
static uint32_t storeNextSequence = 1u;
static bandy_session_store_record_t storeLastRecord;

static const fram_mb85rs256b_t storeFram =
{
  .spi = &hspi2,
  .cs_port = FRAM_CS_GPIO_Port,
  .cs_pin = FRAM_CS_Pin,
  .timeout_ms = BANDY_SESSION_STORE_TIMEOUT_MS
};

static bool BandySessionStore_IsSpiReady(void)
{
  return HAL_SPI_GetState(&hspi2) == HAL_SPI_STATE_READY;
}

static bandy_session_store_status_t BandySessionStore_MapCoreStatus(
  bandy_session_store_core_status_t status)
{
  switch (status)
  {
    case BANDY_SESSION_STORE_CORE_OK:
      return BANDY_SESSION_STORE_OK;
    case BANDY_SESSION_STORE_CORE_EMPTY:
      return BANDY_SESSION_STORE_EMPTY;
    case BANDY_SESSION_STORE_CORE_INVALID:
      return BANDY_SESSION_STORE_INVALID;
    case BANDY_SESSION_STORE_CORE_ERROR_PARAM:
    default:
      return BANDY_SESSION_STORE_ERROR_PARAM;
  }
}

static bandy_session_store_status_t BandySessionStore_WriteRecord(
  const bandy_session_store_record_t *record)
{
  uint8_t raw[BANDY_SESSION_STORE_RECORD_SIZE] = {0};

  if (BandySessionStoreCore_EncodeRecord(record, raw) != BANDY_SESSION_STORE_CORE_OK)
  {
    return BANDY_SESSION_STORE_ERROR_PARAM;
  }

  if (!BandySessionStore_IsSpiReady())
  {
    return BANDY_SESSION_STORE_ERROR_BUSY;
  }

  if (fram_mb85rs256b_write(&storeFram,
                            BANDY_SESSION_STORE_ADDRESS,
                            raw,
                            sizeof(raw)) != FRAM_MB85RS256B_OK)
  {
    return BANDY_SESSION_STORE_ERROR_IO;
  }

  return BANDY_SESSION_STORE_OK;
}

void BandySessionStore_Init(void)
{
  bandy_session_store_record_t record;
  bandy_session_store_status_t status;

  storeInitialized = true;
  storeHasActiveRecord = false;
  storeNextSequence = 1u;
  BandySessionStoreCore_MakeInvalidRecord(0u, &storeLastRecord);

  status = BandySessionStore_Read(&record);
  if ((status == BANDY_SESSION_STORE_OK) || (status == BANDY_SESSION_STORE_EMPTY))
  {
    storeNextSequence = record.sequence + 1u;
    if (status == BANDY_SESSION_STORE_OK)
    {
      storeLastRecord = record;
      storeHasActiveRecord = true;
    }
  }
}

bandy_session_store_status_t BandySessionStore_ProcessSnapshot(
  const bandy_session_store_snapshot_t *snapshot)
{
  bandy_session_store_action_t action;
  bandy_session_store_record_t record;
  bandy_session_store_status_t status;

  if (snapshot == NULL)
  {
    return BANDY_SESSION_STORE_ERROR_PARAM;
  }

  if (!storeInitialized)
  {
    BandySessionStore_Init();
  }

  action = BandySessionStoreCore_EvaluateSnapshot(snapshot,
                                                 &storeLastRecord,
                                                 storeHasActiveRecord);
  if (action == BANDY_SESSION_STORE_ACTION_NONE)
  {
    return BANDY_SESSION_STORE_OK;
  }

  if (action == BANDY_SESSION_STORE_ACTION_WRITE_RECORD)
  {
    BandySessionStoreCore_RecordFromSnapshot(snapshot, storeNextSequence, &record);
    status = BandySessionStore_WriteRecord(&record);
    if (status == BANDY_SESSION_STORE_OK)
    {
      storeLastRecord = record;
      storeHasActiveRecord = true;
      storeNextSequence++;
    }
    return status;
  }

  return BandySessionStore_Invalidate();
}

bandy_session_store_status_t BandySessionStore_Read(
  bandy_session_store_record_t *record)
{
  uint8_t raw[BANDY_SESSION_STORE_RECORD_SIZE] = {0};
  bandy_session_store_core_status_t coreStatus;

  if (record == NULL)
  {
    return BANDY_SESSION_STORE_ERROR_PARAM;
  }

  if (!BandySessionStore_IsSpiReady())
  {
    return BANDY_SESSION_STORE_ERROR_BUSY;
  }

  if (fram_mb85rs256b_read(&storeFram,
                           BANDY_SESSION_STORE_ADDRESS,
                           raw,
                           sizeof(raw)) != FRAM_MB85RS256B_OK)
  {
    return BANDY_SESSION_STORE_ERROR_IO;
  }

  coreStatus = BandySessionStoreCore_DecodeRecord(raw, record);
  return BandySessionStore_MapCoreStatus(coreStatus);
}

bandy_session_store_status_t BandySessionStore_Invalidate(void)
{
  bandy_session_store_record_t record;
  bandy_session_store_status_t status;

  if (!storeInitialized)
  {
    BandySessionStore_Init();
  }

  BandySessionStoreCore_MakeInvalidRecord(storeNextSequence, &record);
  status = BandySessionStore_WriteRecord(&record);
  if (status == BANDY_SESSION_STORE_OK)
  {
    storeLastRecord = record;
    storeHasActiveRecord = false;
    storeNextSequence++;
  }

  return status;
}
