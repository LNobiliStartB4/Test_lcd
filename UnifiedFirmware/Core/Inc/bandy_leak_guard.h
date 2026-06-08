#ifndef INC_BANDY_LEAK_GUARD_H_
#define INC_BANDY_LEAK_GUARD_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum
{
  BANDY_LEAK_GUARD_FAULT_NONE = 0,
  BANDY_LEAK_GUARD_FAULT_TIMEOUT = 1
} bandy_leak_guard_fault_t;

typedef struct
{
  uint32_t startTickMs;
  bool targetReached;
  bandy_leak_guard_fault_t fault;
} bandy_leak_guard_t;

static inline void BandyLeakGuard_Reset(bandy_leak_guard_t *guard)
{
  if (guard == NULL)
  {
    return;
  }

  guard->startTickMs = 0U;
  guard->targetReached = false;
  guard->fault = BANDY_LEAK_GUARD_FAULT_NONE;
}

static inline void BandyLeakGuard_Start(bandy_leak_guard_t *guard, uint32_t nowMs)
{
  if (guard == NULL)
  {
    return;
  }

  guard->startTickMs = nowMs;
  guard->targetReached = false;
  guard->fault = BANDY_LEAK_GUARD_FAULT_NONE;
}

static inline void BandyLeakGuard_Update(bandy_leak_guard_t *guard,
                                         uint32_t nowMs,
                                         int32_t pressureMbar,
                                         int32_t controlTargetMbar,
                                         uint32_t timeoutMs)
{
  if ((guard == NULL) || (guard->fault != BANDY_LEAK_GUARD_FAULT_NONE) || guard->targetReached)
  {
    return;
  }

  if (pressureMbar >= controlTargetMbar)
  {
    guard->targetReached = true;
    return;
  }

  if ((uint32_t)(nowMs - guard->startTickMs) >= timeoutMs)
  {
    guard->fault = BANDY_LEAK_GUARD_FAULT_TIMEOUT;
  }
}

static inline bool BandyLeakGuard_HasReachedTarget(const bandy_leak_guard_t *guard)
{
  return (guard != NULL) && guard->targetReached;
}

static inline bool BandyLeakGuard_ShouldCountDown(const bandy_leak_guard_t *guard)
{
  return (guard != NULL) &&
         guard->targetReached &&
         (guard->fault == BANDY_LEAK_GUARD_FAULT_NONE);
}

static inline bandy_leak_guard_fault_t BandyLeakGuard_GetFault(const bandy_leak_guard_t *guard)
{
  if (guard == NULL)
  {
    return BANDY_LEAK_GUARD_FAULT_NONE;
  }

  return guard->fault;
}

#endif /* INC_BANDY_LEAK_GUARD_H_ */
