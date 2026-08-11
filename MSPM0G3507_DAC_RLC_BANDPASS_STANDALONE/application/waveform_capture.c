#include "waveform_capture.h"
#include <stdbool.h>
#include <stdint.h>
#include "adc.h"
#include "adc_fml.h"
#include "tim.h"
#include "uart.h"

#define WAVEFORM_SAMPLE_COUNT      (80U)
#define WAVEFORM_SAMPLE_PERIOD_MS  (1U)
#define WAVEFORM_ADC_TIMEOUT_LOOPS (20000U)
#define WAVEFORM_GRAPH_WIDTH       (48U)
#define VOFA_SAMPLE_PERIOD_MS      (5U)

static uint16_t g_rawSamples[WAVEFORM_SAMPLE_COUNT];
static uint16_t g_filteredSamples[WAVEFORM_SAMPLE_COUNT];
static bool g_vofaStreaming;
static uint32_t g_vofaNextTick;
static uint16_t g_vofaHistory[3];
static uint8_t g_vofaHistoryCount;

static uint16_t Average3(uint16_t left, uint16_t center, uint16_t right)
{
    return (uint16_t) (((uint32_t) left + center + right) / 3U);
}

static void WriteLineEnd(void)
{
    Uart_WriteString("\r\n");
}

static void WriteSampleIndex(uint8_t index)
{
    if (index < 10U) {
        Uart_WriteString("00");
    } else if (index < 100U) {
        Uart_WriteByte('0');
    }
    Uart_WriteU32(index);
}

static bool CaptureSamples(void)
{
    uint32_t nextTick = Timer_GetTickMs();

    for (uint8_t i = 0U; i < WAVEFORM_SAMPLE_COUNT; i++) {
        while ((int32_t) (Timer_GetTickMs() - nextTick) < 0) {
            __WFI();
        }

        if (!ADC_SampleBlocking(&g_rawSamples[i], WAVEFORM_ADC_TIMEOUT_LOOPS)) {
            return false;
        }

        nextTick += WAVEFORM_SAMPLE_PERIOD_MS;
    }

    g_filteredSamples[0] = Average3(g_rawSamples[0], g_rawSamples[0], g_rawSamples[1]);
    for (uint8_t i = 1U; i < (WAVEFORM_SAMPLE_COUNT - 1U); i++) {
        g_filteredSamples[i] =
            Average3(g_rawSamples[i - 1U], g_rawSamples[i], g_rawSamples[i + 1U]);
    }
    g_filteredSamples[WAVEFORM_SAMPLE_COUNT - 1U] =
        Average3(g_rawSamples[WAVEFORM_SAMPLE_COUNT - 2U],
            g_rawSamples[WAVEFORM_SAMPLE_COUNT - 1U],
            g_rawSamples[WAVEFORM_SAMPLE_COUNT - 1U]);

    return true;
}

static void CalculateStats(uint16_t *minRaw, uint16_t *maxRaw, uint16_t *avgRaw)
{
    uint32_t sum = 0U;
    uint16_t minValue = g_filteredSamples[0];
    uint16_t maxValue = g_filteredSamples[0];

    for (uint8_t i = 0U; i < WAVEFORM_SAMPLE_COUNT; i++) {
        uint16_t value = g_filteredSamples[i];

        if (value < minValue) {
            minValue = value;
        }
        if (value > maxValue) {
            maxValue = value;
        }
        sum += value;
    }

    *minRaw = minValue;
    *maxRaw = maxValue;
    *avgRaw = (uint16_t) (sum / WAVEFORM_SAMPLE_COUNT);
}

static void PrintStats(uint16_t minRaw, uint16_t maxRaw, uint16_t avgRaw)
{
    Uart_WriteString("OK wave samples=");
    Uart_WriteU32(WAVEFORM_SAMPLE_COUNT);
    Uart_WriteString(" period_ms=");
    Uart_WriteU32(WAVEFORM_SAMPLE_PERIOD_MS);
    Uart_WriteString(" min=");
    Uart_WriteU32(minRaw);
    Uart_WriteString(" max=");
    Uart_WriteU32(maxRaw);
    Uart_WriteString(" avg=");
    Uart_WriteU32(avgRaw);
    Uart_WriteString(" pkpk=");
    Uart_WriteU32((uint32_t) maxRaw - minRaw);
    Uart_WriteString(" avg_mv=");
    Uart_WriteU32(ADC_Fml_RawToMillivolt(avgRaw));
    WriteLineEnd();
}

