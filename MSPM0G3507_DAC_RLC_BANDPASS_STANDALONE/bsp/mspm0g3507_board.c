#include "mspm0g3507_board.h"
#include "adc.h"
#include "adc_dma_scope.h"
#include "adc_apl.h"
#include "adc_fml.h"
#include "ad9833.h"
#include "ad9959.h"
#include "app_params.h"
#include "capture_input.h"
#include "delay.h"
#include "dma_port.h"
#include "frequency_meter.h"
#include "gpio_io.h"
#include "i2c_bus.h"
#include "oled_ssd1306.h"
#include "phase_meter.h"
#include "pwm_out.h"
#include "rms_meter.h"
#include "signal_tools_lab.h"
#include "spi.h"
#include "test_console.h"
#include "tim.h"
#include "tjc_screen.h"
#include "uart.h"
#include "waveform_capture.h"

#define MSPM0G3507_ADC_TIMEOUT_LOOPS (20000U)

static MSPM0G3507_Status_t ToBoardStatus(Periph_Status_t status)
{
    switch (status) {
        case PERIPH_STATUS_OK:
            return MSPM0G3507_STATUS_OK;
        case PERIPH_STATUS_BAD_PARAM:
            return MSPM0G3507_STATUS_BAD_PARAM;
        case PERIPH_STATUS_TIMEOUT:
            return MSPM0G3507_STATUS_TIMEOUT;
        case PERIPH_STATUS_NOT_CONFIGURED:
            return MSPM0G3507_STATUS_NOT_CONFIGURED;
        case PERIPH_STATUS_ERROR:
        default:
            return MSPM0G3507_STATUS_ERROR;
    }
}

static AD9833_Waveform_t ToAd9833Waveform(MSPM0G3507_Waveform_t waveform)
{
    switch (waveform) {
        case MSPM0G3507_WAVE_TRIANGLE:
            return AD9833_WAVE_TRIANGLE;
        case MSPM0G3507_WAVE_SQUARE:
            return AD9833_WAVE_SQUARE;
        case MSPM0G3507_WAVE_SINE:
        default:
            return AD9833_WAVE_SINE;
    }
}

static AD9833_Channel_t ToAd9833Channel(
    MSPM0G3507_AD9833_Channel_t channel)
{
    return (AD9833_Channel_t) channel;
}

void MSPM0G3507_Board_Init(void)
{
    GPIOIO_Init();
    AppParams_Init();
    Timer_Start();
    SoftSPI_Init();
    Uart_Init();
    TjcScreen_Init();
    ADC_Apl_Init();
    SignalToolsLab_Init();
    TestConsole_Init();
}

void MSPM0G3507_Board_Proc(void)
{
    if (!Waveform_IsVofaStreaming() && !SignalToolsLab_IsStreaming()) {
        ADC_Apl_Proc();
    }
    TjcScreen_Proc();
    TestConsole_Proc();
}

uint32_t MSPM0G3507_GetTickMs(void)
{
    return Timer_GetTickMs();
}

uint32_t MSPM0G3507_GetTimer10msTicks(void)
{
    return Timer_GetTimer1Ticks();
}

void MSPM0G3507_DelayMs(uint32_t delayMs)
{
    Delay_Ms(delayMs);
}

void MSPM0G3507_DelayUs(uint32_t delayUs)
{
    Delay_Us(delayUs);
}

void MSPM0G3507_LedOn(void)
{
    GPIOIO_LedOn();
}

void MSPM0G3507_LedOff(void)
{
    GPIOIO_LedOff();
}

void MSPM0G3507_LedToggle(void)
{
    GPIOIO_LedToggle();
}

bool MSPM0G3507_KeyIsPressed(void)
{
    return GPIOIO_KeyIsPressed();
}

void MSPM0G3507_BuzzerOn(void)
{
    GPIOIO_BuzzerOn();
}

