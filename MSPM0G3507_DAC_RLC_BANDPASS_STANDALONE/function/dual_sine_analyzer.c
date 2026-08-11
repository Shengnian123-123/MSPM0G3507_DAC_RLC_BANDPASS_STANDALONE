#include "dual_sine_analyzer.h"
#include "adc_dma_scope.h"
#include "adc_fml.h"

#define DUAL_SINE_MAX_SAMPLES       (1024U)
#define DUAL_SINE_MIN_SAMPLES       (256U)
#define DUAL_SINE_MIN_FREQ_HZ       (1000UL)
#define DUAL_SINE_MAX_FREQ_HZ       (100000UL)
#define DUAL_SINE_MIN_SEPARATION_HZ (1000UL)
#define DUAL_SINE_MIN_POWER_RATIO   (25ULL)
#define DUAL_SINE_SINE_BITS         (8U)
#define DUAL_SINE_SINE_SIZE         (1U << DUAL_SINE_SINE_BITS)
#define DUAL_SINE_PHASE_BITS        (16U)

static int32_t g_fftReal[DUAL_SINE_MAX_SAMPLES];
static int32_t g_fftImag[DUAL_SINE_MAX_SAMPLES];
static uint64_t g_power[DUAL_SINE_MAX_SAMPLES / 2U];
static const int16_t g_sine[DUAL_SINE_SINE_SIZE] = {
    0, 804, 1608, 2410, 3212, 4011, 4808, 5602, 6393, 7179, 7962, 8739, 9512, 10278, 11039, 11793,
    12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530, 18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
    23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790, 27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
    30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971, 32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
    32767, 32757, 32728, 32678, 32609, 32521, 32412, 32285, 32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
    30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683, 27245, 26790, 26319, 25832, 25329, 24811, 24279, 23731,
    23170, 22594, 22005, 21403, 20787, 20159, 19519, 18868, 18204, 17530, 16846, 16151, 15446, 14732, 14010, 13279,
    12539, 11793, 11039, 10278, 9512, 8739, 7962, 7179, 6393, 5602, 4808, 4011, 3212, 2410, 1608, 804,
    0, -804, -1608, -2410, -3212, -4011, -4808, -5602, -6393, -7179, -7962, -8739, -9512, -10278, -11039, -11793,
    -12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530, -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
    -23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790, -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956,
    -30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971, -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
    -32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285, -32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
    -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683, -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731,
    -23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868, -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
    -12539, -11793, -11039, -10278, -9512, -8739, -7962, -7179, -6393, -5602, -4808, -4011, -3212, -2410, -1608, -804,
};

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

static int16_t SinQ15(uint16_t phase)
{
    return g_sine[phase >> (DUAL_SINE_PHASE_BITS - DUAL_SINE_SINE_BITS)];
}

static int16_t CosQ15(uint16_t phase)
{
    return SinQ15((uint16_t) (phase + 16384U));
}

static bool IsPowerOfTwo(uint16_t value)
{
    return (value >= DUAL_SINE_MIN_SAMPLES) &&
        (value <= DUAL_SINE_MAX_SAMPLES) &&
        ((value & (value - 1U)) == 0U);
}

static uint16_t AverageRaw(const uint16_t *samples, uint16_t count)
{
    uint64_t sum = 0U;

    for (uint16_t i = 0U; i < count; i++) {
        sum += samples[i];
    }
    return (uint16_t) (sum / count);
}

