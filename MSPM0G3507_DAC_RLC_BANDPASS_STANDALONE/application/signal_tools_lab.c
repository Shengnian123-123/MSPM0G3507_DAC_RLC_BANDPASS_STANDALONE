#include "signal_tools_lab.h"
#include <stdint.h>
#include "adc_dma_scope.h"
#include "adc_fml.h"
#include "frequency_meter.h"
#include "phase_meter.h"
#include "rms_meter.h"
#include "signal_measure.h"
#include "uart.h"

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

static bool ParseTwoU32Args(const char *text, uint32_t *first, uint32_t *second)
{
    if (!ParseU32Arg(&text, first)) {
        return false;
    }
    if (!ParseU32Arg(&text, second)) {
        return false;
    }
    while (*text == ' ') {
        text++;
    }
    return (*text == '\0');
}

static bool EndOfArgs(const char *text)
{
    while (*text == ' ') {
        text++;
    }
    return (*text == '\0');
}

static uint16_t ScopeCountFromU32(uint32_t count)
{
    if (count == 0U) {
        return 1U;
    }
    if (count > ADC_DMA_SCOPE_MAX_SAMPLES) {
        return ADC_DMA_SCOPE_MAX_SAMPLES;
    }
    return (uint16_t) count;
}

static void PrintScopeFrame(const AdcDmaScope_Frame_t *frame)
{
    SignalMeasure_Result_t measure;

    Uart_Printf("OK scope rate_hz=%lu count=%u mode=%s adc_pin=PA25\r\n",
        (unsigned long) frame->sampleRateHz, frame->sampleCount,
        frame->usedDma ? "dma" : "blocking_compat");
    if (SignalMeasure_Analyze(frame->samples, frame->sampleCount,
            frame->sampleRateHz, &measure)) {
        Uart_Printf("stats min=%u max=%u avg=%u pkpk=%u ac_rms_mv=%u freq_hz=%lu duty_permille=%u\r\n",
            measure.minRaw, measure.maxRaw, measure.avgRaw, measure.pkpkRaw,
            measure.acRmsMv, (unsigned long) measure.frequencyHz,
            measure.dutyPermille);
    }

    Uart_WriteString("idx,raw,mv\r\n");
    for (uint16_t i = 0U; i < frame->sampleCount; i++) {
        Uart_Printf("%u,%u,%u\r\n", i, frame->samples[i],
            ADC_Fml_RawToMillivolt(frame->samples[i]));
    }
}

static bool HandleScopeCommand(const char *line)
{
    static AdcDmaScope_Frame_t frame;
    uint32_t rate;
    uint32_t count;

    if (StrEquals(line, "scope stop")) {
        AdcDmaScope_StopVofa();
        Uart_WriteString("OK scope stop\r\n");
        return true;
    }
    if (StartsWith(line, "scope vofa ")) {
        const char *arg = &line[11];

        if (!ParseU32Arg(&arg, &rate) || !EndOfArgs(arg)) {
            Uart_WriteString("ERR scope vofa command: scope vofa 100\r\n");
            return true;
        }
        AdcDmaScope_StartVofa(rate);
        Uart_WriteString("OK scope vofa FireWater raw,mv,ac_mv adc_pin=PA25\r\n");
        return true;
    }
    if (!StartsWith(line, "scope ")) {
        return false;
    }
    if (!ParseTwoU32Args(&line[6], &rate, &count)) {
        Uart_WriteString("ERR scope command: scope 1000000 1024\r\n");
        return true;
    }
    if (!AdcDmaScope_Capture(rate, ScopeCountFromU32(count), &frame)) {
        Uart_WriteString("ERR scope ADC timeout\r\n");
        return true;
    }
    PrintScopeFrame(&frame);
    return true;
}

static void PrintFrequency(void)
{
    FrequencyMeter_Result_t result;

    if (!FrequencyMeter_Read(&result)) {
        Uart_Printf("ERR freq no complete period pin=%s edges=%lu\r\n",
            FREQUENCY_METER_INPUT_PIN_NAME, (unsigned long) result.edgeCount);
        return;
    }
    Uart_Printf("OK freq pin=%s freq_hz=%lu period_us=%lu duty_permille=%u edges=%lu age_ms=%lu\r\n",
        FREQUENCY_METER_INPUT_PIN_NAME, (unsigned long) result.frequencyHz,
        (unsigned long) result.periodUs, result.dutyPermille,
        (unsigned long) result.edgeCount, (unsigned long) result.ageMs);
}

static void PrintPhase(void)
{
    PhaseMeter_Result_t result;

    if (!PhaseMeter_Read(&result)) {
        Uart_Printf("ERR phase need signals A=%s B=%s edgeA=%lu edgeB=%lu\r\n",
            PHASE_METER_A_PIN_NAME, PHASE_METER_B_PIN_NAME,
            (unsigned long) result.edgeCountA,
            (unsigned long) result.edgeCountB);
        return;
    }
    Uart_Printf("OK phase A=%s B=%s phase_deg=%u signed_deg=%d delta_us=%lu period_us=%lu edgeA=%lu edgeB=%lu\r\n",
        PHASE_METER_A_PIN_NAME, PHASE_METER_B_PIN_NAME, result.phaseDeg,
        result.signedDeg, (unsigned long) result.deltaUs,
        (unsigned long) result.periodUs, (unsigned long) result.edgeCountA,
        (unsigned long) result.edgeCountB);
}

