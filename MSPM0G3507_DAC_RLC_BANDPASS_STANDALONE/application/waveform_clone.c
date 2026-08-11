#include "waveform_clone.h"
#include "adc_dma_scope.h"
#include "ad9833.h"
#include "ad9959.h"
#include "dual_sine_analyzer.h"
#include "frequency_meter.h"
#include "signal_measure.h"
#include "signal_spectral.h"
#include "uart.h"

#define WAVEFORM_CLONE_DEFAULT_RATE_HZ       (2000000UL)
#define WAVEFORM_CLONE_DEFAULT_COUNT         (1024U)
#define WAVEFORM_CLONE_DUAL_DEFAULT_RATE_HZ  (200000UL)
#define WAVEFORM_CLONE_MIN_PKPK_RAW          (40U)
#define WAVEFORM_CLONE_MIN_CYCLES            (2U)
#define WAVEFORM_CLONE_MIN_SAMPLES_PER_CYCLE (10U)
#define WAVEFORM_CLONE_MID_SAMPLES_PER_CYCLE (5U)
#define WAVEFORM_CLONE_TARGET_SAMPLES_PER_CYCLE (32U)
#define WAVEFORM_CLONE_LOW_MAX_HZ            (100000UL)
#define WAVEFORM_CLONE_MID_MAX_HZ            (200000UL)
#define WAVEFORM_CLONE_HF_AUTO_MIN_HZ        (200000UL)
#define WAVEFORM_CLONE_HF_AUTO_MAX_HZ        (2000000UL)
#define WAVEFORM_CLONE_ADC_MAX_SHAPE_HZ      WAVEFORM_CLONE_HF_AUTO_MAX_HZ
#define WAVEFORM_CLONE_DEFAULT_FULL_SCALE_MV (3300U)
#define WAVEFORM_CLONE_DEFAULT_GAIN_PERMILLE (1000UL)
#define WAVEFORM_CLONE_HF_DIVIDER_DEFAULT    (1UL)
#define WAVEFORM_CLONE_HF_INPUT_MAX_HZ       (60000000UL)
#define WAVEFORM_CLONE_FREQ_AGE_LIMIT_MS     (100U)

static uint32_t g_ad9833FullScaleMv = WAVEFORM_CLONE_DEFAULT_FULL_SCALE_MV;
static uint32_t g_ad9959FullScaleMv = WAVEFORM_CLONE_DEFAULT_FULL_SCALE_MV;
static uint32_t g_outputGainPermille = WAVEFORM_CLONE_DEFAULT_GAIN_PERMILLE;
static uint32_t g_highFreqDivider = WAVEFORM_CLONE_HF_DIVIDER_DEFAULT;

static bool StrEquals(const char *left, const char *right)
{
    while ((*left != '\0') && (*right != '\0')) {
        if (*left != *right) {
            return false;
        }
        left++;
        right++;
    }
    return ((*left == '\0') && (*right == '\0'));
}

static bool StartsWith(const char *text, const char *prefix)
{
    while (*prefix != '\0') {
        if (*text != *prefix) {
            return false;
        }
        text++;
        prefix++;
    }
    return true;
}

static bool ParseU32Arg(const char **text, uint32_t *value)
{
    uint32_t result = 0U;
    bool hasDigit = false;

    while (**text == ' ') {
        (*text)++;
    }
    while ((**text >= '0') && (**text <= '9')) {
        uint32_t digit = (uint32_t) (**text - '0');

        if (result > ((0xFFFFFFFFUL - digit) / 10UL)) {
            return false;
        }
        result = (result * 10UL) + digit;
        hasDigit = true;
        (*text)++;
    }
    *value = result;
    return hasDigit;
}

static bool EndOfArgs(const char *text)
{
    while (*text == ' ') {
        text++;
    }
    return (*text == '\0');
}

static bool ParseRateCount(const char *text, uint32_t *rate, uint32_t *count)
{
    if (!ParseU32Arg(&text, rate)) {
        return false;
    }
    if (!ParseU32Arg(&text, count)) {
        return false;
    }
    return EndOfArgs(text);
}

static bool ParseChannelArg(const char *line, const char *prefix,
    uint8_t *channel, const char **arg)
{
    uint32_t index = 0U;

    while (prefix[index] != '\0') {
        if (line[index] != prefix[index]) {
            return false;
        }
        index++;
    }
    if ((line[index] < '0') || (line[index] > '3')) {
        return false;
    }
    *channel = (uint8_t) (line[index] - '0');
    *arg = &line[index + 1U];
    return true;
}

static uint32_t RequiredSamplesPerCycle(uint32_t frequencyHz)
{
    if (frequencyHz <= WAVEFORM_CLONE_LOW_MAX_HZ) {
        return WAVEFORM_CLONE_MIN_SAMPLES_PER_CYCLE;
    }
    if (frequencyHz <= WAVEFORM_CLONE_MID_MAX_HZ) {
        return WAVEFORM_CLONE_MID_SAMPLES_PER_CYCLE;
    }
    (void) frequencyHz;
    return 1UL;
}

static uint16_t ClampSampleCount(uint32_t count)
{
    if (count == 0U) {
        return 1U;
    }
    if (count > ADC_DMA_SCOPE_MAX_SAMPLES) {
        return ADC_DMA_SCOPE_MAX_SAMPLES;
    }
    return (uint16_t) count;
}

static uint16_t ScaleToU16(uint32_t value, uint32_t fullScale,
    uint32_t maxOutput)
{
    uint32_t scaled;

    if (fullScale == 0U) {
        fullScale = WAVEFORM_CLONE_DEFAULT_FULL_SCALE_MV;
    }
    scaled = (value * maxOutput) / fullScale;
    if (scaled > maxOutput) {
        scaled = maxOutput;
    }
    if ((value != 0U) && (scaled == 0U)) {
        scaled = 1U;
    }
    return (uint16_t) scaled;
}

static void SetDdsAmplitudes(WaveformClone_Result_t *result)
{
    if (result == 0) {
        return;
    }
    result->ad9833Amplitude = (uint8_t) ScaleToU16(result->targetPkpkMv,
        g_ad9833FullScaleMv, 255U);
    result->ad9959Amplitude = ScaleToU16(result->targetPkpkMv,
        g_ad9959FullScaleMv, AD9959_MAX_AMPLITUDE);
}

static uint16_t CalculateTargetPkpkMv(uint16_t inputPkpkMv)
{
    uint64_t target =
        (((uint64_t) inputPkpkMv) * g_outputGainPermille) / 1000ULL;

    if (target > 0xFFFFULL) {
        target = 0xFFFFULL;
    }
    return (uint16_t) target;
}

