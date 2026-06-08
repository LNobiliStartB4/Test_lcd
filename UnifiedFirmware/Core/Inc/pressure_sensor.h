#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

void PressureSensor_Init(void);
void PressureSensor_Process(void);
void PressureSensor_CaptureZero(void);
void PressureSensor_StartTest(void);
void PressureSensor_StopTest(void);
int32_t PressureSensor_GetRelativeMbar(void);
int32_t PressureSensor_GetRawRelativeMbar(void);
int32_t PressureSensor_GetZeroOffsetMbar(void);
bool PressureSensor_IsValid(void);
bool PressureSensor_IsTestActive(void);
bool PressureSensor_HasReachedTarget(void);
uint32_t PressureSensor_GetTimeToTargetMs(void);
uint16_t PressureSensor_GetAmbientRawAdc(void);
uint16_t PressureSensor_GetChamberRawAdc(void);
uint16_t PressureSensor_GetAmbientAbsMbar(void);
uint16_t PressureSensor_GetChamberAbsMbar(void);

#endif
