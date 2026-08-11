#include "tjc_screen.h"
#include "ad9833.h"
#include "ad9959.h"
#include "screen_uart.h"
#include "uart.h"
#include <ctype.h>
#include <string.h>

#define TJC_SCREEN_LINE_SIZE 48U
#define TJC_SCREEN_VALUE_SIZE 16U
#define TJC_SCREEN_LAST_FRAME_SIZE 16U

typedef enum {
    TJC_PENDING_NONE = 0,
    TJC_PENDING_FREQ_2KHZ,
    TJC_PENDING_AMP,
    TJC_PENDING_PHASE_SLIDER,
} TjcPending_t;

static TjcScreen_Status_t g_status;
static TjcPending_t g_pending;
static TjcScreen_Target_t g_pendingTarget;
static char g_line[TJC_SCREEN_LINE_SIZE];
static uint8_t g_lineLength;
static uint8_t g_valueBuffer[TJC_SCREEN_VALUE_SIZE];
static uint8_t g_valueLength;
static uint8_t g_ffCount;

static void SaveLastFrame(const uint8_t *data, uint8_t length)
{
    uint8_t copyLength = length;

    if (copyLength > TJC_SCREEN_LAST_FRAME_SIZE) {
        copyLength = TJC_SCREEN_LAST_FRAME_SIZE;
    }

    g_status.lastFrameLength = copyLength;
    for (uint8_t i = 0U; i < copyLength; i++) {
        g_status.lastFrame[i] = data[i];
        if ((data[i] >= 0x20U) && (data[i] <= 0x7EU) &&
            (i < (sizeof(g_status.lastText) - 1U))) {
            g_status.lastText[i] = (char) data[i];
        } else if (i < (sizeof(g_status.lastText) - 1U)) {
            g_status.lastText[i] = '.';
        }
    }

    if (copyLength < sizeof(g_status.lastText)) {
        g_status.lastText[copyLength] = '\0';
    } else {
        g_status.lastText[sizeof(g_status.lastText) - 1U] = '\0';
    }
}

static void CountErrorLine(const char *line)
{
    SaveLastFrame((const uint8_t *) line, (uint8_t) strlen(line));
    g_status.errorCount++;
}

static void CountErrorBytes(const uint8_t *data, uint8_t length)
{
    SaveLastFrame(data, length);
    g_status.errorCount++;
}

static bool StrEqualsNoCase(const char *a, const char *b)
{
    while ((*a != '\0') && (*b != '\0')) {
        if (tolower((unsigned char) *a) != tolower((unsigned char) *b)) {
            return false;
        }
        a++;
        b++;
    }
    return (*a == '\0') && (*b == '\0');
}

static bool ParseU32Text(const uint8_t *data, uint8_t length, uint32_t *value)
{
    uint32_t result = 0U;
    bool hasDigit = false;

    if ((data == 0) || (value == 0)) {
        return false;
    }

    for (uint8_t i = 0U; i < length; i++) {
        uint8_t c = data[i];

        if (c == '\r') {
            continue;
        }
        if ((c < '0') || (c > '9')) {
            return false;
        }
        hasDigit = true;
        result = (result * 10U) + (uint32_t) (c - '0');
    }

    if (!hasDigit) {
        return false;
    }
    *value = result;
    return true;
}

static uint32_t ParseU32LittleEndian(const uint8_t *data)
{
    return ((uint32_t) data[0]) | ((uint32_t) data[1] << 8U) |
           ((uint32_t) data[2] << 16U) | ((uint32_t) data[3] << 24U);
}

static AD9833_Waveform_t ToAd9833Wave(uint8_t waveform)
{
    switch (waveform) {
        case 1U:
            return AD9833_WAVE_TRIANGLE;
        case 2U:
            return AD9833_WAVE_SQUARE;
        case 0U:
        default:
            return AD9833_WAVE_SINE;
    }
}

static uint32_t ScaleFrequency(uint32_t raw)
{
    if (raw == 0U) {
        return 0U;
    }
    if (raw < 100000U) {
        return raw * 2000U;
    }
    return raw;
}

