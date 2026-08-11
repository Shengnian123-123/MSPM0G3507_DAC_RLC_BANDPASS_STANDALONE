#include "test_console.h"
#include <stdbool.h>
#include <stdint.h>
#include "adc_apl.h"
#include "adc_bll.h"
#include "adc_fml.h"
#include "ads1220.h"
#include "ad9833.h"
#include "ad9959.h"
#include "delay.h"
#include "gpio_io.h"
#include "p1_lab.h"
#include "p2_lab.h"
#include "p3_lab.h"
#include "p4_lab.h"
#include "signal_tools_lab.h"
#include "spi.h"
#include "tim.h"
#include "tjc_screen.h"
#include "uart.h"
#include "waveform_clone.h"
#include "waveform_capture.h"

#define TEST_LINE_SIZE (64U)

static char g_line[TEST_LINE_SIZE];
static uint8_t g_lineLength;

typedef void (*ConsoleExactHandler_t)(void);

typedef struct {
    const char *command;
    ConsoleExactHandler_t handler;
} ConsoleExactCommand_t;

static bool Str_Equals(const char *left, const char *right)
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

static bool Str_StartsWith(const char *text, const char *prefix)
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

static uint8_t HexDigitValue(char digit)
{
    if ((digit >= '0') && (digit <= '9')) {
        return (uint8_t) (digit - '0');
    }
    if ((digit >= 'a') && (digit <= 'f')) {
        return (uint8_t) (digit - 'a' + 10);
    }
    if ((digit >= 'A') && (digit <= 'F')) {
        return (uint8_t) (digit - 'A' + 10);
    }

    return 0xFFU;
}

static bool ParseHexByte(const char *text, uint8_t *value)
{
    uint8_t high;
    uint8_t low;

    if ((text == 0) || (value == 0)) {
        return false;
    }

    if (Str_StartsWith(text, "0x") || Str_StartsWith(text, "0X")) {
        text += 2;
    }

    high = HexDigitValue(text[0]);
    if (high == 0xFFU) {
        return false;
    }

    low = HexDigitValue(text[1]);
    if (low == 0xFFU) {
        *value = high;
        return true;
    }

    *value = (uint8_t) ((high << 4U) | low);
    return true;
}

static bool ParseU32(const char *text, uint32_t *value)
{
    uint32_t result = 0U;
    bool hasDigit   = false;

    if ((text == 0) || (value == 0)) {
        return false;
    }

    while (*text == ' ') {
        text++;
    }

    while ((*text >= '0') && (*text <= '9')) {
        uint32_t digit = (uint32_t) (*text - '0');

        if (result > ((0xFFFFFFFFUL - digit) / 10UL)) {
            return false;
        }
        result   = (result * 10UL) + digit;
        hasDigit = true;
        text++;
    }

    while (*text == ' ') {
        text++;
    }

    if ((*text != '\0') || !hasDigit) {
        return false;
    }

    *value = result;
    return true;
}

static bool ParseTwoU32(const char *text, uint32_t *first, uint32_t *second)
{
    uint32_t a = 0U;
    uint32_t b = 0U;
    bool hasDigit;

    if ((text == 0) || (first == 0) || (second == 0)) {
        return false;
    }

    while (*text == ' ') {
        text++;
    }

    hasDigit = false;
    while ((*text >= '0') && (*text <= '9')) {
        uint32_t digit = (uint32_t) (*text - '0');

        if (a > ((0xFFFFFFFFUL - digit) / 10UL)) {
            return false;
        }
        a        = (a * 10UL) + digit;
        hasDigit = true;
        text++;
    }
    if (!hasDigit || (*text != ' ')) {
        return false;
    }

    while (*text == ' ') {
        text++;
    }

    hasDigit = false;
    while ((*text >= '0') && (*text <= '9')) {
        uint32_t digit = (uint32_t) (*text - '0');

        if (b > ((0xFFFFFFFFUL - digit) / 10UL)) {
            return false;
        }
        b        = (b * 10UL) + digit;
        hasDigit = true;
        text++;
    }

    while (*text == ' ') {
        text++;
    }

    if ((*text != '\0') || !hasDigit) {
        return false;
    }

    *first  = a;
    *second = b;
    return true;
}

static const char *AdcStateText(ADC_Bll_State_t state)
{
    switch (state) {
        case ADC_BLL_STATE_LOW:
            return "LOW";
        case ADC_BLL_STATE_HIGH:
            return "HIGH";
        case ADC_BLL_STATE_NORMAL:
        default:
            return "NORMAL";
    }
}

static void WriteOkPrefix(const char *name)
{
    Uart_WriteString("OK ");
    Uart_WriteString(name);
    Uart_WriteString(" ");
}

