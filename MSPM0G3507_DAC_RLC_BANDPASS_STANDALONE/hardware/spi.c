#include "spi.h"

static SoftSPI_GPIOConfig_t g_softSpi = {
    GPIO_SOFT_SPI_PORT,
    GPIO_SOFT_SPI_SCLK_PIN,
    GPIO_SOFT_SPI_MOSI_PIN,
    GPIO_SOFT_SPI_MISO_PIN,
    GPIO_SOFT_SPI_AD9959_CS_PIN,
    16U,
};

void SoftSPI_Init(void)
{
    DL_GPIO_setPins(g_softSpi.port, g_softSpi.csPin);
    DL_GPIO_clearPins(g_softSpi.port, g_softSpi.sclkPin | g_softSpi.mosiPin);
    delay_cycles(g_softSpi.delayCycles);
}

void SoftSPI_SetDelay(uint16_t delayCycles)
{
    g_softSpi.delayCycles = delayCycles;
}

void SoftSPI_Select(void)
{
    DL_GPIO_clearPins(g_softSpi.port, g_softSpi.csPin);
    delay_cycles(g_softSpi.delayCycles);
}

void SoftSPI_Release(void)
{
    DL_GPIO_setPins(g_softSpi.port, g_softSpi.csPin);
    delay_cycles(g_softSpi.delayCycles);
}

uint8_t SoftSPI_TransferByte(uint8_t txData)
{
    uint8_t rxData = 0;

    SoftSPI_GPIOTransfer(&g_softSpi, &txData, &rxData, 1U, false);

    return rxData;
}

void SoftSPI_GPIOTransfer(const SoftSPI_GPIOConfig_t *config, const uint8_t *tx,
    uint8_t *rx, size_t length, bool controlCs)
{
    if ((config == 0) || (length == 0U)) {
        return;
    }

    if (controlCs) {
        DL_GPIO_clearPins(config->port, config->csPin);
        delay_cycles(config->delayCycles);
    }

    for (size_t i = 0; i < length; i++) {
        uint8_t txByte = (tx == 0) ? 0xFFU : tx[i];
        uint8_t rxByte = 0U;

        for (uint8_t mask = 0x80U; mask != 0U; mask >>= 1U) {
            if ((txByte & mask) != 0U) {
                DL_GPIO_setPins(config->port, config->mosiPin);
            } else {
                DL_GPIO_clearPins(config->port, config->mosiPin);
            }
            delay_cycles(config->delayCycles);

            DL_GPIO_setPins(config->port, config->sclkPin);
            delay_cycles(config->delayCycles);

            rxByte <<= 1U;
            if ((DL_GPIO_readPins(config->port, config->misoPin) &
                 config->misoPin) != 0U) {
                rxByte |= 1U;
            }

            DL_GPIO_clearPins(config->port, config->sclkPin);
            delay_cycles(config->delayCycles);
        }

        if (rx != 0) {
            rx[i] = rxByte;
        }
    }

    if (controlCs) {
        DL_GPIO_setPins(config->port, config->csPin);
        delay_cycles(config->delayCycles);
    }
}

void SoftSPI_Transfer(const uint8_t *tx, uint8_t *rx, size_t length)
{
    SoftSPI_GPIOTransfer(&g_softSpi, tx, rx, length, true);
}
