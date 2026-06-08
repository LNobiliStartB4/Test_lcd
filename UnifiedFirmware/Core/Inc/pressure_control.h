#ifndef PRESSURE_CONTROL_H
#define PRESSURE_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#define PRESSURE_CONTROL_MAX_RELATIVE_MBAR (490)
#define PRESSURE_CONTROL_DEFAULT_TARGET_RELATIVE_MBAR (470)

typedef enum
{
  PRESSURE_CONTROL_STATE_IDLE = 0,
  PRESSURE_CONTROL_STATE_RUNNING,
  PRESSURE_CONTROL_STATE_FAULT
} pressure_control_state_t;

typedef enum
{
  PRESSURE_CONTROL_FAULT_NONE = 0,
  PRESSURE_CONTROL_FAULT_PRESSURE_INVALID,
  PRESSURE_CONTROL_FAULT_SATURATION_TIMEOUT,
  PRESSURE_CONTROL_FAULT_MAX_PRESSURE
} pressure_control_fault_t;

void PressureControl_Init(void);
void PressureControl_Process(void);
bool PressureControl_Start(int32_t targetRelativeMbar);
void PressureControl_Stop(void);
bool PressureControl_IsActive(void);
pressure_control_state_t PressureControl_GetState(void);
pressure_control_fault_t PressureControl_GetFault(void);
int32_t PressureControl_GetTargetRelativeMbar(void);
uint8_t PressureControl_GetCommandDutyPercent(void);
bool PressureControl_IsTargetReached(void);

#endif /* PRESSURE_CONTROL_H */