static void WriteAdcResult(void)
{
    ADC_Bll_Result_t result;

    if (!ADC_Bll_SampleAndDetect(&result)) {
        Uart_WriteString("ERR ADC timeout\r\n");
        return;
    }

    WriteOkPrefix("adc");
    Uart_WriteString("raw=");
    Uart_WriteU32(result.raw);
    Uart_WriteString(" mv=");
    Uart_WriteU32(result.millivolt);
    Uart_WriteString(" state=");
    Uart_WriteString(AdcStateText(result.state));
    Uart_WriteString("\r\n");
}

static void WriteTimerResult(void)
{
    uint32_t startMs = Timer_GetTickMs();
    uint32_t start10 = Timer_GetTimer1Ticks();

    while ((Timer_GetTickMs() - startMs) < 50U) {
        __WFI();
    }

    WriteOkPrefix("timer");
    Uart_WriteString("delta_ms=");
    Uart_WriteU32(Timer_GetTickMs() - startMs);
    Uart_WriteString(" delta_10ms=");
    Uart_WriteU32(Timer_GetTimer1Ticks() - start10);
    Uart_WriteString(" tick_ms=");
    Uart_WriteU32(Timer_GetTickMs());
    Uart_WriteString("\r\n");
}

static void WriteSpiResult(uint8_t txData)
{
    uint8_t rxData = 0U;

    SoftSPI_Transfer(&txData, &rxData, 1U);

    WriteOkPrefix("spi");
    Uart_WriteString("tx=0x");
    Uart_WriteHex8(txData);
    Uart_WriteString(" rx=0x");
    Uart_WriteHex8(rxData);
    Uart_WriteString(" loopback=");
    Uart_WriteString((rxData == txData) ? "PASS" : "OPEN_OR_FAIL");
    Uart_WriteString("\r\n");
}

static void WriteP0Info(void)
{
    WriteOkPrefix("board");
    Uart_Printf("cpu_hz=%lu uart=115200 led=PA0 key=PB10 buzzer=PB11\r\n",
        (unsigned long) CPUCLK_FREQ);
}

static void WriteAllDiagnostics(void)
{
    WriteOkPrefix("diag");
    Uart_WriteString("start\r\n");
    WriteAdcResult();
    WriteTimerResult();
    WriteSpiResult(0xA5U);
    Waveform_PrintAscii();
    WriteOkPrefix("diag");
    Uart_WriteString("done\r\n");
}

static void WritePrintfTest(void)
{
    Uart_Printf("OK printf tick_ms=%lu timer10=%lu cpu_hz=%lu\r\n",
        (unsigned long) Timer_GetTickMs(),
        (unsigned long) Timer_GetTimer1Ticks(), (unsigned long) CPUCLK_FREQ);
}

static void WriteLedOn(void)
{
    GPIOIO_LedOn();
    Uart_WriteString("OK led on\r\n");
}

static void WriteLedOff(void)
{
    GPIOIO_LedOff();
    Uart_WriteString("OK led off\r\n");
}

static void WriteLedToggle(void)
{
    GPIOIO_LedToggle();
    Uart_WriteString("OK led toggle\r\n");
}

static void WriteKeyState(void)
{
    WriteOkPrefix("key");
    Uart_WriteString(GPIOIO_KeyIsPressed() ? "pressed=1\r\n" : "pressed=0\r\n");
}

static void WriteBuzzerOn(void)
{
    GPIOIO_BuzzerOn();
    Uart_WriteString("OK buzz on\r\n");
}

static void WriteBuzzerOff(void)
{
    GPIOIO_BuzzerOff();
    Uart_WriteString("OK buzz off\r\n");
}

static void WriteBuzzerBeep(void)
{
    GPIOIO_BuzzerBeep(100U);
    Uart_WriteString("OK buzz beep 100ms\r\n");
}

static void WriteDdsSingleTone(uint8_t channel, uint32_t frequencyHz)
{
    if ((channel >= AD9959_CHANNEL_COUNT) ||
        !AD9959_SetSingleTone((AD9959_Channel_t) channel, frequencyHz, 0U,
            AD9959_MAX_AMPLITUDE)) {
        Uart_WriteString("ERR dds expects ch0..ch3 and frequency below AD9959 sysclk/2\r\n");
        return;
    }

    WriteOkPrefix("dds");
    Uart_WriteString("ch=");
    Uart_WriteU32(channel);
    Uart_WriteString(" freq_hz=");
    Uart_WriteU32(frequencyHz);
    Uart_WriteString(" amp=1023 phase_deg=0 sysclk_hz=");
    Uart_WriteU32(AD9959_GetSysclkHz());
    Uart_WriteString("\r\n");
}

static void WriteDdsTest(void)
{
    if (!AD9959_SelfTestPattern()) {
        Uart_WriteString("ERR dds test failed\r\n");
        return;
    }

    Uart_WriteString("OK dds test CH0=1MHz CH1=2MHz CH2=3MHz CH3=4MHz\r\n");
}

static void WriteDdsPower(bool down)
{
    AD9959_PowerDown(down);
    Uart_WriteString(down ? "OK dds power down\r\n" : "OK dds power up\r\n");
}