static uint16_t ScaleAd9959Amplitude(uint32_t raw)
{
    if (raw <= 10U) {
        raw *= 100U;
    } else if (raw <= 100U) {
        raw *= 10U;
    }
    if (raw > AD9959_MAX_AMPLITUDE) {
        raw = AD9959_MAX_AMPLITUDE;
    }
    return (uint16_t) raw;
}

static uint16_t ScaleAd9833Amplitude(uint32_t raw)
{
    if (raw <= 10U) {
        raw *= 25U;
    } else if (raw <= 100U) {
        raw = (raw * 255U) / 100U;
    }
    if (raw > 255U) {
        raw = 255U;
    }
    return (uint16_t) raw;
}

static uint16_t ScalePhase(uint32_t raw)
{
    int32_t phase = (int32_t) raw * 36L - 1800L;

    while (phase < 0L) {
        phase += 3600L;
    }
    phase %= 3600L;
    return (uint16_t) (phase / 10L);
}

static void ApplyOutputFor(TjcScreen_Target_t target)
{
    bool anyOk = false;

    if ((target == TJC_SCREEN_TARGET_BOTH) ||
        (target == TJC_SCREEN_TARGET_AD9833)) {
        AD9833_Channel_t channel = (g_status.ad9833Channel == 0U) ?
            AD9833_CHANNEL_1 : AD9833_CHANNEL_2;
        (void) AD9833_SetAmplitudeChannel(channel,
            (uint8_t) g_status.ad9833Amplitude);
        anyOk = AD9833_SetOutputChannel(channel,
                    ToAd9833Wave(g_status.waveform), g_status.frequencyHz,
                    g_status.phaseDeg) ||
                anyOk;
    }

    if ((target == TJC_SCREEN_TARGET_BOTH) ||
        (target == TJC_SCREEN_TARGET_AD9959)) {
        anyOk = AD9959_SetSingleTone((AD9959_Channel_t) g_status.ad9959Channel,
                    g_status.frequencyHz, g_status.phaseDeg,
                    g_status.ad9959Amplitude) ||
                anyOk;
    }

    if (anyOk) {
        g_status.commandCount++;
    } else {
        g_status.errorCount++;
    }
}

static void ApplyValue(uint32_t raw)
{
    switch (g_pending) {
        case TJC_PENDING_FREQ_2KHZ:
            g_status.frequencyHz = ScaleFrequency(raw);
            ApplyOutputFor(g_pendingTarget);
            break;
        case TJC_PENDING_AMP:
            g_status.ad9833Amplitude = ScaleAd9833Amplitude(raw);
            g_status.ad9959Amplitude = ScaleAd9959Amplitude(raw);
            ApplyOutputFor(g_pendingTarget);
            break;
        case TJC_PENDING_PHASE_SLIDER:
            g_status.phaseDeg = ScalePhase(raw);
            ApplyOutputFor(g_pendingTarget);
            break;
        case TJC_PENDING_NONE:
        default:
            g_status.errorCount++;
            break;
    }

    g_pending = TJC_PENDING_NONE;
    g_valueLength = 0U;
}

static void StartPending(TjcPending_t pending, TjcScreen_Target_t target)
{
    g_pending = pending;
    g_pendingTarget = target;
    g_status.target = target;
    g_valueLength = 0U;
}

