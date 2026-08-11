#include "dac_waveform.h"
#include "ti_msp_dl_config.h"

/*
 * MATLAB reference:
 *   waveform: 50 kHz rectangular, 35% duty
 *   filter:   RLC band-pass, R = 1 kOhm, C = 1 nF, L = 1 mH
 *   DAC:      12-bit, 1 MSPS
 *
 * These 20 samples are one 50 kHz period of the band-pass-filtered
 * rectangular wave, scaled to avoid DAC clipping on real hardware.
 */
static const uint16_t gWaveform[DAC_WAVEFORM_SAMPLE_COUNT] = {
    2658U, 3982U, 3346U, 2349U, 1812U,
    1757U, 1900U, 1580U, 171U, 709U,
    1689U, 2256U, 2348U, 2206U, 2063U,
    2004U, 2007U, 2026U, 2042U, 2071U,
};

static bool gDacWaveformEnabled;

void DacWaveform_Init(void)
{
    DL_DMA_setSrcAddr(DMA, DMA_CH1_CHAN_ID, (uint32_t) &gWaveform[0]);
    DL_DMA_setDestAddr(DMA, DMA_CH1_CHAN_ID, (uint32_t) &(DAC0->DATA0));
    DL_DMA_setTransferSize(DMA, DMA_CH1_CHAN_ID, DAC_WAVEFORM_SAMPLE_COUNT);
    DL_DMA_enableChannel(DMA, DMA_CH1_CHAN_ID);
    gDacWaveformEnabled = true;
}

void DacWaveform_SetEnabled(bool enabled)
{
    gDacWaveformEnabled = enabled;
}

bool DacWaveform_IsEnabled(void)
{
    return gDacWaveformEnabled;
}