static void Fft(uint16_t count)
{
    uint16_t j = 0U;

    for (uint16_t i = 0U; i < count; i++) {
        if (i < j) {
            int32_t temp = g_fftReal[i];
            g_fftReal[i] = g_fftReal[j];
            g_fftReal[j] = temp;
        }
        uint16_t bit = count >> 1U;
        while ((bit != 0U) && ((j & bit) != 0U)) {
            j ^= bit;
            bit >>= 1U;
        }
        j ^= bit;
    }

    for (uint16_t length = 2U; length <= count; length <<= 1U) {
        uint16_t half = length >> 1U;
        uint32_t phaseStep = 65536UL / length;

        for (uint16_t base = 0U; base < count; base += length) {
            for (uint16_t offset = 0U; offset < half; offset++) {
                uint16_t phase = (uint16_t) (offset * phaseStep);
                int32_t wr = CosQ15(phase);
                int32_t wi = -SinQ15(phase);
                int32_t br = g_fftReal[base + offset + half];
                int32_t bi = g_fftImag[base + offset + half];
                int32_t tr = (int32_t) ((((int64_t) br * wr) -
                    ((int64_t) bi * wi)) >> 15U);
                int32_t ti = (int32_t) ((((int64_t) br * wi) +
                    ((int64_t) bi * wr)) >> 15U);
                int32_t ar = g_fftReal[base + offset];
                int32_t ai = g_fftImag[base + offset];

                g_fftReal[base + offset] = ar + tr;
                g_fftImag[base + offset] = ai + ti;
                g_fftReal[base + offset + half] = ar - tr;
                g_fftImag[base + offset + half] = ai - ti;
            }
        }
    }
}

static uint16_t FindPeakBin(uint16_t firstBin, uint16_t lastBin,
    uint16_t excludedBin, uint16_t exclusionBins, uint16_t count)
{
    uint16_t bestBin = 0U;
    uint64_t bestPower = 0U;

    for (uint16_t bin = firstBin; bin <= lastBin; bin++) {
        uint16_t distance = (bin > excludedBin) ?
            (bin - excludedBin) : (excludedBin - bin);

        if ((excludedBin != 0U) && (distance < exclusionBins)) {
            continue;
        }
        if ((bin == 0U) || (bin >= (count / 2U - 1U))) {
            continue;
        }
        if ((g_power[bin] >= g_power[bin - 1U]) &&
            (g_power[bin] >= g_power[bin + 1U]) &&
            (g_power[bin] > bestPower)) {
            bestPower = g_power[bin];
            bestBin = bin;
        }
    }
    return bestBin;
}

static uint64_t TonePower(const uint16_t *samples, uint16_t count,
    uint32_t sampleRateHz, uint16_t dcRaw, uint64_t frequencyMilliHz)
{
    int64_t sumCos = 0LL;
    int64_t sumSin = 0LL;
    uint32_t phaseStep = (uint32_t) ((frequencyMilliHz <<
        DUAL_SINE_PHASE_BITS) / ((uint64_t) sampleRateHz * 1000ULL));

    for (uint16_t i = 0U; i < count; i++) {
        uint16_t phase = (uint16_t) (phaseStep * i);
        int32_t centered = (int32_t) samples[i] - (int32_t) dcRaw;

        sumCos += (int64_t) centered * CosQ15(phase);
        sumSin -= (int64_t) centered * SinQ15(phase);
    }

    return (uint64_t) (sumCos * sumCos) +
        (uint64_t) (sumSin * sumSin);
}

static uint64_t RefineFrequencyMilliHz(const uint16_t *samples,
    uint16_t count, uint32_t sampleRateHz, uint16_t dcRaw, uint16_t peakBin)
{
    uint64_t binWidthMilliHz = ((uint64_t) sampleRateHz * 1000ULL) / count;
    uint64_t centerMilliHz = (uint64_t) peakBin * binWidthMilliHz;
    uint64_t searchStepMilliHz = binWidthMilliHz / 32ULL;
    uint64_t bestFrequencyMilliHz = centerMilliHz;
    uint64_t bestPower = 0ULL;

    if (searchStepMilliHz == 0ULL) {
        searchStepMilliHz = 1ULL;
    }
    /* A 33-point local DFT search removes most rectangular-window bin bias. */
    for (int32_t offset = -16; offset <= 16; offset++) {
        int64_t candidate = (int64_t) centerMilliHz +
            ((int64_t) offset * (int64_t) searchStepMilliHz);
        uint64_t power;

        if (candidate <= 0LL) {
            continue;
        }
        power = TonePower(samples, count, sampleRateHz, dcRaw,
            (uint64_t) candidate);
        if (power > bestPower) {
            bestPower = power;
            bestFrequencyMilliHz = (uint64_t) candidate;
        }
    }
    return bestFrequencyMilliHz;
}

