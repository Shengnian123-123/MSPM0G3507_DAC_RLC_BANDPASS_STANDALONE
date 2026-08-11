#include "ad9959.h"
#include <stddef.h>
#include "main.h"
#include "spi.h"

#define AD9959_REG_CSR       (0x00U)
#define AD9959_REG_FR1       (0x01U)
#define AD9959_REG_CFR       (0x03U)
#define AD9959_REG_CFTW0     (0x04U)
#define AD9959_REG_CPOW0     (0x05U)
#define AD9959_REG_ACR       (0x06U)

#define AD9959_CHANNEL_MASK(channel) ((uint8_t) (0x10U << (uint8_t) (channel)))

static uint32_t g_channelFrequency[AD9959_CHANNEL_COUNT];
static uint16_t g_channelPhase[AD9959_CHANNEL_COUNT];
static uint16_t g_channelAmplitude[AD9959_CHANNEL_COUNT];
static uint32_t g_sysclkHz = AD9959_DEFAULT_SYSCLK_HZ;

static bool IsValidChannel(AD9959_Channel_t channel)
{
    return ((uint8_t) channel < AD9959_CHANNEL_COUNT);
}

static void DelayShort(void)
{
    delay_cycles(1600U);
}

static void DelayMsApprox(uint32_t delayMs)
{
    while (delayMs > 0U) {
        delay_cycles(CPUCLK_FREQ / 1000U);
        delayMs--;
    }
}

static void SetIoUpdate(bool high)
{
    if (high) {
        DL_GPIO_setPins(GPIO_AD9959_CTRL_PORT, GPIO_AD9959_CTRL_IO_UPDATE_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_AD9959_CTRL_PORT, GPIO_AD9959_CTRL_IO_UPDATE_PIN);
    }
}

static void PulseIoUpdate(void)
{
    SetIoUpdate(false);
    DelayShort();
    SetIoUpdate(true);
    DelayShort();
    SetIoUpdate(false);
    DelayShort();
}

static void WriteRegister(uint8_t address, const uint8_t *data, size_t length,
    bool update)
{
#ifdef GPIO_ADS1220_CTRL_ADS1220_CS_PIN
    DL_GPIO_setPins(GPIO_ADS1220_CTRL_PORT,
        GPIO_ADS1220_CTRL_ADS1220_CS_PIN);
#endif
    SoftSPI_Select();
    (void) SoftSPI_TransferByte(address);
    for (size_t i = 0; i < length; i++) {
        (void) SoftSPI_TransferByte(data[i]);
    }
    SoftSPI_Release();
    if (update) {
        PulseIoUpdate();
    }
}

static void SelectChannel(AD9959_Channel_t channel)
{
    uint8_t csr = AD9959_CHANNEL_MASK(channel);

    WriteRegister(AD9959_REG_CSR, &csr, 1U, false);
}

static uint32_t FrequencyToTuningWord(uint32_t frequencyHz)
{
    return (uint32_t) ((((uint64_t) frequencyHz) << 32U) / g_sysclkHz);
}

static uint16_t PhaseToTuningWord(uint16_t phaseDeg)
{
    return (uint16_t) ((((uint32_t) (phaseDeg % 360U)) * 16384UL) / 360UL);
}

static bool SetPhase(AD9959_Channel_t channel, uint16_t phaseDeg, bool update)
{
    uint16_t pow = PhaseToTuningWord(phaseDeg);
    uint8_t data[2];

    if (!IsValidChannel(channel)) {
        return false;
    }

    SelectChannel(channel);
    data[0] = (uint8_t) (pow >> 8U);
    data[1] = (uint8_t) pow;
    WriteRegister(AD9959_REG_CPOW0, data, sizeof(data), update);

    g_channelPhase[(uint8_t) channel] = (uint16_t) (phaseDeg % 360U);
    return true;
}

static bool SetAmplitude(AD9959_Channel_t channel, uint16_t amplitude,
    bool update)
{
    uint8_t data[3];

    if (!IsValidChannel(channel)) {
        return false;
    }
    if (amplitude > AD9959_MAX_AMPLITUDE) {
        amplitude = AD9959_MAX_AMPLITUDE;
    }

    SelectChannel(channel);
    data[0] = 0x00U;
    data[1] = (uint8_t) (0x10U | ((amplitude >> 8U) & 0x03U));
    data[2] = (uint8_t) amplitude;
    WriteRegister(AD9959_REG_ACR, data, sizeof(data), update);

    g_channelAmplitude[(uint8_t) channel] = amplitude;
    return true;
}

