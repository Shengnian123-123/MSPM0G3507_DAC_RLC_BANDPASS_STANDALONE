#include "signal_spectral.h"

static const int16_t g_sin64[SIGNAL_SPECTRAL_DFT_COUNT] = {
    0, 3212, 6393, 9512, 12540, 15447, 18205, 20787,
    23170, 25330, 27245, 28898, 30274, 31357, 32138, 32610,
    32767, 32610, 32138, 31357, 30274, 28898, 27245, 25330,
    23170, 20787, 18205, 15447, 12540, 9512, 6393, 3212,
    0, -3212, -6393, -9512, -12540, -15447, -18205, -20787,
    -23170, -25330, -27245, -28898, -30274, -31357, -32138, -32610,
    -32767, -32610, -32138, -31357, -30274, -28898, -27245, -25330,
    -23170, -20787, -18205, -15447, -12540, -9512, -6393, -3212,
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

static uint64_t RatioPermilleSquared(uint64_t numerator, uint64_t denominator)
{
    uint64_t quotient;
    uint64_t remainder;

    if (denominator == 0U) {
        return 0U;
    }

    quotient = numerator / denominator;
    remainder = numerator % denominator;
    if (quotient > (0xFFFFFFFFFFFFFFFFULL / 1000000ULL)) {
        return 0xFFFFFFFFFFFFFFFFULL;
    }

    return (quotient * 1000000ULL) +
        ((remainder * 1000000ULL) / denominator);
}

static uint16_t AverageRaw(const uint16_t *samples, uint16_t count)
{
    uint32_t sum = 0U;

    for (uint16_t i = 0U; i < count; i++) {
        sum += samples[i];
    }
    return (uint16_t) (sum / count);
}

static int16_t Sin64(uint16_t index)
{
    return g_sin64[index & (SIGNAL_SPECTRAL_DFT_COUNT - 1U)];
}

static int16_t Cos64(uint16_t index)
{
    return Sin64((uint16_t) (index + 16U));
}

static bool FindFirstRising(const uint16_t *samples, uint16_t count,
    uint16_t *index)
{
    uint16_t threshold;
    bool previousHigh;

    if ((samples == 0) || (index == 0) || (count < 2U)) {
        return false;
    }

    threshold = AverageRaw(samples, count);
    previousHigh = (samples[0] >= threshold);

    for (uint16_t i = 1U; i < count; i++) {
        bool high = (samples[i] >= threshold);

        if (high && !previousHigh) {
            *index = i;
            return true;
        }
        previousHigh = high;
    }

    return false;
}

bool SignalSpectral_DftBin64(const uint16_t *samples, uint16_t bin,
    uint32_t sampleRateHz, SignalSpectral_BinResult_t *result)
{
    uint16_t avg;
    int64_t accCos = 0;
    int64_t accSin = 0;
    int64_t scaledCos;
    int64_t scaledSin;
    uint64_t power;

    if ((samples == 0) || (result == 0) ||
        (bin >= (SIGNAL_SPECTRAL_DFT_COUNT / 2U))) {
        return false;
    }

    avg = AverageRaw(samples, SIGNAL_SPECTRAL_DFT_COUNT);
    for (uint16_t i = 0U; i < SIGNAL_SPECTRAL_DFT_COUNT; i++) {
        uint16_t phase = (uint16_t) ((bin * i) &
            (SIGNAL_SPECTRAL_DFT_COUNT - 1U));
        int32_t centered = (int32_t) samples[i] - (int32_t) avg;

        accCos += ((int64_t) centered) * Cos64(phase);
        accSin -= ((int64_t) centered) * Sin64(phase);
    }

    scaledCos = accCos / (int64_t) SIGNAL_SPECTRAL_DFT_COUNT;
    scaledSin = accSin / (int64_t) SIGNAL_SPECTRAL_DFT_COUNT;
    power = (uint64_t) (scaledCos * scaledCos) +
        (uint64_t) (scaledSin * scaledSin);

    result->bin = bin;
    result->binFrequencyHz =
        ((uint32_t) bin * sampleRateHz) / SIGNAL_SPECTRAL_DFT_COUNT;
    result->power = power;
    result->magnitudeRaw = IsqrtU64(power) / 32768UL;
    return true;
}

bool SignalSpectral_GoertzelHz64(const uint16_t *samples, uint32_t targetHz,
    uint32_t sampleRateHz, SignalSpectral_BinResult_t *result)
{
    uint32_t roundedBin;

    if ((samples == 0) || (result == 0) || (targetHz == 0U) ||
        (sampleRateHz == 0U) || (targetHz >= (sampleRateHz / 2UL))) {
        return false;
    }

    roundedBin =
        ((targetHz * SIGNAL_SPECTRAL_DFT_COUNT) + (sampleRateHz / 2UL)) /
        sampleRateHz;
    if ((roundedBin == 0U) ||
        (roundedBin >= (SIGNAL_SPECTRAL_DFT_COUNT / 2U))) {
        return false;
    }

    return SignalSpectral_DftBin64(samples, (uint16_t) roundedBin, sampleRateHz,
        result);
}

bool SignalSpectral_Thd64(const uint16_t *samples, uint32_t fundamentalHz,
    uint32_t sampleRateHz, SignalSpectral_ThdResult_t *result)
{
    SignalSpectral_BinResult_t fundamental;
    uint64_t harmonicPower = 0U;

    if ((samples == 0) || (result == 0) ||
        !SignalSpectral_GoertzelHz64(samples, fundamentalHz, sampleRateHz,
            &fundamental) ||
        (fundamental.power == 0U)) {
        return false;
    }

    for (uint16_t h = 2U; h <= SIGNAL_SPECTRAL_MAX_HARMONIC; h++) {
        uint16_t harmonicBin = (uint16_t) (fundamental.bin * h);
        SignalSpectral_BinResult_t harmonic;

        if (harmonicBin >= (SIGNAL_SPECTRAL_DFT_COUNT / 2U)) {
            break;
        }
        if (SignalSpectral_DftBin64(samples, harmonicBin, sampleRateHz,
                &harmonic)) {
            harmonicPower += harmonic.power;
        }
    }

    result->fundamentalBin = fundamental.bin;
    result->fundamentalHz = fundamental.binFrequencyHz;
    result->fundamentalPower = fundamental.power;
    result->harmonicPower = harmonicPower;
    result->thdPermille =
        IsqrtU64(RatioPermilleSquared(harmonicPower, fundamental.power));
    return true;
}

bool SignalSpectral_PhaseDiffDeg(const uint16_t *samplesA,
    const uint16_t *samplesB, uint16_t count, uint32_t signalFrequencyHz,
    uint32_t sampleRateHz, int16_t *phaseDeg)
{
    uint16_t edgeA;
    uint16_t edgeB;
    int32_t sampleDiff;
    int32_t phase;

    if ((samplesA == 0) || (samplesB == 0) || (phaseDeg == 0) ||
        (signalFrequencyHz == 0U) || (sampleRateHz == 0U) ||
        !FindFirstRising(samplesA, count, &edgeA) ||
        !FindFirstRising(samplesB, count, &edgeB)) {
        return false;
    }

    sampleDiff = (int32_t) edgeB - (int32_t) edgeA;
    phase = (sampleDiff * 360L * (int32_t) signalFrequencyHz) /
        (int32_t) sampleRateHz;

    while (phase > 180L) {
        phase -= 360L;
    }
    while (phase < -180L) {
        phase += 360L;
    }

    *phaseDeg = (int16_t) phase;
    return true;
}
