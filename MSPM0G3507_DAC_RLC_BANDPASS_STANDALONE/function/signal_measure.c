#include "signal_measure.h"
#include "adc_fml.h"

static uint32_t IsqrtU64(uint64_t value)
{
    uint64_t bit = 1ULL << 62U;
    uint64_t result = 0U;

    while (bit > value) {
        bit >>= 2U;
    }

    while (bit != 0U) {
        if (value >= (result + bit)) {
            value -= result + bit;
            result = (result >> 1U) + bit;
        } else {
            result >>= 1U;
        }
        bit >>= 2U;
    }

    return (uint32_t) result;
}

static uint16_t RawDeltaToMillivolt(uint16_t rawDelta)
{
    return ADC_Fml_RawToMillivolt(rawDelta);
}

bool SignalMeasure_Analyze(const uint16_t *samples, uint16_t count,
    uint32_t sampleRateHz, SignalMeasure_Result_t *result)
{
    uint64_t sum = 0U;
    uint64_t sumSquare = 0U;
    uint64_t acSquare = 0U;
    uint16_t highCount = 0U;
    uint16_t threshold;
    uint16_t highThreshold;
    uint16_t lowThreshold;
    uint16_t hysteresis;
    uint16_t firstRising = 0U;
    uint16_t lastRising = 0U;
    bool previousHigh;

    if ((samples == 0) || (result == 0) || (count == 0U)) {
        return false;
    }

    result->minRaw = 0xFFFFU;
    result->maxRaw = 0U;
    result->risingEdges = 0U;
    result->frequencyHz = 0U;

    for (uint16_t i = 0U; i < count; i++) {
        uint16_t sample = samples[i];

        if (sample < result->minRaw) {
            result->minRaw = sample;
        }
        if (sample > result->maxRaw) {
            result->maxRaw = sample;
        }
        sum += sample;
        sumSquare += ((uint64_t) sample) * sample;
    }

    result->avgRaw = (uint16_t) (sum / count);
    result->pkpkRaw = result->maxRaw - result->minRaw;
    result->rmsRaw = (uint16_t) IsqrtU64(sumSquare / count);

    for (uint16_t i = 0U; i < count; i++) {
        int32_t delta = (int32_t) samples[i] - (int32_t) result->avgRaw;
        acSquare += (uint64_t) (delta * delta);
    }
    result->acRmsRaw = (uint16_t) IsqrtU64(acSquare / count);

    threshold = result->avgRaw;
    hysteresis = result->pkpkRaw / 16U;
    if (hysteresis < 2U) {
        hysteresis = 2U;
    }
    highThreshold = threshold + hysteresis;
    lowThreshold = (threshold > hysteresis) ? (threshold - hysteresis) : 0U;
    previousHigh = (samples[0] >= threshold);
    if (previousHigh) {
        highCount++;
    }

    for (uint16_t i = 1U; i < count; i++) {
        bool high = previousHigh;

        if (samples[i] >= highThreshold) {
            high = true;
        } else if (samples[i] <= lowThreshold) {
            high = false;
        }

        if (high && !previousHigh) {
            if (result->risingEdges == 0U) {
                firstRising = i;
            }
            lastRising = i;
            result->risingEdges++;
        }
        if (samples[i] >= threshold) {
            highCount++;
        }
        previousHigh = high;
    }

    result->dutyPermille = (uint16_t) (((uint32_t) highCount * 1000UL) / count);
    if ((sampleRateHz > 0U) && (result->risingEdges >= 2U) &&
        (lastRising > firstRising)) {
        uint32_t periods = (uint32_t) result->risingEdges - 1UL;
        result->frequencyHz =
            (periods * sampleRateHz) / ((uint32_t) lastRising - firstRising);
    }

    result->minMv = ADC_Fml_RawToMillivolt(result->minRaw);
    result->maxMv = ADC_Fml_RawToMillivolt(result->maxRaw);
    result->avgMv = ADC_Fml_RawToMillivolt(result->avgRaw);
    result->pkpkMv = RawDeltaToMillivolt(result->pkpkRaw);
    result->rmsMv = ADC_Fml_RawToMillivolt(result->rmsRaw);
    result->acRmsMv = RawDeltaToMillivolt(result->acRmsRaw);
    return true;
}
