#ifndef MSPM0G3507_BOARD_H
#define MSPM0G3507_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MSPM0G3507_ADC_INPUT_PORT_NAME    "PA25"
#define MSPM0G3507_ADC_INPUT_CHANNEL_NAME "ADC0_CH2"
#define MSPM0G3507_UART_PORT_NAME         "UART0_PA10_TX_PA11_RX"
#define MSPM0G3507_UART_BAUD_RATE         (115200U)
#define MSPM0G3507_TJC_SCREEN_UART_NAME   "UART3_PB12_TX_PB13_RX"
#define MSPM0G3507_TJC_SCREEN_BAUD_RATE   (115200U)
#define MSPM0G3507_AD9959_SPI_PINS        "SCK_PA8_SD0_PA9_CS_PA13"
#define MSPM0G3507_AD9959_CTRL_PINS       "IU_PB0_RST_PB1_PDN_PB2_P0_PB3_P1_PB4_P2_PB7_P3_PB6"
#define MSPM0G3507_ADS1220_SPI_PINS       "SCLK_PA8_DIN_PA9_DOUT_DRDY_PA12_CS_PB5"
#define MSPM0G3507_AD9833_PINS            "SCLK_PA8_DATA_PA9_CS1_PB8_CS2_PB9"
#define MSPM0G3507_KEY_PIN_NAME           "PB10_ACTIVE_LOW"
#define MSPM0G3507_BUZZER_PIN_NAME        "PB11_ACTIVE_HIGH"
#define MSPM0G3507_SCOPE_MAX_SAMPLES      (1024U)

typedef enum {
    MSPM0G3507_STATUS_OK = 0,
    MSPM0G3507_STATUS_BAD_PARAM,
    MSPM0G3507_STATUS_TIMEOUT,
    MSPM0G3507_STATUS_NOT_CONFIGURED,
    MSPM0G3507_STATUS_ERROR,
} MSPM0G3507_Status_t;

typedef enum {
    MSPM0G3507_ADC_STATE_LOW = 0,
    MSPM0G3507_ADC_STATE_NORMAL,
    MSPM0G3507_ADC_STATE_HIGH,
} MSPM0G3507_AdcState_t;

typedef enum {
    MSPM0G3507_WAVE_SINE = 0,
    MSPM0G3507_WAVE_TRIANGLE,
    MSPM0G3507_WAVE_SQUARE,
} MSPM0G3507_Waveform_t;

typedef enum {
    MSPM0G3507_AD9833_ALL = 0,
    MSPM0G3507_AD9833_CH1,
    MSPM0G3507_AD9833_CH2,
} MSPM0G3507_AD9833_Channel_t;

typedef struct {
    uint16_t raw;
    uint16_t millivolt;
    MSPM0G3507_AdcState_t state;
} MSPM0G3507_AdcSample_t;

typedef struct {
    uint32_t sampleRateHz;
    uint16_t sampleCount;
    bool usedDma;
    uint16_t samples[MSPM0G3507_SCOPE_MAX_SAMPLES];
} MSPM0G3507_ScopeFrame_t;

typedef struct {
    bool valid;
    uint32_t frequencyHz;
    uint32_t periodUs;
    uint16_t dutyPermille;
    uint32_t edgeCount;
    uint32_t ageMs;
} MSPM0G3507_Frequency_t;

typedef struct {
    bool valid;
    int16_t signedDeg;
    uint16_t phaseDeg;
    uint32_t deltaUs;
    uint32_t periodUs;
    uint32_t edgeCountA;
    uint32_t edgeCountB;
} MSPM0G3507_Phase_t;

typedef struct {
    int32_t zeroRaw;
    uint32_t gainPermille;
} MSPM0G3507_RmsCalibration_t;

typedef struct {
    uint32_t sampleRateHz;
    uint16_t sampleCount;
    uint16_t minRaw;
    uint16_t maxRaw;
    uint16_t avgRaw;
    uint16_t pkpkRaw;
    int32_t minMv;
    int32_t maxMv;
    int32_t avgMv;
    uint32_t pkpkMv;
    uint32_t rmsMv;
    uint32_t acRmsMv;
} MSPM0G3507_RmsResult_t;

typedef struct {
    uint32_t frequencyHz;
    uint16_t dutyPermille;
    uint16_t phaseDeg;
} MSPM0G3507_CaptureResult_t;

void MSPM0G3507_Board_Init(void);
void MSPM0G3507_Board_Proc(void);

uint32_t MSPM0G3507_GetTickMs(void);
uint32_t MSPM0G3507_GetTimer10msTicks(void);
void MSPM0G3507_DelayMs(uint32_t delayMs);
void MSPM0G3507_DelayUs(uint32_t delayUs);

