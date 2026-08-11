#ifndef SIGNAL_SPECTRAL_H
#define SIGNAL_SPECTRAL_H

#include <stdbool.h>
#include <stdint.h>

#define SIGNAL_SPECTRAL_DFT_COUNT (64U)
#define SIGNAL_SPECTRAL_MAX_HARMONIC (5U)

typedef struct {
    uint16_t bin;
    uint32_t binFrequencyHz;
    uint64_t power;
    uint32_t magnitudeRaw;
} SignalSpectral_BinResult_t;

typedef struct {
    uint16_t fundamentalBin;
    uint32_t fundamentalHz;
    uint64_t fundamentalPower;
    uint64_t harmonicPower;
    uint32_t thdPermille;
} SignalSpectral_ThdResult_t;

bool SignalSpectral_DftBin64(const uint16_t *samples, uint16_t bin,
    uint32_t sampleRateHz, SignalSpectral_BinResult_t *result);
bool SignalSpectral_GoertzelHz64(const uint16_t *samples, uint32_t targetHz,
    uint32_t sampleRateHz, SignalSpectral_BinResult_t *result);
bool SignalSpectral_Thd64(const uint16_t *samples, uint32_t fundamentalHz,
    uint32_t sampleRateHz, SignalSpectral_ThdResult_t *result);
bool SignalSpectral_PhaseDiffDeg(const uint16_t *samplesA,
    const uint16_t *samplesB, uint16_t count, uint32_t signalFrequencyHz,
    uint32_t sampleRateHz, int16_t *phaseDeg);

#endif
