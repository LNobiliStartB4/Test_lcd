#include "bandy_session_store_core.h"

#include <stdio.h>
#include <string.h>

typedef struct
{
  uint8_t raw[BANDY_SESSION_STORE_RECORD_SIZE];
  unsigned int write_count;
  bool fail_write;
} fake_fram_t;

static int failures;

#define EXPECT_TRUE(condition) do { if (!(condition)) { report_failure(__LINE__, #condition); } } while (0)
#define EXPECT_EQ_INT(expected, actual) do { if ((int)(expected) != (int)(actual)) { report_failure(__LINE__, #actual); } } while (0)
#define EXPECT_EQ_U32(expected, actual) do { if ((uint32_t)(expected) != (uint32_t)(actual)) { report_failure(__LINE__, #actual); } } while (0)

static void report_failure(int line, const char *expression)
{
  failures++;
  printf("FAIL line %d: %s\n", line, expression);
}

static bandy_session_store_snapshot_t valid_snapshot(uint16_t remaining_seconds)
{
  bandy_session_store_snapshot_t snapshot;

  memset(&snapshot, 0, sizeof(snapshot));
  snapshot.valid = true;
  snapshot.bandy_state = 2u;
  snapshot.remaining_seconds = remaining_seconds;
  snapshot.duration_minutes = 15u;
  snapshot.pause_remaining_seconds = 0u;
  snapshot.pauses_used = 1u;
  snapshot.pauses_max = 3u;
  snapshot.target_mbar = 490;

  return snapshot;
}

static bool fake_write(fake_fram_t *fram, const bandy_session_store_record_t *record)
{
  if (fram->fail_write)
  {
    return false;
  }

  if (BandySessionStoreCore_EncodeRecord(record, fram->raw) != BANDY_SESSION_STORE_CORE_OK)
  {
    return false;
  }

  fram->write_count++;
  return true;
}

static bool fake_apply_snapshot(fake_fram_t *fram,
                                const bandy_session_store_snapshot_t *snapshot,
                                bandy_session_store_record_t *last_record,
                                bool *has_active_record,
                                uint32_t *sequence)
{
  bandy_session_store_action_t action =
    BandySessionStoreCore_EvaluateSnapshot(snapshot, last_record, *has_active_record);
  bandy_session_store_record_t record;

  if (action == BANDY_SESSION_STORE_ACTION_NONE)
  {
    return true;
  }

  if (action == BANDY_SESSION_STORE_ACTION_INVALIDATE_RECORD)
  {
    BandySessionStoreCore_MakeInvalidRecord(*sequence, &record);
  }
  else
  {
    BandySessionStoreCore_RecordFromSnapshot(snapshot, *sequence, &record);
  }

  if (!fake_write(fram, &record))
  {
    return false;
  }

  *sequence += 1u;
  *last_record = record;
  *has_active_record = record.valid && (record.remaining_seconds > 0u);
  return true;
}

static void test_encode_decode_record_valid(void)
{
  uint8_t raw[BANDY_SESSION_STORE_RECORD_SIZE];
  bandy_session_store_record_t source;
  bandy_session_store_record_t decoded;

  memset(&source, 0, sizeof(source));
  source.valid = true;
  source.bandy_state = 2u;
  source.remaining_seconds = 899u;
  source.duration_minutes = 15u;
  source.pause_remaining_seconds = 0u;
  source.pauses_used = 2u;
  source.pauses_max = 3u;
  source.target_mbar = 490;
  source.sequence = 42u;

  EXPECT_EQ_INT(BANDY_SESSION_STORE_CORE_OK, BandySessionStoreCore_EncodeRecord(&source, raw));
  EXPECT_EQ_INT(BANDY_SESSION_STORE_CORE_OK, BandySessionStoreCore_DecodeRecord(raw, &decoded));
  EXPECT_TRUE(decoded.valid);
  EXPECT_EQ_INT(899u, decoded.remaining_seconds);
  EXPECT_EQ_INT(15u, decoded.duration_minutes);
  EXPECT_EQ_INT(2u, decoded.pauses_used);
  EXPECT_EQ_INT(3u, decoded.pauses_max);
  EXPECT_EQ_INT(490, decoded.target_mbar);
  EXPECT_EQ_U32(42u, decoded.sequence);
}