static void SetTargetLevels(WaveformClone_Result_t *result,
    uint16_t targetPkpkMv)
{
    int16_t half = (int16_t) (targetPkpkMv / 2U);

    result->targetPkpkMv = targetPkpkMv;
    result->targetHighMv = half;
    result->targetLowMv  = (int16_t) -half;
}

static uint16_t CalculateRailCountPermille(const uint16_t *samples,
    uint16_t count, uint16_t minRaw, uint16_t maxRaw)
{
    uint16_t pkpk = maxRaw - minRaw;
    uint16_t lowLimit;
    uint16_t highLimit;
    uint16_t railCount = 0U;

    if ((samples == 0) || (count == 0U) || (pkpk == 0U)) {
        return 0U;
    }

    lowLimit = minRaw + (pkpk / 12U);
    highLimit = maxRaw - (pkpk / 12U);
    for (uint16_t i = 0U; i < count; i++) {
        if ((samples[i] <= lowLimit) || (samples[i] >= highLimit)) {
            railCount++;
        }
    }
    return (uint16_t) (((uint32_t) railCount * 1000UL) / count);
}

static uint16_t CalculateMadPermille(const uint16_t *samples, uint16_t count,
    uint16_t avgRaw, uint16_t pkpkRaw)
{
    uint32_t sumAbs = 0U;

    if ((samples == 0) || (count == 0U) || (pkpkRaw == 0U)) {
        return 0U;
    }

    for (uint16_t i = 0U; i < count; i++) {
        if (samples[i] >= avgRaw) {
            sumAbs += (uint32_t) (samples[i] - avgRaw);
        } else {
            sumAbs += (uint32_t) (avgRaw - samples[i]);
        }
    }

    return (uint16_t) ((sumAbs * 1000UL) /
                       (((uint32_t) count) * pkpkRaw));
}

static uint16_t CalculateBandCountPermille(const uint16_t *samples,
    uint16_t count, uint16_t minRaw, uint16_t maxRaw, uint16_t lowPermille,
    uint16_t highPermille)
{
    uint16_t pkpk = maxRaw - minRaw;
    uint16_t bandCount = 0U;

    if ((samples == 0) || (count == 0U) || (pkpk == 0U) ||
        (lowPermille > highPermille)) {
        return 0U;
    }

    for (uint16_t i = 0U; i < count; i++) {
        uint16_t norm = (uint16_t) ((((uint32_t) (samples[i] - minRaw)) *
                                     1000UL) / pkpk);

        if ((norm >= lowPermille) && (norm <= highPermille)) {
            bandCount++;
        }
    }
    return (uint16_t) ((((uint32_t) bandCount) * 1000UL) / count);
}

static uint16_t CalculateEdgeBandPermille(const uint16_t *samples,
    uint16_t count, uint16_t minRaw, uint16_t maxRaw)
{
    uint16_t pkpk = maxRaw - minRaw;
    uint16_t edgeCount = 0U;

    if ((samples == 0) || (count == 0U) || (pkpk == 0U)) {
        return 0U;
    }

    for (uint16_t i = 0U; i < count; i++) {
        uint16_t norm = (uint16_t) ((((uint32_t) (samples[i] - minRaw)) *
                                     1000UL) / pkpk);

        if ((norm <= 100U) || (norm >= 900U)) {
            edgeCount++;
        }
    }
    return (uint16_t) ((((uint32_t) edgeCount) * 1000UL) / count);
}

static void CalculateSlopeStats(const uint16_t *samples, uint16_t count,
    uint16_t pkpkRaw, uint32_t frequencyHz, uint32_t sampleRateHz,
    uint16_t *flatPermille, uint16_t *steepPermille)
{
    uint32_t samplesPerCycle;
    uint32_t flatThreshold;
    uint32_t steepThreshold;
    uint16_t flatCount = 0U;
    uint16_t steepCount = 0U;
    uint16_t diffCount;

    if (flatPermille != 0) {
        *flatPermille = 0U;
    }
    if (steepPermille != 0) {
        *steepPermille = 0U;
    }
    if ((samples == 0) || (count < 2U) || (pkpkRaw == 0U) ||
        (frequencyHz == 0U) || (sampleRateHz == 0U)) {
        return;
    }

    samplesPerCycle = sampleRateHz / frequencyHz;
    if (samplesPerCycle == 0U) {
        samplesPerCycle = 1U;
    }

    flatThreshold = (((uint32_t) pkpkRaw) * 1200UL) /
        (samplesPerCycle * 1000UL);
    if (flatThreshold < 2U) {
        flatThreshold = 2U;
    }

    steepThreshold = (((uint32_t) pkpkRaw) * 6000UL) /
        (samplesPerCycle * 1000UL);
    if (steepThreshold < (((uint32_t) pkpkRaw) / 8UL)) {
        steepThreshold = ((uint32_t) pkpkRaw) / 8UL;
    }
    if (steepThreshold < 4U) {
        steepThreshold = 4U;
    }

    diffCount = count - 1U;
    for (uint16_t i = 1U; i < count; i++) {
        uint16_t diff = (samples[i] >= samples[i - 1U]) ?
            (samples[i] - samples[i - 1U]) :
            (samples[i - 1U] - samples[i]);

        if (diff <= flatThreshold) {
            flatCount++;
        }
        if (diff >= steepThreshold) {
            steepCount++;
        }
    }

    if (flatPermille != 0) {
        *flatPermille =
            (uint16_t) ((((uint32_t) flatCount) * 1000UL) / diffCount);
    }
    if (steepPermille != 0) {
        *steepPermille =
            (uint16_t) ((((uint32_t) steepCount) * 1000UL) / diffCount);
    }
}

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

static uint16_t CalculateHarmonicPermille(const uint16_t *samples,
    uint16_t count, uint32_t fundamentalHz, uint32_t sampleRateHz,
    uint8_t harmonic)
{
    SignalSpectral_BinResult_t fundamental;
    SignalSpectral_BinResult_t harmonicResult;
    uint32_t harmonicHz;
    uint64_t quotient;
    uint64_t remainder;
    uint64_t ratioPermillion;

    if ((samples == 0) || (count < SIGNAL_SPECTRAL_DFT_COUNT) ||
        (fundamentalHz == 0U) || (sampleRateHz == 0U) ||
        (harmonic < 2U)) {
        return 0U;
    }
    if (fundamentalHz > (0xFFFFFFFFUL / harmonic)) {
        return 0U;
    }
    harmonicHz = fundamentalHz * harmonic;
    if (harmonicHz >= (sampleRateHz / 2UL)) {
        return 0U;
    }

    if (!SignalSpectral_GoertzelHz64(samples, fundamentalHz, sampleRateHz,
            &fundamental) ||
        !SignalSpectral_GoertzelHz64(samples, harmonicHz, sampleRateHz,
            &harmonicResult) ||
        (fundamental.power == 0ULL)) {
        return 0U;
    }

    quotient = harmonicResult.power / fundamental.power;
    remainder = harmonicResult.power % fundamental.power;
    if (quotient > (0xFFFFFFFFFFFFFFFFULL / 1000000ULL)) {
        return 0xFFFFU;
    }
    ratioPermillion = (quotient * 1000000ULL) +
        ((remainder * 1000000ULL) / fundamental.power);
    if (ratioPermillion > 0xFFFFFFFFULL) {
        return 0xFFFFU;
    }
    return (uint16_t) IsqrtU64(ratioPermillion);
}

