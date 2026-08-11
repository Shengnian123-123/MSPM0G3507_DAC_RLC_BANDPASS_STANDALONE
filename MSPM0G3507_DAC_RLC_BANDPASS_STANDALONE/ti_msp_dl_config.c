/*
 * Copyright (c) 2023, Texas Instruments Incorporated
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
 *  ============ ti_msp_dl_config.c =============
 *  Configured MSPM0 DriverLib module definitions
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */

#include "ti_msp_dl_config.h"

DL_TimerG_backupConfig gTIMER_1Backup;
DL_TimerG_backupConfig gADC_SAMPLE_TIMERBackup;
DL_UART_Main_backupConfig gUART_SCREENBackup;

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform any initialization needed before using any board APIs
 */
SYSCONFIG_WEAK void SYSCFG_DL_init(void)
{
    SYSCFG_DL_initPower();
    SYSCFG_DL_GPIO_init();
    /* Module-Specific Initializations*/
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_TIMER_0_init();
    SYSCFG_DL_TIMER_1_init();
    SYSCFG_DL_ADC_SAMPLE_TIMER_init();
    SYSCFG_DL_UART_0_init();
    SYSCFG_DL_UART_SCREEN_init();
    SYSCFG_DL_ADC12_0_init();
    SYSCFG_DL_DMA_init();
    SYSCFG_DL_DAC12_init();
    /* Ensure backup structures have no valid state */
	gTIMER_1Backup.backupRdy 	= false;
	gADC_SAMPLE_TIMERBackup.backupRdy 	= false;
	gUART_SCREENBackup.backupRdy 	= false;

}
/*
 * User should take care to save and restore register configuration in application.
 * See Retention Configuration section for more details.
 */
SYSCONFIG_WEAK bool SYSCFG_DL_saveConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerG_saveConfiguration(TIMER_1_INST, &gTIMER_1Backup);
	retStatus &= DL_TimerG_saveConfiguration(ADC_SAMPLE_TIMER_INST, &gADC_SAMPLE_TIMERBackup);
	retStatus &= DL_UART_Main_saveConfiguration(UART_SCREEN_INST, &gUART_SCREENBackup);

    return retStatus;
}


SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerG_restoreConfiguration(TIMER_1_INST, &gTIMER_1Backup, false);
	retStatus &= DL_TimerG_restoreConfiguration(ADC_SAMPLE_TIMER_INST, &gADC_SAMPLE_TIMERBackup, false);
	retStatus &= DL_UART_Main_restoreConfiguration(UART_SCREEN_INST, &gUART_SCREENBackup);

    return retStatus;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerG_reset(TIMER_0_INST);
    DL_TimerG_reset(TIMER_1_INST);
    DL_TimerG_reset(ADC_SAMPLE_TIMER_INST);
    DL_UART_Main_reset(UART_0_INST);
    DL_UART_Main_reset(UART_SCREEN_INST);
    DL_ADC12_reset(ADC12_0_INST);

    DL_DAC12_reset(DAC0);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerG_enablePower(TIMER_0_INST);
    DL_TimerG_enablePower(TIMER_1_INST);
    DL_TimerG_enablePower(ADC_SAMPLE_TIMER_INST);
    DL_UART_Main_enablePower(UART_0_INST);
    DL_UART_Main_enablePower(UART_SCREEN_INST);
    DL_ADC12_enablePower(ADC12_0_INST);

    DL_DAC12_enablePower(DAC0);
    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_0_IOMUX_TX, GPIO_UART_0_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_0_IOMUX_RX, GPIO_UART_0_IOMUX_RX_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_SCREEN_IOMUX_TX, GPIO_UART_SCREEN_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_SCREEN_IOMUX_RX, GPIO_UART_SCREEN_IOMUX_RX_FUNC);

    DL_GPIO_initDigitalOutput(GPIO_STATUS_LED_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_SOFT_SPI_SCLK_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_SOFT_SPI_MOSI_IOMUX);

    DL_GPIO_initDigitalInputFeatures(GPIO_SOFT_SPI_MISO_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalOutput(GPIO_SOFT_SPI_AD9959_CS_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_AD9959_CTRL_IO_UPDATE_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_AD9959_CTRL_RESET_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_AD9959_CTRL_PWR_D_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_AD9959_CTRL_P0_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_AD9959_CTRL_P1_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_AD9959_CTRL_P2_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_AD9959_CTRL_P3_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_AD9833_CTRL_CS1_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_AD9833_CTRL_CS2_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_ADS1220_CTRL_ADS1220_CS_IOMUX);

    DL_GPIO_initDigitalInputFeatures(GPIO_UI_KEY_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalOutput(GPIO_UI_BUZZER_IOMUX);

    DL_GPIO_clearPins(GPIOA, GPIO_SOFT_SPI_SCLK_PIN |
		GPIO_SOFT_SPI_MOSI_PIN);
    DL_GPIO_setPins(GPIOA, GPIO_STATUS_LED_PIN |
		GPIO_SOFT_SPI_AD9959_CS_PIN);
    DL_GPIO_enableOutput(GPIOA, GPIO_STATUS_LED_PIN |
		GPIO_SOFT_SPI_SCLK_PIN |
		GPIO_SOFT_SPI_MOSI_PIN |
		GPIO_SOFT_SPI_AD9959_CS_PIN);
    DL_GPIO_clearPins(GPIOB, GPIO_AD9959_CTRL_IO_UPDATE_PIN |
		GPIO_AD9959_CTRL_RESET_PIN |
		GPIO_AD9959_CTRL_PWR_D_PIN |
		GPIO_AD9959_CTRL_P0_PIN |
		GPIO_AD9959_CTRL_P1_PIN |
		GPIO_AD9959_CTRL_P2_PIN |
		GPIO_AD9959_CTRL_P3_PIN |
		GPIO_UI_BUZZER_PIN);
    DL_GPIO_setPins(GPIOB, GPIO_AD9833_CTRL_CS1_PIN |
		GPIO_AD9833_CTRL_CS2_PIN |
		GPIO_ADS1220_CTRL_ADS1220_CS_PIN);
    DL_GPIO_enableOutput(GPIOB, GPIO_AD9959_CTRL_IO_UPDATE_PIN |
		GPIO_AD9959_CTRL_RESET_PIN |
		GPIO_AD9959_CTRL_PWR_D_PIN |
		GPIO_AD9959_CTRL_P0_PIN |
		GPIO_AD9959_CTRL_P1_PIN |
		GPIO_AD9959_CTRL_P2_PIN |
		GPIO_AD9959_CTRL_P3_PIN |
		GPIO_AD9833_CTRL_CS1_PIN |
		GPIO_AD9833_CTRL_CS2_PIN |
		GPIO_ADS1220_CTRL_ADS1220_CS_PIN |
		GPIO_UI_BUZZER_PIN);

}



SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{

	//Low Power Mode is configured to be SLEEP0
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

    
	DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
	/* Set default configuration */
	DL_SYSCTL_disableHFXT();
	DL_SYSCTL_disableSYSPLL();
    DL_SYSCTL_enableMFPCLK();
	DL_SYSCTL_setMFPCLKSource(DL_SYSCTL_MFPCLK_SOURCE_SYSOSC);

}



/*
 * Timer clock configuration to be sourced by BUSCLK /  (32000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   1032258.0645161291 Hz = 32000000 Hz / (1 * (30 + 1))
 */
static const DL_TimerG_ClockConfig gTIMER_0ClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 30U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_0_INST_LOAD_VALUE = (1 ms * 1032258.0645161291 Hz) - 1
 */
static const DL_TimerG_TimerConfig gTIMER_0TimerConfig = {
    .period     = TIMER_0_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_STOP,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_0_init(void) {

    DL_TimerG_setClockConfig(TIMER_0_INST,
        (DL_TimerG_ClockConfig *) &gTIMER_0ClockConfig);

    DL_TimerG_initTimerMode(TIMER_0_INST,
        (DL_TimerG_TimerConfig *) &gTIMER_0TimerConfig);
    DL_TimerG_enableInterrupt(TIMER_0_INST , DL_TIMERG_INTERRUPT_ZERO_EVENT);
    DL_TimerG_enableClock(TIMER_0_INST);





}

/*
 * Timer clock configuration to be sourced by BUSCLK /  (32000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   1032258.0645161291 Hz = 32000000 Hz / (1 * (30 + 1))
 */
static const DL_TimerG_ClockConfig gTIMER_1ClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 30U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_1_INST_LOAD_VALUE = (10 ms * 1032258.0645161291 Hz) - 1
 */
static const DL_TimerG_TimerConfig gTIMER_1TimerConfig = {
    .period     = TIMER_1_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_STOP,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_1_init(void) {

    DL_TimerG_setClockConfig(TIMER_1_INST,
        (DL_TimerG_ClockConfig *) &gTIMER_1ClockConfig);

    DL_TimerG_initTimerMode(TIMER_1_INST,
        (DL_TimerG_TimerConfig *) &gTIMER_1TimerConfig);
    DL_TimerG_enableInterrupt(TIMER_1_INST , DL_TIMERG_INTERRUPT_ZERO_EVENT);
    DL_TimerG_enableClock(TIMER_1_INST);





}

/*
 * Timer clock configuration to be sourced by BUSCLK /  (32000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   32000000 Hz = 32000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gADC_SAMPLE_TIMERClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 0U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * ADC_SAMPLE_TIMER_INST_LOAD_VALUE = (5 us * 32000000 Hz) - 1
 */
static const DL_TimerG_TimerConfig gADC_SAMPLE_TIMERTimerConfig = {
    .period     = ADC_SAMPLE_TIMER_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_STOP,
};

SYSCONFIG_WEAK void SYSCFG_DL_ADC_SAMPLE_TIMER_init(void) {

    DL_TimerG_setClockConfig(ADC_SAMPLE_TIMER_INST,
        (DL_TimerG_ClockConfig *) &gADC_SAMPLE_TIMERClockConfig);

    DL_TimerG_initTimerMode(ADC_SAMPLE_TIMER_INST,
        (DL_TimerG_TimerConfig *) &gADC_SAMPLE_TIMERTimerConfig);
    DL_TimerG_enableClock(ADC_SAMPLE_TIMER_INST);


    DL_TimerG_enableEvent(ADC_SAMPLE_TIMER_INST, DL_TIMERG_EVENT_ROUTE_1, (DL_TIMERG_EVENT_ZERO_EVENT));

    DL_TimerG_setPublisherChanID(ADC_SAMPLE_TIMER_INST, DL_TIMERG_PUBLISHER_INDEX_0, ADC_SAMPLE_TIMER_INST_PUB_0_CH);



}


static const DL_UART_Main_ClockConfig gUART_0ClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_0Config = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_0_init(void)
{
    DL_UART_Main_setClockConfig(UART_0_INST, (DL_UART_Main_ClockConfig *) &gUART_0ClockConfig);

    DL_UART_Main_init(UART_0_INST, (DL_UART_Main_Config *) &gUART_0Config);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115211.52
     */
    DL_UART_Main_setOversampling(UART_0_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_0_INST, UART_0_IBRD_32_MHZ_115200_BAUD, UART_0_FBRD_32_MHZ_115200_BAUD);


    /* Configure Interrupts */
    DL_UART_Main_enableInterrupt(UART_0_INST,
                                 DL_UART_MAIN_INTERRUPT_RX);

    /* Configure FIFOs */
    DL_UART_Main_enableFIFOs(UART_0_INST);
    DL_UART_Main_setRXFIFOThreshold(UART_0_INST, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    DL_UART_Main_setTXFIFOThreshold(UART_0_INST, DL_UART_TX_FIFO_LEVEL_ONE_ENTRY);

    DL_UART_Main_enable(UART_0_INST);
}
static const DL_UART_Main_ClockConfig gUART_SCREENClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_SCREENConfig = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_SCREEN_init(void)
{
    DL_UART_Main_setClockConfig(UART_SCREEN_INST, (DL_UART_Main_ClockConfig *) &gUART_SCREENClockConfig);

    DL_UART_Main_init(UART_SCREEN_INST, (DL_UART_Main_Config *) &gUART_SCREENConfig);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115211.52
     */
    DL_UART_Main_setOversampling(UART_SCREEN_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_SCREEN_INST, UART_SCREEN_IBRD_32_MHZ_115200_BAUD, UART_SCREEN_FBRD_32_MHZ_115200_BAUD);


    /* Configure Interrupts */
    DL_UART_Main_enableInterrupt(UART_SCREEN_INST,
                                 DL_UART_MAIN_INTERRUPT_RX);

    /* Configure FIFOs */
    DL_UART_Main_enableFIFOs(UART_SCREEN_INST);
    DL_UART_Main_setRXFIFOThreshold(UART_SCREEN_INST, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    DL_UART_Main_setTXFIFOThreshold(UART_SCREEN_INST, DL_UART_TX_FIFO_LEVEL_ONE_ENTRY);

    DL_UART_Main_enable(UART_SCREEN_INST);
}

/* ADC12_0 Initialization */
static const DL_ADC12_ClockConfig gADC12_0ClockConfig = {
    .clockSel       = DL_ADC12_CLOCK_ULPCLK,
    .divideRatio    = DL_ADC12_CLOCK_DIVIDE_1,
    .freqRange      = DL_ADC12_CLOCK_FREQ_RANGE_24_TO_32,
};
SYSCONFIG_WEAK void SYSCFG_DL_ADC12_0_init(void)
{
    DL_ADC12_setClockConfig(ADC12_0_INST, (DL_ADC12_ClockConfig *) &gADC12_0ClockConfig);
    DL_ADC12_initSingleSample(ADC12_0_INST,
        DL_ADC12_REPEAT_MODE_ENABLED, DL_ADC12_SAMPLING_SOURCE_AUTO, DL_ADC12_TRIG_SRC_EVENT,
        DL_ADC12_SAMP_CONV_RES_12_BIT, DL_ADC12_SAMP_CONV_DATA_FORMAT_UNSIGNED);
    DL_ADC12_configConversionMem(ADC12_0_INST, ADC12_0_ADCMEM_0,
        DL_ADC12_INPUT_CHAN_2, DL_ADC12_REFERENCE_VOLTAGE_VDDA, DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0, DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED, DL_ADC12_TRIGGER_MODE_TRIGGER_NEXT, DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_enableFIFO(ADC12_0_INST);
    DL_ADC12_setPowerDownMode(ADC12_0_INST,DL_ADC12_POWER_DOWN_MODE_MANUAL);
    DL_ADC12_setSampleTime0(ADC12_0_INST,2);
    DL_ADC12_enableDMA(ADC12_0_INST);
    DL_ADC12_setDMASamplesCnt(ADC12_0_INST,6);
    DL_ADC12_enableDMATrigger(ADC12_0_INST,(DL_ADC12_DMA_MEM10_RESULT_LOADED));
    DL_ADC12_setSubscriberChanID(ADC12_0_INST,ADC12_0_INST_SUB_CH);
    /* Enable ADC12 interrupt */
    DL_ADC12_clearInterruptStatus(ADC12_0_INST,(DL_ADC12_INTERRUPT_DMA_DONE));
    DL_ADC12_enableInterrupt(ADC12_0_INST,(DL_ADC12_INTERRUPT_DMA_DONE));
    DL_ADC12_enableConversions(ADC12_0_INST);
}

static const DL_DMA_Config gDMA_CH1Config = {
    .transferMode   = DL_DMA_FULL_CH_REPEAT_SINGLE_TRANSFER_MODE,
    .extendedMode   = DL_DMA_NORMAL_MODE,
    .destIncrement  = DL_DMA_ADDR_UNCHANGED,
    .srcIncrement   = DL_DMA_ADDR_INCREMENT,
    .destWidth      = DL_DMA_WIDTH_HALF_WORD,
    .srcWidth       = DL_DMA_WIDTH_HALF_WORD,
    .trigger        = DAC12_INST_DMA_TRIGGER,
    .triggerType    = DL_DMA_TRIGGER_TYPE_EXTERNAL,
};

SYSCONFIG_WEAK void SYSCFG_DL_DMA_CH1_init(void)
{
    DL_DMA_initChannel(DMA, DMA_CH1_CHAN_ID , (DL_DMA_Config *) &gDMA_CH1Config);
}
static const DL_DMA_Config gDMA_CH0Config = {
    .transferMode   = DL_DMA_SINGLE_TRANSFER_MODE,
    .extendedMode   = DL_DMA_NORMAL_MODE,
    .destIncrement  = DL_DMA_ADDR_INCREMENT,
    .srcIncrement   = DL_DMA_ADDR_UNCHANGED,
    .destWidth      = DL_DMA_WIDTH_WORD,
    .srcWidth       = DL_DMA_WIDTH_WORD,
    .trigger        = ADC12_0_INST_DMA_TRIGGER,
    .triggerType    = DL_DMA_TRIGGER_TYPE_EXTERNAL,
};

SYSCONFIG_WEAK void SYSCFG_DL_DMA_CH0_init(void)
{
    DL_DMA_initChannel(DMA, DMA_CH0_CHAN_ID , (DL_DMA_Config *) &gDMA_CH0Config);
}
SYSCONFIG_WEAK void SYSCFG_DL_DMA_init(void){
    SYSCFG_DL_DMA_CH1_init();
    SYSCFG_DL_DMA_CH0_init();
}


static const DL_DAC12_Config gDAC12Config = {
    .outputEnable              = DL_DAC12_OUTPUT_ENABLED,
    .resolution                = DL_DAC12_RESOLUTION_12BIT,
    .representation            = DL_DAC12_REPRESENTATION_BINARY,
    .voltageReferenceSource    = DL_DAC12_VREF_SOURCE_VDDA_VSSA,
    .amplifierSetting          = DL_DAC12_AMP_ON,
    .fifoEnable                = DL_DAC12_FIFO_ENABLED,
    .fifoTriggerSource         = DL_DAC12_FIFO_TRIGGER_SAMPLETIMER,
    .dmaTriggerEnable          = DL_DAC12_DMA_TRIGGER_ENABLED,
    .dmaTriggerThreshold       = DL_DAC12_FIFO_THRESHOLD_TWO_QTRS_EMPTY,
    .sampleTimeGeneratorEnable = DL_DAC12_SAMPLETIMER_ENABLE,
    .sampleRate                = DL_DAC12_SAMPLES_PER_SECOND_1M,
};
SYSCONFIG_WEAK void SYSCFG_DL_DAC12_init(void)
{
    DL_DAC12_init(DAC0, (DL_DAC12_Config *) &gDAC12Config);
    DL_DAC12_enableInterrupt(DAC0, (DL_DAC12_INTERRUPT_DMA_DONE));
    DL_DAC12_enable(DAC0);
}