static void WriteTjcStatus(void)
{
    const TjcScreen_Status_t status = TjcScreen_GetStatus();

    Uart_WriteString("OK tjc uart=UART3 PB12_TX PB13_RX baud=115200 target=");
    switch (status.target) {
        case TJC_SCREEN_TARGET_AD9833:
            Uart_WriteString("ad9833");
            break;
        case TJC_SCREEN_TARGET_AD9959:
            Uart_WriteString("ad9959");
            break;
        case TJC_SCREEN_TARGET_BOTH:
        default:
            Uart_WriteString("both");
            break;
    }
    Uart_WriteString(" ad9833_ch=");
    Uart_WriteU32(status.ad9833Channel);
    Uart_WriteString(" ad9959_ch=");
    Uart_WriteU32(status.ad9959Channel);
    Uart_WriteString(" freq_hz=");
    Uart_WriteU32(status.frequencyHz);
    Uart_WriteString(" ad9833_amp=");
    Uart_WriteU32(status.ad9833Amplitude);
    Uart_WriteString(" ad9959_amp=");
    Uart_WriteU32(status.ad9959Amplitude);
    Uart_WriteString(" phase_deg=");
    Uart_WriteU32(status.phaseDeg);
    Uart_WriteString(" wave=");
    if (status.waveform == 1U) {
        Uart_WriteString("tri");
    } else if (status.waveform == 2U) {
        Uart_WriteString("square");
    } else {
        Uart_WriteString("sine");
    }
    Uart_WriteString(" ok=");
    Uart_WriteU32(status.commandCount);
    Uart_WriteString(" err=");
    Uart_WriteU32(status.errorCount);
    Uart_WriteString(" last_hex=");
    for (uint8_t i = 0U; i < status.lastFrameLength; i++) {
        Uart_WriteHex8(status.lastFrame[i]);
    }
    Uart_WriteString(" last_text=");
    Uart_WriteString(status.lastText);
    Uart_WriteString("\r\n");
}

static bool ParseDdsChannelValue(const char *text, uint8_t *channel,
    uint32_t *value)
{
    if ((text == 0) || (channel == 0) || (value == 0)) {
        return false;
    }
    if (!Str_StartsWith(text, "ch") || (text[2] < '0') || (text[2] > '3') ||
        (text[3] != ' ')) {
        return false;
    }

    *channel = (uint8_t) (text[2] - '0');
    return ParseU32(&text[4], value);
}

static bool HandleDdsChannelCommand(const char *line)
{
    uint32_t frequencyHz;
    uint8_t channel;

    if (!Str_StartsWith(line, "dds ch")) {
        return false;
    }
    if ((line[6] < '0') || (line[6] > '3') || (line[7] != ' ')) {
        Uart_WriteString("ERR dds channel command: dds ch0 1000000\r\n");
        return true;
    }
    if (!ParseU32(&line[8], &frequencyHz)) {
        Uart_WriteString("ERR dds frequency must be decimal Hz\r\n");
        return true;
    }

    channel = (uint8_t) (line[6] - '0');
    WriteDdsSingleTone(channel, frequencyHz);
    return true;
}

static bool HandleDdsAmpCommand(const char *line)
{
    uint32_t amplitude;
    uint8_t channel;

    if (!Str_StartsWith(line, "dds amp ")) {
        return false;
    }
    if (!ParseDdsChannelValue(&line[8], &channel, &amplitude) ||
        (amplitude > AD9959_MAX_AMPLITUDE)) {
        Uart_WriteString("ERR dds amp command: dds amp ch0 1023\r\n");
        return true;
    }
    if (!AD9959_SetAmplitude((AD9959_Channel_t) channel, (uint16_t) amplitude)) {
        Uart_WriteString("ERR dds amp failed\r\n");
        return true;
    }

    WriteOkPrefix("dds");
    Uart_WriteString("ch=");
    Uart_WriteU32(channel);
    Uart_WriteString(" amp=");
    Uart_WriteU32(amplitude);
    Uart_WriteString("\r\n");
    return true;
}

static bool HandleDdsPhaseCommand(const char *line)
{
    uint32_t phaseDeg;
    uint8_t channel;

    if (!Str_StartsWith(line, "dds phase ")) {
        return false;
    }
    if (!ParseDdsChannelValue(&line[10], &channel, &phaseDeg) ||
        (phaseDeg >= 360UL)) {
        Uart_WriteString("ERR dds phase command: dds phase ch0 90\r\n");
        return true;
    }
    if (!AD9959_SetPhase((AD9959_Channel_t) channel, (uint16_t) phaseDeg)) {
        Uart_WriteString("ERR dds phase failed\r\n");
        return true;
    }

    WriteOkPrefix("dds");
    Uart_WriteString("ch=");
    Uart_WriteU32(channel);
    Uart_WriteString(" phase_deg=");
    Uart_WriteU32(phaseDeg);
    Uart_WriteString("\r\n");
    return true;
}

