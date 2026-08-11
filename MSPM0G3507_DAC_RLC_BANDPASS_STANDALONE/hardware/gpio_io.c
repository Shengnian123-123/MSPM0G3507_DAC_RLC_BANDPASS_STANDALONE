#include "gpio_io.h"
#include "delay.h"

static bool g_ledOn;
static bool g_buzzerOn;

void GPIOIO_Init(void)
{
    g_ledOn    = true;
    g_buzzerOn = false;
    DL_GPIO_setPins(GPIO_STATUS_PORT, GPIO_STATUS_LED_PIN);
    DL_GPIO_clearPins(GPIO_UI_PORT, GPIO_UI_BUZZER_PIN);
}

void GPIOIO_LedOn(void)
{
    DL_GPIO_setPins(GPIO_STATUS_PORT, GPIO_STATUS_LED_PIN);
    g_ledOn = true;
}

void GPIOIO_LedOff(void)
{
    DL_GPIO_clearPins(GPIO_STATUS_PORT, GPIO_STATUS_LED_PIN);
    g_ledOn = false;
}

void GPIOIO_LedToggle(void)
{
    if (g_ledOn) {
        GPIOIO_LedOff();
    } else {
        GPIOIO_LedOn();
    }
}

bool GPIOIO_LedIsOn(void)
{
    return g_ledOn;
}

bool GPIOIO_KeyIsPressed(void)
{
    return ((DL_GPIO_readPins(GPIO_UI_PORT, GPIO_UI_KEY_PIN) &
                GPIO_UI_KEY_PIN) == 0U);
}

void GPIOIO_BuzzerOn(void)
{
    DL_GPIO_setPins(GPIO_UI_PORT, GPIO_UI_BUZZER_PIN);
    g_buzzerOn = true;
}

void GPIOIO_BuzzerOff(void)
{
    DL_GPIO_clearPins(GPIO_UI_PORT, GPIO_UI_BUZZER_PIN);
    g_buzzerOn = false;
}

void GPIOIO_BuzzerBeep(uint32_t onMs)
{
    GPIOIO_BuzzerOn();
    Delay_Ms(onMs);
    GPIOIO_BuzzerOff();
}

bool GPIOIO_BuzzerIsOn(void)
{
    return g_buzzerOn;
}