void AD9959_Init(void)
{
    const uint8_t fr1[3] = {0xD0U, 0x00U, 0x00U};
    const uint8_t cfr[3] = {0x00U, 0x23U, 0x30U};

    SoftSPI_Init();
    DL_GPIO_clearPins(GPIO_AD9959_CTRL_PORT,
        GPIO_AD9959_CTRL_IO_UPDATE_PIN | GPIO_AD9959_CTRL_RESET_PIN |
            GPIO_AD9959_CTRL_PWR_D_PIN | GPIO_AD9959_CTRL_P0_PIN |
            GPIO_AD9959_CTRL_P1_PIN | GPIO_AD9959_CTRL_P2_PIN |
            GPIO_AD9959_CTRL_P3_PIN);
    DelayShort();

    DL_GPIO_setPins(GPIO_AD9959_CTRL_PORT, GPIO_AD9959_CTRL_RESET_PIN);
    DelayMsApprox(10U);
    DL_GPIO_clearPins(GPIO_AD9959_CTRL_PORT, GPIO_AD9959_CTRL_RESET_PIN);
    DelayMsApprox(10U);

    SelectChannel(AD9959_CH0);
    WriteRegister(AD9959_REG_FR1, fr1, sizeof(fr1), false);
    DelayMsApprox(2U);

    for (uint8_t channel = 0U; channel < AD9959_CHANNEL_COUNT; channel++) {
        g_channelFrequency[channel] = 0U;
        g_channelPhase[channel]     = 0U;
        g_channelAmplitude[channel] = AD9959_MAX_AMPLITUDE;

        SelectChannel((AD9959_Channel_t) channel);
        WriteRegister(AD9959_REG_CFR, cfr, sizeof(cfr), false);
    }
    PulseIoUpdate();
    DelayMsApprox(2U);
}

void AD9959_SetSysclkHz(uint32_t sysclkHz)
{
    if (sysclkHz != 0U) {
        g_sysclkHz = sysclkHz;
    }
}

uint32_t AD9959_GetSysclkHz(void)
{
    return g_sysclkHz;
}

void AD9959_PowerDown(bool enable)
{
    if (enable) {
        DL_GPIO_setPins(GPIO_AD9959_CTRL_PORT, GPIO_AD9959_CTRL_PWR_D_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_AD9959_CTRL_PORT, GPIO_AD9959_CTRL_PWR_D_PIN);
    }
}

bool AD9959_SetSingleTone(AD9959_Channel_t channel, uint32_t frequencyHz,
    uint16_t phaseDeg, uint16_t amplitude)
{
    if (!IsValidChannel(channel) || (g_sysclkHz == 0U) ||
        (frequencyHz > AD9959_MAX_OUTPUT_HZ) ||
        (frequencyHz >= (g_sysclkHz / 2U))) {
        return false;
    }

    SelectChannel(channel);
    if (!SetPhase(channel, phaseDeg, false)) {
        return false;
    }
    if (!SetAmplitude(channel, amplitude, false)) {
        return false;
    }
    return AD9959_SetFrequency(channel, frequencyHz);
}

bool AD9959_SetFrequency(AD9959_Channel_t channel, uint32_t frequencyHz)
{
    uint32_t ftw;
    uint8_t data[4];

    if (!IsValidChannel(channel) || (g_sysclkHz == 0U) ||
        (frequencyHz > AD9959_MAX_OUTPUT_HZ) ||
        (frequencyHz >= (g_sysclkHz / 2U))) {
        return false;
    }

    SelectChannel(channel);
    ftw     = FrequencyToTuningWord(frequencyHz);
    data[0] = (uint8_t) (ftw >> 24U);
    data[1] = (uint8_t) (ftw >> 16U);
    data[2] = (uint8_t) (ftw >> 8U);
    data[3] = (uint8_t) ftw;
    WriteRegister(AD9959_REG_CFTW0, data, sizeof(data), true);

    g_channelFrequency[(uint8_t) channel] = frequencyHz;
    return true;
}

bool AD9959_SetPhase(AD9959_Channel_t channel, uint16_t phaseDeg)
{
    return SetPhase(channel, phaseDeg, true);
}

bool AD9959_SetAmplitude(AD9959_Channel_t channel, uint16_t amplitude)
{
    return SetAmplitude(channel, amplitude, true);
}

bool AD9959_SetProfilePins(uint8_t value)
{
    uint32_t pins = 0U;

    if ((value & 0x01U) != 0U) {
        pins |= GPIO_AD9959_CTRL_P0_PIN;
    }
    if ((value & 0x02U) != 0U) {
        pins |= GPIO_AD9959_CTRL_P1_PIN;
    }
    if ((value & 0x04U) != 0U) {
        pins |= GPIO_AD9959_CTRL_P2_PIN;
    }
    if ((value & 0x08U) != 0U) {
        pins |= GPIO_AD9959_CTRL_P3_PIN;
    }

    DL_GPIO_clearPins(GPIO_AD9959_CTRL_PORT, GPIO_AD9959_CTRL_P0_PIN |
        GPIO_AD9959_CTRL_P1_PIN | GPIO_AD9959_CTRL_P2_PIN |
        GPIO_AD9959_CTRL_P3_PIN);
    DL_GPIO_setPins(GPIO_AD9959_CTRL_PORT, pins);
    return true;
}

bool AD9959_SelfTestPattern(void)
{
    AD9959_Init();

    return AD9959_SetSingleTone(AD9959_CH0, 1000000UL, 0U,
               AD9959_MAX_AMPLITUDE) &&
           AD9959_SetSingleTone(AD9959_CH1, 2000000UL, 0U,
               AD9959_MAX_AMPLITUDE) &&
           AD9959_SetSingleTone(AD9959_CH2, 3000000UL, 0U,
               AD9959_MAX_AMPLITUDE) &&
           AD9959_SetSingleTone(AD9959_CH3, 4000000UL, 0U,
               AD9959_MAX_AMPLITUDE);
}
