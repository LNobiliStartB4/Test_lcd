#ifndef DISCHARGE_CONTROLLER_H
#define DISCHARGE_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void DischargeController_Init(void);
void DischargeController_Close(void);
void DischargeController_StartRelease(uint32_t nowMs);
void DischargeController_Process(uint32_t nowMs, bool pressureValid, int32_t pressureMbar);
bool DischargeController_IsReleasing(void);

#ifdef __cplusplus
}
#endif

#endif /* DISCHARGE_CONTROLLER_H */
