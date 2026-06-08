#include "pressure_sensor.h"

#include "adc_reader.h"

#define PRESSURE_SENSOR_PERIOD_MS 10U
#define PRESSURE_SENSOR_SAMPLE_TIME ADC_SAMPLETIME_84CYCLES
#define PRESSURE_SENSOR_FILTER_SAMPLES 8U
#define PRESSURE_SENSOR_ADC_PLAUSIBLE_MIN 50U
#define PRESSURE_SENSOR_ADC_PLAUSIBLE_MAX 4050U
#define PRESSURE_SENSOR_FACTORY_ZERO_ADC 166
#define PRESSURE_SENSOR_FACTORY_500_MBAR_ADC 2028
#define PRESSURE_SENSOR_FACTORY_SPAN_MBAR 500
#define PRESSURE_SENSOR_TARGET_RELATIVE_MBAR 450

static uint16_t pressureSamples[PRESSURE_SENSOR_FILTER_SAMPLES];
static uint32_t pressureSampleSum;
static uint8_t pressureSampleIndex;
static uint8_t pressureSampleCount;
static uint16_t pressureRawAdc;
static uint16_t pressureFilteredAdc;
static int32_t pressureFactoryMbar;
static int32_t pressureRelativeMbar;
static int32_t pressureZeroOffsetMbar;
static uint32_t pressureLastTickMs;
static uint32_t pressureTestStartTickMs;
static uint32_t pressureTimeToTargetMs;
static bool pressureValid;
static bool pressureTestActive;
static bool pressureTargetReached;

volatile int32_t monitorPressureZeroOffsetMbar;
volatile int32_t monitorPressureRawDeltaMbar;
volatile int32_t monitorPressureRelativeMbar;

static void PressureSensor_ResetFilter(void)
{
  uint8_t i;

  for (i = 0U; i < PRESSURE_SENSOR_FILTER_SAMPLES; ++i)
  {
    pressureSamples[i] = 0U;
  }

  pressureSampleSum = 0U;
  pressureSampleIndex = 0U;
  pressureSampleCount = 0U;
}

static uint16_t PressureSensor_FilterSample(uint16_t sample)
{
  pressureSampleSum -= pressureSamples[pressureSampleIndex];
  pressureSamples[pressureSampleIndex] = sample;
  pressureSampleSum += sample;

  if (pressureSampleCount < PRESSURE_SENSOR_FILTER_SAMPLES)
  {
    pressureSampleCount++;
  }

  pressureSampleIndex++;
  if (pressureSampleIndex >= PRESSURE_SENSOR_FILTER_SAMPLES)
  {
    pressureSampleIndex = 0U;
  }

  return (uint16_t)(pressureSampleSum / pressureSampleCount);
}

static bool PressureSensor_IsPlausible(uint16_t adcCounts)
{
  return (adcCounts >= PRESSURE_SENSOR_ADC_PLAUSIBLE_MIN) &&
         (adcCounts <= PRESSURE_SENSOR_ADC_PLAUSIBLE_MAX);
}

static int32_t PressureSensor_AdcToFactoryMbar(uint16_t adcCounts)
{
  int32_t deltaCounts;
  int32_t spanCounts;

  if (!PressureSensor_IsPlausible(adcCounts))
  {
    return 0;
  }

  deltaCounts = (int32_t)adcCounts - PRESSURE_SENSOR_FACTORY_ZERO_ADC;
  if (deltaCounts <= 0)
  {
    return 0;
  }

  spanCounts = PRESSURE_SENSOR_FACTORY_500_MBAR_ADC - PRESSURE_SENSOR_FACTORY_ZERO_ADC;
  return (deltaCounts * PRESSURE_SENSOR_FACTORY_SPAN_MBAR + (spanCounts / 2)) / spanCounts;
}

