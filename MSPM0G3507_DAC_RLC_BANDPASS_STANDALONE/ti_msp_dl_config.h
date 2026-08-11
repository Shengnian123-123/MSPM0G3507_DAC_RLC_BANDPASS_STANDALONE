/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     32000000



/* Defines for TIMER_0 */
#define TIMER_0_INST                                                     (TIMG0)
#define TIMER_0_INST_IRQHandler                                 TIMG0_IRQHandler
#define TIMER_0_INST_INT_IRQN                                   (TIMG0_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                          (1031U)
/* Defines for TIMER_1 */
#define TIMER_1_INST                                                     (TIMG7)
#define TIMER_1_INST_IRQHandler                                 TIMG7_IRQHandler
#define TIMER_1_INST_INT_IRQN                                   (TIMG7_INT_IRQn)
#define TIMER_1_INST_LOAD_VALUE                                         (10322U)
/* Defines for ADC_SAMPLE_TIMER */
#define ADC_SAMPLE_TIMER_INST                                            (TIMG6)
#define ADC_SAMPLE_TIMER_INST_IRQHandler                        TIMG6_IRQHandler
#define ADC_SAMPLE_TIMER_INST_INT_IRQN                          (TIMG6_INT_IRQn)
#define ADC_SAMPLE_TIMER_INST_LOAD_VALUE                                  (159U)
#define ADC_SAMPLE_TIMER_INST_PUB_0_CH                                       (3)



/* Defines for UART_0 */
#define UART_0_INST                                                        UART0
#define UART_0_INST_FREQUENCY                                           32000000
#define UART_0_INST_IRQHandler                                  UART0_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART0_INT_IRQn
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                        DL_GPIO_PIN_11
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_10
#define GPIO_UART_0_IOMUX_RX                                     (IOMUX_PINCM22)
#define GPIO_UART_0_IOMUX_TX                                     (IOMUX_PINCM21)
#define GPIO_UART_0_IOMUX_RX_FUNC                      IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                      IOMUX_PINCM21_PF_UART0_TX
#define UART_0_BAUD_RATE                                                (115200)
#define UART_0_IBRD_32_MHZ_115200_BAUD                                      (17)
#define UART_0_FBRD_32_MHZ_115200_BAUD                                      (23)
/* Defines for UART_SCREEN */
#define UART_SCREEN_INST                                                   UART3
#define UART_SCREEN_INST_FREQUENCY                                      32000000
#define UART_SCREEN_INST_IRQHandler                             UART3_IRQHandler
#define UART_SCREEN_INST_INT_IRQN                                 UART3_INT_IRQn
#define GPIO_UART_SCREEN_RX_PORT                                           GPIOB
#define GPIO_UART_SCREEN_TX_PORT                                           GPIOB
#define GPIO_UART_SCREEN_RX_PIN                                   DL_GPIO_PIN_13
#define GPIO_UART_SCREEN_TX_PIN                                   DL_GPIO_PIN_12
#define GPIO_UART_SCREEN_IOMUX_RX                                (IOMUX_PINCM30)
#define GPIO_UART_SCREEN_IOMUX_TX                                (IOMUX_PINCM29)
#define GPIO_UART_SCREEN_IOMUX_RX_FUNC                 IOMUX_PINCM30_PF_UART3_RX
#define GPIO_UART_SCREEN_IOMUX_TX_FUNC                 IOMUX_PINCM29_PF_UART3_TX
#define UART_SCREEN_BAUD_RATE                                           (115200)
#define UART_SCREEN_IBRD_32_MHZ_115200_BAUD                                 (17)
#define UART_SCREEN_FBRD_32_MHZ_115200_BAUD                                 (23)