static bool HandleDdsSysclkCommand(const char *line)
{
    uint32_t sysclkHz;

    if (!Str_StartsWith(line, "dds sysclk ")) {
        return false;
    }
    if (!ParseU32(&line[11], &sysclkHz) || (sysclkHz == 0U)) {
        Uart_WriteString("ERR dds sysclk command: dds sysclk 500000000\r\n");
        return true;
    }

    AD9959_SetSysclkHz(sysclkHz);
    WriteOkPrefix("dds");
    Uart_WriteString("sysclk_hz=");
    Uart_WriteU32(AD9959_GetSysclkHz());
    Uart_WriteString("\r\n");
    return true;
}

static const char *Ad9833WaveText(AD9833_Waveform_t waveform)
{
    switch (waveform) {
        case AD9833_WAVE_TRIANGLE:
            return "triangle";
        case AD9833_WAVE_SQUARE:
            return "square";
        case AD9833_WAVE_SINE:
        default:
            return "sine";
    }
}

static void WriteAd9833Output(AD9833_Channel_t channel,
    AD9833_Waveform_t waveform, uint32_t frequencyHz)
{
    if (!AD9833_SetOutputChannel(channel, waveform, frequencyHz, 0U)) {
        Uart_WriteString("ERR ad9833 expects frequency < MCLK/2 and phase 0..359\r\n");
        return;
    }

    WriteOkPrefix("ad9833");
    Uart_WriteString("wave=");
    Uart_WriteString(Ad9833WaveText(waveform));
    Uart_WriteString(" ch=");
    if (channel == AD9833_CHANNEL_1) {
        Uart_WriteString("1");
    } else if (channel == AD9833_CHANNEL_2) {
        Uart_WriteString("2");
    } else {
        Uart_WriteString("all");
    }
    Uart_WriteString(" freq_hz=");
    Uart_WriteU32(frequencyHz);
    Uart_WriteString(" mclk_hz=");
    Uart_WriteU32(AD9833_GetMclkChannel(channel));
    Uart_WriteString("\r\n");
}

static bool ParseAd9833WaveFrequency(const char *text,
    AD9833_Waveform_t *waveform, uint32_t *frequencyHz)
{
    if (Str_StartsWith(text, "sine ")) {
        *waveform = AD9833_WAVE_SINE;
        return ParseU32(&text[5], frequencyHz);
    }
    if (Str_StartsWith(text, "tri ")) {
        *waveform = AD9833_WAVE_TRIANGLE;
        return ParseU32(&text[4], frequencyHz);
    }
    if (Str_StartsWith(text, "square ")) {
        *waveform = AD9833_WAVE_SQUARE;
        return ParseU32(&text[7], frequencyHz);
    }
    return false;
}

static bool HandleAd9833ChannelCommand(const char *line)
{
    AD9833_Channel_t channel;
    AD9833_Waveform_t waveform;
    uint32_t frequencyHz;
    const char *args;

    if (Str_StartsWith(line, "ad9833 ch1 ")) {
        channel = AD9833_CHANNEL_1;
        args = &line[11];
    } else if (Str_StartsWith(line, "ad9833 ch2 ")) {
        channel = AD9833_CHANNEL_2;
        args = &line[11];
    } else {
        return false;
    }

    if (Str_Equals(args, "off")) {
        AD9833_ResetOutputChannel(channel);
        Uart_WriteString("OK ad9833 off ch=");
        Uart_WriteString((channel == AD9833_CHANNEL_1) ? "1\r\n" : "2\r\n");
        return true;
    }

    if (!ParseAd9833WaveFrequency(args, &waveform, &frequencyHz)) {
        Uart_WriteString("ERR ad9833 channel command: ad9833 ch1|ch2 sine|tri|square <hz>\r\n");
        return true;
    }
    WriteAd9833Output(channel, waveform, frequencyHz);
    return true;
}