static WaveformClone_Type_t ClassifyWaveform(uint16_t rmsToPkpkPermille,
    uint16_t madToPkpkPermille, uint16_t railCountPermille,
    uint16_t dutyPermille, uint16_t edgeBandPermille,
    uint16_t flatSlopePermille, uint16_t steepSlopePermille,
    uint16_t harmonic3Permille, uint16_t harmonic5Permille)
{
    bool dutyLooksPossible = ((dutyPermille >= 120U) &&
        (dutyPermille <= 880U));

    if (dutyLooksPossible &&
        ((edgeBandPermille >= 600U) ||
            ((flatSlopePermille >= 450U) &&
                (steepSlopePermille >= 15U)) ||
            (madToPkpkPermille >= 340U) ||
            ((harmonic3Permille >= 210U) &&
                (rmsToPkpkPermille >= 330U)))) {
        return WAVEFORM_CLONE_TYPE_SQUARE;
    }

    if ((harmonic3Permille <= 75U) && (harmonic5Permille <= 75U) &&
        (madToPkpkPermille >= 275U) && (rmsToPkpkPermille >= 315U) &&
        (rmsToPkpkPermille <= 390U) && (railCountPermille >= 70U)) {
        return WAVEFORM_CLONE_TYPE_SINE;
    }

    if ((harmonic3Permille >= 55U) && (harmonic3Permille <= 180U) &&
        (harmonic5Permille <= 95U) &&
        (madToPkpkPermille >= 210U) && (madToPkpkPermille <= 285U) &&
        (rmsToPkpkPermille >= 245U) && (rmsToPkpkPermille <= 325U)) {
        return WAVEFORM_CLONE_TYPE_TRIANGLE;
    }

    if ((madToPkpkPermille >= 295U) && (rmsToPkpkPermille >= 325U) &&
        (edgeBandPermille >= 180U)) {
        return WAVEFORM_CLONE_TYPE_SINE;
    }
    if ((madToPkpkPermille <= 280U) && (rmsToPkpkPermille <= 320U) &&
        (edgeBandPermille <= 450U) && (flatSlopePermille < 420U)) {
        return WAVEFORM_CLONE_TYPE_TRIANGLE;
    }
    return WAVEFORM_CLONE_TYPE_UNKNOWN;
}

static AD9833_Waveform_t ToAd9833Waveform(WaveformClone_Type_t type)
{
    switch (type) {
        case WAVEFORM_CLONE_TYPE_TRIANGLE:
            return AD9833_WAVE_TRIANGLE;
        case WAVEFORM_CLONE_TYPE_SQUARE:
            return AD9833_WAVE_SQUARE;
        case WAVEFORM_CLONE_TYPE_SINE:
        default:
            return AD9833_WAVE_SINE;
    }
}

static bool IsAdcShapeFrequencyUsable(const SignalMeasure_Result_t *measure,
    uint32_t sampleRateHz, uint16_t sampleCount)
{
    uint32_t cycles;
    uint32_t requiredSamplesPerCycle;

    if ((measure == 0) || (measure->frequencyHz == 0U)) {
        return false;
    }
    if (measure->frequencyHz > WAVEFORM_CLONE_ADC_MAX_SHAPE_HZ) {
        return false;
    }

    requiredSamplesPerCycle = RequiredSamplesPerCycle(measure->frequencyHz);
    if (sampleRateHz < (measure->frequencyHz * requiredSamplesPerCycle)) {
        return false;
    }

    cycles = (((uint32_t) sampleCount) * measure->frequencyHz) / sampleRateHz;
    return (cycles >= WAVEFORM_CLONE_MIN_CYCLES);
}

static const char *GetRejectReason(const SignalMeasure_Result_t *measure,
    uint32_t sampleRateHz, uint16_t sampleCount)
{
    uint32_t cycles;
    uint32_t requiredSamplesPerCycle;

    if (measure == 0) {
        return "internal";
    }
    if (measure->pkpkRaw < WAVEFORM_CLONE_MIN_PKPK_RAW) {
        return "amplitude_too_small";
    }
    if (measure->frequencyHz == 0U) {
        return "frequency_not_found";
    }
    if (measure->frequencyHz > WAVEFORM_CLONE_ADC_MAX_SHAPE_HZ) {
        return "frequency_over_adc_shape_limit";
    }

    requiredSamplesPerCycle = RequiredSamplesPerCycle(measure->frequencyHz);
    if (sampleRateHz < (measure->frequencyHz * requiredSamplesPerCycle)) {
        return "sample_rate_too_low";
    }

    cycles = (((uint32_t) sampleCount) * measure->frequencyHz) / sampleRateHz;
    if (cycles < WAVEFORM_CLONE_MIN_CYCLES) {
        return "capture_window_too_short";
    }
    return "classification_threshold";
}

static void ApplyAdcFrequencyPolicy(WaveformClone_Result_t *result)
{
    if ((result == 0) || (result->frequencyHz == 0U)) {
        return;
    }

    if ((result->frequencyHz > WAVEFORM_CLONE_LOW_MAX_HZ) &&
        (result->frequencyHz <= WAVEFORM_CLONE_MID_MAX_HZ) &&
        (result->type == WAVEFORM_CLONE_TYPE_SQUARE)) {
        result->type = WAVEFORM_CLONE_TYPE_UNKNOWN;
        result->rejectReason = "mid_band_only_sine_triangle";
        return;
    }

    if ((result->frequencyHz > WAVEFORM_CLONE_MID_MAX_HZ) &&
        (result->frequencyHz <= WAVEFORM_CLONE_HF_AUTO_MAX_HZ)) {
        result->type = WAVEFORM_CLONE_TYPE_SINE;
        result->rejectReason = "adc_high_band_forced_sine";
    }
}

