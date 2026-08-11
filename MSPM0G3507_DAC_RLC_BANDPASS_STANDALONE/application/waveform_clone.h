#ifndef APP_WAVEFORM_CLONE_H
#define APP_WAVEFORM_CLONE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    WAVEFORM_CLONE_TYPE_UNKNOWN = 0,
    WAVEFORM_CLONE_TYPE_SINE,
    WAVEFORM_CLONE_TYPE_TRIANGLE,
    WAVEFORM_CLONE_TYPE_SQUARE,
} WaveformClone_Type_t;

typedef struct {
    bool valid;
    bool usedDma;
    WaveformClone_Type_t type;
    uint32_t sampleRateHz;
    uint16_t sampleCount;
    uint32_t frequencyHz;
    uint16_t minMv;
    uint16_t maxMv;
    uint16_t avgMv;
    uint16_t pkpkMv;
    uint16_t acRmsMv;
    uint16_t targetPkpkMv;
    int16_t targetHighMv;
    int16_t targetLowMv;
    uint16_t dutyPermille;
    uint16_t rmsToPkpkPermille;
    uint16_t madToPkpkPermille;
    uint16_t railCountPermille;
    uint16_t edgeBandPermille;
    uint16_t centerBandPermille;
    uint16_t flatSlopePermille;
    uint16_t steepSlopePermille;
    uint16_t harmonic3Permille;
    uint16_t harmonic5Permille;
    const char *rejectReason;
    uint8_t ad9833Amplitude;
    uint16_t ad9959Amplitude;
} WaveformClone_Result_t;

void WaveformClone_WriteHelp(void);
bool WaveformClone_HandleCommand(const char *line);
const char *WaveformClone_TypeText(WaveformClone_Type_t type);
bool WaveformClone_Analyze(uint32_t sampleRateHz, uint16_t sampleCount,
    WaveformClone_Result_t *result);
bool WaveformClone_OutputAd9833(const WaveformClone_Result_t *result);
bool WaveformClone_OutputAd9959(uint8_t channel,
    const WaveformClone_Result_t *result);

#endif