void PressureSensor_Init(void)
{
  pressureRawAdc = 0U;
  pressureFilteredAdc = 0U;
  pressureFactoryMbar = 0;
  pressureRelativeMbar = 0;
  pressureZeroOffsetMbar = 0;
  pressureLastTickMs = HAL_GetTick();
  pressureTestStartTickMs = 0U;
  pressureTimeToTargetMs = 0U;
  pressureValid = false;
  pressureTestActive = false;
  pressureTargetReached = false;
  monitorPressureZeroOffsetMbar = 0;
  monitorPressureRawDeltaMbar = 0;
  monitorPressureRelativeMbar = 0;
  PressureSensor_ResetFilter();
}

void PressureSensor_CaptureZero(void)
{
  pressureZeroOffsetMbar = pressureValid ? pressureFactoryMbar : 0;
  pressureRelativeMbar = 0;
  monitorPressureZeroOffsetMbar = pressureZeroOffsetMbar;
  monitorPressureRelativeMbar = 0;
}

void PressureSensor_StartTest(void)
{
  PressureSensor_CaptureZero();
  pressureTestActive = true;
  pressureTargetReached = false;
  pressureTimeToTargetMs = 0U;
  pressureTestStartTickMs = HAL_GetTick();
}

void PressureSensor_StopTest(void)
{
  pressureTestActive = false;
}

void PressureSensor_Process(void)
{
  uint16_t rawAdc;
  uint32_t now = HAL_GetTick();

  if ((uint32_t)(now - pressureLastTickMs) < PRESSURE_SENSOR_PERIOD_MS)
  {
    return;
  }

  pressureLastTickMs = now;
  if (!Adc1_ReadSingleChannel(ADC_CHANNEL_15, PRESSURE_SENSOR_SAMPLE_TIME, &rawAdc))
  {
    pressureValid = false;
    return;
  }

  pressureRawAdc = rawAdc;
  pressureFilteredAdc = PressureSensor_FilterSample(rawAdc);
  pressureValid = PressureSensor_IsPlausible(pressureFilteredAdc);
  if (!pressureValid)
  {
    return;
  }

  pressureFactoryMbar = PressureSensor_AdcToFactoryMbar(pressureFilteredAdc);
  monitorPressureRawDeltaMbar = pressureFactoryMbar;
  pressureRelativeMbar = pressureFactoryMbar - pressureZeroOffsetMbar;
  if (pressureRelativeMbar < 0)
  {
    pressureRelativeMbar = 0;
  }
  monitorPressureRelativeMbar = pressureRelativeMbar;

  if (pressureTestActive &&
      !pressureTargetReached &&
      (pressureRelativeMbar >= PRESSURE_SENSOR_TARGET_RELATIVE_MBAR))
  {
    pressureTargetReached = true;
    pressureTimeToTargetMs = now - pressureTestStartTickMs;
  }
}

bool PressureSensor_IsTestActive(void) { return pressureTestActive; }
bool PressureSensor_HasReachedTarget(void) { return pressureTargetReached; }
uint32_t PressureSensor_GetTimeToTargetMs(void) { return pressureTimeToTargetMs; }
int32_t PressureSensor_GetRelativeMbar(void) { return pressureRelativeMbar; }
int32_t PressureSensor_GetRawRelativeMbar(void) { return pressureFactoryMbar - pressureZeroOffsetMbar; }
int32_t PressureSensor_GetZeroOffsetMbar(void) { return pressureZeroOffsetMbar; }
bool PressureSensor_IsValid(void) { return pressureValid; }
uint16_t PressureSensor_GetAmbientRawAdc(void) { return 0U; }
uint16_t PressureSensor_GetChamberRawAdc(void) { return pressureRawAdc; }
uint16_t PressureSensor_GetAmbientAbsMbar(void) { return 0U; }
uint16_t PressureSensor_GetChamberAbsMbar(void)
{
  return (pressureFactoryMbar > 0) ? (uint16_t)pressureFactoryMbar : 0U;
}