static void ClearResult(WaveformClone_Result_t *result)
{
    result->valid = false;
    result->usedDma = false;
    result->type = WAVEFORM_CLONE_TYPE_UNKNOWN;
    result->sampleRateHz = 0U;
    result->sampleCount = 0U;
    result->frequencyHz = 0U;
    result->minMv = 0U;
    result->maxMv = 0U;
    result->avgMv = 0U;
    result->pkpkMv = 0U;
    result->acRmsMv = 0U;
    result->targetPkpkMv = 0U;
    result->targetHighMv = 0;
    result->targetLowMv = 0;
    result->dutyPermille = 0U;
    result->rmsToPkpkPermille = 0U;
    result->madToPkpkPermille = 0U;
    result->railCountPermille = 0U;
    result->edgeBandPermille = 0U;
    result->centerBandPermille = 0U;
    result->flatSlopePermille = 0U;
    result->steepSlopePermille = 0U;
    result->harmonic3Permille = 0U;
    result->harmonic5Permille = 0U;
    result->rejectReason = "not_analyzed";
    result->ad9833Amplitude = 0U;
    result->ad9959Amplitude = 0U;
}

const char *WaveformClone_TypeText(WaveformClone_Type_t type)
{
    switch (type) {
        case WAVEFORM_CLONE_TYPE_SINE:
            return "sine";
        case WAVEFORM_CLONE_TYPE_TRIANGLE:
            return "triangle";
        case WAVEFORM_CLONE_TYPE_SQUARE:
            return "square";
        case WAVEFORM_CLONE_TYPE_UNKNOWN:
        default:
            return "unknown";
    }
}

bool WaveformClone_Analyze(uint32_t sampleRateHz, uint16_t sampleCount,
    WaveformClone_Result_t *result)
{
    static AdcDmaScope_Frame_t frame;
    SignalMeasure_Result_t measure;

    if (result == 0) {
        return false;
    }

    ClearResult(result);
    sampleCount = ClampSampleCount(sampleCount);

    if (!AdcDmaScope_Capture(sampleRateHz, sampleCount, &frame)) {
        return false;
    }
    if (!SignalMeasure_Analyze(frame.samples, frame.sampleCount,
            frame.sampleRateHz, &measure)) {
        return false;
    }

    result->sampleRateHz = frame.sampleRateHz;
    result->sampleCount = frame.sampleCount;
    result->usedDma = frame.usedDma;
    result->frequencyHz = measure.frequencyHz;
    result->minMv = measure.minMv;
    result->maxMv = measure.maxMv;
    result->avgMv = measure.avgMv;
    result->pkpkMv = measure.pkpkMv;
    result->acRmsMv = measure.acRmsMv;
    SetTargetLevels(result, CalculateTargetPkpkMv(measure.pkpkMv));
    SetDdsAmplitudes(result);
    result->dutyPermille = measure.dutyPermille;
    result->railCountPermille = CalculateRailCountPermille(frame.samples,
        frame.sampleCount, measure.minRaw, measure.maxRaw);
    result->edgeBandPermille = CalculateEdgeBandPermille(frame.samples,
        frame.sampleCount, measure.minRaw, measure.maxRaw);
    result->centerBandPermille = CalculateBandCountPermille(frame.samples,
        frame.sampleCount, measure.minRaw, measure.maxRaw, 450U, 550U);

    if ((measure.pkpkRaw < WAVEFORM_CLONE_MIN_PKPK_RAW) ||
        !IsAdcShapeFrequencyUsable(&measure, frame.sampleRateHz,
            frame.sampleCount)) {
        result->rejectReason = GetRejectReason(&measure, frame.sampleRateHz,
            frame.sampleCount);
        return true;
    }

    result->rmsToPkpkPermille =
        (uint16_t) (((uint32_t) measure.acRmsRaw * 1000UL) /
                    measure.pkpkRaw);
    result->madToPkpkPermille = CalculateMadPermille(frame.samples,
        frame.sampleCount, measure.avgRaw, measure.pkpkRaw);
    CalculateSlopeStats(frame.samples, frame.sampleCount, measure.pkpkRaw,
        measure.frequencyHz, frame.sampleRateHz, &result->flatSlopePermille,
        &result->steepSlopePermille);
    result->harmonic3Permille = CalculateHarmonicPermille(frame.samples,
        frame.sampleCount, measure.frequencyHz, frame.sampleRateHz, 3U);
    result->harmonic5Permille = CalculateHarmonicPermille(frame.samples,
        frame.sampleCount, measure.frequencyHz, frame.sampleRateHz, 5U);
    result->type = ClassifyWaveform(result->rmsToPkpkPermille,
        result->madToPkpkPermille, result->railCountPermille,
        result->dutyPermille, result->edgeBandPermille,
        result->flatSlopePermille, result->steepSlopePermille,
        result->harmonic3Permille, result->harmonic5Permille);
    ApplyAdcFrequencyPolicy(result);
    result->valid = (result->type != WAVEFORM_CLONE_TYPE_UNKNOWN);
    if (result->valid) {
        result->rejectReason = "none";
    } else if (result->rejectReason == 0) {
        result->rejectReason = "classification_threshold";
    } else if (StrEquals(result->rejectReason, "not_analyzed")) {
        result->rejectReason = "classification_threshold";
    }
    return true;
}

bool WaveformClone_OutputAd9833(const WaveformClone_Result_t *result)
{
    if ((result == 0) || !result->valid || (result->frequencyHz == 0U) ||
        (result->frequencyHz > AD9833_GetMaxOutputHz())) {
        return false;
    }
    (void) AD9833_SetAmplitude(result->ad9833Amplitude);
    return AD9833_SetOutput(ToAd9833Waveform(result->type),
        result->frequencyHz, 0U);
}

bool WaveformClone_OutputAd9959(uint8_t channel,
    const WaveformClone_Result_t *result)
{
    if ((result == 0) || !result->valid ||
        (result->type != WAVEFORM_CLONE_TYPE_SINE) ||
        (channel >= AD9959_CHANNEL_COUNT)) {
        return false;
    }
    return AD9959_SetSingleTone((AD9959_Channel_t) channel,
        result->frequencyHz, 0U, result->ad9959Amplitude);
}