static void test_checksum_detects_corruption(void)
{
  uint8_t raw[BANDY_SESSION_STORE_RECORD_SIZE];
  bandy_session_store_snapshot_t snapshot = valid_snapshot(900u);
  bandy_session_store_record_t source;
  bandy_session_store_record_t decoded;

  BandySessionStoreCore_RecordFromSnapshot(&snapshot, 1u, &source);
  EXPECT_EQ_INT(BANDY_SESSION_STORE_CORE_OK, BandySessionStoreCore_EncodeRecord(&source, raw));
  raw[10] ^= 0x01u;
  EXPECT_EQ_INT(BANDY_SESSION_STORE_CORE_INVALID, BandySessionStoreCore_DecodeRecord(raw, &decoded));
}

static void test_magic_and_version_errors_are_rejected(void)
{
  uint8_t raw[BANDY_SESSION_STORE_RECORD_SIZE];
  bandy_session_store_snapshot_t snapshot = valid_snapshot(900u);
  bandy_session_store_record_t source;
  bandy_session_store_record_t decoded;

  BandySessionStoreCore_RecordFromSnapshot(&snapshot, 1u, &source);
  EXPECT_EQ_INT(BANDY_SESSION_STORE_CORE_OK, BandySessionStoreCore_EncodeRecord(&source, raw));
  raw[0] = 0u;
  EXPECT_EQ_INT(BANDY_SESSION_STORE_CORE_INVALID, BandySessionStoreCore_DecodeRecord(raw, &decoded));

  EXPECT_EQ_INT(BANDY_SESSION_STORE_CORE_OK, BandySessionStoreCore_EncodeRecord(&source, raw));
  raw[4] = 2u;
  EXPECT_EQ_INT(BANDY_SESSION_STORE_CORE_INVALID, BandySessionStoreCore_DecodeRecord(raw, &decoded));
}

static void test_first_session_writes_record(void)
{
  fake_fram_t fram = {0};
  bandy_session_store_record_t last = {0};
  bool has_active_record = false;
  uint32_t sequence = 1u;
  bandy_session_store_snapshot_t snapshot = valid_snapshot(900u);

  EXPECT_TRUE(fake_apply_snapshot(&fram, &snapshot, &last, &has_active_record, &sequence));
  EXPECT_EQ_INT(1u, fram.write_count);
  EXPECT_TRUE(has_active_record);
  EXPECT_EQ_INT(900u, last.remaining_seconds);
}

static void test_same_remaining_does_not_rewrite(void)
{
  fake_fram_t fram = {0};
  bandy_session_store_record_t last = {0};
  bool has_active_record = false;
  uint32_t sequence = 1u;
  bandy_session_store_snapshot_t snapshot = valid_snapshot(900u);

  EXPECT_TRUE(fake_apply_snapshot(&fram, &snapshot, &last, &has_active_record, &sequence));
  EXPECT_TRUE(fake_apply_snapshot(&fram, &snapshot, &last, &has_active_record, &sequence));
  EXPECT_EQ_INT(1u, fram.write_count);
}

static void test_changed_remaining_writes_once(void)
{
  fake_fram_t fram = {0};
  bandy_session_store_record_t last = {0};
  bool has_active_record = false;
  uint32_t sequence = 1u;
  bandy_session_store_snapshot_t snapshot = valid_snapshot(900u);

  EXPECT_TRUE(fake_apply_snapshot(&fram, &snapshot, &last, &has_active_record, &sequence));
  snapshot.remaining_seconds = 899u;
  EXPECT_TRUE(fake_apply_snapshot(&fram, &snapshot, &last, &has_active_record, &sequence));
  EXPECT_EQ_INT(2u, fram.write_count);
  EXPECT_EQ_INT(899u, last.remaining_seconds);
}