void MSPM0G3507_BuzzerOff(void)
{
    GPIOIO_BuzzerOff();
}

void MSPM0G3507_BuzzerBeep(uint32_t onMs)
{
    GPIOIO_BuzzerBeep(onMs);
}

bool MSPM0G3507_ReadAdc(MSPM0G3507_AdcSample_t *sample)
{
    ADC_Bll_Result_t result;

    if ((sample == 0) || !ADC_Bll_SampleAndDetect(&result)) {
        return false;
    }

    sample->raw       = result.raw;
    sample->millivolt = result.millivolt;
    sample->state     = (MSPM0G3507_AdcState_t) result.state;
    return true;
}

bool MSPM0G3507_ReadAdcRaw(uint16_t *raw)
{
    return ADC_SampleBlocking(raw, MSPM0G3507_ADC_TIMEOUT_LOOPS);
}

uint16_t MSPM0G3507_AdcRawToMillivolt(uint16_t raw)
{
    return ADC_Fml_RawToMillivolt(raw);
}

uint8_t MSPM0G3507_SoftSpiTransferByte(uint8_t txData)
{
    uint8_t rxData = 0U;

    SoftSPI_Transfer(&txData, &rxData, 1U);
    return rxData;
}

void MSPM0G3507_SoftSpiTransfer(const uint8_t *tx, uint8_t *rx, size_t length)
{
    SoftSPI_Transfer(tx, rx, length);
}

bool MSPM0G3507_UartReadByte(uint8_t *data)
{
    return Uart_ReadByte(data);
}

void MSPM0G3507_UartWriteByte(uint8_t data)
{
    Uart_WriteByte(data);
}

void MSPM0G3507_UartWriteString(const char *text)
{
    Uart_WriteString(text);
}

void MSPM0G3507_UartWriteU32(uint32_t value)
{
    Uart_WriteU32(value);
}

void MSPM0G3507_VofaStart(void)
{
    SignalToolsLab_StopStreaming();
    Waveform_VofaStart();
}

void MSPM0G3507_VofaStop(void)
{
    Waveform_VofaStop();
}

bool MSPM0G3507_VofaIsRunning(void)
{
    return Waveform_IsVofaStreaming();
}

bool MSPM0G3507_ScopeCapture(uint32_t sampleRateHz, uint16_t sampleCount,
    MSPM0G3507_ScopeFrame_t *frame)
{
    AdcDmaScope_Frame_t internalFrame;

    if ((frame == 0) ||
        !AdcDmaScope_Capture(sampleRateHz, sampleCount, &internalFrame)) {
        return false;
    }

    frame->sampleRateHz = internalFrame.sampleRateHz;
    frame->sampleCount = internalFrame.sampleCount;
    frame->usedDma = internalFrame.usedDma;
    for (uint16_t i = 0U; i < internalFrame.sampleCount; i++) {
        frame->samples[i] = internalFrame.samples[i];
    }
    return true;
}

bool MSPM0G3507_ScopeVofaStart(uint32_t sampleRateHz)
{
    Waveform_VofaStop();
    return AdcDmaScope_StartVofa(sampleRateHz);
}

void MSPM0G3507_ScopeVofaStop(void)
{
    AdcDmaScope_StopVofa();
}

bool MSPM0G3507_ScopeVofaIsRunning(void)
{
    return AdcDmaScope_IsVofaRunning();
}

bool MSPM0G3507_ReadFrequency(MSPM0G3507_Frequency_t *result)
{
    FrequencyMeter_Result_t internalResult;

    if ((result == 0) || !FrequencyMeter_Read(&internalResult)) {
        return false;
    }
    result->valid = internalResult.valid;
    result->frequencyHz = internalResult.frequencyHz;
    result->periodUs = internalResult.periodUs;
    result->dutyPermille = internalResult.dutyPermille;
    result->edgeCount = internalResult.edgeCount;
    result->ageMs = internalResult.ageMs;
    return true;
}