static void PrintResult(const WaveformClone_Result_t *result)
{
    Uart_Printf("OK clone input=PA25 mode=%s type=%s valid=%u rate_hz=%lu count=%u freq_hz=%lu\r\n",
        result->usedDma ? "dma" : "blocking",
        WaveformClone_TypeText(result->type), result->valid ? 1U : 0U,
        (unsigned long) result->sampleRateHz, result->sampleCount,
        (unsigned long) result->frequencyHz);
    Uart_Printf("mv min=%u max=%u avg=%u pkpk=%u ac_rms=%u duty_permille=%u\r\n",
        result->minMv, result->maxMv, result->avgMv, result->pkpkMv,
        result->acRmsMv, result->dutyPermille);
    Uart_Printf("target center_mv=0 pkpk_mv=%u high_mv=%d low_mv=%d gain_permille=%lu\r\n",
        result->targetPkpkMv, result->targetHighMv, result->targetLowMv,
        (unsigned long) g_outputGainPermille);
    Uart_Printf("shape rms_pkpk_permille=%u mad_pkpk_permille=%u rail_permille=%u\r\n",
        result->rmsToPkpkPermille, result->madToPkpkPermille,
        result->railCountPermille);
    Uart_Printf("dds ad9833_amp=%u ad9833_full_mv=%lu ad9959_amp=%u ad9959_full_mv=%lu\r\n",
        result->ad9833Amplitude, (unsigned long) g_ad9833FullScaleMv,
        result->ad9959Amplitude, (unsigned long) g_ad9959FullScaleMv);
    Uart_Printf("shape2 edge_band_permille=%u center_band_permille=%u flat_slope_permille=%u steep_slope_permille=%u h3_permille=%u h5_permille=%u\r\n",
        result->edgeBandPermille, result->centerBandPermille,
        result->flatSlopePermille, result->steepSlopePermille,
        result->harmonic3Permille, result->harmonic5Permille);
    if (result->valid && (result->frequencyHz > WAVEFORM_CLONE_MID_MAX_HZ)) {
        Uart_WriteString("INFO clone adc_high_band=rough_forced_sine alias_possible\r\n");
    }
    if (!result->valid) {
        Uart_Printf("INFO clone reject=%s adc_low_max_hz=%lu adc_mid_max_hz=%lu adc_high_max_hz=%lu low_rate>=10x mid_rate>=5x high_rate>=1x cycles>=2\r\n",
            result->rejectReason,
            (unsigned long) WAVEFORM_CLONE_LOW_MAX_HZ,
            (unsigned long) WAVEFORM_CLONE_MID_MAX_HZ,
            (unsigned long) WAVEFORM_CLONE_ADC_MAX_SHAPE_HZ);
    }
}

static uint32_t SelectSampleRateForFrequency(uint32_t frequencyHz,
    uint16_t sampleCount)
{
    uint32_t rate;
    uint32_t maxRateForMinCycles;

    if (frequencyHz == 0U) {
        return WAVEFORM_CLONE_DEFAULT_RATE_HZ;
    }

    if (frequencyHz > (0xFFFFFFFFUL /
            WAVEFORM_CLONE_TARGET_SAMPLES_PER_CYCLE)) {
        rate = ADC_DMA_SCOPE_MAX_RATE_HZ;
    } else {
        rate = frequencyHz * WAVEFORM_CLONE_TARGET_SAMPLES_PER_CYCLE;
    }

    maxRateForMinCycles =
        (((uint32_t) sampleCount) * frequencyHz) / WAVEFORM_CLONE_MIN_CYCLES;
    if ((maxRateForMinCycles != 0U) && (rate > maxRateForMinCycles)) {
        rate = maxRateForMinCycles;
    }
    if (rate < ADC_DMA_SCOPE_MIN_RATE_HZ) {
        rate = ADC_DMA_SCOPE_MIN_RATE_HZ;
    }
    if (rate > ADC_DMA_SCOPE_MAX_RATE_HZ) {
        rate = ADC_DMA_SCOPE_MAX_RATE_HZ;
    }
    return rate;
}

static bool AnalyzeWithDefaults(uint32_t rate, uint32_t count,
    WaveformClone_Result_t *result)
{
    uint32_t selectedRate;

    if (rate == 0U) {
        rate = WAVEFORM_CLONE_DEFAULT_RATE_HZ;
    }
    if (count == 0U) {
        count = WAVEFORM_CLONE_DEFAULT_COUNT;
    }
    if (!WaveformClone_Analyze(rate, ClampSampleCount(count), result)) {
        return false;
    }

    selectedRate = SelectSampleRateForFrequency(result->frequencyHz,
        ClampSampleCount(count));
    if ((result->frequencyHz != 0U) && (selectedRate != result->sampleRateHz) &&
        ((result->sampleRateHz < (result->frequencyHz *
              WAVEFORM_CLONE_TARGET_SAMPLES_PER_CYCLE)) ||
            !result->valid)) {
        return WaveformClone_Analyze(selectedRate, ClampSampleCount(count),
            result);
    }
    return true;
}

static bool ReadHighFrequencySine(uint32_t *inputFrequencyHz)
{
    FrequencyMeter_Result_t freq;
    uint64_t scaled;

    if ((inputFrequencyHz == 0) || !FrequencyMeter_Read(&freq) ||
        !freq.valid || (freq.frequencyHz == 0U) ||
        (freq.ageMs > WAVEFORM_CLONE_FREQ_AGE_LIMIT_MS)) {
        return false;
    }

    scaled = ((uint64_t) freq.frequencyHz) * g_highFreqDivider;
    if ((scaled == 0ULL) || (scaled > WAVEFORM_CLONE_HF_INPUT_MAX_HZ)) {
        return false;
    }

    *inputFrequencyHz = (uint32_t) scaled;
    return true;
}

static bool ReadAutoHighFrequencySine(uint32_t *inputFrequencyHz)
{
    if (!ReadHighFrequencySine(inputFrequencyHz)) {
        return false;
    }
    return ((*inputFrequencyHz >= WAVEFORM_CLONE_HF_AUTO_MIN_HZ) &&
            (*inputFrequencyHz <= WAVEFORM_CLONE_HF_AUTO_MAX_HZ));
}

static bool OutputAutoHighFrequencySineAd9833(uint8_t amplitude)
{
    uint32_t frequencyHz;

    if (!ReadAutoHighFrequencySine(&frequencyHz)) {
        return false;
    }
    if (amplitude == 0U) {
        amplitude = 255U;
    }
    if (frequencyHz > AD9833_GetMaxOutputHz()) {
        Uart_Printf("ERR clone hfauto ad9833 freq_hz=%lu exceeds max_hz=%lu mclk_hz=%lu\r\n",
            (unsigned long) frequencyHz,
            (unsigned long) AD9833_GetMaxOutputHz(),
            (unsigned long) AD9833_GetMclkHz());
        return true;
    }

    (void) AD9833_SetAmplitude(amplitude);
    if (!AD9833_SetOutput(AD9833_WAVE_SINE, frequencyHz, 0U)) {
        Uart_WriteString("ERR clone hfauto ad9833 output failed\r\n");
        return true;
    }
    Uart_Printf("OK clone hfauto target=ad9833 wave=sine freq_hz=%lu divider=%lu amp=%u range_hz=%lu..%lu note=frequency_only\r\n",
        (unsigned long) frequencyHz, (unsigned long) g_highFreqDivider,
        amplitude, (unsigned long) WAVEFORM_CLONE_HF_AUTO_MIN_HZ,
        (unsigned long) WAVEFORM_CLONE_HF_AUTO_MAX_HZ);
    return true;
}