static void PrintRms(uint32_t rate, uint16_t count)
{
    RmsMeter_Result_t result;
    RmsMeter_Calibration_t cal;

    if (!RmsMeter_Measure(rate, count, &result)) {
        Uart_WriteString("ERR rms ADC timeout\r\n");
        return;
    }
    cal = RmsMeter_GetCalibration();
    Uart_Printf("OK rms adc_pin=PA25 rate_hz=%lu count=%u zero_raw=%ld gain_permille=%lu\r\n",
        (unsigned long) result.sampleRateHz, result.sampleCount,
        (long) cal.zeroRaw, (unsigned long) cal.gainPermille);
    Uart_Printf("raw min=%u max=%u avg=%u pkpk=%u\r\n",
        result.minRaw, result.maxRaw, result.avgRaw, result.pkpkRaw);
    Uart_Printf("mv min=%ld max=%ld avg=%ld pkpk=%lu rms=%lu ac_rms=%lu\r\n",
        (long) result.minMv, (long) result.maxMv, (long) result.avgMv,
        (unsigned long) result.pkpkMv, (unsigned long) result.rmsMv,
        (unsigned long) result.acRmsMv);
}

static bool HandleRmsCommand(const char *line)
{
    uint32_t first;
    uint32_t second;
    RmsMeter_Calibration_t cal;

    if (StrEquals(line, "rms cal")) {
        cal = RmsMeter_GetCalibration();
        Uart_Printf("OK rms cal zero_raw=%ld gain_permille=%lu\r\n",
            (long) cal.zeroRaw, (unsigned long) cal.gainPermille);
        return true;
    }
    if (StrEquals(line, "rms calzero")) {
        if (RmsMeter_CalibrateZero(10000UL, 128U)) {
            cal = RmsMeter_GetCalibration();
            Uart_Printf("OK rms calzero zero_raw=%ld\r\n", (long) cal.zeroRaw);
        } else {
            Uart_WriteString("ERR rms calzero ADC timeout\r\n");
        }
        return true;
    }
    if (StartsWith(line, "rms zero ")) {
        const char *arg = &line[9];

        if (ParseU32Arg(&arg, &first) && EndOfArgs(arg) &&
            RmsMeter_SetZeroRaw((int32_t) first)) {
            Uart_Printf("OK rms zero_raw=%lu\r\n", (unsigned long) first);
        } else {
            Uart_WriteString("ERR rms zero command: rms zero 2048, range 0..4095\r\n");
        }
        return true;
    }
    if (StartsWith(line, "rms gain ")) {
        const char *arg = &line[9];

        if (ParseU32Arg(&arg, &first) && EndOfArgs(arg) &&
            RmsMeter_SetGainPermille(first)) {
            cal = RmsMeter_GetCalibration();
            Uart_Printf("OK rms gain_permille=%lu\r\n",
                (unsigned long) cal.gainPermille);
        } else {
            Uart_WriteString("ERR rms gain command: rms gain 1000, range 100..10000\r\n");
        }
        return true;
    }
    if (!StartsWith(line, "rms ")) {
        return false;
    }
    if (!ParseTwoU32Args(&line[4], &first, &second)) {
        Uart_WriteString("ERR rms command: rms 10000 128\r\n");
        return true;
    }
    PrintRms(first, ScopeCountFromU32(second));
    return true;
}

void SignalToolsLab_Init(void)
{
    FrequencyMeter_Init();
    PhaseMeter_Init();
    RmsMeter_Init();
}

void SignalToolsLab_WriteHelp(void)
{
    Uart_WriteString("Signal tools: scope <rate> <count> | scope vofa <rate> | scope stop | freq | phase | rms <rate> <count> | rms cal/calzero/zero/gain | meter\r\n");
    Uart_WriteString("signal pins: ADC/RMS/scope=PA25, frequency=PB20, phase A=PB20 B=PB21. scope vofa outputs raw,mv,ac_mv.\r\n");
}

bool SignalToolsLab_HandleCommand(const char *line)
{
    if (HandleScopeCommand(line)) {
        return true;
    }
    if (StrEquals(line, "freq")) {
        PrintFrequency();
        return true;
    }
    if (StrEquals(line, "phase")) {
        PrintPhase();
        return true;
    }
    if (HandleRmsCommand(line)) {
        return true;
    }
    if (StrEquals(line, "meter")) {
        PrintFrequency();
        PrintPhase();
        PrintRms(10000UL, 128U);
        return true;
    }
    return false;
}

void SignalToolsLab_Proc(void)
{
    AdcDmaScope_Proc();
}

void SignalToolsLab_StopStreaming(void)
{
    AdcDmaScope_StopVofa();
}

bool SignalToolsLab_IsStreaming(void)
{
    return AdcDmaScope_IsVofaRunning();
}
