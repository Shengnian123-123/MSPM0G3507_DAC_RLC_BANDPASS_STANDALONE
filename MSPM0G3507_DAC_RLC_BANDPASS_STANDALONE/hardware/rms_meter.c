#include "rms_meter.h"
#include "adc_fml.h"

static RmsMeter_Calibration_t g_calibration;

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

static int32_t RawToCalibratedMillivolt(int32_t raw)
{
    int32_t centeredRaw = raw - g_calibration.zeroRaw;
    int64_t mv = (int64_t) centeredRaw * ADC_FML_VREF_MV *
                 (int64_t) g_calibration.gainPermille;

    mv /= ((int64_t) ADC_FML_FULL_SCALE_RAW * 1000LL);
    return (int32_t) mv;
}

static uint32_t RawDeltaToCalibratedMillivolt(uint32_t rawDelta)
{
    uint64_t mv = (uint64_t) rawDelta * ADC_FML_VREF_MV *
                  g_calibration.gainPermille;

    mv /= ((uint64_t) ADC_FML_FULL_SCALE_RAW * 1000ULL);
    return (uint32_t) mv;
}

void RmsMeter_Init(void)
{
    g_calibration.zeroRaw = 0;
    g_calibration.gainPermille = 1000U;
}

bool RmsMeter_Measure(uint32_t sampleRateHz, uint16_t sampleCount,
    RmsMeter_Result_t *result)
{
    static AdcDmaScope_Frame_t frame;
    uint64_t sum = 0U;
    uint64_t squareSum = 0U;
    uint64_t acSquareSum = 0U;

    if (result == 0) {
        return false;
    }

    if (!AdcDmaScope_Capture(sampleRateHz, sampleCount, &frame) ||
        (frame.sampleCount == 0U)) {
        return false;
    }

    result->sampleRateHz = frame.sampleRateHz;
    result->sampleCount = frame.sampleCount;
    result->minRaw = 0xFFFFU;
    result->maxRaw = 0U;

    for (uint16_t i = 0U; i < frame.sampleCount; i++) {
        uint16_t sample = frame.samples[i];
        int32_t calibratedMv;

        if (sample < result->minRaw) {
            result->minRaw = sample;
        }
        if (sample > result->maxRaw) {
            result->maxRaw = sample;
        }
        sum += sample;
        calibratedMv = RawToCalibratedMillivolt(sample);
        squareSum += (uint64_t) ((int64_t) calibratedMv * calibratedMv);
    }

    result->avgRaw = (uint16_t) (sum / frame.sampleCount);
    result->pkpkRaw = result->maxRaw - result->minRaw;

    for (uint16_t i = 0U; i < frame.sampleCount; i++) {
        int32_t acRaw = (int32_t) frame.samples[i] - (int32_t) result->avgRaw;
        acSquareSum += (uint64_t) ((int64_t) acRaw * acRaw);
    }

    result->minMv = RawToCalibratedMillivolt(result->minRaw);
    result->maxMv = RawToCalibratedMillivolt(result->maxRaw);
    result->avgMv = RawToCalibratedMillivolt(result->avgRaw);
    result->pkpkMv = RawDeltaToCalibratedMillivolt(result->pkpkRaw);
    result->rmsMv = IsqrtU64(squareSum / frame.sampleCount);
    result->acRmsMv = RawDeltaToCalibratedMillivolt(
        IsqrtU64(acSquareSum / frame.sampleCount));
    return true;
}

bool RmsMeter_CalibrateZero(uint32_t sampleRateHz, uint16_t sampleCount)
{
    RmsMeter_Result_t result;

    if (!RmsMeter_Measure(sampleRateHz, sampleCount, &result)) {
        return false;
    }
    g_calibration.zeroRaw = result.avgRaw;
    return true;
}

bool RmsMeter_SetZeroRaw(int32_t zeroRaw)
{
    if ((zeroRaw < 0) || (zeroRaw > ADC_FML_FULL_SCALE_RAW)) {
        return false;
    }
    g_calibration.zeroRaw = zeroRaw;
    return true;
}

bool RmsMeter_SetGainPermille(uint32_t gainPermille)
{
    if ((gainPermille >= 100U) && (gainPermille <= 10000U)) {
        g_calibration.gainPermille = gainPermille;
        return true;
    }
    return false;
}

RmsMeter_Calibration_t RmsMeter_GetCalibration(void)
{
    return g_calibration;
}