static void test_pause_same_remaining_does_not_write_continuously(void)
{
  fake_fram_t fram = {0};
  bandy_session_store_record_t last = {0};
  bool has_active_record = false;
  uint32_t sequence = 1u;
  bandy_session_store_snapshot_t snapshot = valid_snapshot(840u);

  EXPECT_TRUE(fake_apply_snapshot(&fram, &snapshot, &last, &has_active_record, &sequence));
  snapshot.bandy_state = 3u;
  snapshot.pause_remaining_seconds = 30u;
  EXPECT_TRUE(fake_apply_snapshot(&fram, &snapshot, &last, &has_active_record, &sequence));
  for (uint16_t pause = 29u; pause > 20u; pause--)
  {
    snapshot.pause_remaining_seconds = pause;
    EXPECT_TRUE(fake_apply_snapshot(&fram, &snapshot, &last, &has_active_record, &sequence));
  }

  EXPECT_EQ_INT(2u, fram.write_count);
}

static void test_zero_remaining_invalidates_once(void)
{
  fake_fram_t fram = {0};
  bandy_session_store_record_t last = {0};
  bool has_active_record = false;
  uint32_t sequence = 1u;
  bandy_session_store_snapshot_t snapshot = valid_snapshot(10u);
  bandy_session_store_record_t decoded;

  EXPECT_TRUE(fake_apply_snapshot(&fram, &snapshot, &last, &has_active_record, &sequence));
  snapshot.remaining_seconds = 0u;
  EXPECT_TRUE(fake_apply_snapshot(&fram, &snapshot, &last, &has_active_record, &sequence));
  EXPECT_TRUE(fake_apply_snapshot(&fram, &snapshot, &last, &has_active_record, &sequence));

  EXPECT_EQ_INT(2u, fram.write_count);
  EXPECT_EQ_INT(BANDY_SESSION_STORE_CORE_EMPTY, BandySessionStoreCore_DecodeRecord(fram.raw, &decoded));
}

static void test_wait_rfid_invalidates_once(void)
{
  fake_fram_t fram = {0};
  bandy_session_store_record_t last = {0};
  bool has_active_record = false;
  uint32_t sequence = 1u;
  bandy_session_store_snapshot_t snapshot = valid_snapshot(500u);

  EXPECT_TRUE(fake_apply_snapshot(&fram, &snapshot, &last, &has_active_record, &sequence));
  snapshot.bandy_state = BANDY_SESSION_STORE_STATE_WAIT_RFID;
  EXPECT_TRUE(fake_apply_snapshot(&fram, &snapshot, &last, &has_active_record, &sequence));
  EXPECT_TRUE(fake_apply_snapshot(&fram, &snapshot, &last, &has_active_record, &sequence));
  EXPECT_EQ_INT(2u, fram.write_count);
}

static void test_write_error_is_reported_without_state_change(void)
{
  fake_fram_t fram = {0};
  bandy_session_store_record_t last = {0};
  bool has_active_record = false;
  uint32_t sequence = 1u;
  bandy_session_store_snapshot_t snapshot = valid_snapshot(900u);

  fram.fail_write = true;
  EXPECT_TRUE(!fake_apply_snapshot(&fram, &snapshot, &last, &has_active_record, &sequence));
  EXPECT_EQ_INT(0u, fram.write_count);
  EXPECT_TRUE(!has_active_record);
  EXPECT_EQ_U32(1u, sequence);
}

int main(void)
{
  test_encode_decode_record_valid();
  test_checksum_detects_corruption();
  test_magic_and_version_errors_are_rejected();
  test_first_session_writes_record();
  test_same_remaining_does_not_rewrite();
  test_changed_remaining_writes_once();
  test_pause_same_remaining_does_not_write_continuously();
  test_zero_remaining_invalidates_once();
  test_wait_rfid_invalidates_once();
  test_write_error_is_reported_without_state_change();

  if (failures != 0)
  {
    printf("%d test failure(s)\n", failures);
    return 1;
  }

  printf("bandy_session_store tests passed\n");
  return 0;
}
