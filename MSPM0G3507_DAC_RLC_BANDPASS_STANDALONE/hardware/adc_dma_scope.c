#include "adc_dma_scope.h"
#include "adc.h"
#include "adc_fml.h"
#include "frequency_meter.h"
#include "main.h"
#include "tim.h"
#include "uart.h"

#define ADC_DMA_SCOPE_ADC_TIMEOUT_LOOPS (20000U)
#define ADC_DMA_SCOPE_DMA_TIMEOUT_MS    (20U)
#define ADC_DMA_SCOPE_DMA_MIN_RATE_HZ   (200000UL)
#define ADC_DMA_SCOPE_FIFO_PACKED_SAMPLES_PER_WORD (2U)
#define ADC_DMA_SCOPE_SAMPLE_TIMER_CLOCK_HZ (32000000UL)

static uint32_t g_dmaWords[(ADC_DMA_SCOPE_MAX_SAMPLES +
    ADC_DMA_SCOPE_FIFO_PACKED_SAMPLES_PER_WORD - 1U) /
    ADC_DMA_SCOPE_FIFO_PACKED_SAMPLES_PER_WORD];
static volatile bool g_dmaDone;

static bool g_vofaRunning;
static uint32_t g_vofaPeriodMs;
static uint32_t g_vofaNextTick;
static uint16_t g_vofaOffsetRaw = 2048U;

static uint32_t ClampRate(uint32_t sampleRateHz, uint32_t maxRateHz)
{
    if (sampleRateHz < ADC_DMA_SCOPE_MIN_RATE_HZ) {
        return ADC_DMA_SCOPE_MIN_RATE_HZ;
    }
    if (sampleRateHz > maxRateHz) {
        return maxRateHz;
    }
    return sampleRateHz;
}

static uint16_t ClampCount(uint16_t sampleCount)
{
    if (sampleCount == 0U) {
        return 1U;
    }
    if (sampleCount > ADC_DMA_SCOPE_MAX_SAMPLES) {
        return ADC_DMA_SCOPE_MAX_SAMPLES;
    }
    return sampleCount;
}

static int32_t RawToSignedMillivolt(uint16_t raw, uint16_t offsetRaw)
{
    int32_t delta = (int32_t) raw - (int32_t) offsetRaw;

    return (delta * (int32_t) ADC_FML_VREF_MV) /
           (int32_t) ADC_FML_FULL_SCALE_RAW;
}