static bool HandleAd9833Command(const char *line)
{
    uint32_t value;

    if (HandleAd9833ChannelCommand(line)) {
        return true;
    }

    if (Str_Equals(line, "ad9833 init")) {
        AD9833_Init();
        Uart_WriteString("OK ad9833 init\r\n");
        return true;
    }
    if (Str_Equals(line, "ad9833 test")) {
        if (AD9833_SelfTestPattern()) {
            Uart_WriteString("OK ad9833 test sine 1000Hz\r\n");
        } else {
            Uart_WriteString("ERR ad9833 test failed\r\n");
        }
        return true;
    }
    if (Str_Equals(line, "ad9833 off")) {
        AD9833_ResetOutput();
        Uart_WriteString("OK ad9833 off\r\n");
        return true;
    }
    if (Str_StartsWith(line, "ad9833 sine ")) {
        if (ParseU32(&line[12], &value)) {
            WriteAd9833Output(AD9833_CHANNEL_ALL, AD9833_WAVE_SINE, value);
        } else {
            Uart_WriteString("ERR ad9833 sine command: ad9833 sine 1000\r\n");
        }
        return true;
    }
    if (Str_StartsWith(line, "ad9833 tri ")) {
        if (ParseU32(&line[11], &value)) {
            WriteAd9833Output(AD9833_CHANNEL_ALL, AD9833_WAVE_TRIANGLE, value);
        } else {
            Uart_WriteString("ERR ad9833 tri command: ad9833 tri 1000\r\n");
        }
        return true;
    }
    if (Str_StartsWith(line, "ad9833 square ")) {
        if (ParseU32(&line[14], &value)) {
            WriteAd9833Output(AD9833_CHANNEL_ALL, AD9833_WAVE_SQUARE, value);
        } else {
            Uart_WriteString("ERR ad9833 square command: ad9833 square 1000\r\n");
        }
        return true;
    }
    if (Str_StartsWith(line, "ad9833 amp ")) {
        if (ParseU32(&line[11], &value) && (value <= 255UL)) {
            (void) AD9833_SetAmplitude((uint8_t) value);
            WriteOkPrefix("ad9833");
            Uart_WriteString("amp=");
            Uart_WriteU32(value);
            Uart_WriteString("\r\n");
        } else {
            Uart_WriteString("ERR ad9833 amp command: ad9833 amp 128\r\n");
        }
        return true;
    }
    if (Str_StartsWith(line, "ad9833 mclk ")) {
        if (ParseU32(&line[12], &value) && (value > 0UL)) {
            AD9833_SetMclkHz(value);
            WriteOkPrefix("ad9833");
            Uart_WriteString("mclk_hz=");
            Uart_WriteU32(AD9833_GetMclkHz());
            Uart_WriteString("\r\n");
        } else {
            Uart_WriteString("ERR ad9833 mclk command: ad9833 mclk 25000000\r\n");
        }
        return true;
    }
    if (Str_StartsWith(line, "ad9833 mclk1 ")) {
        if (ParseU32(&line[13], &value) && (value > 0UL)) {
            AD9833_SetMclkChannel(AD9833_CHANNEL_1, value);
            WriteOkPrefix("ad9833");
            Uart_WriteString("mclk1_hz=");
            Uart_WriteU32(AD9833_GetMclkChannel(AD9833_CHANNEL_1));
            Uart_WriteString("\r\n");
        } else {
            Uart_WriteString("ERR ad9833 mclk1 command: ad9833 mclk1 25000000\r\n");
        }
        return true;
    }
    if (Str_StartsWith(line, "ad9833 mclk2 ")) {
        if (ParseU32(&line[13], &value) && (value > 0UL)) {
            AD9833_SetMclkChannel(AD9833_CHANNEL_2, value);
            WriteOkPrefix("ad9833");
            Uart_WriteString("mclk2_hz=");
            Uart_WriteU32(AD9833_GetMclkChannel(AD9833_CHANNEL_2));
            Uart_WriteString("\r\n");
        } else {
            Uart_WriteString("ERR ad9833 mclk2 command: ad9833 mclk2 25000000\r\n");
        }
        return true;
    }

    return false;
}

static bool HandleAds1220Command(const char *line)
{
    ADS1220_Result_t result;
    ADS1220_Status_t status;
    uint32_t value;

    if (!Str_StartsWith(line, "ads1220")) {
        return false;
    }
    if (Str_Equals(line, "ads1220 init")) {
        ADS1220_Init();
        Uart_WriteString("OK ads1220 init pins=CS_PB5 SCLK_PA8 DIN_PA9 DOUT_DRDY_PA12 vref_uv=2048000\r\n");
        return true;
    }
    if (Str_Equals(line, "ads1220 reg")) {
        uint8_t reg0;
        uint8_t reg1;
        uint8_t reg2;
        uint8_t reg3;

        if ((ADS1220_ReadRegister(0U, &reg0) != ADS1220_STATUS_OK) ||
            (ADS1220_ReadRegister(1U, &reg1) != ADS1220_STATUS_OK) ||
            (ADS1220_ReadRegister(2U, &reg2) != ADS1220_STATUS_OK) ||
            (ADS1220_ReadRegister(3U, &reg3) != ADS1220_STATUS_OK)) {
            Uart_WriteString("ERR ads1220 reg read failed\r\n");
            return true;
        }
        Uart_Printf("OK ads1220 reg0=0x%02X reg1=0x%02X reg2=0x%02X reg3=0x%02X\r\n",
            reg0, reg1, reg2, reg3);
        return true;
    }
    if (Str_StartsWith(line, "ads1220 mux ")) {
        if (!ParseU32(&line[12], &value) || (value > 11UL)) {
            Uart_WriteString("ERR ads1220 mux command: ads1220 mux <0..11>\r\n");
            return true;
        }
        status = ADS1220_SelectInput((ADS1220_Mux_t) value);
        Uart_Printf("%s ads1220 mux=%lu text=%s\r\n",
            (status == ADS1220_STATUS_OK) ? "OK" : "ERR",
            (unsigned long) value, ADS1220_MuxText((uint8_t) value));
        return true;
    }
    if (Str_Equals(line, "ads1220 read") ||
        Str_StartsWith(line, "ads1220 read ")) {
        if (Str_Equals(line, "ads1220 read")) {
            value = ADS1220_MUX_AIN0_AVSS;
        } else if (!ParseU32(&line[13], &value) || (value > 11UL)) {
            Uart_WriteString("ERR ads1220 read command: ads1220 read [mux 0..11]\r\n");
            return true;
        }
        status = ADS1220_ReadInput((ADS1220_Mux_t) value, &result, 200U);
        if (status != ADS1220_STATUS_OK) {
            Uart_Printf("ERR ads1220 read status=%s mux=%lu\r\n",
                ADS1220_StatusText(status), (unsigned long) value);
            return true;
        }
        Uart_Printf("OK ads1220 mux=%s raw=%ld uv=%ld mv=%ld\r\n",
            ADS1220_MuxText(result.mux), (long) result.raw,
            (long) result.microvolt, (long) result.millivolt);
        return true;
    }

    Uart_WriteString("ERR ads1220 command: ads1220 init|reg|mux <0..11>|read [mux]\r\n");
    return true;
}

