#ifndef BANDY_SESSION_STORE_H
#define BANDY_SESSION_STORE_H

#include "bandy_session_store_core.h"

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

void BandySessionStore_Init(void);
bandy_session_store_status_t BandySessionStore_ProcessSnapshot(
  const bandy_session_store_snapshot_t *snapshot);
bandy_session_store_status_t BandySessionStore_Read(
  bandy_session_store_record_t *record);
bandy_session_store_status_t BandySessionStore_Invalidate(void);

#ifdef __cplusplus
}
#endif

#endif /* BANDY_SESSION_STORE_H */