static void HandleLineCommand(const char *line)
{
    if ((line == 0) || (line[0] == '\0')) {
        return;
    }

    if (StrEqualsNoCase(line, "CH1")) {
        if (g_status.target == TJC_SCREEN_TARGET_AD9833) {
            g_status.ad9833Channel = 0U;
        } else {
            g_status.ad9959Channel = 0U;
            if (g_status.target == TJC_SCREEN_TARGET_BOTH) {
                g_status.ad9833Channel = 0U;
            }
        }
    } else if (StrEqualsNoCase(line, "CH2")) {
        if (g_status.target == TJC_SCREEN_TARGET_AD9833) {
            g_status.ad9833Channel = 1U;
        } else {
            g_status.ad9959Channel = 1U;
            if (g_status.target == TJC_SCREEN_TARGET_BOTH) {
                g_status.ad9833Channel = 1U;
            }
        }
    } else if (StrEqualsNoCase(line, "CH3")) {
        g_status.ad9959Channel = 2U;
    } else if (StrEqualsNoCase(line, "CH4")) {
        g_status.ad9959Channel = 3U;
    } else if (StrEqualsNoCase(line, "AD9833")) {
        g_status.target = TJC_SCREEN_TARGET_AD9833;
    } else if (StrEqualsNoCase(line, "AD9959")) {
        g_status.target = TJC_SCREEN_TARGET_AD9959;
    } else if (StrEqualsNoCase(line, "BOTH") ||
               StrEqualsNoCase(line, "ALL")) {
        g_status.target = TJC_SCREEN_TARGET_BOTH;
    } else if (StrEqualsNoCase(line, "AD9959_Fre") ||
               StrEqualsNoCase(line, "AD9959_FREQ") ||
               StrEqualsNoCase(line, "AD9833_Fre") ||
               StrEqualsNoCase(line, "AD9833_FREQ") ||
               StrEqualsNoCase(line, "FREQ") ||
               StrEqualsNoCase(line, "FRE")) {
        if (StrEqualsNoCase(line, "AD9833_Fre") ||
            StrEqualsNoCase(line, "AD9833_FREQ")) {
            StartPending(TJC_PENDING_FREQ_2KHZ, TJC_SCREEN_TARGET_AD9833);
        } else if (StrEqualsNoCase(line, "AD9959_Fre") ||
                   StrEqualsNoCase(line, "AD9959_FREQ")) {
            StartPending(TJC_PENDING_FREQ_2KHZ, TJC_SCREEN_TARGET_AD9959);
        } else {
            StartPending(TJC_PENDING_FREQ_2KHZ, g_status.target);
        }
    } else if (StrEqualsNoCase(line, "AD9959_amp") ||
               StrEqualsNoCase(line, "AD9959_AMP") ||
               StrEqualsNoCase(line, "AD9833_amp") ||
               StrEqualsNoCase(line, "AD9833_AMP") ||
               StrEqualsNoCase(line, "AMP")) {
        if (StrEqualsNoCase(line, "AD9833_amp") ||
            StrEqualsNoCase(line, "AD9833_AMP")) {
            StartPending(TJC_PENDING_AMP, TJC_SCREEN_TARGET_AD9833);
        } else if (StrEqualsNoCase(line, "AD9959_amp") ||
                   StrEqualsNoCase(line, "AD9959_AMP")) {
            StartPending(TJC_PENDING_AMP, TJC_SCREEN_TARGET_AD9959);
        } else {
            StartPending(TJC_PENDING_AMP, g_status.target);
        }
    } else if (StrEqualsNoCase(line, "AD9959_PHASE") ||
               StrEqualsNoCase(line, "AD9959_Pha") ||
               StrEqualsNoCase(line, "AD9833_PHASE") ||
               StrEqualsNoCase(line, "PHASE")) {
        if (StrEqualsNoCase(line, "AD9833_PHASE")) {
            StartPending(TJC_PENDING_PHASE_SLIDER, TJC_SCREEN_TARGET_AD9833);
        } else if (StrEqualsNoCase(line, "AD9959_PHASE") ||
                   StrEqualsNoCase(line, "AD9959_Pha")) {
            StartPending(TJC_PENDING_PHASE_SLIDER, TJC_SCREEN_TARGET_AD9959);
        } else {
            StartPending(TJC_PENDING_PHASE_SLIDER, g_status.target);
        }
    } else if (StrEqualsNoCase(line, "sin") ||
               StrEqualsNoCase(line, "sine")) {
        g_status.waveform = 0U;
        ApplyOutputFor(TJC_SCREEN_TARGET_AD9833);
    } else if (StrEqualsNoCase(line, "triangle") ||
               StrEqualsNoCase(line, "tri")) {
        g_status.waveform = 1U;
        ApplyOutputFor(TJC_SCREEN_TARGET_AD9833);
    } else if (StrEqualsNoCase(line, "square")) {
        g_status.waveform = 2U;
        ApplyOutputFor(TJC_SCREEN_TARGET_AD9833);
    } else if (StrEqualsNoCase(line, "key_val 01") ||
               StrEqualsNoCase(line, "key_val 02") ||
               StrEqualsNoCase(line, "key_val 03") ||
               StrEqualsNoCase(line, "key_val 04")) {
        SaveLastFrame((const uint8_t *) line, (uint8_t) strlen(line));
    } else {
        CountErrorLine(line);
    }
}

