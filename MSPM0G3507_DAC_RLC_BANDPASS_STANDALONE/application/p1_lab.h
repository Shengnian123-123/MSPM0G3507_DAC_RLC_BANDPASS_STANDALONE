#ifndef P1_LAB_H
#define P1_LAB_H

#include <stdbool.h>
#include <stdint.h>

#define P1_ADC_STREAM_MAX_SAMPLES (128U)

typedef enum {
    P1_STATUS_OK = 0,
    P1_STATUS_BAD_PARAM,
    P1_STATUS_ADC_TIMEOUT,
    P1_STATUS_NOT_CONFIGURED,
} P1_Status_t;

typedef struct {
    uint32_t requestedRateHz;
    uint16_t sampleCount;
    uint16_t minRaw;
    uint16_t maxRaw;
    uint16_t avgRaw;
    uint16_t samples[P1_ADC_STREAM_MAX_SAMPLES];
} P1_AdcStream_t;

P1_Status_t P1_SoftPwmLed(uint32_t frequencyHz, uint16_t dutyPermille,
    uint32_t durationMs);
bool P1_IsSupportedAdcRate(uint32_t rateHz);
P1_Status_t P1_CaptureAdcBlocking(uint32_t rateHz, uint16_t count,
    P1_AdcStream_t *stream);

P1_Status_t P1_GetCaptureFrequency(uint32_t *frequencyHz);
P1_Status_t P1_GetCaptureDuty(uint16_t *dutyPermille);
P1_Status_t P1_GetCapturePhase(uint16_t *phaseDeg);
P1_Status_t P1_StartTimerTriggeredAdc(uint32_t rateHz);
P1_Status_t P1_StartAdcDma(uint32_t rateHz);

const char *P1_StatusText(P1_Status_t status);

#endif
