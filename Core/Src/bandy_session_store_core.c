#include "bandy_session_store_core.h"

#include <string.h>

#define BANDY_SESSION_STORE_MAGIC (0x52545342u) /* "BSTR", little-endian. */
#define BANDY_SESSION_STORE_VERSION (1u)
#define BANDY_SESSION_STORE_FLAG_VALID (0x0001u)
#define BANDY_SESSION_STORE_CRC_OFFSET (30u)

static void write_u16_le(uint8_t *data, uint16_t value)
{
  data[0] = (uint8_t)(value & 0xFFu);
  data[1] = (uint8_t)((value >> 8) & 0xFFu);
}

static void write_u32_le(uint8_t *data, uint32_t value)
{
  data[0] = (uint8_t)(value & 0xFFu);
  data[1] = (uint8_t)((value >> 8) & 0xFFu);
  data[2] = (uint8_t)((value >> 16) & 0xFFu);
  data[3] = (uint8_t)((value >> 24) & 0xFFu);
}

static uint16_t read_u16_le(const uint8_t *data)
{
  return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t read_u32_le(const uint8_t *data)
{
  return ((uint32_t)data[0]) |
         ((uint32_t)data[1] << 8) |
         ((uint32_t)data[2] << 16) |
         ((uint32_t)data[3] << 24);
}

uint16_t BandySessionStoreCore_Crc16(const uint8_t *data, size_t length)
{
  uint16_t crc = 0xFFFFu;

  if ((data == NULL) && (length > 0u))
  {
    return 0u;
  }

  for (size_t index = 0u; index < length; index++)
  {
    crc ^= (uint16_t)data[index] << 8;
    for (uint8_t bit = 0u; bit < 8u; bit++)
    {
      if ((crc & 0x8000u) != 0u)
      {
        crc = (uint16_t)((crc << 1) ^ 0x1021u);
      }
      else
      {
        crc = (uint16_t)(crc << 1);
      }
    }
  }

  return crc;
}

bandy_session_store_core_status_t BandySessionStoreCore_EncodeRecord(
  const bandy_session_store_record_t *record,
  uint8_t raw[BANDY_SESSION_STORE_RECORD_SIZE])
{
  uint16_t flags;
  uint16_t crc;

  if ((record == NULL) || (raw == NULL))
  {
    return BANDY_SESSION_STORE_CORE_ERROR_PARAM;
  }

  memset(raw, 0, BANDY_SESSION_STORE_RECORD_SIZE);
  flags = record->valid ? BANDY_SESSION_STORE_FLAG_VALID : 0u;

  write_u32_le(&raw[0], BANDY_SESSION_STORE_MAGIC);
  write_u16_le(&raw[4], BANDY_SESSION_STORE_VERSION);
  write_u16_le(&raw[6], BANDY_SESSION_STORE_RECORD_SIZE);
  write_u16_le(&raw[8], flags);
  write_u16_le(&raw[10], record->remaining_seconds);
  write_u16_le(&raw[12], record->duration_minutes);
  write_u16_le(&raw[14], record->pause_remaining_seconds);
  write_u32_le(&raw[16], (uint32_t)record->target_mbar);
  write_u32_le(&raw[20], record->sequence);
  raw[24] = record->bandy_state;
  raw[25] = record->pauses_used;
  raw[26] = record->pauses_max;

  crc = BandySessionStoreCore_Crc16(raw, BANDY_SESSION_STORE_CRC_OFFSET);
  write_u16_le(&raw[BANDY_SESSION_STORE_CRC_OFFSET], crc);
  return BANDY_SESSION_STORE_CORE_OK;
}

bandy_session_store_core_status_t BandySessionStoreCore_DecodeRecord(
  const uint8_t raw[BANDY_SESSION_STORE_RECORD_SIZE],
  bandy_session_store_record_t *record)
{
  uint16_t stored_crc;
  uint16_t computed_crc;
  uint16_t flags;

  if ((raw == NULL) || (record == NULL))
  {
    return BANDY_SESSION_STORE_CORE_ERROR_PARAM;
  }

  memset(record, 0, sizeof(*record));

  if ((read_u32_le(&raw[0]) != BANDY_SESSION_STORE_MAGIC) ||
      (read_u16_le(&raw[4]) != BANDY_SESSION_STORE_VERSION) ||
      (read_u16_le(&raw[6]) != BANDY_SESSION_STORE_RECORD_SIZE))
  {
    return BANDY_SESSION_STORE_CORE_INVALID;
  }

  stored_crc = read_u16_le(&raw[BANDY_SESSION_STORE_CRC_OFFSET]);
  computed_crc = BandySessionStoreCore_Crc16(raw, BANDY_SESSION_STORE_CRC_OFFSET);
  if (stored_crc != computed_crc)
  {
    return BANDY_SESSION_STORE_CORE_INVALID;
  }

  flags = read_u16_le(&raw[8]);
  record->valid = (flags & BANDY_SESSION_STORE_FLAG_VALID) != 0u;
  record->remaining_seconds = read_u16_le(&raw[10]);
  record->duration_minutes = read_u16_le(&raw[12]);
  record->pause_remaining_seconds = read_u16_le(&raw[14]);
  record->target_mbar = (int32_t)read_u32_le(&raw[16]);
  record->sequence = read_u32_le(&raw[20]);
  record->bandy_state = raw[24];
  record->pauses_used = raw[25];
  record->pauses_max = raw[26];

  if (!record->valid || (record->remaining_seconds == 0u))
  {
    record->valid = false;
    return BANDY_SESSION_STORE_CORE_EMPTY;
  }

  return BANDY_SESSION_STORE_CORE_OK;
}

void BandySessionStoreCore_RecordFromSnapshot(
  const bandy_session_store_snapshot_t *snapshot,
  uint32_t sequence,
  bandy_session_store_record_t *record)
{
  if ((snapshot == NULL) || (record == NULL))
  {
    return;
  }

  record->valid = true;
  record->bandy_state = snapshot->bandy_state;
  record->remaining_seconds = snapshot->remaining_seconds;
  record->duration_minutes = snapshot->duration_minutes;
  record->pause_remaining_seconds = snapshot->pause_remaining_seconds;
  record->pauses_used = snapshot->pauses_used;
  record->pauses_max = snapshot->pauses_max;
  record->target_mbar = snapshot->target_mbar;
  record->sequence = sequence;
}

void BandySessionStoreCore_MakeInvalidRecord(
  uint32_t sequence,
  bandy_session_store_record_t *record)
{
  if (record == NULL)
  {
    return;
  }

  memset(record, 0, sizeof(*record));
  record->sequence = sequence;
}

bandy_session_store_action_t BandySessionStoreCore_EvaluateSnapshot(
  const bandy_session_store_snapshot_t *snapshot,
  const bandy_session_store_record_t *last_record,
  bool has_active_record)
{
  bool session_active;

  if (snapshot == NULL)
  {
    return BANDY_SESSION_STORE_ACTION_NONE;
  }

  session_active = snapshot->valid &&
                   (snapshot->bandy_state != BANDY_SESSION_STORE_STATE_WAIT_RFID) &&
                   (snapshot->remaining_seconds > 0u);

  if (!session_active)
  {
    return has_active_record ?
           BANDY_SESSION_STORE_ACTION_INVALIDATE_RECORD :
           BANDY_SESSION_STORE_ACTION_NONE;
  }

  if (!has_active_record || (last_record == NULL))
  {
    return BANDY_SESSION_STORE_ACTION_WRITE_RECORD;
  }

  if ((last_record->remaining_seconds != snapshot->remaining_seconds) ||
      (last_record->duration_minutes != snapshot->duration_minutes) ||
      (last_record->bandy_state != snapshot->bandy_state) ||
      (last_record->pauses_used != snapshot->pauses_used) ||
      (last_record->pauses_max != snapshot->pauses_max) ||
      (last_record->target_mbar != snapshot->target_mbar))
  {
    return BANDY_SESSION_STORE_ACTION_WRITE_RECORD;
  }

  return BANDY_SESSION_STORE_ACTION_NONE;
}