void MSPM0G3507_LedOn(void);
void MSPM0G3507_LedOff(void);
void MSPM0G3507_LedToggle(void);
bool MSPM0G3507_KeyIsPressed(void);
void MSPM0G3507_BuzzerOn(void);
void MSPM0G3507_BuzzerOff(void);
void MSPM0G3507_BuzzerBeep(uint32_t onMs);

bool MSPM0G3507_ReadAdc(MSPM0G3507_AdcSample_t *sample);
bool MSPM0G3507_ReadAdcRaw(uint16_t *raw);
uint16_t MSPM0G3507_AdcRawToMillivolt(uint16_t raw);

uint8_t MSPM0G3507_SoftSpiTransferByte(uint8_t txData);
void MSPM0G3507_SoftSpiTransfer(const uint8_t *tx, uint8_t *rx, size_t length);

bool MSPM0G3507_UartReadByte(uint8_t *data);
void MSPM0G3507_UartWriteByte(uint8_t data);
void MSPM0G3507_UartWriteString(const char *text);
void MSPM0G3507_UartWriteU32(uint32_t value);

void MSPM0G3507_VofaStart(void);
void MSPM0G3507_VofaStop(void);
bool MSPM0G3507_VofaIsRunning(void);

bool MSPM0G3507_ScopeCapture(uint32_t sampleRateHz, uint16_t sampleCount,
    MSPM0G3507_ScopeFrame_t *frame);
bool MSPM0G3507_ScopeVofaStart(uint32_t sampleRateHz);
void MSPM0G3507_ScopeVofaStop(void);
bool MSPM0G3507_ScopeVofaIsRunning(void);
bool MSPM0G3507_ReadFrequency(MSPM0G3507_Frequency_t *result);
bool MSPM0G3507_ReadPhase(MSPM0G3507_Phase_t *result);
bool MSPM0G3507_ReadRms(uint32_t sampleRateHz, uint16_t sampleCount,
    MSPM0G3507_RmsResult_t *result);
bool MSPM0G3507_RmsCalibrateZero(uint32_t sampleRateHz, uint16_t sampleCount);
bool MSPM0G3507_RmsSetZeroRaw(int32_t zeroRaw);
bool MSPM0G3507_RmsSetGainPermille(uint32_t gainPermille);
MSPM0G3507_RmsCalibration_t MSPM0G3507_RmsGetCalibration(void);

void MSPM0G3507_AD9959_Init(void);
void MSPM0G3507_AD9959_SetSysclkHz(uint32_t sysclkHz);
uint32_t MSPM0G3507_AD9959_GetSysclkHz(void);
void MSPM0G3507_AD9959_PowerDown(bool enable);
bool MSPM0G3507_AD9959_SetSingleTone(uint8_t channel,
    uint32_t frequencyHz, uint16_t phaseDeg, uint16_t amplitude);
bool MSPM0G3507_AD9959_SelfTestPattern(void);

void MSPM0G3507_AD9833_Init(void);
void MSPM0G3507_AD9833_SetMclkHz(uint32_t mclkHz);
bool MSPM0G3507_AD9833_SetOutput(MSPM0G3507_Waveform_t waveform,
    uint32_t frequencyHz, uint16_t phaseDeg);
bool MSPM0G3507_AD9833_SetOutputChannel(
    MSPM0G3507_AD9833_Channel_t channel, MSPM0G3507_Waveform_t waveform,
    uint32_t frequencyHz, uint16_t phaseDeg);
bool MSPM0G3507_AD9833_SetAmplitude(uint8_t amplitude);

MSPM0G3507_Status_t MSPM0G3507_I2CScan(uint8_t *addresses, uint8_t maxCount,
    uint8_t *foundCount);
MSPM0G3507_Status_t MSPM0G3507_OledTest(void);
MSPM0G3507_Status_t MSPM0G3507_SoftPwmLed(uint32_t frequencyHz,
    uint16_t dutyPermille, uint32_t durationMs);
MSPM0G3507_Status_t MSPM0G3507_HardwarePwmStart(uint32_t frequencyHz,
    uint16_t dutyPermille);
MSPM0G3507_Status_t MSPM0G3507_CaptureRead(MSPM0G3507_CaptureResult_t *result);
MSPM0G3507_Status_t MSPM0G3507_DmaMemCopy(uint32_t *dst, const uint32_t *src,
    uint16_t wordCount);
MSPM0G3507_Status_t MSPM0G3507_AdcDmaStart(uint16_t *buffer, uint16_t count);

const char *MSPM0G3507_StatusText(MSPM0G3507_Status_t status);

#endif