static bool OutputAutoHighFrequencySineAd9959(uint8_t channel,
    uint16_t amplitude)
{
    uint32_t frequencyHz;

    if (!ReadAutoHighFrequencySine(&frequencyHz)) {
        return false;
    }
    if (amplitude == 0U) {
        amplitude = AD9959_MAX_AMPLITUDE;
    }
    if (!AD9959_SetSingleTone((AD9959_Channel_t) channel, frequencyHz, 0U,
            amplitude)) {
        Uart_WriteString("ERR clone hfauto AD9959 frequency out of range or sysclk wrong\r\n");
        return true;
    }
    Uart_Printf("OK clone hfauto target=ad9959 ch=%u wave=sine freq_hz=%lu divider=%lu amp=%u range_hz=%lu..%lu note=frequency_only\r\n",
        channel, (unsigned long) frequencyHz, (unsigned long) g_highFreqDivider,
        amplitude, (unsigned long) WAVEFORM_CLONE_HF_AUTO_MIN_HZ,
        (unsigned long) WAVEFORM_CLONE_HF_AUTO_MAX_HZ);
    return true;
}

static bool OutputHighFrequencySineAd9959(uint8_t channel, uint16_t amplitude)
{
    uint32_t frequencyHz;

    if (!ReadHighFrequencySine(&frequencyHz)) {
        Uart_WriteString("ERR clone hfsine no valid digital frequency on PB20; use comparator/divider and set clone hfdiv <n>\r\n");
        return true;
    }
    if (!AD9959_SetSingleTone((AD9959_Channel_t) channel, frequencyHz, 0U,
            amplitude)) {
        Uart_WriteString("ERR clone hfsine AD9959 frequency out of range or sysclk wrong\r\n");
        return true;
    }
    Uart_Printf("OK clone hfsine target=ad9959 ch=%u wave=sine freq_hz=%lu divider=%lu amp=%u sysclk_hz=%lu\r\n",
        channel, (unsigned long) frequencyHz, (unsigned long) g_highFreqDivider,
        amplitude, (unsigned long) AD9959_GetSysclkHz());
    return true;
}

static bool ParseWaveformArg(const char **text, WaveformClone_Type_t *type)
{
    const char *arg;

    if ((text == 0) || (*text == 0) || (type == 0)) {
        return false;
    }
    arg = *text;
    while (*arg == ' ') {
        arg++;
    }

    if (StartsWith(arg, "sine")) {
        *type = WAVEFORM_CLONE_TYPE_SINE;
        *text = &arg[4];
        return true;
    }
    if (StartsWith(arg, "triangle")) {
        *type = WAVEFORM_CLONE_TYPE_TRIANGLE;
        *text = &arg[8];
        return true;
    }
    if (StartsWith(arg, "tri")) {
        *type = WAVEFORM_CLONE_TYPE_TRIANGLE;
        *text = &arg[3];
        return true;
    }
    if (StartsWith(arg, "square")) {
        *type = WAVEFORM_CLONE_TYPE_SQUARE;
        *text = &arg[6];
        return true;
    }
    return false;
}

static bool OutputHighFrequencyAd9833(WaveformClone_Type_t waveform,
    uint8_t amplitude)
{
    uint32_t frequencyHz;

    if (!ReadHighFrequencySine(&frequencyHz)) {
        Uart_WriteString("ERR clone hffreq no valid digital frequency on PB20; use comparator/divider and set clone hfdiv <n>\r\n");
        return true;
    }
    if (frequencyHz > AD9833_GetMaxOutputHz()) {
        Uart_Printf("ERR clone hffreq ad9833 freq_hz=%lu exceeds max_hz=%lu mclk_hz=%lu\r\n",
            (unsigned long) frequencyHz,
            (unsigned long) AD9833_GetMaxOutputHz(),
            (unsigned long) AD9833_GetMclkHz());
        return true;
    }

    (void) AD9833_SetAmplitude(amplitude);
    if (!AD9833_SetOutput(ToAd9833Waveform(waveform), frequencyHz, 0U)) {
        Uart_WriteString("ERR clone hffreq ad9833 output failed\r\n");
        return true;
    }
    Uart_Printf("OK clone hffreq target=ad9833 wave=%s freq_hz=%lu divider=%lu amp=%u mclk_hz=%lu\r\n",
        WaveformClone_TypeText(waveform), (unsigned long) frequencyHz,
        (unsigned long) g_highFreqDivider, amplitude,
        (unsigned long) AD9833_GetMclkHz());
    return true;
}

static bool HandleAd9959Clone(const char *line)
{
    WaveformClone_Result_t result;
    uint32_t rate = WAVEFORM_CLONE_DEFAULT_RATE_HZ;
    uint32_t count = WAVEFORM_CLONE_DEFAULT_COUNT;
    uint8_t channel;
    const char *arg;

    if (!ParseChannelArg(line, "clone ad9959 ch", &channel, &arg)) {
        return false;
    }
    if (!EndOfArgs(arg) && !ParseRateCount(arg, &rate, &count)) {
        Uart_WriteString("ERR clone ad9959 command: clone ad9959 ch0 2000000 1024\r\n");
        return true;
    }
    if (!AnalyzeWithDefaults(rate, count, &result)) {
        Uart_WriteString("ERR clone ADC timeout\r\n");
        return true;
    }
    PrintResult(&result);
    if (!result.valid) {
        if (OutputAutoHighFrequencySineAd9959(channel,
                result.ad9959Amplitude)) {
            return true;
        }
        Uart_WriteString("ERR clone no confident low-frequency waveform match\r\n");
        return true;
    }
    if (!WaveformClone_OutputAd9959(channel, &result)) {
        Uart_WriteString("ERR clone ad9959 only supports recognized low-frequency sine on ch0..ch3\r\n");
        return true;
    }
    Uart_Printf("OK clone ad9959 ch=%u wave=sine freq_hz=%lu amp=%u sysclk_hz=%lu\r\n",
        channel, (unsigned long) result.frequencyHz, result.ad9959Amplitude,
        (unsigned long) AD9959_GetSysclkHz());
    return true;
}

static bool HandleHighFrequencyClone(const char *line)
{
    uint8_t channel;
    const char *arg;
    uint32_t amplitude = AD9959_MAX_AMPLITUDE;

    if (!ParseChannelArg(line, "clone hfsine ad9959 ch", &channel, &arg)) {
        return false;
    }
    if (!EndOfArgs(arg) &&
        (!ParseU32Arg(&arg, &amplitude) || !EndOfArgs(arg) ||
            (amplitude > AD9959_MAX_AMPLITUDE))) {
        Uart_WriteString("ERR clone hfsine command: clone hfsine ad9959 ch0 [0..1023]\r\n");
        return true;
    }
    return OutputHighFrequencySineAd9959(channel, (uint16_t) amplitude);
}

