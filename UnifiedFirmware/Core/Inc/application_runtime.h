#ifndef APPLICATION_RUNTIME_H
#define APPLICATION_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
  APPLICATION_BANDY_STATE_WAIT_RFID = 0,
  APPLICATION_BANDY_STATE_AUTHORIZED = 1,
  APPLICATION_BANDY_STATE_RUNNING = 2,
  APPLICATION_BANDY_STATE_PAUSED = 3
} application_bandy_state_t;

typedef enum
{
  APPLICATION_ACTIVE_PRODUCT_NONE = 0,
  APPLICATION_ACTIVE_PRODUCT_BANDY = 1,
  APPLICATION_ACTIVE_PRODUCT_HEMORFLOW = 2
} application_active_product_t;

void App_init(void);
void App_process(void);
void ApplicationRuntime_SetManualPumpDutyPercent(uint8_t targetDutyPercent);
void ApplicationRuntime_StartRfidScan(void);
void ApplicationRuntime_StopRfidScan(void);
void ApplicationRuntime_StartVacuumCycle(void);
void ApplicationRuntime_StartHemorflowCycle(void);
void ApplicationRuntime_StopVacuumCycle(void);
void ApplicationRuntime_PauseVacuumCycle(void);
void ApplicationRuntime_ResumeVacuumCycle(void);
void ApplicationRuntime_EndVacuumSession(void);
bool ApplicationRuntime_SetBandyTargetRelativeMbar(int32_t targetMbar);
bool ApplicationRuntime_IsVacuumCycleActive(void);
uint8_t ApplicationRuntime_GetVacuumCycleState(void);
uint8_t ApplicationRuntime_GetVacuumCycleFault(void);
int32_t ApplicationRuntime_GetVacuumCycleTargetRelativeMbar(void);
int32_t ApplicationRuntime_GetVacuumCycleControlTargetRelativeMbar(void);
int32_t ApplicationRuntime_GetBandyTargetMinRelativeMbar(void);
int32_t ApplicationRuntime_GetBandyTargetMaxRelativeMbar(void);
uint8_t ApplicationRuntime_GetVacuumCycleCommandDutyPercent(void);
application_active_product_t ApplicationRuntime_GetActiveProduct(void);
application_bandy_state_t ApplicationRuntime_GetBandyState(void);
uint16_t ApplicationRuntime_GetBandyDurationMinutes(void);
uint16_t ApplicationRuntime_GetBandyRemainingSeconds(void);
uint16_t ApplicationRuntime_GetBandyPauseRemainingSeconds(void);
uint8_t ApplicationRuntime_GetBandyPauseCount(void);
uint8_t ApplicationRuntime_GetBandyMaxPauseCount(void);
uint16_t ApplicationRuntime_GetTagRemainingExams(void);

#endif
