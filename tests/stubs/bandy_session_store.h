#ifndef BANDY_SESSION_STORE_H
#define BANDY_SESSION_STORE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  BANDY_SESSION_STORE_OK = 0,
  BANDY_SESSION_STORE_EMPTY,
  BANDY_SESSION_STORE_INVALID,
  BANDY_SESSION_STORE_ERROR_PARAM,
  BANDY_SESSION_STORE_ERROR_BUSY,
  BANDY_SESSION_STORE_ERROR_IO
} bandy_session_store_status_t;

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

bandy_session_store_status_t BandySessionStore_ProcessSnapshot(
  const bandy_session_store_snapshot_t *snapshot);
bandy_session_store_status_t BandySessionStore_Read(
  bandy_session_store_record_t *record);

#ifdef __cplusplus
}
#endif

#endif