static uint8_t ScaleToColumn(uint16_t value, uint16_t minRaw, uint16_t maxRaw)
{
    uint32_t span = (uint32_t) maxRaw - minRaw;

    if (span == 0U) {
        return WAVEFORM_GRAPH_WIDTH / 2U;
    }

    return (uint8_t) ((((uint32_t) value - minRaw) * (WAVEFORM_GRAPH_WIDTH - 1U)) /
                      span);
}

void Waveform_PrintAscii(void)
{
    uint16_t minRaw;
    uint16_t maxRaw;
    uint16_t avgRaw;

    Uart_WriteString("OK wave capture start\r\n");
    if (!CaptureSamples()) {
        Uart_WriteString("ERR wave ADC timeout\r\n");
        return;
    }

    CalculateStats(&minRaw, &maxRaw, &avgRaw);
    PrintStats(minRaw, maxRaw, avgRaw);

    Uart_WriteString("idx raw filt mv waveform\r\n");
    for (uint8_t i = 0U; i < WAVEFORM_SAMPLE_COUNT; i++) {
        uint8_t column = ScaleToColumn(g_filteredSamples[i], minRaw, maxRaw);

        WriteSampleIndex(i);
        Uart_WriteByte(' ');
        Uart_WriteU32(g_rawSamples[i]);
        Uart_WriteByte(' ');
        Uart_WriteU32(g_filteredSamples[i]);
        Uart_WriteByte(' ');
        Uart_WriteU32(ADC_Fml_RawToMillivolt(g_filteredSamples[i]));
        Uart_WriteString(" |");

        for (uint8_t x = 0U; x < WAVEFORM_GRAPH_WIDTH; x++) {
            Uart_WriteByte((x == column) ? '*' : ' ');
        }
        Uart_WriteString("|\r\n");
    }
    Uart_WriteString("OK wave done\r\n");
}

void Waveform_PrintCsv(void)
{
    uint16_t minRaw;
    uint16_t maxRaw;
    uint16_t avgRaw;

    Uart_WriteString("OK wavecsv capture start\r\n");
    if (!CaptureSamples()) {
        Uart_WriteString("ERR wave ADC timeout\r\n");
        return;
    }

    CalculateStats(&minRaw, &maxRaw, &avgRaw);
    PrintStats(minRaw, maxRaw, avgRaw);

    Uart_WriteString("idx,raw,filtered,mv\r\n");
    for (uint8_t i = 0U; i < WAVEFORM_SAMPLE_COUNT; i++) {
        Uart_WriteU32(i);
        Uart_WriteByte(',');
        Uart_WriteU32(g_rawSamples[i]);
        Uart_WriteByte(',');
        Uart_WriteU32(g_filteredSamples[i]);
        Uart_WriteByte(',');
        Uart_WriteU32(ADC_Fml_RawToMillivolt(g_filteredSamples[i]));
        WriteLineEnd();
    }
    Uart_WriteString("OK wavecsv done\r\n");
}

void Waveform_VofaStart(void)
{
    g_vofaStreaming    = true;
    g_vofaNextTick     = Timer_GetTickMs();
    g_vofaHistory[0]   = 0U;
    g_vofaHistory[1]   = 0U;
    g_vofaHistory[2]   = 0U;
    g_vofaHistoryCount = 0U;
}

void Waveform_VofaStop(void)
{
    g_vofaStreaming = false;
}

bool Waveform_IsVofaStreaming(void)
{
    return g_vofaStreaming;
}

void Waveform_VofaProc(void)
{
    uint16_t raw;
    uint16_t filtered;

    if (!g_vofaStreaming) {
        return;
    }

    if ((int32_t) (Timer_GetTickMs() - g_vofaNextTick) < 0) {
        return;
    }
    g_vofaNextTick += VOFA_SAMPLE_PERIOD_MS;

    if (!ADC_SampleBlocking(&raw, WAVEFORM_ADC_TIMEOUT_LOOPS)) {
        return;
    }

    g_vofaHistory[0] = g_vofaHistory[1];
    g_vofaHistory[1] = g_vofaHistory[2];
    g_vofaHistory[2] = raw;
    if (g_vofaHistoryCount < 3U) {
        g_vofaHistoryCount++;
    }

    if (g_vofaHistoryCount < 3U) {
        filtered = raw;
    } else {
        filtered = Average3(g_vofaHistory[0], g_vofaHistory[1], g_vofaHistory[2]);
    }

    Uart_WriteU32(raw);
    Uart_WriteByte(',');
    Uart_WriteU32(filtered);
    Uart_WriteByte(',');
    Uart_WriteU32(ADC_Fml_RawToMillivolt(filtered));
    WriteLineEnd();
}
