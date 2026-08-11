#include "app_params.h"
#include "adc_fml.h"
#include "main.h"

#define APP_PARAMS_MAGIC       (0x5033524DU)
#define APP_PARAMS_VERSION     (1U)
#define APP_PARAMS_FLASH_ADDR  (0x0001FC00UL)
#define APP_PARAMS_FLASH_BYTES (1024U)

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    AppParams_t params;
    uint32_t crc;
} AppParams_Record_t;

static AppParams_t g_params;
static uint32_t g_flashBufferWords[APP_PARAMS_FLASH_BYTES / sizeof(uint32_t)];

static bool ParamsAreSane(const AppParams_t *params)
{
    if (params == 0) {
        return false;
    }
    if ((params->adcGainPermille == 0U) ||
        (params->adcZeroRaw > ADC_FML_FULL_SCALE_RAW) ||
        (params->adcRefRaw > ADC_FML_FULL_SCALE_RAW) ||
        (params->adcRefMv == 0U) ||
        (params->adcRefMv > ADC_FML_VREF_MV) ||
        (params->pid.shift > 15U) ||
        (params->agcGainMin > params->agcGainMax) ||
        (params->agcInitialGain < params->agcGainMin) ||
        (params->agcInitialGain > params->agcGainMax)) {
        return false;
    }

    for (uint8_t i = 0U; i < APP_PARAMS_OUTPUT_CAL_COUNT; i++) {
        if (params->outputCal[i] > 10000U) {
            return false;
        }
    }

    return true;
}

static uint32_t Crc32Update(uint32_t crc, const uint8_t *data, uint32_t length)
{
    while (length > 0U) {
        crc ^= *data++;
        for (uint8_t bit = 0U; bit < 8U; bit++) {
            if ((crc & 1UL) != 0U) {
                crc = (crc >> 1U) ^ 0xEDB88320UL;
            } else {
                crc >>= 1U;
            }
        }
        length--;
    }
    return crc;
}

static uint32_t ParamsCrc(const AppParams_Record_t *record)
{
    uint32_t crc = 0xFFFFFFFFUL;

    crc = Crc32Update(crc, (const uint8_t *) &record->magic,
        sizeof(record->magic));
    crc = Crc32Update(crc, (const uint8_t *) &record->version,
        sizeof(record->version));
    crc = Crc32Update(crc, (const uint8_t *) &record->size,
        sizeof(record->size));
    crc = Crc32Update(crc, (const uint8_t *) &record->params,
        sizeof(record->params));
    return ~crc;
}

void AppParams_ResetDefaults(void)
{
    g_params.adcOffsetRaw = 0;
    g_params.adcGainPermille = 1000U;
    g_params.adcZeroRaw = 0U;
    g_params.adcRefRaw = ADC_FML_FULL_SCALE_RAW;
    g_params.adcRefMv = ADC_FML_VREF_MV;
    g_params.pid.kp = 256;
    g_params.pid.ki = 16;
    g_params.pid.kd = 0;
    g_params.pid.shift = 8U;
    g_params.agcTargetPkpkRaw = 1800U;
    g_params.agcHysteresisRaw = 200U;
    g_params.agcGainMin = 0U;
    g_params.agcGainMax = 15U;
    g_params.agcInitialGain = 8U;

    for (uint8_t i = 0U; i < APP_PARAMS_OUTPUT_CAL_COUNT; i++) {
        g_params.outputCal[i] = (uint16_t) ((i * 1000U) /
            (APP_PARAMS_OUTPUT_CAL_COUNT - 1U));
    }
}

void AppParams_Init(void)
{
    AppParams_ResetDefaults();
    (void) AppParams_LoadFromFlash();
}

const AppParams_t *AppParams_Get(void)
{
    return &g_params;
}

AppParams_t *AppParams_Edit(void)
{
    return &g_params;
}

uint16_t AppParams_ApplyAdcCalibration(uint16_t raw)
{
    int32_t corrected = (int32_t) raw + (int32_t) g_params.adcOffsetRaw;

    corrected = (corrected * (int32_t) g_params.adcGainPermille) / 1000L;
    if (corrected < 0) {
        corrected = 0;
    } else if (corrected > ADC_FML_FULL_SCALE_RAW) {
        corrected = ADC_FML_FULL_SCALE_RAW;
    }

    return (uint16_t) corrected;
}

AppParams_Status_t AppParams_SetAdcZero(uint16_t raw)
{
    if (raw > ADC_FML_FULL_SCALE_RAW) {
        return APP_PARAMS_STATUS_BAD_PARAM;
    }

    g_params.adcZeroRaw = raw;
    g_params.adcOffsetRaw = -(int16_t) raw;
    return APP_PARAMS_STATUS_OK;
}