static void StopAdcDmaCapture(void)
{
#if defined(ADC_SAMPLE_TIMER_INST)
    DL_TimerG_stopCounter(ADC_SAMPLE_TIMER_INST);
#endif
#if defined(DMA_CH0_CHAN_ID) && defined(ADC12_0_INST_DMA_TRIGGER)
    DL_ADC12_stopConversion(ADC12_0_INST);
    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
    DL_ADC12_clearInterruptStatus(ADC12_0_INST,
        DL_ADC12_INTERRUPT_DMA_DONE | DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
#endif
}

static uint32_t ConfigureSampleTimer(uint32_t requestedRateHz)
{
    uint32_t periodTicks;

#if defined(ADC_SAMPLE_TIMER_INST)
    periodTicks = (ADC_DMA_SCOPE_SAMPLE_TIMER_CLOCK_HZ +
        (requestedRateHz / 2U)) / requestedRateHz;
    if (periodTicks < 2U) {
        periodTicks = 2U;
    }
    DL_TimerG_stopCounter(ADC_SAMPLE_TIMER_INST);
    DL_TimerG_setLoadValue(ADC_SAMPLE_TIMER_INST, periodTicks - 1U);
    DL_TimerG_setTimerCount(ADC_SAMPLE_TIMER_INST, periodTicks - 1U);
    return ADC_DMA_SCOPE_SAMPLE_TIMER_CLOCK_HZ / periodTicks;
#else
    (void) requestedRateHz;
    return 0U;
#endif
}

static void UnpackDmaWords(AdcDmaScope_Frame_t *frame, uint16_t sampleCount)
{
    for (uint16_t i = 0U; i < sampleCount; i++) {
        uint32_t packed = g_dmaWords[i /
            ADC_DMA_SCOPE_FIFO_PACKED_SAMPLES_PER_WORD];

        if ((i & 1U) == 0U) {
            frame->samples[i] = (uint16_t) (packed & 0x0FFFU);
        } else {
            frame->samples[i] = (uint16_t) ((packed >> 16U) & 0x0FFFU);
        }
    }
}

static bool CaptureDma(uint32_t sampleRateHz, uint16_t sampleCount,
    AdcDmaScope_Frame_t *frame)
{
    uint16_t packedCount;
    uint64_t startTicks;
    uint64_t endTicks;
    uint64_t timeoutTicks;
    uint32_t ticksPerSecond;
    uint32_t actualRateHz;

#if defined(DMA_CH0_CHAN_ID) && defined(ADC12_0_INST_DMA_TRIGGER)
    if ((frame == 0) || (sampleRateHz < ADC_DMA_SCOPE_DMA_MIN_RATE_HZ)) {
        return false;
    }

    if ((sampleCount & 1U) != 0U) {
        sampleCount--;
    }
    if (sampleCount < 2U) {
        return false;
    }

    StopAdcDmaCapture();
    g_dmaDone = false;
    packedCount = sampleCount / ADC_DMA_SCOPE_FIFO_PACKED_SAMPLES_PER_WORD;

    for (uint16_t i = 0U; i < packedCount; i++) {
        g_dmaWords[i] = 0U;
    }

    DL_ADC12_enableFIFO(ADC12_0_INST);
    DL_ADC12_enableDMA(ADC12_0_INST);
    DL_ADC12_setDMASamplesCnt(ADC12_0_INST, 6U);
    DL_ADC12_enableDMATrigger(ADC12_0_INST, DL_ADC12_DMA_MEM10_RESULT_LOADED);
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID,
        (uint32_t) DL_ADC12_getFIFOAddress(ADC12_0_INST));
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t) &g_dmaWords[0]);
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, packedCount);
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);

    DL_ADC12_clearInterruptStatus(ADC12_0_INST,
        DL_ADC12_INTERRUPT_DMA_DONE | DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    NVIC_ClearPendingIRQ(ADC12_0_INST_INT_IRQN);
    NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);

    actualRateHz = ConfigureSampleTimer(sampleRateHz);
    if (actualRateHz == 0U) {
        StopAdcDmaCapture();
        return false;
    }

    ticksPerSecond = FrequencyMeter_GetTicksPerSecond();
    timeoutTicks = ((uint64_t) ticksPerSecond * ADC_DMA_SCOPE_DMA_TIMEOUT_MS) /
        1000ULL;
    if (timeoutTicks == 0ULL) {
        timeoutTicks = 1ULL;
    }

    startTicks = FrequencyMeter_ReadTimestampTicks();
    DL_ADC12_startConversion(ADC12_0_INST);
#if defined(ADC_SAMPLE_TIMER_INST)
    DL_TimerG_startCounter(ADC_SAMPLE_TIMER_INST);
#endif
    while (!g_dmaDone) {
        endTicks = FrequencyMeter_ReadTimestampTicks();
        if ((endTicks - startTicks) > timeoutTicks) {
            StopAdcDmaCapture();
            frame->sampleCount = 0U;
            return false;
        }
    }
    endTicks = FrequencyMeter_ReadTimestampTicks();
    StopAdcDmaCapture();

    frame->sampleCount = sampleCount;
    frame->usedDma = true;
    UnpackDmaWords(frame, sampleCount);
    frame->sampleRateHz = actualRateHz;
    return true;
#else
    (void) sampleRateHz;
    (void) sampleCount;
    (void) frame;
    return false;
#endif
}