/* Defines for ADC12_0 */
#define ADC12_0_INST                                                        ADC0
#define ADC12_0_INST_IRQHandler                                  ADC0_IRQHandler
#define ADC12_0_INST_INT_IRQN                                    (ADC0_INT_IRQn)
#define ADC12_0_ADCMEM_0                                      DL_ADC12_MEM_IDX_0
#define ADC12_0_ADCMEM_0_REF                     DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC12_0_ADCMEM_0_REF_VOLTAGE_V                                       3.3
#define ADC12_0_INST_SUB_CH                                                  (3)
#define GPIO_ADC12_0_C2_PORT                                               GPIOA
#define GPIO_ADC12_0_C2_PIN                                       DL_GPIO_PIN_25
#define GPIO_ADC12_0_IOMUX_C2                                    (IOMUX_PINCM55)
#define GPIO_ADC12_0_IOMUX_C2_FUNC                (IOMUX_PINCM55_PF_UNCONNECTED)



/* Defines for DMA_CH1 */
#define DMA_CH1_CHAN_ID                                                      (1)
#define DAC12_INST_DMA_TRIGGER                          (DMA_DAC0_EVT_BD_1_TRIG)
/* Defines for DMA_CH0 */
#define DMA_CH0_CHAN_ID                                                      (0)
#define ADC12_0_INST_DMA_TRIGGER                      (DMA_ADC0_EVT_GEN_BD_TRIG)


/* Port definition for Pin Group GPIO_STATUS */
#define GPIO_STATUS_PORT                                                 (GPIOA)

/* Defines for LED: GPIOA.0 with pinCMx 1 on package pin 33 */
#define GPIO_STATUS_LED_PIN                                      (DL_GPIO_PIN_0)
#define GPIO_STATUS_LED_IOMUX                                     (IOMUX_PINCM1)
/* Port definition for Pin Group GPIO_SOFT_SPI */
#define GPIO_SOFT_SPI_PORT                                               (GPIOA)

/* Defines for SCLK: GPIOA.8 with pinCMx 19 on package pin 54 */
#define GPIO_SOFT_SPI_SCLK_PIN                                   (DL_GPIO_PIN_8)
#define GPIO_SOFT_SPI_SCLK_IOMUX                                 (IOMUX_PINCM19)
/* Defines for MOSI: GPIOA.9 with pinCMx 20 on package pin 55 */
#define GPIO_SOFT_SPI_MOSI_PIN                                   (DL_GPIO_PIN_9)
#define GPIO_SOFT_SPI_MOSI_IOMUX                                 (IOMUX_PINCM20)
/* Defines for MISO: GPIOA.12 with pinCMx 34 on package pin 5 */
#define GPIO_SOFT_SPI_MISO_PIN                                  (DL_GPIO_PIN_12)
#define GPIO_SOFT_SPI_MISO_IOMUX                                 (IOMUX_PINCM34)
/* Defines for AD9959_CS: GPIOA.13 with pinCMx 35 on package pin 6 */
#define GPIO_SOFT_SPI_AD9959_CS_PIN                             (DL_GPIO_PIN_13)
#define GPIO_SOFT_SPI_AD9959_CS_IOMUX                            (IOMUX_PINCM35)
/* Port definition for Pin Group GPIO_AD9959_CTRL */
#define GPIO_AD9959_CTRL_PORT                                            (GPIOB)