static void FeedPendingValue(uint8_t data)
{
    if (g_valueLength >= TJC_SCREEN_VALUE_SIZE) {
        CountErrorBytes(g_valueBuffer, g_valueLength);
        g_pending = TJC_PENDING_NONE;
        g_valueLength = 0U;
        return;
    }

    g_valueBuffer[g_valueLength++] = data;

    if ((g_valueLength == 6U) && (g_valueBuffer[4] == '\r') &&
        (g_valueBuffer[5] == '\n')) {
        ApplyValue(ParseU32LittleEndian(g_valueBuffer));
        return;
    }

    if (data == '\n') {
        uint32_t value;
        uint8_t length = (g_valueLength > 0U) ? (g_valueLength - 1U) : 0U;

        if (ParseU32Text(g_valueBuffer, length, &value)) {
            ApplyValue(value);
        } else if (g_valueLength >= 6U) {
            CountErrorBytes(g_valueBuffer, g_valueLength);
            g_pending = TJC_PENDING_NONE;
            g_valueLength = 0U;
        }
    } else if (g_valueLength >= 6U) {
        CountErrorBytes(g_valueBuffer, g_valueLength);
        g_pending = TJC_PENDING_NONE;
        g_valueLength = 0U;
    }
}

static void FeedLineByte(uint8_t data)
{
    if (data == '\n') {
        if ((g_lineLength > 0U) && (g_line[g_lineLength - 1U] == '\r')) {
            g_lineLength--;
        }
        g_line[g_lineLength] = '\0';
        SaveLastFrame((const uint8_t *) g_line, g_lineLength);
        HandleLineCommand(g_line);
        g_lineLength = 0U;
    } else if (g_lineLength < (TJC_SCREEN_LINE_SIZE - 1U)) {
        g_line[g_lineLength++] = (char) data;
    } else {
        CountErrorBytes((const uint8_t *) g_line, g_lineLength);
        g_lineLength = 0U;
    }
}

void TjcScreen_Init(void)
{
    memset(&g_status, 0, sizeof(g_status));
    g_status.target = TJC_SCREEN_TARGET_BOTH;
    g_status.frequencyHz = 1000U;
    g_status.ad9833Amplitude = 128U;
    g_status.ad9959Amplitude = AD9959_MAX_AMPLITUDE;
    g_pending = TJC_PENDING_NONE;
    g_pendingTarget = TJC_SCREEN_TARGET_BOTH;
    g_lineLength = 0U;
    g_valueLength = 0U;
    g_ffCount = 0U;

    ScreenUart_Init();
    TjcScreen_SendCommand("bkcmd=0");
}

void TjcScreen_Proc(void)
{
    uint8_t data;

    while (ScreenUart_ReadByte(&data)) {
        if (data == 0xFFU) {
            if (g_ffCount < 3U) {
                g_ffCount++;
            }
            if (g_ffCount >= 3U) {
                g_lineLength = 0U;
                g_valueLength = 0U;
                g_ffCount = 0U;
            }
            continue;
        }
        g_ffCount = 0U;

        if (g_pending != TJC_PENDING_NONE) {
            FeedPendingValue(data);
        } else {
            FeedLineByte(data);
        }
    }
}

TjcScreen_Status_t TjcScreen_GetStatus(void)
{
    return g_status;
}

void TjcScreen_SendCommand(const char *command)
{
    ScreenUart_WriteString(command);
    ScreenUart_WriteCommandEnd();
}