bool MSPM0G3507_ReadPhase(MSPM0G3507_Phase_t *result)
{
    PhaseMeter_Result_t internalResult;

    if ((result == 0) || !PhaseMeter_Read(&internalResult)) {
        return false;
    }
    result->valid = internalResult.valid;
    result->signedDeg = internalResult.signedDeg;
    result->phaseDeg = internalResult.phaseDeg;
    result->deltaUs = internalResult.deltaUs;
    result->periodUs = internalResult.periodUs;
    result->edgeCountA = internalResult.edgeCountA;
    result->edgeCountB = internalResult.edgeCountB;
    return true;
}

bool MSPM0G3507_ReadRms(uint32_t sampleRateHz, uint16_t sampleCount,
    MSPM0G3507_RmsResult_t *result)
{
    RmsMeter_Result_t internalResult;

    if ((result == 0) ||
        !RmsMeter_Measure(sampleRateHz, sampleCount, &internalResult)) {
        return false;
    }
    result->sampleRateHz = internalResult.sampleRateHz;
    result->sampleCount = internalResult.sampleCount;
    result->minRaw = internalResult.minRaw;
    result->maxRaw = internalResult.maxRaw;
    result->avgRaw = internalResult.avgRaw;
    result->pkpkRaw = internalResult.pkpkRaw;
    result->minMv = internalResult.minMv;
    result->maxMv = internalResult.maxMv;
    result->avgMv = internalResult.avgMv;
    result->pkpkMv = internalResult.pkpkMv;
    result->rmsMv = internalResult.rmsMv;
    result->acRmsMv = internalResult.acRmsMv;
    return true;
}

bool MSPM0G3507_RmsCalibrateZero(uint32_t sampleRateHz, uint16_t sampleCount)
{
    return RmsMeter_CalibrateZero(sampleRateHz, sampleCount);
}

bool MSPM0G3507_RmsSetZeroRaw(int32_t zeroRaw)
{
    return RmsMeter_SetZeroRaw(zeroRaw);
}

bool MSPM0G3507_RmsSetGainPermille(uint32_t gainPermille)
{
    return RmsMeter_SetGainPermille(gainPermille);
}

MSPM0G3507_RmsCalibration_t MSPM0G3507_RmsGetCalibration(void)
{
    RmsMeter_Calibration_t internalCalibration = RmsMeter_GetCalibration();
    MSPM0G3507_RmsCalibration_t calibration;

    calibration.zeroRaw = internalCalibration.zeroRaw;
    calibration.gainPermille = internalCalibration.gainPermille;
    return calibration;
}

void MSPM0G3507_AD9959_Init(void)
{
    AD9959_Init();
}

void MSPM0G3507_AD9959_SetSysclkHz(uint32_t sysclkHz)
{
    AD9959_SetSysclkHz(sysclkHz);
}

uint32_t MSPM0G3507_AD9959_GetSysclkHz(void)
{
    return AD9959_GetSysclkHz();
}

void MSPM0G3507_AD9959_PowerDown(bool enable)
{
    AD9959_PowerDown(enable);
}

bool MSPM0G3507_AD9959_SetSingleTone(uint8_t channel,
    uint32_t frequencyHz, uint16_t phaseDeg, uint16_t amplitude)
{
    if (channel >= AD9959_CHANNEL_COUNT) {
        return false;
    }
    return AD9959_SetSingleTone((AD9959_Channel_t) channel, frequencyHz,
        phaseDeg, amplitude);
}

bool MSPM0G3507_AD9959_SelfTestPattern(void)
{
    return AD9959_SelfTestPattern();
}

void MSPM0G3507_AD9833_Init(void)
{
    AD9833_Init();
}

void MSPM0G3507_AD9833_SetMclkHz(uint32_t mclkHz)
{
    AD9833_SetMclkHz(mclkHz);
}

