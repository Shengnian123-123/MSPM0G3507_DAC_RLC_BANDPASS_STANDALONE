#include "ad9833.h"
#include "main.h"

#define AD9833_CTRL_B28       (0x2000U)
#define AD9833_CTRL_RESET     (0x0100U)
#define AD9833_CTRL_MODE      (0x0002U)
#define AD9833_CTRL_OPBITEN   (0x0020U)
#define AD9833_CTRL_DIV2      (0x0008U)
#define AD9833_FREQ0_ADDR     (0x4000U)
#define AD9833_PHASE0_ADDR    (0xC000U)

static uint32_t g_mclkHz1 = AD9833_DEFAULT_MCLK_HZ;
static uint32_t g_mclkHz2 = AD9833_DEFAULT_MCLK_HZ;

static void DelayShort(void)
{
    delay_cycles(32U);
}

static void SclkHigh(void)
{
    DL_GPIO_setPins(GPIO_SOFT_SPI_PORT, GPIO_SOFT_SPI_SCLK_PIN);
}

static void SclkLow(void)
{
    DL_GPIO_clearPins(GPIO_SOFT_SPI_PORT, GPIO_SOFT_SPI_SCLK_PIN);
}

static void DataHigh(void)
{
    DL_GPIO_setPins(GPIO_SOFT_SPI_PORT, GPIO_SOFT_SPI_MOSI_PIN);
}

static void DataLow(void)
{
    DL_GPIO_clearPins(GPIO_SOFT_SPI_PORT, GPIO_SOFT_SPI_MOSI_PIN);
}

static void Cs1High(void)
{
    DL_GPIO_setPins(GPIO_AD9833_CTRL_PORT, GPIO_AD9833_CTRL_CS1_PIN);
}

static void Cs1Low(void)
{
    DL_GPIO_clearPins(GPIO_AD9833_CTRL_PORT, GPIO_AD9833_CTRL_CS1_PIN);
}

static void Cs2High(void)
{
    DL_GPIO_setPins(GPIO_AD9833_CTRL_PORT, GPIO_AD9833_CTRL_CS2_PIN);
}

static void Cs2Low(void)
{
    DL_GPIO_clearPins(GPIO_AD9833_CTRL_PORT, GPIO_AD9833_CTRL_CS2_PIN);
}

static bool SelectChannel(AD9833_Channel_t channel)
{
    Cs1High();
    Cs2High();

    switch (channel) {
        case AD9833_CHANNEL_ALL:
            Cs1Low();
            Cs2Low();
            return true;
        case AD9833_CHANNEL_1:
            Cs1Low();
            return true;
        case AD9833_CHANNEL_2:
            Cs2Low();
            return true;
        default:
            return false;
    }
}

static void ReleaseChannels(void)
{
    Cs1High();
    Cs2High();
}

static void Write16WithSelect(uint16_t value, AD9833_Channel_t channel)
{
    if (!SelectChannel(channel)) {
        return;
    }

    SclkHigh();
    DelayShort();

    for (uint8_t i = 0U; i < 16U; i++) {
        if ((value & 0x8000U) != 0U) {
            DataHigh();
        } else {
            DataLow();
        }
        DelayShort();
        SclkLow();
        DelayShort();
        SclkHigh();
        value <<= 1U;
    }

    ReleaseChannels();
    DelayShort();
}

static void WriteAd9833(uint16_t value, AD9833_Channel_t channel)
{
    Write16WithSelect(value, channel);
}

static uint32_t FrequencyToWord(uint32_t frequencyHz, uint32_t mclkHz)
{
    return (uint32_t) ((((uint64_t) frequencyHz) << 28U) / mclkHz);
}

static uint16_t PhaseToWord(uint16_t phaseDeg)
{
    return (uint16_t) ((((uint32_t) (phaseDeg % 360U)) * 4096UL) / 360UL);
}

static uint16_t ControlForWaveform(AD9833_Waveform_t waveform)
{
    switch (waveform) {
        case AD9833_WAVE_TRIANGLE:
            return AD9833_CTRL_B28 | AD9833_CTRL_MODE;
        case AD9833_WAVE_SQUARE:
            return AD9833_CTRL_B28 | AD9833_CTRL_OPBITEN | AD9833_CTRL_DIV2;
        case AD9833_WAVE_SINE:
        default:
            return AD9833_CTRL_B28;
    }
}

void AD9833_Init(void)
{
    Cs1High();
    Cs2High();
    SclkHigh();
    DataLow();
    AD9833_ResetOutput();
}