static uint32_t MilliHzToHz(uint64_t frequencyMilliHz)
{
    return (uint32_t) ((frequencyMilliHz + 500ULL) / 1000ULL);
}

static int16_t Atan2Deg(int64_t y, int64_t x)
{
    int64_t absY = (y < 0LL) ? -y : y;
    int64_t angle;

    if ((x == 0LL) && (y == 0LL)) {
        return 0;
    }
    if (x >= 0LL) {
        int64_t ratio = ((x - absY) * 1000LL) / (x + absY + 1LL);
        angle = 45LL - ((45LL * ratio) / 1000LL);
    } else {
        int64_t ratio = ((x + absY) * 1000LL) / (absY - x + 1LL);
        angle = 135LL - ((45LL * ratio) / 1000LL);
    }
    if (y < 0LL) {
        angle = -angle;
    }
    return (int16_t) angle;
}

static void EstimateTone(const uint16_t *samples, uint16_t count,
    uint32_t sampleRateHz, uint16_t dcRaw, uint64_t frequencyMilliHz,
    uint16_t *amplitudeRaw, int16_t *phaseDeg)
{
    int64_t sumCos = 0LL;
    int64_t sumSin = 0LL;
    uint32_t phaseStep = (uint32_t) ((frequencyMilliHz <<
        DUAL_SINE_PHASE_BITS) / ((uint64_t) sampleRateHz * 1000ULL));

    for (uint16_t i = 0U; i < count; i++) {
        uint16_t phase = (uint16_t) (phaseStep * i);
        int32_t centered = (int32_t) samples[i] - (int32_t) dcRaw;

        sumCos += (int64_t) centered * CosQ15(phase);
        sumSin -= (int64_t) centered * SinQ15(phase);
    }

    uint64_t magnitude = IsqrtU64((uint64_t) (sumCos * sumCos) +
        (uint64_t) (sumSin * sumSin));
    uint64_t amplitude = (2ULL * magnitude) /
        ((uint64_t) count * 32767ULL);

    if (amplitude > 0xFFFFULL) {
        amplitude = 0xFFFFULL;
    }
    *amplitudeRaw = (uint16_t) amplitude;
    *phaseDeg = Atan2Deg(sumSin, sumCos);
    if (*phaseDeg < 0) {
        *phaseDeg = (int16_t) (*phaseDeg + 360);
    }
}

static void ClearResult(DualSine_Result_t *result)
{
    result->valid = false;
    result->usedDma = false;
    result->sampleRateHz = 0U;
    result->sampleCount = 0U;
    result->frequency1Hz = 0U;
    result->frequency2Hz = 0U;
    result->amplitude1Raw = 0U;
    result->amplitude2Raw = 0U;
    result->amplitude1Mv = 0U;
    result->amplitude2Mv = 0U;
    result->phase1Deg = 0;
    result->phase2Deg = 0;
    result->dcRaw = 0U;
    result->noiseRaw = 0U;
    result->separationHz = 0U;
    result->rejectReason = "not_analyzed";
}

