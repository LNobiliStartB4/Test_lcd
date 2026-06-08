#ifndef BANDY_SESSION_STORE_CORE_H
#define BANDY_SESSION_STORE_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BANDY_SESSION_STORE_RECORD_SIZE (32u)
#define BANDY_SESSION_STORE_STATE_WAIT_RFID (0u)

typedef enum
{
  BANDY_SESSION_STORE_CORE_OK = 0,
  BANDY_SESSION_STORE_CORE_EMPTY,
  BANDY_SESSION_STORE_CORE_INVALID,
  BANDY_SESSION_STORE_CORE_ERROR_PARAM
} bandy_session_store_core_status_t;

typedef enum
{
  BANDY_SESSION_STORE_ACTION_NONE = 0,
  BANDY_SESSION_STORE_ACTION_WRITE_RECORD,
  BANDY_SESSION_STORE_ACTION_INVALIDATE_RECORD
} bandy_session_store_action_t;

typedef struct
{
  bool valid;
  uint8_t bandy_state;
  uint16_t remaining_seconds;
  uint16_t duration_minutes;
  uint16_t pause_remaining_seconds;
  uint8_t pauses_used;
  uint8_t pauses_max;
  int32_t target_mbar;
} bandy_session_store_snapshot_t;

typedef struct
{
  bool valid;
  uint8_t bandy_state;
  uint16_t remaining_seconds;
  uint16_t duration_minutes;
  uint16_t pause_remaining_seconds;
  uint8_t pauses_used;
  uint8_t pauses_max;
  int32_t target_mbar;
  uint32_t sequence;
} bandy_session_store_record_t;

uint16_t BandySessionStoreCore_Crc16(const uint8_t *data, size_t length);
bandy_session_store_core_status_t BandySessionStoreCore_EncodeRecord(
  const bandy_session_store_record_t *record,
  uint8_t raw[BANDY_SESSION_STORE_RECORD_SIZE]);
bandy_session_store_core_status_t BandySessionStoreCore_DecodeRecord(
  const uint8_t raw[BANDY_SESSION_STORE_RECORD_SIZE],
  bandy_session_store_record_t *record);
void BandySessionStoreCore_RecordFromSnapshot(
  const bandy_session_store_snapshot_t *snapshot,
  uint32_t sequence,
  bandy_session_store_record_t *record);
void BandySessionStoreCore_MakeInvalidRecord(
  uint32_t sequence,
  bandy_session_store_record_t *record);
bandy_session_store_action_t BandySessionStoreCore_EvaluateSnapshot(
  const bandy_session_store_snapshot_t *snapshot,
  const bandy_session_store_record_t *last_record,
  bool has_active_record);

#ifdef __cplusplus
}
#endif

#endif /* BANDY_SESSION_STORE_CORE_H */
