#ifndef APP_PARAMS_H
#define APP_PARAMS_H

#include <stdbool.h>
#include <stdint.h>

#define APP_PARAMS_OUTPUT_CAL_COUNT (8U)

typedef enum {
    APP_PARAMS_STATUS_OK = 0,
    APP_PARAMS_STATUS_BAD_PARAM,
    APP_PARAMS_STATUS_CRC_FAIL,
    APP_PARAMS_STATUS_FLASH_FAIL,
} AppParams_Status_t;

typedef struct {
    int32_t kp;
    int32_t ki;
    int32_t kd;
    uint8_t shift;
} AppParams_Pid_t;

typedef struct {
    int16_t adcOffsetRaw;
    uint16_t adcGainPermille;
    uint16_t adcZeroRaw;
    uint16_t adcRefRaw;
    uint16_t adcRefMv;
    AppParams_Pid_t pid;
    uint16_t agcTargetPkpkRaw;
    uint16_t agcHysteresisRaw;
    uint8_t agcGainMin;
    uint8_t agcGainMax;
    uint8_t agcInitialGain;
    uint16_t outputCal[APP_PARAMS_OUTPUT_CAL_COUNT];
} AppParams_t;

void AppParams_Init(void);
void AppParams_ResetDefaults(void);
const AppParams_t *AppParams_Get(void);
AppParams_t *AppParams_Edit(void);
uint16_t AppParams_ApplyAdcCalibration(uint16_t raw);
AppParams_Status_t AppParams_SetAdcZero(uint16_t raw);
AppParams_Status_t AppParams_SetAdcGain(uint16_t measuredRaw,
    uint16_t expectedMv);
AppParams_Status_t AppParams_SaveToFlash(void);
AppParams_Status_t AppParams_LoadFromFlash(void);
const char *AppParams_StatusText(AppParams_Status_t status);

#endif
