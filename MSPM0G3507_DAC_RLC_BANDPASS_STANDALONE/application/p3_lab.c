#include "p3_lab.h"
#include <stdint.h>
#include "app_params.h"
#include "control_agc.h"
#include "control_pid.h"
#include "p1_lab.h"
#include "uart.h"

static ControlPid_t g_pid;

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

static bool ParseI32Token(const char **text, int32_t *value)
{
    int32_t sign = 1;
    int32_t result = 0;
    bool hasDigit = false;
    const char *cursor;

    if ((text == 0) || (*text == 0) || (value == 0)) {
        return false;
    }

    cursor = *text;
    while (*cursor == ' ') {
        cursor++;
    }

    if (*cursor == '-') {
        sign = -1;
        cursor++;
    }

    while ((*cursor >= '0') && (*cursor <= '9')) {
        int32_t digit = (int32_t) (*cursor - '0');

        if (result > ((0x7FFFFFFFL - digit) / 10L)) {
            return false;
        }
        result = (result * 10L) + digit;
        hasDigit = true;
        cursor++;
    }

    *text = cursor;
    *value = result * sign;
    return hasDigit;
}

static bool ParseU32Token(const char **text, uint32_t *value)
{
    int32_t parsed;

    if (!ParseI32Token(text, &parsed) || (parsed < 0)) {
        return false;
    }

    *value = (uint32_t) parsed;
    return true;
}

static bool IsEnd(const char *text)
{
    while (*text == ' ') {
        text++;
    }
    return (*text == '\0');
}

static void PidLoadFromParams(void)
{
    const AppParams_t *params = AppParams_Get();

    ControlPid_Init(&g_pid, params->pid.kp, params->pid.ki, params->pid.kd,
        0, 1000, params->pid.shift);
}

static void WriteParams(void)
{
    const AppParams_t *params = AppParams_Get();

    Uart_Printf("OK p3 adc_offset=%d adc_gain_permille=%u zero_raw=%u ref_raw=%u ref_mv=%u\r\n",
        params->adcOffsetRaw, params->adcGainPermille, params->adcZeroRaw,
        params->adcRefRaw, params->adcRefMv);
    Uart_Printf("OK p3 pid kp=%d ki=%d kd=%d shift=%u\r\n",
        params->pid.kp, params->pid.ki, params->pid.kd, params->pid.shift);
    Uart_Printf("OK p3 agc target_pkpk=%u hyst=%u gain_min=%u gain_max=%u gain_init=%u\r\n",
        params->agcTargetPkpkRaw, params->agcHysteresisRaw,
        params->agcGainMin, params->agcGainMax, params->agcInitialGain);
    Uart_WriteString("OK p3 output_cal=");
    for (uint8_t i = 0U; i < APP_PARAMS_OUTPUT_CAL_COUNT; i++) {
        Uart_WriteU32(params->outputCal[i]);
        Uart_WriteByte((i + 1U) == APP_PARAMS_OUTPUT_CAL_COUNT ? '\r' : ',');
    }
    Uart_WriteByte('\n');
}

static bool HandlePid(const char *line)
{
    const char *args;
    int32_t first;
    int32_t second;
    int32_t third;
    uint32_t shift;
    AppParams_t *params;
    int32_t output;

    if (StrEquals(line, "pid show")) {
        PidLoadFromParams();
        Uart_Printf("OK pid kp=%d ki=%d kd=%d shift=%u output_min=%d output_max=%d\r\n",
            g_pid.kp, g_pid.ki, g_pid.kd, g_pid.shift, g_pid.outputMin,
            g_pid.outputMax);
        return true;
    }

    if (StrStartsWith(line, "pid set ")) {
        args = &line[8];
        if (!ParseI32Token(&args, &first) || !ParseI32Token(&args, &second) ||
            !ParseI32Token(&args, &third) || !ParseU32Token(&args, &shift) ||
            !IsEnd(args) || (shift > 15U)) {
            Uart_WriteString("ERR pid set command: pid set 256 16 0 8\r\n");
            return true;
        }

        params = AppParams_Edit();
        params->pid.kp = first;
        params->pid.ki = second;
        params->pid.kd = third;
        params->pid.shift = (uint8_t) shift;
        PidLoadFromParams();
        Uart_WriteString("OK pid set\r\n");
        return true;
    }

    if (StrStartsWith(line, "pid run ")) {
        args = &line[8];
        if (!ParseI32Token(&args, &first) || !ParseI32Token(&args, &second) ||
            !IsEnd(args)) {
            Uart_WriteString("ERR pid run command: pid run 2000 1200\r\n");
            return true;
        }

        PidLoadFromParams();
        output = ControlPid_Update(&g_pid, first, second);
        Uart_Printf("OK pid setpoint=%d feedback=%d output=%d\r\n", first,
            second, output);
        return true;
    }

    return false;
}