AppParams_Status_t AppParams_SetAdcGain(uint16_t measuredRaw,
    uint16_t expectedMv)
{
    uint32_t expectedRaw;
    uint32_t gainPermille;
    int32_t zeroCorrected;

    if ((measuredRaw == 0U) || (expectedMv == 0U) ||
        (expectedMv > ADC_FML_VREF_MV)) {
        return APP_PARAMS_STATUS_BAD_PARAM;
    }

    expectedRaw = ((uint32_t) expectedMv * ADC_FML_FULL_SCALE_RAW) /
        ADC_FML_VREF_MV;
    zeroCorrected = (int32_t) measuredRaw + (int32_t) g_params.adcOffsetRaw;
    if (zeroCorrected <= 0) {
        return APP_PARAMS_STATUS_BAD_PARAM;
    }

    gainPermille = (expectedRaw * 1000UL) / (uint32_t) zeroCorrected;
    if ((gainPermille == 0U) || (gainPermille > 0xFFFFUL)) {
        return APP_PARAMS_STATUS_BAD_PARAM;
    }
    g_params.adcRefRaw = measuredRaw;
    g_params.adcRefMv = expectedMv;
    g_params.adcGainPermille = (uint16_t) gainPermille;
    return APP_PARAMS_STATUS_OK;
}

AppParams_Status_t AppParams_LoadFromFlash(void)
{
    const AppParams_Record_t *record =
        (const AppParams_Record_t *) APP_PARAMS_FLASH_ADDR;

    if ((record->magic != APP_PARAMS_MAGIC) ||
        (record->version != APP_PARAMS_VERSION) ||
        (record->size != sizeof(AppParams_t))) {
        return APP_PARAMS_STATUS_CRC_FAIL;
    }

    if (ParamsCrc(record) != record->crc) {
        return APP_PARAMS_STATUS_CRC_FAIL;
    }

    if (!ParamsAreSane(&record->params)) {
        return APP_PARAMS_STATUS_BAD_PARAM;
    }

    g_params = record->params;
    return APP_PARAMS_STATUS_OK;
}

AppParams_Status_t AppParams_SaveToFlash(void)
{
    AppParams_Record_t record;
    uint8_t *flashBytes = (uint8_t *) g_flashBufferWords;
    uint32_t *words = g_flashBufferWords;
    uint32_t lengthBytes = sizeof(record);
    uint32_t paddedBytes = (lengthBytes + 7UL) & ~7UL;
    DL_FLASHCTL_COMMAND_STATUS eraseStatus;

    if (paddedBytes > APP_PARAMS_FLASH_BYTES) {
        return APP_PARAMS_STATUS_BAD_PARAM;
    }

    for (uint32_t i = 0U; i < APP_PARAMS_FLASH_BYTES; i++) {
        flashBytes[i] = 0xFFU;
    }
    record.magic = APP_PARAMS_MAGIC;
    record.version = APP_PARAMS_VERSION;
    record.size = sizeof(AppParams_t);
    record.params = g_params;
    record.crc = ParamsCrc(&record);
    for (uint32_t i = 0U; i < sizeof(record); i++) {
        flashBytes[i] = ((uint8_t *) &record)[i];
    }

    __disable_irq();
    DL_FlashCTL_executeClearStatus(FLASHCTL);
    DL_FlashCTL_unprotectSector(FLASHCTL, APP_PARAMS_FLASH_ADDR,
        DL_FLASHCTL_REGION_SELECT_MAIN);
    eraseStatus = DL_FlashCTL_eraseMemoryFromRAM(FLASHCTL,
        APP_PARAMS_FLASH_ADDR, DL_FLASHCTL_COMMAND_SIZE_SECTOR);
    if (eraseStatus != DL_FLASHCTL_COMMAND_STATUS_PASSED) {
        __enable_irq();
        return APP_PARAMS_STATUS_FLASH_FAIL;
    }

    for (uint32_t offset = 0U; offset < paddedBytes; offset += 8U) {
        DL_FLASHCTL_COMMAND_STATUS status;

        DL_FlashCTL_executeClearStatus(FLASHCTL);
        DL_FlashCTL_unprotectSector(FLASHCTL, APP_PARAMS_FLASH_ADDR + offset,
            DL_FLASHCTL_REGION_SELECT_MAIN);
        status = DL_FlashCTL_programMemoryFromRAM64WithECCGenerated(FLASHCTL,
            APP_PARAMS_FLASH_ADDR + offset, &words[offset / 4U]);
        if (status != DL_FLASHCTL_COMMAND_STATUS_PASSED) {
            __enable_irq();
            return APP_PARAMS_STATUS_FLASH_FAIL;
        }
    }
    __enable_irq();

    return AppParams_LoadFromFlash();
}

const char *AppParams_StatusText(AppParams_Status_t status)
{
    switch (status) {
        case APP_PARAMS_STATUS_OK:
            return "OK";
        case APP_PARAMS_STATUS_BAD_PARAM:
            return "BAD_PARAM";
        case APP_PARAMS_STATUS_CRC_FAIL:
            return "CRC_FAIL";
        case APP_PARAMS_STATUS_FLASH_FAIL:
        default:
            return "FLASH_FAIL";
    }
}
