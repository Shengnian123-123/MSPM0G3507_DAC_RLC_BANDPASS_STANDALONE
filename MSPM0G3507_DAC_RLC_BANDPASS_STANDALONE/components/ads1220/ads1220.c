#include "ads1220.h"

#include "delay.h"
#include "main.h"
#include "spi.h"
#include "tim.h"

#define ADS1220_CMD_POWERDOWN (0x02U)
#define ADS1220_CMD_RESET     (0x06U)
#define ADS1220_CMD_START     (0x08U)
#define ADS1220_CMD_RDATA     (0x10U)
#define ADS1220_CMD_RREG      (0x20U)
#define ADS1220_CMD_WREG      (0x40U)

#define ADS1220_CONFIG0_DEFAULT (0x01U)
#define ADS1220_CONFIG1_DEFAULT (0x00U)
#define ADS1220_CONFIG2_DEFAULT (0x10U)
#define ADS1220_CONFIG3_DEFAULT (0x02U)

static uint8_t g_currentMux = ADS1220_MUX_AIN0_AIN1;
static ADS1220_Gain_t g_gain = ADS1220_GAIN_1;

static void Select(void)
{
    SoftSPI_Release();
    DL_GPIO_clearPins(GPIO_ADS1220_CTRL_PORT,
        GPIO_ADS1220_CTRL_ADS1220_CS_PIN);
    delay_cycles(16U);
}

static void Release(void)
{
    DL_GPIO_setPins(GPIO_ADS1220_CTRL_PORT,
        GPIO_ADS1220_CTRL_ADS1220_CS_PIN);
    delay_cycles(16U);
}

static void WriteCommand(uint8_t command)
{
    Select();
    (void) SoftSPI_TransferByte(command);
    Release();
}

static bool IsReady(void)
{
    return (DL_GPIO_readPins(GPIO_SOFT_SPI_PORT, GPIO_SOFT_SPI_MISO_PIN) &
        GPIO_SOFT_SPI_MISO_PIN) == 0U;
}

static uint8_t Config0(ADS1220_Mux_t mux, ADS1220_Gain_t gain,
    bool pgaBypass)
{
    if ((mux >= ADS1220_MUX_AIN0_AVSS) && (mux <= ADS1220_MUX_AIN3_AVSS)) {
        pgaBypass = true;
    }
    if (pgaBypass && (gain > ADS1220_GAIN_4)) {
        gain = ADS1220_GAIN_1;
    }
    return (uint8_t) (((uint8_t) mux << 4U) | ((uint8_t) gain << 1U) |
        (pgaBypass ? 1U : 0U));
}

static ADS1220_Gain_t EffectiveGain(ADS1220_Mux_t mux, ADS1220_Gain_t gain,
    bool pgaBypass)
{
    if ((mux >= ADS1220_MUX_AIN0_AVSS) && (mux <= ADS1220_MUX_AIN3_AVSS)) {
        pgaBypass = true;
    }
    if (pgaBypass && (gain > ADS1220_GAIN_4)) {
        return ADS1220_GAIN_1;
    }
    return gain;
}

static int32_t GainDivider(void)
{
    return (int32_t) (1L << (uint8_t) g_gain);
}

static uint8_t Config1(ADS1220_DataRate_t dataRate)
{
    return (uint8_t) ((uint8_t) dataRate << 5U);
}

void ADS1220_Init(void)
{
    SoftSPI_Init();
    Release();
    ADS1220_Reset();
    (void) ADS1220_WriteRegister(0U, ADS1220_CONFIG0_DEFAULT);
    (void) ADS1220_WriteRegister(1U, ADS1220_CONFIG1_DEFAULT);
    (void) ADS1220_WriteRegister(2U, ADS1220_CONFIG2_DEFAULT);
    (void) ADS1220_WriteRegister(3U, ADS1220_CONFIG3_DEFAULT);
    g_currentMux = ADS1220_MUX_AIN0_AIN1;
    g_gain = ADS1220_GAIN_1;
}

void ADS1220_Reset(void)
{
    Release();
    Delay_Ms(1U);
    WriteCommand(ADS1220_CMD_RESET);
    Delay_Ms(2U);
}

ADS1220_Status_t ADS1220_WriteRegister(uint8_t reg, uint8_t value)
{
    if (reg > 3U) {
        return ADS1220_STATUS_BAD_PARAM;
    }
    Select();
    (void) SoftSPI_TransferByte((uint8_t) (ADS1220_CMD_WREG | (reg << 2U)));
    (void) SoftSPI_TransferByte(value);
    Release();
    return ADS1220_STATUS_OK;
}

ADS1220_Status_t ADS1220_ReadRegister(uint8_t reg, uint8_t *value)
{
    if ((reg > 3U) || (value == 0)) {
        return ADS1220_STATUS_BAD_PARAM;
    }
    Select();
    (void) SoftSPI_TransferByte((uint8_t) (ADS1220_CMD_RREG | (reg << 2U)));
    *value = SoftSPI_TransferByte(0xFFU);
    Release();
    return ADS1220_STATUS_OK;
}

