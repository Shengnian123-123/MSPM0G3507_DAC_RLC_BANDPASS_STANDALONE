#ifndef HW_RMS_METER_H
#define HW_RMS_METER_H

#include <stdbool.h>
#include <stdint.h>
#include "adc_dma_scope.h"

typedef struct {
    int32_t zeroRaw;
    uint32_t gainPermille;
} RmsMeter_Calibration_t;

typedef struct {
    uint32_t sampleRateHz;
    uint16_t sampleCount;
    uint16_t minRaw;
    uint16_t maxRaw;
    uint16_t avgRaw;
    uint16_t pkpkRaw;
    int32_t minMv;
    int32_t maxMv;
    int32_t avgMv;
    uint32_t pkpkMv;
    uint32_t rmsMv;
    uint32_t acRmsMv;
} RmsMeter_Result_t;

void RmsMeter_Init(void);
bool RmsMeter_Measure(uint32_t sampleRateHz, uint16_t sampleCount,
    RmsMeter_Result_t *result);
bool RmsMeter_CalibrateZero(uint32_t sampleRateHz, uint16_t sampleCount);
bool RmsMeter_SetZeroRaw(int32_t zeroRaw);
bool RmsMeter_SetGainPermille(uint32_t gainPermille);
RmsMeter_Calibration_t RmsMeter_GetCalibration(void);

#endif