static void WriteUnsupportedP1(const char *name, P1_Status_t status)
{
    Uart_WriteString("ERR ");
    Uart_WriteString(name);
    Uart_WriteString(" ");
    Uart_WriteString(P1_StatusText(status));
    Uart_WriteString(" enable the matching SysConfig PWM/CAPTURE/ADC-DMA module first\r\n");
}

static bool HandleP1Command(const char *line)
{
    uint32_t first;
    uint32_t second;

    if (Str_StartsWith(line, "pwm ")) {
        if (!ParseTwoU32(&line[4], &first, &second)) {
            Uart_WriteString("ERR pwm command: pwm 100 500\r\n");
            return true;
        }
        P1_Status_t status = P1_SoftPwmLed(first, (uint16_t) second, 1000U);
        if (status == P1_STATUS_OK) {
            Uart_Printf("OK pwm soft_led freq_hz=%lu duty_permille=%lu duration_ms=1000 pin=PA0\r\n",
                (unsigned long) first, (unsigned long) second);
        } else {
            Uart_WriteString("ERR pwm supports freq 1..1000Hz duty 0..1000\r\n");
        }
        return true;
    }

    if (Str_StartsWith(line, "adcstream ")) {
        P1_AdcStream_t stream;
        P1_Status_t status;

        if (!ParseTwoU32(&line[10], &first, &second) ||
            (second > P1_ADC_STREAM_MAX_SAMPLES)) {
            Uart_WriteString("ERR adcstream command: adcstream 10000 64\r\n");
            return true;
        }

        status = P1_CaptureAdcBlocking(first, (uint16_t) second, &stream);
        if (status != P1_STATUS_OK) {
            Uart_WriteString("ERR adcstream ");
            Uart_WriteString(P1_StatusText(status));
            Uart_WriteString(" rates=10000/50000/100000/200000 count<=128\r\n");
            return true;
        }

        Uart_Printf("OK adcstream mode=blocking requested_rate_hz=%lu count=%u min=%u max=%u avg=%u avg_mv=%u\r\n",
            (unsigned long) stream.requestedRateHz, stream.sampleCount,
            stream.minRaw, stream.maxRaw, stream.avgRaw,
            ADC_Fml_RawToMillivolt(stream.avgRaw));
        Uart_WriteString("idx,raw,mv\r\n");
        for (uint16_t i = 0U; i < stream.sampleCount; i++) {
            Uart_Printf("%u,%u,%u\r\n", i, stream.samples[i],
                ADC_Fml_RawToMillivolt(stream.samples[i]));
        }
        return true;
    }

    if (Str_Equals(line, "cap")) {
        uint32_t frequencyHz;
        P1_Status_t status = P1_GetCaptureFrequency(&frequencyHz);
        if (status == P1_STATUS_OK) {
            Uart_Printf("OK cap freq_hz=%lu\r\n", (unsigned long) frequencyHz);
        } else {
            WriteUnsupportedP1("cap", status);
        }
        return true;
    }

    if (Str_Equals(line, "duty")) {
        uint16_t dutyPermille;
        P1_Status_t status = P1_GetCaptureDuty(&dutyPermille);
        if (status == P1_STATUS_OK) {
            Uart_Printf("OK duty permille=%u\r\n", dutyPermille);
        } else {
            WriteUnsupportedP1("duty", status);
        }
        return true;
    }

    if (Str_Equals(line, "phase")) {
        uint16_t phaseDeg;
        P1_Status_t status = P1_GetCapturePhase(&phaseDeg);
        if (status == P1_STATUS_OK) {
            Uart_Printf("OK phase deg=%u\r\n", phaseDeg);
        } else {
            WriteUnsupportedP1("phase", status);
        }
        return true;
    }

    if (Str_StartsWith(line, "trigadc ")) {
        if (!ParseU32(&line[8], &first)) {
            Uart_WriteString("ERR trigadc command: trigadc 10000\r\n");
            return true;
        }
        WriteUnsupportedP1("trigadc", P1_StartTimerTriggeredAdc(first));
        return true;
    }

    if (Str_StartsWith(line, "adcdma ")) {
        if (!ParseU32(&line[7], &first)) {
            Uart_WriteString("ERR adcdma command: adcdma 100000\r\n");
            return true;
        }
        WriteUnsupportedP1("adcdma", P1_StartAdcDma(first));
        return true;
    }

    return false;
}