bool DualSine_AnalyzeSamples(const uint16_t *samples, uint16_t sampleCount,
    uint32_t sampleRateHz, DualSine_Result_t *result)
{
    uint16_t dcRaw;
    uint16_t firstBin;
    uint16_t lastBin;
    uint16_t minSeparationBins;
    uint16_t firstPeak;
    uint16_t secondPeak;
    uint32_t frequency1Hz;
    uint32_t frequency2Hz;
    uint64_t frequency1MilliHz;
    uint64_t frequency2MilliHz;

    if ((samples == 0) || (result == 0) || (sampleRateHz == 0U) ||
        !IsPowerOfTwo(sampleCount)) {
        return false;
    }

    ClearResult(result);
    dcRaw = AverageRaw(samples, sampleCount);
    result->dcRaw = dcRaw;
    result->sampleRateHz = sampleRateHz;
    result->sampleCount = sampleCount;

    for (uint16_t i = 0U; i < sampleCount; i++) {
        g_fftReal[i] = (int32_t) samples[i] - (int32_t) dcRaw;
        g_fftImag[i] = 0;
    }
    Fft(sampleCount);

    for (uint16_t bin = 0U; bin < (sampleCount / 2U); bin++) {
        g_power[bin] = (uint64_t) g_fftReal[bin] * g_fftReal[bin] +
            (uint64_t) g_fftImag[bin] * g_fftImag[bin];
    }

    firstBin = (uint16_t) (((uint64_t) DUAL_SINE_MIN_FREQ_HZ *
        sampleCount) / sampleRateHz);
    if (firstBin < 1U) {
        firstBin = 1U;
    }
    lastBin = (uint16_t) (((uint64_t) DUAL_SINE_MAX_FREQ_HZ *
        sampleCount) / sampleRateHz);
    if (lastBin >= (sampleCount / 2U - 1U)) {
        lastBin = (uint16_t) (sampleCount / 2U - 2U);
    }
    if ((firstBin < 1U) || (lastBin <= firstBin)) {
        result->rejectReason = "frequency_range_invalid";
        return true;
    }

    minSeparationBins = (uint16_t) (((uint64_t)
        DUAL_SINE_MIN_SEPARATION_HZ * sampleCount) / sampleRateHz);
    if (minSeparationBins < 3U) {
        minSeparationBins = 3U;
    }

    firstPeak = FindPeakBin(firstBin, lastBin, 0U, 0U, sampleCount);
    if (firstPeak == 0U) {
        result->rejectReason = "first_frequency_not_found";
        return true;
    }
    secondPeak = FindPeakBin(firstBin, lastBin, firstPeak,
        minSeparationBins, sampleCount);
    if (secondPeak == 0U) {
        result->rejectReason = "second_frequency_not_found";
        return true;
    }
    if (g_power[secondPeak] * DUAL_SINE_MIN_POWER_RATIO <
        g_power[firstPeak]) {
        result->rejectReason = "second_signal_too_small";
        return true;
    }

    frequency1MilliHz = RefineFrequencyMilliHz(samples, sampleCount,
        sampleRateHz, dcRaw, firstPeak);
    frequency2MilliHz = RefineFrequencyMilliHz(samples, sampleCount,
        sampleRateHz, dcRaw, secondPeak);
    if (frequency1MilliHz > frequency2MilliHz) {
        uint64_t temp = frequency1MilliHz;
        frequency1MilliHz = frequency2MilliHz;
        frequency2MilliHz = temp;
    }
    frequency1Hz = MilliHzToHz(frequency1MilliHz);
    frequency2Hz = MilliHzToHz(frequency2MilliHz);
    if ((frequency1Hz < DUAL_SINE_MIN_FREQ_HZ) ||
        (frequency2Hz > DUAL_SINE_MAX_FREQ_HZ) ||
        ((frequency2Hz - frequency1Hz) < DUAL_SINE_MIN_SEPARATION_HZ)) {
        result->rejectReason = "frequency_separation_too_small";
        return true;
    }

    EstimateTone(samples, sampleCount, sampleRateHz, dcRaw, frequency1MilliHz,
        &result->amplitude1Raw, &result->phase1Deg);
    EstimateTone(samples, sampleCount, sampleRateHz, dcRaw, frequency2MilliHz,
        &result->amplitude2Raw, &result->phase2Deg);
    result->frequency1Hz = frequency1Hz;
    result->frequency2Hz = frequency2Hz;
    result->amplitude1Mv = ADC_Fml_RawToMillivolt(result->amplitude1Raw);
    result->amplitude2Mv = ADC_Fml_RawToMillivolt(result->amplitude2Raw);
    result->separationHz = frequency2Hz - frequency1Hz;
    result->valid = true;
    result->rejectReason = "none";
    return true;
}

bool DualSine_CaptureAndAnalyze(uint32_t sampleRateHz, uint16_t sampleCount,
    DualSine_Result_t *result)
{
    static AdcDmaScope_Frame_t frame;

    if ((result == 0) || !IsPowerOfTwo(sampleCount) ||
        !AdcDmaScope_Capture(sampleRateHz, sampleCount, &frame)) {
        return false;
    }
    if (!DualSine_AnalyzeSamples(frame.samples, frame.sampleCount,
            frame.sampleRateHz, result)) {
        return false;
    }
    result->usedDma = frame.usedDma;
    return true;
}
