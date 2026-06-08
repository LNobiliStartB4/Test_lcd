#ifndef INC_BANDY_PAUSE_GUARD_H_
#define INC_BANDY_PAUSE_GUARD_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
  uint8_t pauseCount;
} bandy_pause_guard_t;

static inline void BandyPauseGuard_Reset(bandy_pause_guard_t *guard)
{
  if (guard == NULL)
  {
    return;
  }

  guard->pauseCount = 0U;
}

static inline bool BandyPauseGuard_RecordAcceptedPause(bandy_pause_guard_t *guard, uint8_t maxPauses)
{
  if ((guard == NULL) || (guard->pauseCount >= maxPauses))
  {
    return false;
  }

  guard->pauseCount++;
  return true;
}

static inline uint8_t BandyPauseGuard_GetCount(const bandy_pause_guard_t *guard)
{
  if (guard == NULL)
  {
    return 0U;
  }

  return guard->pauseCount;
}

#endif /* INC_BANDY_PAUSE_GUARD_H_ */
