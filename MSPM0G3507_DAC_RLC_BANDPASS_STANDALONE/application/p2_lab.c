#include "p2_lab.h"
#include <stdint.h>
#include "adc_fml.h"
#include "p1_lab.h"
#include "signal_filter.h"
#include "signal_measure.h"
#include "signal_spectral.h"
#include "uart.h"

static uint16_t g_filtered[P1_ADC_STREAM_MAX_SAMPLES];

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

static bool StrStartsWith(const char *text, const char *prefix)
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

static bool ParseU32Token(const char **text, uint32_t *value)
{
    uint32_t result = 0U;
    bool hasDigit = false;
    const char *cursor;

    if ((text == 0) || (*text == 0) || (value == 0)) {
        return false;
    }

    cursor = *text;
    while (*cursor == ' ') {
        cursor++;
    }

    while ((*cursor >= '0') && (*cursor <= '9')) {
        uint32_t digit = (uint32_t) (*cursor - '0');

        if (result > ((0xFFFFFFFFUL - digit) / 10UL)) {
            return false;
        }
        result = (result * 10UL) + digit;
        hasDigit = true;
        cursor++;
    }

    *text = cursor;
    *value = result;
    return hasDigit;
}

static bool ParseRateCount(const char *text, uint32_t *rateHz, uint16_t *count)
{
    uint32_t parsedCount;

    if (!ParseU32Token(&text, rateHz)) {
        return false;
    }
    if (!ParseU32Token(&text, &parsedCount) ||
        (parsedCount > P1_ADC_STREAM_MAX_SAMPLES)) {
        return false;
    }

    while (*text == ' ') {
        text++;
    }

    if ((*text != '\0') || (parsedCount == 0U)) {
        return false;
    }

    *count = (uint16_t) parsedCount;
    return true;
}

static bool ParseRateTarget(const char *text, uint32_t *rateHz,
    uint32_t *targetHz)
{
    if (!ParseU32Token(&text, rateHz) || !ParseU32Token(&text, targetHz)) {
        return false;
    }

    while (*text == ' ') {
        text++;
    }

    return (*text == '\0');
}

static bool CaptureForP2(uint32_t rateHz, uint16_t count, P1_AdcStream_t *stream)
{
    P1_Status_t status = P1_CaptureAdcBlocking(rateHz, count, stream);

    if (status != P1_STATUS_OK) {
        Uart_WriteString("ERR p2 capture ");
        Uart_WriteString(P1_StatusText(status));
        Uart_WriteString(" rates=10000/50000/100000/200000 count<=128\r\n");
        return false;
    }

    return true;
}

static void PrintMeasureResult(const char *name, uint32_t rateHz,
    uint16_t count, const SignalMeasure_Result_t *result)
{
    Uart_Printf("OK %s rate_hz=%lu count=%u min=%u max=%u avg=%u pkpk=%u rms=%u ac_rms=%u freq_hz=%lu duty_permille=%u edges=%u\r\n",
        name, (unsigned long) rateHz, count, result->minRaw, result->maxRaw,
        result->avgRaw, result->pkpkRaw, result->rmsRaw, result->acRmsRaw,
        (unsigned long) result->frequencyHz, result->dutyPermille,
        result->risingEdges);
    Uart_Printf("OK %s_mv min=%u max=%u avg=%u pkpk=%u rms=%u ac_rms=%u\r\n",
        name, result->minMv, result->maxMv, result->avgMv, result->pkpkMv,
        result->rmsMv, result->acRmsMv);
}

static void PrintFilteredCsv(const P1_AdcStream_t *stream)
{
    Uart_WriteString("idx,raw,filtered,mv\r\n");
    for (uint16_t i = 0U; i < stream->sampleCount; i++) {
        Uart_Printf("%u,%u,%u,%u\r\n", i, stream->samples[i], g_filtered[i],
            ADC_Fml_RawToMillivolt(g_filtered[i]));
    }
}

static bool HandleMeasure(const char *line)
{
    P1_AdcStream_t stream;
    SignalMeasure_Result_t result;
    uint32_t rateHz;
    uint16_t count;

    if (!StrStartsWith(line, "measure ")) {
        return false;
    }

    if (!ParseRateCount(&line[8], &rateHz, &count)) {
        Uart_WriteString("ERR measure command: measure 10000 128\r\n");
        return true;
    }

    if (!CaptureForP2(rateHz, count, &stream)) {
        return true;
    }

    SignalFilter_MovingAverage3(stream.samples, g_filtered, stream.sampleCount);
    if (!SignalMeasure_Analyze(g_filtered, stream.sampleCount, rateHz, &result)) {
        Uart_WriteString("ERR measure analyze failed\r\n");
        return true;
    }

    PrintMeasureResult("measure", rateHz, stream.sampleCount, &result);
    return true;
}