static const ConsoleExactCommand_t g_exactCommands[] = {
    {"board", WriteP0Info},
    {"info", WriteP0Info},
    {"io", WriteP0Info},
    {"p0", WriteP0Info},
    {"diag", WriteAllDiagnostics},
    {"all", WriteAllDiagnostics},
    {"printf", WritePrintfTest},
    {"led on", WriteLedOn},
    {"led off", WriteLedOff},
    {"led toggle", WriteLedToggle},
    {"key", WriteKeyState},
    {"buzz on", WriteBuzzerOn},
    {"buzz off", WriteBuzzerOff},
    {"buzz beep", WriteBuzzerBeep},
};

static bool HandleExactCommand(const char *line)
{
    for (uint8_t i = 0U;
         i < (uint8_t) (sizeof(g_exactCommands) / sizeof(g_exactCommands[0]));
         i++) {
        if (Str_Equals(line, g_exactCommands[i].command)) {
            g_exactCommands[i].handler();
            return true;
        }
    }

    return false;
}

static void WriteHelp(void)
{
    Uart_WriteString("MSPM0G3507 SignalLab commands:\r\n");
    Uart_WriteString("  board | ping | adc | timer | led on/off/toggle | key | buzz on/off/beep | delayus <n> | delayms <n>\r\n");
    Uart_WriteString("  scope <rate> <count> | scope vofa <rate> | scope stop | freq | phase | rms <rate> <count> | meter\r\n");
    Uart_WriteString("  clone | clone2 [rate count] | clone measure <rate> <count> | clone auto [rate count] | clone ad9833 [rate count] | clone ad9959 chN [rate count]\r\n");
    Uart_WriteString("  dds init | dds chN <hz> | dds amp chN <0..1023> | dds phase chN <0..359> | dds sysclk <hz>\r\n");
    Uart_WriteString("  ad9833 sine|tri|square <hz> (both) | ad9833 ch1|ch2 sine|tri|square <hz> | ad9833 amp <0..255> | ad9833 mclk|mclk1|mclk2 <hz> | ad9833 off\r\n");
    Uart_WriteString("  ads1220 init | ads1220 read [mux] | ads1220 mux <0..11> | ads1220 reg\r\n");
    Uart_WriteString("  tjc | serial screen: UART3 PB12_TX PB13_RX 115200 8N1\r\n");
    Uart_WriteString("Pins: PA25=ADC/scope/clone/rms, PB20=freq/phaseA/HF fallback, PB21=phaseB, UART=PA10/PA11, TJC=PB12/PB13, ADS1220=PA8/PA9/PA12/PB5.\r\n");
}

static void WriteFullHelp(void)
{
    WriteHelp();
    Uart_WriteString("Legacy/lab commands:\r\n");
    Uart_WriteString("  p0 | printf | spi <hex> | wave | wavecsv | vofa | pwm <hz> <0..1000> | adcstream <rate> <count>\r\n");
    Uart_WriteString("  cap | duty | trigadc <rate> | adcdma <rate> are kept for compatibility but currently not configured.\r\n");
    P2Lab_WriteHelp();
    P3Lab_WriteHelp();
    P4Lab_WriteHelp();
    SignalToolsLab_WriteHelp();
    WaveformClone_WriteHelp();
    Uart_WriteString("cmd: dds test | dds on/off | all | help | ?\r\n");
    Uart_WriteString("spi loopback PASS requires MOSI(PA9) connected to MISO(PA12).\r\n");
    Uart_WriteString("vofa outputs FireWater lines: raw,filtered,mv\r\n");
    Uart_WriteString("P0 pins: LED=PA0 KEY=PB10 active-low BUZZER=PB11 active-high.\r\n");
    Uart_WriteString("P1 adcstream rates: 10000, 50000, 100000, 200000 Hz; current mode is blocking capture, not DMA.\r\n");
    Uart_WriteString("TJC screen: UART3 PB12 TX -> screen RX, PB13 RX <- screen TX, 115200 8N1.\r\n");
    Uart_WriteString("AD9959: SCK=PA8 SD0=PA9 CS=PA13 IU=PB0 RST=PB1 PDN=PB2 P0=PB3 P1=PB4 P2=PB7 P3=PB6\r\n");
    Uart_WriteString("AD9833 dual: SCLK=PA8 DATA=PA9 CS1=PB8 CS2=PB9, default MCLK=25MHz\r\n");
    Uart_WriteString("ADS1220 optional: CS=PB5 SCLK=PA8 DIN=PA9 DOUT/DRDY=PA12, default read mux AIN0_AVSS.\r\n");
}

