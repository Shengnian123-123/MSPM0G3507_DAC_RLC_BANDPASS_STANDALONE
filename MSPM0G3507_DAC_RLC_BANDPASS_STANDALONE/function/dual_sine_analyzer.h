#ifndef FUNCTION_DUAL_SINE_ANALYZER_H
#define FUNCTION_DUAL_SINE_ANALYZER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool valid;
    bool usedDma;
    uint32_t sampleRateHz;
    uint16_t sampleCount;
    uint32_t frequency1Hz;
    uint32_t frequency2Hz;
    uint16_t amplitude1Raw;
    uint16_t amplitude2Raw;
    uint16_t amplitude1Mv;
    uint16_t amplitude2Mv;
    int16_t phase1Deg;
    int16_t phase2Deg;
    uint16_t dcRaw;
    uint16_t noiseRaw;
    uint32_t separationHz;
    const char *rejectReason;
} DualSine_Result_t;

bool DualSine_AnalyzeSamples(const uint16_t *samples, uint16_t sampleCount,
    uint32_t sampleRateHz, DualSine_Result_t *result);
bool DualSine_CaptureAndAnalyze(uint32_t sampleRateHz, uint16_t sampleCount,
    DualSine_Result_t *result);

#endif