static bool HandleFilter(const char *line)
{
    P1_AdcStream_t stream;
    SignalMeasure_Result_t result;
    uint32_t rateHz;
    uint16_t count;
    const char *args;
    const char *filterName;

    if (!StrStartsWith(line, "filter ")) {
        return false;
    }

    args = &line[7];
    if (StrStartsWith(args, "avg3 ")) {
        filterName = "avg3";
        args += 5;
    } else if (StrStartsWith(args, "median3 ")) {
        filterName = "median3";
        args += 8;
    } else if (StrStartsWith(args, "iir ")) {
        filterName = "iir";
        args += 4;
    } else if (StrStartsWith(args, "none ")) {
        filterName = "none";
        args += 5;
    } else {
        Uart_WriteString("ERR filter command: filter avg3 10000 64\r\n");
        return true;
    }

    if (!ParseRateCount(args, &rateHz, &count)) {
        Uart_WriteString("ERR filter command: filter avg3 10000 64\r\n");
        return true;
    }

    if (!CaptureForP2(rateHz, count, &stream)) {
        return true;
    }

    if (StrEquals(filterName, "avg3")) {
        SignalFilter_MovingAverage3(stream.samples, g_filtered, stream.sampleCount);
    } else if (StrEquals(filterName, "median3")) {
        SignalFilter_Median3(stream.samples, g_filtered, stream.sampleCount);
    } else if (StrEquals(filterName, "iir")) {
        SignalFilter_IirLowPass(stream.samples, g_filtered, stream.sampleCount, 2U);
    } else {
        SignalFilter_Copy(stream.samples, g_filtered, stream.sampleCount);
    }

    if (!SignalMeasure_Analyze(g_filtered, stream.sampleCount, rateHz, &result)) {
        Uart_WriteString("ERR filter analyze failed\r\n");
        return true;
    }

    Uart_WriteString("OK filter ");
    Uart_WriteString(filterName);
    Uart_WriteString("\r\n");
    PrintMeasureResult("filter", rateHz, stream.sampleCount, &result);
    PrintFilteredCsv(&stream);
    return true;
}

static bool HandleGoertzel(const char *line)
{
    P1_AdcStream_t stream;
    SignalSpectral_BinResult_t result;
    uint32_t rateHz;
    uint32_t targetHz;

    if (!StrStartsWith(line, "goertzel ")) {
        return false;
    }

    if (!ParseRateTarget(&line[9], &rateHz, &targetHz)) {
        Uart_WriteString("ERR goertzel command: goertzel 10000 1000\r\n");
        return true;
    }

    if (!CaptureForP2(rateHz, SIGNAL_SPECTRAL_DFT_COUNT, &stream)) {
        return true;
    }

    SignalFilter_MovingAverage3(stream.samples, g_filtered, stream.sampleCount);
    if (!SignalSpectral_GoertzelHz64(g_filtered, targetHz, rateHz, &result)) {
        Uart_WriteString("ERR goertzel expects 0<target<rate/2 and 64-point bin fit\r\n");
        return true;
    }

    Uart_Printf("OK goertzel rate_hz=%lu target_hz=%lu bin=%u bin_hz=%lu power=",
        (unsigned long) rateHz, (unsigned long) targetHz, result.bin,
        (unsigned long) result.binFrequencyHz);
    Uart_WriteU64(result.power);
    Uart_Printf(" magnitude_raw=%lu count=64\r\n",
        (unsigned long) result.magnitudeRaw);
    return true;
}

static bool HandleThd(const char *line)
{
    P1_AdcStream_t stream;
    SignalSpectral_ThdResult_t result;
    uint32_t rateHz;
    uint32_t fundamentalHz;

    if (!StrStartsWith(line, "thd ")) {
        return false;
    }

    if (!ParseRateTarget(&line[4], &rateHz, &fundamentalHz)) {
        Uart_WriteString("ERR thd command: thd 10000 1000\r\n");
        return true;
    }

    if (!CaptureForP2(rateHz, SIGNAL_SPECTRAL_DFT_COUNT, &stream)) {
        return true;
    }

    SignalFilter_MovingAverage3(stream.samples, g_filtered, stream.sampleCount);
    if (!SignalSpectral_Thd64(g_filtered, fundamentalHz, rateHz, &result)) {
        Uart_WriteString("ERR thd expects usable fundamental below rate/2\r\n");
        return true;
    }

    Uart_Printf("OK thd rate_hz=%lu target_hz=%lu fundamental_bin=%u fundamental_bin_hz=%lu fundamental_power=",
        (unsigned long) rateHz, (unsigned long) fundamentalHz,
        result.fundamentalBin, (unsigned long) result.fundamentalHz);
    Uart_WriteU64(result.fundamentalPower);
    Uart_WriteString(" harmonic_power=");
    Uart_WriteU64(result.harmonicPower);
    Uart_Printf(" thd_permille=%lu\r\n", (unsigned long) result.thdPermille);
    return true;
}

static void WriteP2Info(void)
{
    Uart_WriteString("OK p2 algorithms=min/max/avg/pkpk/rms/ac_rms/zero_cross_freq/duty/goertzel/thd/phase_diff filters=avg3/median3/iir\r\n");
}

static void WriteP2Test(void)
{
    P1_AdcStream_t stream;
    SignalMeasure_Result_t result;
    const uint32_t rateHz = 10000UL;
    const uint16_t count = 64U;

    if (!CaptureForP2(rateHz, count, &stream)) {
        return;
    }

    SignalFilter_MovingAverage3(stream.samples, g_filtered, stream.sampleCount);
    if (!SignalMeasure_Analyze(g_filtered, stream.sampleCount, rateHz, &result)) {
        Uart_WriteString("ERR p2test analyze failed\r\n");
        return;
    }

    PrintMeasureResult("p2test", rateHz, stream.sampleCount, &result);
}

bool P2Lab_HandleCommand(const char *line)
{
    if (line == 0) {
        return false;
    }

    if (StrEquals(line, "p2")) {
        WriteP2Info();
        return true;
    }
    if (StrEquals(line, "p2test")) {
        WriteP2Test();
        return true;
    }
    if (HandleMeasure(line)) {
        return true;
    }
    if (HandleFilter(line)) {
        return true;
    }
    if (HandleGoertzel(line)) {
        return true;
    }
    if (HandleThd(line)) {
        return true;
    }

    return false;
}

void P2Lab_WriteHelp(void)
{
    Uart_WriteString("Signal algorithms: measure <rate> <count> | filter none/avg3/median3/iir <rate> <count> | goertzel <rate> <hz> | thd <rate> <hz> | p2/p2test\r\n");
}