void AD9833_SetMclkHz(uint32_t mclkHz)
{
    if (mclkHz != 0U) {
        g_mclkHz1 = mclkHz;
        g_mclkHz2 = mclkHz;
    }
}

uint32_t AD9833_GetMclkHz(void)
{
    return g_mclkHz1;
}

void AD9833_SetMclkChannel(AD9833_Channel_t channel, uint32_t mclkHz)
{
    if (mclkHz == 0U) {
        return;
    }
    if ((channel == AD9833_CHANNEL_ALL) ||
        (channel == AD9833_CHANNEL_1)) {
        g_mclkHz1 = mclkHz;
    }
    if ((channel == AD9833_CHANNEL_ALL) ||
        (channel == AD9833_CHANNEL_2)) {
        g_mclkHz2 = mclkHz;
    }
}

uint32_t AD9833_GetMclkChannel(AD9833_Channel_t channel)
{
    if (channel == AD9833_CHANNEL_2) {
        return g_mclkHz2;
    }
    return g_mclkHz1;
}

uint32_t AD9833_GetMaxOutputHzChannel(AD9833_Channel_t channel)
{
    uint32_t mclkHz = AD9833_GetMclkChannel(channel);

    if (mclkHz < 2U) {
        return 0U;
    }
    return (mclkHz / 2U) - 1U;
}

uint32_t AD9833_GetMaxOutputHz(void)
{
    return AD9833_GetMaxOutputHzChannel(AD9833_CHANNEL_1);
}

static bool SetOutputOne(AD9833_Channel_t channel,
    AD9833_Waveform_t waveform, uint32_t frequencyHz, uint16_t phaseDeg)
{
    uint32_t ftw;
    uint16_t phaseWord;
    uint32_t mclkHz;

    if ((channel != AD9833_CHANNEL_1) &&
        (channel != AD9833_CHANNEL_2)) {
        return false;
    }
    mclkHz = AD9833_GetMclkChannel(channel);
    if ((mclkHz == 0U) ||
        (frequencyHz > AD9833_GetMaxOutputHzChannel(channel)) ||
        (phaseDeg > AD9833_MAX_PHASE_DEG)) {
        return false;
    }

    ftw       = FrequencyToWord(frequencyHz, mclkHz);
    phaseWord = PhaseToWord(phaseDeg);

    WriteAd9833(AD9833_CTRL_B28 | AD9833_CTRL_RESET, channel);
    WriteAd9833((uint16_t) (AD9833_FREQ0_ADDR | (ftw & 0x3FFFU)), channel);
    WriteAd9833((uint16_t) (AD9833_FREQ0_ADDR | ((ftw >> 14U) & 0x3FFFU)), channel);
    WriteAd9833((uint16_t) (AD9833_PHASE0_ADDR | (phaseWord & 0x0FFFU)), channel);
    WriteAd9833(ControlForWaveform(waveform), channel);
    return true;
}

bool AD9833_SetOutputChannel(AD9833_Channel_t channel,
    AD9833_Waveform_t waveform, uint32_t frequencyHz, uint16_t phaseDeg)
{
    if (channel == AD9833_CHANNEL_ALL) {
        return SetOutputOne(AD9833_CHANNEL_1, waveform, frequencyHz, phaseDeg) &&
               SetOutputOne(AD9833_CHANNEL_2, waveform, frequencyHz, phaseDeg);
    }
    return SetOutputOne(channel, waveform, frequencyHz, phaseDeg);
}

bool AD9833_SetOutput(AD9833_Waveform_t waveform, uint32_t frequencyHz,
    uint16_t phaseDeg)
{
    return AD9833_SetOutputChannel(AD9833_CHANNEL_ALL, waveform,
        frequencyHz, phaseDeg);
}

bool AD9833_SetAmplitudeChannel(AD9833_Channel_t channel, uint8_t amplitude)
{
    /* CS1/CS2 are now dedicated to the two AD9833 devices. */
    if ((channel > AD9833_CHANNEL_2) || (amplitude > 255U)) {
        return false;
    }
    return true;
}

bool AD9833_SetAmplitude(uint8_t amplitude)
{
    return AD9833_SetAmplitudeChannel(AD9833_CHANNEL_ALL, amplitude);
}

void AD9833_ResetOutputChannel(AD9833_Channel_t channel)
{
    WriteAd9833(AD9833_CTRL_RESET, channel);
}

void AD9833_ResetOutput(void)
{
    AD9833_ResetOutputChannel(AD9833_CHANNEL_ALL);
}

bool AD9833_SelfTestPattern(void)
{
    AD9833_Init();
    return AD9833_SetOutput(AD9833_WAVE_SINE, 1000UL, 0U);
}