static bool HandleHighFrequencyAd9833Clone(const char *line)
{
    WaveformClone_Type_t waveform;
    uint32_t amplitude = 255U;
    const char *arg;

    if (!StartsWith(line, "clone hffreq ad9833 ")) {
        return false;
    }

    arg = &line[20];
    if (!ParseWaveformArg(&arg, &waveform)) {
        Uart_WriteString("ERR clone hffreq ad9833 command: clone hffreq ad9833 sine|tri|square [0..255]\r\n");
        return true;
    }
    if (!EndOfArgs(arg) &&
        (!ParseU32Arg(&arg, &amplitude) || !EndOfArgs(arg) ||
            (amplitude > 255U))) {
        Uart_WriteString("ERR clone hffreq ad9833 command: clone hffreq ad9833 sine|tri|square [0..255]\r\n");
        return true;
    }
    return OutputHighFrequencyAd9833(waveform, (uint8_t) amplitude);
}

static bool HandleDualSineClone(const char *line)
{
    DualSine_Result_t result;
    uint32_t rate = WAVEFORM_CLONE_DUAL_DEFAULT_RATE_HZ;
    uint32_t count = WAVEFORM_CLONE_DEFAULT_COUNT;
    const char *arg;
    uint32_t amplitude1Code;
    uint32_t amplitude2Code;
    bool output1;
    bool output2;

    if (!StrEquals(line, "clone2") && !StartsWith(line, "clone2 ")) {
        return false;
    }
    if (StartsWith(line, "clone2 ")) {
        arg = &line[7];
        if (!ParseRateCount(arg, &rate, &count)) {
            Uart_WriteString("ERR clone2 command: clone2 [rate] [count], e.g. clone2 200000 1024\r\n");
            return true;
        }
    }

    if ((count != 256U) && (count != 512U) && (count != 1024U)) {
        Uart_WriteString("ERR clone2 count must be 256, 512, or 1024\r\n");
        return true;
    }
    if (!DualSine_CaptureAndAnalyze(rate, (uint16_t) count, &result)) {
        Uart_WriteString("ERR clone2 ADC capture/analyzer failed\r\n");
        return true;
    }

    Uart_Printf("INFO clone2 sample_rate_hz=%lu count=%u dma=%u dc_raw=%u separation_hz=%lu\r\n",
        (unsigned long) result.sampleRateHz, result.sampleCount,
        result.usedDma ? 1U : 0U, result.dcRaw,
        (unsigned long) result.separationHz);
    Uart_Printf("clone2 f1_hz=%lu amp1_mv=%u phase1_deg=%d f2_hz=%lu amp2_mv=%u phase2_deg=%d valid=%u reason=%s\r\n",
        (unsigned long) result.frequency1Hz, result.amplitude1Mv,
        result.phase1Deg, (unsigned long) result.frequency2Hz,
        result.amplitude2Mv, result.phase2Deg, result.valid ? 1U : 0U,
        result.rejectReason);

    if (!result.valid) {
        Uart_WriteString("ERR clone2 requires two different sine components with sufficient separation and amplitude\r\n");
        return true;
    }
    if ((result.frequency1Hz >
            AD9833_GetMaxOutputHzChannel(AD9833_CHANNEL_1)) ||
        (result.frequency2Hz >
            AD9833_GetMaxOutputHzChannel(AD9833_CHANNEL_2))) {
        Uart_Printf("ERR clone2 frequency exceeds AD9833 ch1_max_hz=%lu ch2_max_hz=%lu\r\n",
            (unsigned long) AD9833_GetMaxOutputHzChannel(AD9833_CHANNEL_1),
            (unsigned long) AD9833_GetMaxOutputHzChannel(AD9833_CHANNEL_2));
        return true;
    }

    amplitude1Code = ScaleToU16((uint32_t) result.amplitude1Mv * 2UL,
        g_ad9833FullScaleMv, 255U);
    amplitude2Code = ScaleToU16((uint32_t) result.amplitude2Mv * 2UL,
        g_ad9833FullScaleMv, 255U);
    (void) AD9833_SetAmplitudeChannel(AD9833_CHANNEL_1,
        (uint8_t) amplitude1Code);
    (void) AD9833_SetAmplitudeChannel(AD9833_CHANNEL_2,
        (uint8_t) amplitude2Code);
    output1 = AD9833_SetOutputChannel(AD9833_CHANNEL_1,
        AD9833_WAVE_SINE, result.frequency1Hz,
        (uint16_t) result.phase1Deg);
    output2 = AD9833_SetOutputChannel(AD9833_CHANNEL_2,
        AD9833_WAVE_SINE, result.frequency2Hz,
        (uint16_t) result.phase2Deg);
    if (!output1 || !output2) {
        AD9833_ResetOutput();
        Uart_WriteString("ERR clone2 AD9833 output failed\r\n");
        return true;
    }

    Uart_Printf("OK clone2 ad9833_ch1=sine:%luHz amp_code=%lu mclk_hz=%lu ch2=sine:%luHz amp_code=%lu mclk_hz=%lu phase_tracking=sample_reference amp_control=external_required\r\n",
        (unsigned long) result.frequency1Hz, (unsigned long) amplitude1Code,
        (unsigned long) AD9833_GetMclkChannel(AD9833_CHANNEL_1),
        (unsigned long) result.frequency2Hz, (unsigned long) amplitude2Code,
        (unsigned long) AD9833_GetMclkChannel(AD9833_CHANNEL_2));
    return true;
}

