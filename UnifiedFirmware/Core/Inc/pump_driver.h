#ifndef PUMP_DRIVER_H
#define PUMP_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

void PumpDriver_Init(void);
void PumpDriver_Process(void);
void PumpDriver_SetEnabled(bool enabled);
void PumpDriver_SetTargetDutyPercent(uint8_t targetDutyPercent);
bool PumpDriver_SetPressureTargetMbar(int32_t targetMbar);
bool PumpDriver_SetPressureProfileMbar(int32_t userTargetMbar, int32_t controlTargetMbar, int32_t restartTargetMbar);
bool PumpDriver_IsEnabled(void);
bool PumpDriver_IsVacuumTestActive(void);
uint16_t PumpDriver_GetCurrentRawAdc(void);
uint16_t PumpDriver_GetCurrentmA(void);
uint16_t PumpDriver_GetFilteredCurrentmA(void);
uint8_t PumpDriver_GetDuty(void);
uint8_t PumpDriver_GetDutyLimitPercent(void);
int32_t PumpDriver_GetPressureStopMbar(void);
int32_t PumpDriver_GetPressureRestartMbar(void);
int32_t PumpDriver_GetPressureTargetMinMbar(void);
int32_t PumpDriver_GetPressureTargetMaxMbar(void);
int32_t PumpDriver_GetUserPressureTargetMbar(void);
int32_t PumpDriver_GetPressureControlTargetMbar(void);
uint16_t PumpDriver_GetMaxMeasurableCurrentmA(void);
void PumpDriver_StartVacuumTest(void);
#endif
