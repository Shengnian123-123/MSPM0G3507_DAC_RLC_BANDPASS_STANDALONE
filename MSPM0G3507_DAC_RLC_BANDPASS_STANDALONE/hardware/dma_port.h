#ifndef HW_DMA_PORT_H
#define HW_DMA_PORT_H

#include <stdint.h>
#include "peripheral_status.h"

Periph_Status_t DmaPort_MemCopy(uint32_t *dst, const uint32_t *src,
    uint16_t wordCount);
Periph_Status_t DmaPort_AdcStart(uint16_t *buffer, uint16_t count);

#endif
