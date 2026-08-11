#include "dma_port.h"

Periph_Status_t DmaPort_MemCopy(uint32_t *dst, const uint32_t *src,
    uint16_t wordCount)
{
    if ((dst == 0) || (src == 0) || (wordCount == 0U)) {
        return PERIPH_STATUS_BAD_PARAM;
    }

    for (uint16_t i = 0U; i < wordCount; i++) {
        dst[i] = src[i];
    }
    return PERIPH_STATUS_OK;
}

Periph_Status_t DmaPort_AdcStart(uint16_t *buffer, uint16_t count)
{
    (void) buffer;
    (void) count;
    return PERIPH_STATUS_NOT_CONFIGURED;
}