static bool CaptureBlocking(uint32_t sampleRateHz, uint16_t sampleCount,
    AdcDmaScope_Frame_t *frame)
{
    uint64_t firstTicks = 0U;
    uint64_t lastTicks = 0U;
    uint64_t targetTicks;
    uint32_t ticksPerSample;
    uint32_t ticksPerSecond;

    if (frame == 0) {
        return false;
    }

    sampleRateHz = ClampRate(sampleRateHz, ADC_DMA_SCOPE_BLOCKING_MAX_RATE_HZ);
    sampleCount  = ClampCount(sampleCount);
    ticksPerSecond = FrequencyMeter_GetTicksPerSecond();
    ticksPerSample = ticksPerSecond / sampleRateHz;
    if (ticksPerSample == 0U) {
        ticksPerSample = 1U;
    }

    frame->sampleRateHz = sampleRateHz;
    frame->sampleCount  = sampleCount;
    frame->usedDma      = false;
    targetTicks = FrequencyMeter_ReadTimestampTicks();

    for (uint16_t i = 0U; i < sampleCount; i++) {
        while (FrequencyMeter_ReadTimestampTicks() < targetTicks) {
        }
        if (i == 0U) {
            firstTicks = FrequencyMeter_ReadTimestampTicks();
        } else if ((i + 1U) == sampleCount) {
            lastTicks = FrequencyMeter_ReadTimestampTicks();
        }

        if (!ADC_SampleBlocking(&frame->samples[i],
                ADC_DMA_SCOPE_ADC_TIMEOUT_LOOPS)) {
            frame->sampleCount = i;
            return false;
        }
        targetTicks += ticksPerSample;
    }

    if ((sampleCount > 1U) && (lastTicks > firstTicks)) {
        frame->sampleRateHz =
            (uint32_t) ((((uint64_t) (sampleCount - 1U)) * ticksPerSecond) /
                        (lastTicks - firstTicks));
    }

    return true;
}

bool AdcDmaScope_Capture(uint32_t sampleRateHz, uint16_t sampleCount,
    AdcDmaScope_Frame_t *frame)
{
    if (frame == 0) {
        return false;
    }

    sampleRateHz = ClampRate(sampleRateHz, ADC_DMA_SCOPE_MAX_RATE_HZ);
    sampleCount  = ClampCount(sampleCount);
    frame->sampleRateHz = sampleRateHz;
    frame->sampleCount = sampleCount;
    frame->usedDma = false;

    if (CaptureDma(sampleRateHz, sampleCount, frame)) {
        return true;
    }
    return CaptureBlocking(sampleRateHz, sampleCount, frame);
}

bool AdcDmaScope_StartVofa(uint32_t sampleRateHz)
{
    sampleRateHz = ClampRate(sampleRateHz, ADC_DMA_SCOPE_VOFA_MAX_RATE_HZ);
    g_vofaPeriodMs = 1000UL / sampleRateHz;
    if (g_vofaPeriodMs == 0U) {
        g_vofaPeriodMs = 1U;
    }
    g_vofaNextTick = Timer_GetTickMs();
    g_vofaRunning  = true;
    return true;
}

void AdcDmaScope_StopVofa(void)
{
    g_vofaRunning = false;
}

bool AdcDmaScope_IsVofaRunning(void)
{
    return g_vofaRunning;
}

void AdcDmaScope_Proc(void)
{
    uint16_t raw;
    uint16_t mv;
    int32_t acMv;

    if (!g_vofaRunning) {
        return;
    }
    if ((int32_t) (Timer_GetTickMs() - g_vofaNextTick) < 0) {
        return;
    }
    g_vofaNextTick += g_vofaPeriodMs;

    if (!ADC_SampleBlocking(&raw, ADC_DMA_SCOPE_ADC_TIMEOUT_LOOPS)) {
        return;
    }

    mv = ADC_Fml_RawToMillivolt(raw);
    acMv = RawToSignedMillivolt(raw, g_vofaOffsetRaw);
    Uart_Printf("%u,%u,%ld\r\n", raw, mv, (long) acMv);
}

void ADC12_0_INST_IRQHandler(void)
{
    switch (DL_ADC12_getPendingInterrupt(ADC12_0_INST)) {
        case DL_ADC12_IIDX_DMA_DONE:
            g_dmaDone = true;
            break;
        default:
            break;
    }
}