static void HandleCommand(const char *line)
{
    if (HandleExactCommand(line)) {
        return;
    } else if (Str_Equals(line, "ping")) {
        WriteOkPrefix("ping");
        Uart_WriteString("tick_ms=");
        Uart_WriteU32(Timer_GetTickMs());
        Uart_WriteString("\r\n");
    } else if (Str_Equals(line, "adc")) {
        WriteAdcResult();
    } else if (Str_Equals(line, "timer")) {
        WriteTimerResult();
    } else if (Str_StartsWith(line, "spi ")) {
        uint8_t txData;
        if (ParseHexByte(&line[4], &txData)) {
            WriteSpiResult(txData);
        } else {
            Uart_WriteString("ERR spi expects hex byte, e.g. spi A5\r\n");
        }
    } else if (Str_StartsWith(line, "delayus ")) {
        uint32_t value;
        if (ParseU32(&line[8], &value)) {
            Delay_Us(value);
            Uart_Printf("OK delayus %lu\r\n", (unsigned long) value);
        } else {
            Uart_WriteString("ERR delayus command: delayus 100\r\n");
        }
    } else if (Str_StartsWith(line, "delayms ")) {
        uint32_t value;
        if (ParseU32(&line[8], &value)) {
            Delay_Ms(value);
            Uart_Printf("OK delayms %lu\r\n", (unsigned long) value);
        } else {
            Uart_WriteString("ERR delayms command: delayms 100\r\n");
        }
    } else if (Str_Equals(line, "wave")) {
        Waveform_PrintAscii();
    } else if (Str_Equals(line, "wavecsv")) {
        Waveform_PrintCsv();
    } else if (Str_Equals(line, "vofa")) {
        SignalToolsLab_StopStreaming();
        Waveform_VofaStart();
    } else if (Str_Equals(line, "stop")) {
        Waveform_VofaStop();
        SignalToolsLab_StopStreaming();
        Uart_WriteString("OK vofa stop\r\n");
    } else if (Str_Equals(line, "dds init")) {
        AD9959_Init();
        Uart_WriteString("OK dds init\r\n");
    } else if (Str_Equals(line, "dds test")) {
        WriteDdsTest();
    } else if (Str_Equals(line, "dds on")) {
        WriteDdsPower(false);
    } else if (Str_Equals(line, "dds off")) {
        WriteDdsPower(true);
    } else if (Str_Equals(line, "tjc")) {
        WriteTjcStatus();
    } else if (HandleDdsChannelCommand(line)) {
        return;
    } else if (HandleDdsAmpCommand(line)) {
        return;
    } else if (HandleDdsPhaseCommand(line)) {
        return;
    } else if (HandleDdsSysclkCommand(line)) {
        return;
    } else if (HandleAd9833Command(line)) {
        return;
    } else if (HandleAds1220Command(line)) {
        return;
    } else if (WaveformClone_HandleCommand(line)) {
        return;
    } else if (SignalToolsLab_HandleCommand(line)) {
        if (Str_StartsWith(line, "scope vofa ")) {
            Waveform_VofaStop();
        }
        return;
    } else if (HandleP1Command(line)) {
        return;
    } else if (P2Lab_HandleCommand(line)) {
        return;
    } else if (P3Lab_HandleCommand(line)) {
        return;
    } else if (P4Lab_HandleCommand(line)) {
        return;
    } else if (Str_Equals(line, "sample")) {
        WriteAdcResult();
    } else if (Str_Equals(line, "help full")) {
        WriteFullHelp();
    } else if (Str_Equals(line, "help") || Str_Equals(line, "?")) {
        WriteHelp();
    } else if (line[0] != '\0') {
        Uart_WriteString("ERR unknown command\r\n");
        WriteHelp();
    }
}

void TestConsole_Init(void)
{
    g_lineLength = 0U;
    Uart_WriteString("\r\nMSPM0G3507 SignalLab ready @115200 8N1\r\n");
    WriteHelp();
}

void TestConsole_Proc(void)
{
    uint8_t data;

    while (Uart_ReadByte(&data)) {
        if ((data == '\r') || (data == '\n')) {
            g_line[g_lineLength] = '\0';
            HandleCommand(g_line);
            g_lineLength = 0U;
        } else if ((data == '\b') || (data == 0x7FU)) {
            if (g_lineLength > 0U) {
                g_lineLength--;
            }
        } else if ((data >= 0x20U) && (data <= 0x7EU)) {
            if (g_lineLength < (TEST_LINE_SIZE - 1U)) {
                g_line[g_lineLength++] = (char) data;
            } else {
                g_lineLength = 0U;
                Uart_WriteString("ERR line too long\r\n");
            }
        }
    }

    Waveform_VofaProc();
    SignalToolsLab_Proc();
}