static bool HandleCal(const char *line)
{
    const char *args;
    uint32_t first;
    uint32_t second;
    AppParams_Status_t status;

    if (StrEquals(line, "cal show")) {
        WriteParams();
        return true;
    }

    if (StrStartsWith(line, "cal zero ")) {
        args = &line[9];
        if (!ParseU32Token(&args, &first) || !IsEnd(args) || (first > 4095U)) {
            Uart_WriteString("ERR cal zero command: cal zero 12\r\n");
            return true;
        }

        status = AppParams_SetAdcZero((uint16_t) first);
        Uart_WriteString("OK cal zero ");
        Uart_WriteString(AppParams_StatusText(status));
        Uart_WriteString("\r\n");
        return true;
    }

    if (StrStartsWith(line, "cal gain ")) {
        args = &line[9];
        if (!ParseU32Token(&args, &first) || !ParseU32Token(&args, &second) ||
            !IsEnd(args) || (first > 4095U) || (second > 3300U)) {
            Uart_WriteString("ERR cal gain command: cal gain 2482 2000\r\n");
            return true;
        }

        status = AppParams_SetAdcGain((uint16_t) first, (uint16_t) second);
        Uart_WriteString("OK cal gain ");
        Uart_WriteString(AppParams_StatusText(status));
        Uart_WriteString("\r\n");
        return true;
    }

    if (StrStartsWith(line, "cal apply ")) {
        args = &line[10];
        if (!ParseU32Token(&args, &first) || !IsEnd(args) || (first > 4095U)) {
            Uart_WriteString("ERR cal apply command: cal apply 2048\r\n");
            return true;
        }

        second = AppParams_ApplyAdcCalibration((uint16_t) first);
        Uart_Printf("OK cal apply raw=%lu corrected=%lu mv=%lu\r\n",
            (unsigned long) first, (unsigned long) second,
            (unsigned long) (((uint32_t) second * 3300UL) / 4095UL));
        return true;
    }

    return false;
}

static bool HandleAgc(const char *line)
{
    const AppParams_t *params = AppParams_Get();
    ControlAgc_t agc;
    P1_AdcStream_t stream;
    uint32_t rateHz = 10000UL;
    uint16_t count = 64U;
    ControlAgc_Action_t action;

    if (!StrEquals(line, "agc test")) {
        return false;
    }

    if (P1_CaptureAdcBlocking(rateHz, count, &stream) != P1_STATUS_OK) {
        Uart_WriteString("ERR agc adc capture failed\r\n");
        return true;
    }

    ControlAgc_Init(&agc, params->agcTargetPkpkRaw, params->agcHysteresisRaw,
        params->agcGainMin, params->agcGainMax, params->agcInitialGain);
    action = ControlAgc_Update(&agc,
        (uint16_t) (stream.maxRaw - stream.minRaw));

    Uart_Printf("OK agc pkpk=%u gain=%u action=%u target=%u hyst=%u\r\n",
        (uint16_t) (stream.maxRaw - stream.minRaw), agc.gain, action,
        params->agcTargetPkpkRaw, params->agcHysteresisRaw);
    return true;
}

static bool HandleParam(const char *line)
{
    AppParams_Status_t status;

    if (StrEquals(line, "param default")) {
        AppParams_ResetDefaults();
        PidLoadFromParams();
        Uart_WriteString("OK param default\r\n");
        return true;
    }
    if (StrEquals(line, "param save")) {
        status = AppParams_SaveToFlash();
        Uart_WriteString("OK param save ");
        Uart_WriteString(AppParams_StatusText(status));
        Uart_WriteString("\r\n");
        return true;
    }
    if (StrEquals(line, "param load")) {
        status = AppParams_LoadFromFlash();
        PidLoadFromParams();
        Uart_WriteString("OK param load ");
        Uart_WriteString(AppParams_StatusText(status));
        Uart_WriteString("\r\n");
        return true;
    }

    return false;
}

static void WriteP3Info(void)
{
    Uart_WriteString("OK p3 modules=pid/agc/adc_cal/output_cal/flash_params flash_page=0x0001FC00 size=1024\r\n");
}

static void WriteP3Test(void)
{
    WriteP3Info();
    PidLoadFromParams();
    Uart_Printf("OK p3test pid_output=%d\r\n",
        ControlPid_Update(&g_pid, 2000, 1200));
    WriteParams();
}

bool P3Lab_HandleCommand(const char *line)
{
    if (line == 0) {
        return false;
    }

    if (StrEquals(line, "p3")) {
        WriteP3Info();
        return true;
    }
    if (StrEquals(line, "p3test")) {
        WriteP3Test();
        return true;
    }
    if (HandlePid(line) || HandleCal(line) || HandleAgc(line) ||
        HandleParam(line)) {
        return true;
    }

    return false;
}

void P3Lab_WriteHelp(void)
{
    Uart_WriteString("Calibration/control: cal show/zero/gain/apply | rms cal/calzero/zero/gain | pid show/set/run | agc test | param default/save/load | p3/p3test\r\n");
}
