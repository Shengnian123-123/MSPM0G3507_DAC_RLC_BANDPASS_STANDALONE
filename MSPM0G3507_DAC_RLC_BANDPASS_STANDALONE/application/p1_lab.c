#include "p1_lab.h"
#include "adc.h"
#include "adc_fml.h"
#include "delay.h"
#include "gpio_io.h"
#include "main.h"
#include "tim.h"

#define P1_ADC_TIMEOUT_LOOPS       (20000U)
#define P1_SOFT_PWM_MAX_FREQ_HZ    (1000U)
#define P1_SOFT_PWM_MIN_PERIOD_US  (1000U)
#define P1_SOFT_PWM_MAX_DURATIONMS (5000U)

static uint32_t RateToPeriodUs(uint32_t rateHz)
{
    if (rateHz == 0U) {
        return 0U;
    }
    return 1000000UL / rateHz;
}

bool P1_IsSupportedAdcRate(uint32_t rateHz)
{
    return ((rateHz == 10000UL) || (rateHz == 50000UL) ||
            (rateHz == 100000UL) || (rateHz == 200000UL));
}

P1_Status_t P1_SoftPwmLed(uint32_t frequencyHz, uint16_t dutyPermille,
    uint32_t durationMs)
{
    uint32_t periodUs;
    uint32_t onUs;
    uint32_t offUs;
    uint32_t startMs;

    if ((frequencyHz == 0U) || (frequencyHz > P1_SOFT_PWM_MAX_FREQ_HZ) ||
        (dutyPermille > 1000U) || (durationMs > P1_SOFT_PWM_MAX_DURATIONMS)) {
        return P1_STATUS_BAD_PARAM;
    }

    periodUs = 1000000UL / frequencyHz;
    if (periodUs < P1_SOFT_PWM_MIN_PERIOD_US) {
        return P1_STATUS_BAD_PARAM;
    }

    onUs    = (periodUs * dutyPermille) / 1000UL;
    offUs   = periodUs - onUs;
    startMs = Timer_GetTickMs();

    while ((Timer_GetTickMs() - startMs) < durationMs) {
        if (onUs > 0U) {
            GPIOIO_LedOn();
            Delay_Us(onUs);
        }
        if (offUs > 0U) {
            GPIOIO_LedOff();
            Delay_Us(offUs);
        }
    }

    return P1_STATUS_OK;
}

P1_Status_t P1_CaptureAdcBlocking(uint32_t rateHz, uint16_t count,
    P1_AdcStream_t *stream)
{
    uint32_t periodUs;
    uint32_t sum = 0U;

    if ((stream == 0) || !P1_IsSupportedAdcRate(rateHz) || (count == 0U) ||
        (count > P1_ADC_STREAM_MAX_SAMPLES)) {
        return P1_STATUS_BAD_PARAM;
    }

    periodUs = RateToPeriodUs(rateHz);
    stream->requestedRateHz = rateHz;
    stream->sampleCount     = count;
    stream->minRaw          = 0xFFFFU;
    stream->maxRaw          = 0U;
    stream->avgRaw          = 0U;

    for (uint16_t i = 0U; i < count; i++) {
        uint16_t raw;

        if (!ADC_SampleBlocking(&raw, P1_ADC_TIMEOUT_LOOPS)) {
            stream->sampleCount = i;
            return P1_STATUS_ADC_TIMEOUT;
        }

        stream->samples[i] = raw;
        if (raw < stream->minRaw) {
            stream->minRaw = raw;
        }
        if (raw > stream->maxRaw) {
            stream->maxRaw = raw;
        }
        sum += raw;

        if (periodUs > 0U) {
            Delay_Us(periodUs);
        }
    }

    stream->avgRaw = (uint16_t) (sum / count);
    return P1_STATUS_OK;
}

P1_Status_t P1_GetCaptureFrequency(uint32_t *frequencyHz)
{
    if (frequencyHz != 0) {
        *frequencyHz = 0U;
    }
    return P1_STATUS_NOT_CONFIGURED;
}

P1_Status_t P1_GetCaptureDuty(uint16_t *dutyPermille)
{
    if (dutyPermille != 0) {
        *dutyPermille = 0U;
    }
    return P1_STATUS_NOT_CONFIGURED;
}

P1_Status_t P1_GetCapturePhase(uint16_t *phaseDeg)
{
    if (phaseDeg != 0) {
        *phaseDeg = 0U;
    }
    return P1_STATUS_NOT_CONFIGURED;
}

P1_Status_t P1_StartTimerTriggeredAdc(uint32_t rateHz)
{
    (void) rateHz;
    return P1_STATUS_NOT_CONFIGURED;
}

P1_Status_t P1_StartAdcDma(uint32_t rateHz)
{
    (void) rateHz;
    return P1_STATUS_NOT_CONFIGURED;
}

const char *P1_StatusText(P1_Status_t status)
{
    switch (status) {
        case P1_STATUS_OK:
            return "OK";
        case P1_STATUS_BAD_PARAM:
            return "BAD_PARAM";
        case P1_STATUS_ADC_TIMEOUT:
            return "ADC_TIMEOUT";
        case P1_STATUS_NOT_CONFIGURED:
        default:
            return "NOT_CONFIGURED";
    }
}