bool WaveformClone_HandleCommand(const char *line)
{
    WaveformClone_Result_t result;
    uint32_t rate = WAVEFORM_CLONE_DEFAULT_RATE_HZ;
    uint32_t count = WAVEFORM_CLONE_DEFAULT_COUNT;
    uint32_t value;
    const char *arg;

    if (HandleDualSineClone(line)) {
        return true;
    }
    if (StrEquals(line, "clone")) {
        if (!AnalyzeWithDefaults(rate, count, &result)) {
            Uart_WriteString("ERR clone ADC timeout\r\n");
            return true;
        }
        PrintResult(&result);
        if (!result.valid) {
            Uart_WriteString("ERR clone no confident low-frequency waveform match\r\n");
        }
        return true;
    }
    if (StartsWith(line, "clone measure ")) {
        arg = &line[14];
        if (!ParseRateCount(arg, &rate, &count)) {
            Uart_WriteString("ERR clone measure command: clone measure 2000000 1024\r\n");
            return true;
        }
        if (!AnalyzeWithDefaults(rate, count, &result)) {
            Uart_WriteString("ERR clone ADC timeout\r\n");
            return true;
        }
        PrintResult(&result);
        return true;
    }
    if (StartsWith(line, "clone auto")) {
        arg = &line[10];
        if (!EndOfArgs(arg) && !ParseRateCount(arg, &rate, &count)) {
            Uart_WriteString("ERR clone auto command: clone auto 2000000 1024\r\n");
            return true;
        }
        if (!AnalyzeWithDefaults(rate, count, &result)) {
            Uart_WriteString("ERR clone ADC timeout\r\n");
            return true;
        }
        PrintResult(&result);
        if (!result.valid) {
            if (OutputAutoHighFrequencySineAd9833(result.ad9833Amplitude)) {
                return true;
            }
            AD9833_ResetOutput();
            Uart_WriteString("ERR clone auto supports PA25 ADC rough shape/frequency/amplitude up to 2MHz, or PB20 frequency-only sine fallback from 200kHz to 2MHz\r\n");
            return true;
        }
        if (!WaveformClone_OutputAd9833(&result)) {
            AD9833_ResetOutput();
            Uart_Printf("ERR clone auto AD9833 failed or freq_hz>%lu\r\n",
                (unsigned long) AD9833_GetMaxOutputHz());
            return true;
        }
        Uart_Printf("OK clone auto target=ad9833 wave=%s freq_hz=%lu amp=%u\r\n",
            WaveformClone_TypeText(result.type),
            (unsigned long) result.frequencyHz, result.ad9833Amplitude);
        return true;
    }
    if (StartsWith(line, "clone ad9833")) {
        arg = &line[13];
        if (!EndOfArgs(arg) && !ParseRateCount(arg, &rate, &count)) {
            Uart_WriteString("ERR clone ad9833 command: clone ad9833 2000000 1024\r\n");
            return true;
        }
        if (!AnalyzeWithDefaults(rate, count, &result)) {
            Uart_WriteString("ERR clone ADC timeout\r\n");
            return true;
        }
        PrintResult(&result);
        if (!result.valid) {
            if (OutputAutoHighFrequencySineAd9833(result.ad9833Amplitude)) {
                return true;
            }
            AD9833_ResetOutput();
            Uart_WriteString("ERR clone no confident low-frequency waveform match\r\n");
            return true;
        }
        if (!WaveformClone_OutputAd9833(&result)) {
            AD9833_ResetOutput();
            Uart_Printf("ERR clone ad9833 unsupported or freq_hz>%lu\r\n",
                (unsigned long) AD9833_GetMaxOutputHz());
            return true;
        }
        Uart_Printf("OK clone ad9833 wave=%s freq_hz=%lu amp=%u\r\n",
            WaveformClone_TypeText(result.type),
            (unsigned long) result.frequencyHz, result.ad9833Amplitude);
        return true;
    }
    if (HandleHighFrequencyClone(line)) {
        return true;
    }
    if (HandleHighFrequencyAd9833Clone(line)) {
        return true;
    }
    if (HandleAd9959Clone(line)) {
        return true;
    }
    if (StartsWith(line, "clone scale ")) {
        arg = &line[12];
        if (!ParseU32Arg(&arg, &value) || !EndOfArgs(arg) ||
            (value == 0U)) {
            Uart_WriteString("ERR clone scale command: clone scale 3300\r\n");
            return true;
        }
        g_ad9833FullScaleMv = value;
        g_ad9959FullScaleMv = value;
        Uart_Printf("OK clone scale ad9833_full_mv=%lu ad9959_full_mv=%lu\r\n",
            (unsigned long) g_ad9833FullScaleMv,
            (unsigned long) g_ad9959FullScaleMv);
        return true;
    }
    if (StartsWith(line, "clone outscale ad9833 ")) {
        arg = &line[22];
        if (!ParseU32Arg(&arg, &value) || !EndOfArgs(arg) ||
            (value == 0U)) {
            Uart_WriteString("ERR clone outscale command: clone outscale ad9833 3300\r\n");
            return true;
        }
        g_ad9833FullScaleMv = value;
        Uart_Printf("OK clone outscale ad9833_full_mv=%lu\r\n",
            (unsigned long) g_ad9833FullScaleMv);
        return true;
    }
    if (StartsWith(line, "clone outscale ad9959 ")) {
        arg = &line[22];
        if (!ParseU32Arg(&arg, &value) || !EndOfArgs(arg) ||
            (value == 0U)) {
            Uart_WriteString("ERR clone outscale command: clone outscale ad9959 3300\r\n");
            return true;
        }
        g_ad9959FullScaleMv = value;
        Uart_Printf("OK clone outscale ad9959_full_mv=%lu\r\n",
            (unsigned long) g_ad9959FullScaleMv);
        return true;
    }
    if (StartsWith(line, "clone gain ")) {
        arg = &line[11];
        if (!ParseU32Arg(&arg, &value) || !EndOfArgs(arg)) {
            Uart_WriteString("ERR clone gain command: clone gain 1000\r\n");
            return true;
        }
        g_outputGainPermille = value;
        Uart_Printf("OK clone gain permille=%lu\r\n",
            (unsigned long) g_outputGainPermille);
        return true;
    }
    if (StartsWith(line, "clone hfdiv ")) {
        arg = &line[12];
        if (!ParseU32Arg(&arg, &value) || !EndOfArgs(arg) ||
            (value == 0U)) {
            Uart_WriteString("ERR clone hfdiv command: clone hfdiv 1\r\n");
            return true;
        }
        g_highFreqDivider = value;
        Uart_Printf("OK clone hfdiv divider=%lu\r\n",
            (unsigned long) g_highFreqDivider);
        return true;
    }
    return false;
}

void WaveformClone_WriteHelp(void)
{
    Uart_WriteString("Wave clone: clone | clone2 [rate count] (two sine components) | clone measure <rate> <count> | clone auto [rate count] | clone ad9833 [rate count] | clone ad9959 chN [rate count]\r\n");
    Uart_WriteString("Wave clone HF/config: clone hffreq ad9833 sine|tri|square [amp] | clone hfsine ad9959 chN [amp] | clone scale <mv> | clone hfdiv <n>\r\n");
    Uart_WriteString("Wave clone amplitude: clone outscale ad9833|ad9959 <full_mvpp> | clone gain <permille>; output target is AC-centered around 0V.\r\n");
    Uart_WriteString("clone bands: PA25 ADC <=100kHz sine/tri/square, 100k..200k sine/tri only, 200k..2MHz rough ADC forced sine.\r\n");
    Uart_WriteString("clone fallback: optional comparator/divider digital input on PB20 supplies 200k..2MHz frequency if PA25 ADC is not valid.\r\n");
    Uart_WriteString("clone2: PA25 composite input, default 200kS/s 1024 points; separates two sine waves from 1kHz..100kHz when separated by >=1kHz.\r\n");
    Uart_WriteString("clone2 amplitude is measured and converted to amp_code; AD9833 amplitude still requires external analog control hardware.\r\n");
}