bool MSPM0G3507_AD9833_SetOutput(MSPM0G3507_Waveform_t waveform,
    uint32_t frequencyHz, uint16_t phaseDeg)
{
    return MSPM0G3507_AD9833_SetOutputChannel(MSPM0G3507_AD9833_ALL,
        waveform, frequencyHz, phaseDeg);
}

bool MSPM0G3507_AD9833_SetOutputChannel(
    MSPM0G3507_AD9833_Channel_t channel, MSPM0G3507_Waveform_t waveform,
    uint32_t frequencyHz, uint16_t phaseDeg)
{
    return AD9833_SetOutputChannel(ToAd9833Channel(channel),
        ToAd9833Waveform(waveform), frequencyHz, phaseDeg);
}

bool MSPM0G3507_AD9833_SetAmplitude(uint8_t amplitude)
{
    return AD9833_SetAmplitude(amplitude);
}

MSPM0G3507_Status_t MSPM0G3507_I2CScan(uint8_t *addresses, uint8_t maxCount,
    uint8_t *foundCount)
{
    return ToBoardStatus(I2CBus_Scan(addresses, maxCount, foundCount));
}

MSPM0G3507_Status_t MSPM0G3507_OledTest(void)
{
    Periph_Status_t status = OledSsd1306_Init(OLED_SSD1306_DEFAULT_ADDR);

    if (status == PERIPH_STATUS_OK) {
        status = OledSsd1306_Clear();
    }
    if (status == PERIPH_STATUS_OK) {
        status = OledSsd1306_WriteText(0U, 0U, "MSPM0G3507");
    }

    return ToBoardStatus(status);
}

MSPM0G3507_Status_t MSPM0G3507_SoftPwmLed(uint32_t frequencyHz,
    uint16_t dutyPermille, uint32_t durationMs)
{
    return ToBoardStatus(PwmOut_StartSoftLed(frequencyHz, dutyPermille,
        durationMs));
}

MSPM0G3507_Status_t MSPM0G3507_HardwarePwmStart(uint32_t frequencyHz,
    uint16_t dutyPermille)
{
    return ToBoardStatus(PwmOut_StartHardware(frequencyHz, dutyPermille));
}

MSPM0G3507_Status_t MSPM0G3507_CaptureRead(
    MSPM0G3507_CaptureResult_t *result)
{
    CaptureInput_Result_t internalResult;
    Periph_Status_t status;

    if (result == 0) {
        return MSPM0G3507_STATUS_BAD_PARAM;
    }
    status = CaptureInput_Read(&internalResult);
    if (status == PERIPH_STATUS_OK) {
        result->frequencyHz = internalResult.frequencyHz;
        result->dutyPermille = internalResult.dutyPermille;
        result->phaseDeg = internalResult.phaseDeg;
    }
    return ToBoardStatus(status);
}

MSPM0G3507_Status_t MSPM0G3507_DmaMemCopy(uint32_t *dst, const uint32_t *src,
    uint16_t wordCount)
{
    return ToBoardStatus(DmaPort_MemCopy(dst, src, wordCount));
}

MSPM0G3507_Status_t MSPM0G3507_AdcDmaStart(uint16_t *buffer, uint16_t count)
{
    return ToBoardStatus(DmaPort_AdcStart(buffer, count));
}

const char *MSPM0G3507_StatusText(MSPM0G3507_Status_t status)
{
    switch (status) {
        case MSPM0G3507_STATUS_OK:
            return "OK";
        case MSPM0G3507_STATUS_BAD_PARAM:
            return "BAD_PARAM";
        case MSPM0G3507_STATUS_TIMEOUT:
            return "TIMEOUT";
        case MSPM0G3507_STATUS_NOT_CONFIGURED:
            return "NOT_CONFIGURED";
        case MSPM0G3507_STATUS_ERROR:
        default:
            return "ERROR";
    }
}
