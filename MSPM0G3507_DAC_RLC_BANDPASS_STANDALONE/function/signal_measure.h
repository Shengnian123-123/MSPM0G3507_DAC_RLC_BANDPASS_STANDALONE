#ifndef SIGNAL_MEASURE_H
#define SIGNAL_MEASURE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t minRaw;
    uint16_t maxRaw;
    uint16_t avgRaw;
    uint16_t pkpkRaw;
    uint16_t rmsRaw;
    uint16_t acRmsRaw;
    uint16_t dutyPermille;
    uint16_t risingEdges;
    uint32_t frequencyHz;
    uint16_t minMv;
    uint16_t maxMv;
    uint16_t avgMv;
    uint16_t pkpkMv;
    uint16_t rmsMv;
    uint16_t acRmsMv;
} SignalMeasure_Result_t;

bool SignalMeasure_Analyze(const uint16_t *samples, uint16_t count,
    uint32_t sampleRateHz, SignalMeasure_Result_t *result);

#endif