ADS1220_Status_t ADS1220_Configure(ADS1220_Mux_t mux, ADS1220_Gain_t gain,
    bool pgaBypass, ADS1220_DataRate_t dataRate)
{
    if (((uint8_t) mux > 0x0BU) || ((uint8_t) gain > ADS1220_GAIN_128) ||
        ((uint8_t) dataRate > ADS1220_DATA_RATE_1000SPS)) {
        return ADS1220_STATUS_BAD_PARAM;
    }

    if (ADS1220_WriteRegister(0U, Config0(mux, gain, pgaBypass)) !=
        ADS1220_STATUS_OK) {
        return ADS1220_STATUS_ERROR;
    }
    if (ADS1220_WriteRegister(1U, Config1(dataRate)) != ADS1220_STATUS_OK) {
        return ADS1220_STATUS_ERROR;
    }
    g_currentMux = (uint8_t) mux;
    g_gain = EffectiveGain(mux, gain, pgaBypass);
    return ADS1220_STATUS_OK;
}

ADS1220_Status_t ADS1220_SelectInput(ADS1220_Mux_t mux)
{
    return ADS1220_Configure(mux, g_gain, true, ADS1220_DATA_RATE_20SPS);
}

ADS1220_Status_t ADS1220_ReadRaw(int32_t *raw, uint32_t timeoutMs)
{
    uint32_t startMs;
    uint8_t byte0;
    uint8_t byte1;
    uint8_t byte2;
    int32_t value;

    if (raw == 0) {
        return ADS1220_STATUS_BAD_PARAM;
    }

    WriteCommand(ADS1220_CMD_START);
    startMs = Timer_GetTickMs();
    while (!IsReady()) {
        if ((Timer_GetTickMs() - startMs) >= timeoutMs) {
            return ADS1220_STATUS_TIMEOUT;
        }
    }

    Select();
    (void) SoftSPI_TransferByte(ADS1220_CMD_RDATA);
    byte0 = SoftSPI_TransferByte(0xFFU);
    byte1 = SoftSPI_TransferByte(0xFFU);
    byte2 = SoftSPI_TransferByte(0xFFU);
    Release();

    value = ((int32_t) byte0 << 16U) | ((int32_t) byte1 << 8U) |
        (int32_t) byte2;
    if ((value & 0x00800000L) != 0) {
        value |= (int32_t) 0xFF000000L;
    }
    *raw = value;
    return ADS1220_STATUS_OK;
}

ADS1220_Status_t ADS1220_ReadInput(ADS1220_Mux_t mux,
    ADS1220_Result_t *result, uint32_t timeoutMs)
{
    ADS1220_Status_t status;
    int32_t raw;

    if (result == 0) {
        return ADS1220_STATUS_BAD_PARAM;
    }
    status = ADS1220_SelectInput(mux);
    if (status != ADS1220_STATUS_OK) {
        return status;
    }
    status = ADS1220_ReadRaw(&raw, timeoutMs);
    if (status != ADS1220_STATUS_OK) {
        return status;
    }
    result->raw = raw;
    result->microvolt = ADS1220_RawToMicrovolt(raw);
    result->millivolt = result->microvolt / 1000L;
    result->mux = (uint8_t) mux;
    return ADS1220_STATUS_OK;
}

int32_t ADS1220_RawToMicrovolt(int32_t raw)
{
    int64_t value = (int64_t) raw * ADS1220_DEFAULT_VREF_UV;

    value /= (int64_t) ADS1220_FULL_SCALE * GainDivider();
    return (int32_t) value;
}

int32_t ADS1220_RawToMillivolt(int32_t raw)
{
    return ADS1220_RawToMicrovolt(raw) / 1000L;
}

const char *ADS1220_StatusText(ADS1220_Status_t status)
{
    switch (status) {
        case ADS1220_STATUS_OK:
            return "OK";
        case ADS1220_STATUS_BAD_PARAM:
            return "BAD_PARAM";
        case ADS1220_STATUS_TIMEOUT:
            return "TIMEOUT";
        case ADS1220_STATUS_NOT_READY:
            return "NOT_READY";
        case ADS1220_STATUS_ERROR:
        default:
            return "ERROR";
    }
}

const char *ADS1220_MuxText(uint8_t mux)
{
    switch (mux) {
        case ADS1220_MUX_AIN0_AIN1:
            return "AIN0_AIN1";
        case ADS1220_MUX_AIN0_AIN2:
            return "AIN0_AIN2";
        case ADS1220_MUX_AIN0_AIN3:
            return "AIN0_AIN3";
        case ADS1220_MUX_AIN1_AIN2:
            return "AIN1_AIN2";
        case ADS1220_MUX_AIN1_AIN3:
            return "AIN1_AIN3";
        case ADS1220_MUX_AIN2_AIN3:
            return "AIN2_AIN3";
        case ADS1220_MUX_AIN1_AIN0:
            return "AIN1_AIN0";
        case ADS1220_MUX_AIN3_AIN2:
            return "AIN3_AIN2";
        case ADS1220_MUX_AIN0_AVSS:
            return "AIN0_AVSS";
        case ADS1220_MUX_AIN1_AVSS:
            return "AIN1_AVSS";
        case ADS1220_MUX_AIN2_AVSS:
            return "AIN2_AVSS";
        case ADS1220_MUX_AIN3_AVSS:
            return "AIN3_AVSS";
        default:
            return "UNKNOWN";
    }
}
