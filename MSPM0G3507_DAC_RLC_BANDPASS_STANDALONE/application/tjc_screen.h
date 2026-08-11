#ifndef APP_TJC_SCREEN_H
#define APP_TJC_SCREEN_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    TJC_SCREEN_TARGET_BOTH = 0,
    TJC_SCREEN_TARGET_AD9833,
    TJC_SCREEN_TARGET_AD9959,
} TjcScreen_Target_t;

typedef struct {
    TjcScreen_Target_t target;
    uint8_t ad9833Channel;
    uint8_t ad9959Channel;
    uint32_t frequencyHz;
    uint16_t ad9833Amplitude;
    uint16_t ad9959Amplitude;
    uint16_t phaseDeg;
    uint8_t waveform;
    uint32_t commandCount;
    uint32_t errorCount;
    uint8_t lastFrameLength;
    uint8_t lastFrame[16];
    char lastText[24];
} TjcScreen_Status_t;

void TjcScreen_Init(void);
void TjcScreen_Proc(void);
TjcScreen_Status_t TjcScreen_GetStatus(void);
void TjcScreen_SendCommand(const char *command);

#endif