/* Defines for IO_UPDATE: GPIOB.0 with pinCMx 12 on package pin 47 */
#define GPIO_AD9959_CTRL_IO_UPDATE_PIN                           (DL_GPIO_PIN_0)
#define GPIO_AD9959_CTRL_IO_UPDATE_IOMUX                         (IOMUX_PINCM12)
/* Defines for RESET: GPIOB.1 with pinCMx 13 on package pin 48 */
#define GPIO_AD9959_CTRL_RESET_PIN                               (DL_GPIO_PIN_1)
#define GPIO_AD9959_CTRL_RESET_IOMUX                             (IOMUX_PINCM13)
/* Defines for PWR_D: GPIOB.2 with pinCMx 15 on package pin 50 */
#define GPIO_AD9959_CTRL_PWR_D_PIN                               (DL_GPIO_PIN_2)
#define GPIO_AD9959_CTRL_PWR_D_IOMUX                             (IOMUX_PINCM15)
/* Defines for P0: GPIOB.3 with pinCMx 16 on package pin 51 */
#define GPIO_AD9959_CTRL_P0_PIN                                  (DL_GPIO_PIN_3)
#define GPIO_AD9959_CTRL_P0_IOMUX                                (IOMUX_PINCM16)
/* Defines for P1: GPIOB.4 with pinCMx 17 on package pin 52 */
#define GPIO_AD9959_CTRL_P1_PIN                                  (DL_GPIO_PIN_4)
#define GPIO_AD9959_CTRL_P1_IOMUX                                (IOMUX_PINCM17)
/* Defines for P2: GPIOB.7 with pinCMx 24 on package pin 59 */
#define GPIO_AD9959_CTRL_P2_PIN                                  (DL_GPIO_PIN_7)
#define GPIO_AD9959_CTRL_P2_IOMUX                                (IOMUX_PINCM24)
/* Defines for P3: GPIOB.6 with pinCMx 23 on package pin 58 */
#define GPIO_AD9959_CTRL_P3_PIN                                  (DL_GPIO_PIN_6)
#define GPIO_AD9959_CTRL_P3_IOMUX                                (IOMUX_PINCM23)
/* Port definition for Pin Group GPIO_AD9833_CTRL */
#define GPIO_AD9833_CTRL_PORT                                            (GPIOB)

/* Defines for CS1: GPIOB.8 with pinCMx 25 on package pin 60 */
#define GPIO_AD9833_CTRL_CS1_PIN                                 (DL_GPIO_PIN_8)
#define GPIO_AD9833_CTRL_CS1_IOMUX                               (IOMUX_PINCM25)
/* Defines for CS2: GPIOB.9 with pinCMx 26 on package pin 61 */
#define GPIO_AD9833_CTRL_CS2_PIN                                 (DL_GPIO_PIN_9)
#define GPIO_AD9833_CTRL_CS2_IOMUX                               (IOMUX_PINCM26)
/* Port definition for Pin Group GPIO_ADS1220_CTRL */
#define GPIO_ADS1220_CTRL_PORT                                           (GPIOB)

/* Defines for ADS1220_CS: GPIOB.5 with pinCMx 18 on package pin 53 */
#define GPIO_ADS1220_CTRL_ADS1220_CS_PIN                         (DL_GPIO_PIN_5)
#define GPIO_ADS1220_CTRL_ADS1220_CS_IOMUX                       (IOMUX_PINCM18)
/* Port definition for Pin Group GPIO_UI */
#define GPIO_UI_PORT                                                     (GPIOB)

/* Defines for KEY: GPIOB.10 with pinCMx 27 on package pin 62 */
#define GPIO_UI_KEY_PIN                                         (DL_GPIO_PIN_10)
#define GPIO_UI_KEY_IOMUX                                        (IOMUX_PINCM27)
/* Defines for BUZZER: GPIOB.11 with pinCMx 28 on package pin 63 */
#define GPIO_UI_BUZZER_PIN                                      (DL_GPIO_PIN_11)
#define GPIO_UI_BUZZER_IOMUX                                     (IOMUX_PINCM28)



/* Defines for DAC12 */
#define DAC12_IRQHandler                                         DAC0_IRQHandler
#define DAC12_INT_IRQN                                           (DAC0_INT_IRQn)
#define GPIO_DAC12_OUT_PORT                                                GPIOA
#define GPIO_DAC12_OUT_PIN                                        DL_GPIO_PIN_15
#define GPIO_DAC12_IOMUX_OUT                                     (IOMUX_PINCM37)
#define GPIO_DAC12_IOMUX_OUT_FUNC                   IOMUX_PINCM37_PF_UNCONNECTED


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_TIMER_0_init(void);
void SYSCFG_DL_TIMER_1_init(void);
void SYSCFG_DL_ADC_SAMPLE_TIMER_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_UART_SCREEN_init(void);
void SYSCFG_DL_ADC12_0_init(void);
void SYSCFG_DL_DMA_init(void);

void SYSCFG_DL_DAC12_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
