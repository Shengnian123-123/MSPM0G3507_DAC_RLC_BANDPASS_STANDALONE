#ifndef COMPONENT_ADS1220_H
#define COMPONENT_ADS1220_H

#include <stdbool.h>
#include <stdint.h>

#define ADS1220_DEFAULT_VREF_UV (2048000L)
#define ADS1220_FULL_SCALE      (8388608L)
#define ADS1220_SPI_PINS        "CS_PB5_SCLK_PA8_DIN_PA9_DOUT_DRDY_PA12"

typedef enum {
    ADS1220_STATUS_OK = 0,
    ADS1220_STATUS_BAD_PARAM,
    ADS1220_STATUS_TIMEOUT,
    ADS1220_STATUS_NOT_READY,
    ADS1220_STATUS_ERROR,
} ADS1220_Status_t;

typedef enum {
    ADS1220_MUX_AIN0_AIN1 = 0x00,
    ADS1220_MUX_AIN0_AIN2 = 0x01,
    ADS1220_MUX_AIN0_AIN3 = 0x02,
    ADS1220_MUX_AIN1_AIN2 = 0x03,
    ADS1220_MUX_AIN1_AIN3 = 0x04,
    ADS1220_MUX_AIN2_AIN3 = 0x05,
    ADS1220_MUX_AIN1_AIN0 = 0x06,
    ADS1220_MUX_AIN3_AIN2 = 0x07,
    ADS1220_MUX_AIN0_AVSS = 0x08,
    ADS1220_MUX_AIN1_AVSS = 0x09,
    ADS1220_MUX_AIN2_AVSS = 0x0A,
    ADS1220_MUX_AIN3_AVSS = 0x0B,
} ADS1220_Mux_t;

typedef enum {
    ADS1220_GAIN_1 = 0,
    ADS1220_GAIN_2,
    ADS1220_GAIN_4,
    ADS1220_GAIN_8,
    ADS1220_GAIN_16,
    ADS1220_GAIN_32,
    ADS1220_GAIN_64,
    ADS1220_GAIN_128,
} ADS1220_Gain_t;

typedef enum {
    ADS1220_DATA_RATE_20SPS = 0,
    ADS1220_DATA_RATE_45SPS,
    ADS1220_DATA_RATE_90SPS,
    ADS1220_DATA_RATE_175SPS,
    ADS1220_DATA_RATE_330SPS,
    ADS1220_DATA_RATE_600SPS,
    ADS1220_DATA_RATE_1000SPS,
} ADS1220_DataRate_t;

typedef struct {
    int32_t raw;
    int32_t microvolt;
    int32_t millivolt;
    uint8_t mux;
} ADS1220_Result_t;

void ADS1220_Init(void);
void ADS1220_Reset(void);
ADS1220_Status_t ADS1220_WriteRegister(uint8_t reg, uint8_t value);
ADS1220_Status_t ADS1220_ReadRegister(uint8_t reg, uint8_t *value);
ADS1220_Status_t ADS1220_SelectInput(ADS1220_Mux_t mux);
ADS1220_Status_t ADS1220_Configure(ADS1220_Mux_t mux, ADS1220_Gain_t gain,
    bool pgaBypass, ADS1220_DataRate_t dataRate);
ADS1220_Status_t ADS1220_ReadRaw(int32_t *raw, uint32_t timeoutMs);
ADS1220_Status_t ADS1220_ReadInput(ADS1220_Mux_t mux,
    ADS1220_Result_t *result, uint32_t timeoutMs);
int32_t ADS1220_RawToMicrovolt(int32_t raw);
int32_t ADS1220_RawToMillivolt(int32_t raw);
const char *ADS1220_StatusText(ADS1220_Status_t status);
const char *ADS1220_MuxText(uint8_t mux);

#endif
