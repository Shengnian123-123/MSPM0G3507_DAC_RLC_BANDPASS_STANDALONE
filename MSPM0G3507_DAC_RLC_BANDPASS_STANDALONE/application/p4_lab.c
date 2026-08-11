#include "p4_lab.h"
#include <stdint.h>
#include "adc.h"
#include "adc_fml.h"
#include "ad9833.h"
#include "ad9959.h"
#include "app_params.h"
#include "capture_input.h"
#include "dma_port.h"
#include "i2c_bus.h"
#include "oled_ssd1306.h"
#include "peripheral_status.h"
#include "pwm_out.h"
#include "spi.h"
#include "uart.h"

static bool StrEquals(const char *left, const char *right)
{
    while ((*left != '\0') && (*right != '\0')) {
        if (*left != *right) {
            return false;
        }
        left++;
        right++;
    }

    return ((*left == '\0') && (*right == '\0'));
}

static bool StrStartsWith(const char *text, const char *prefix)
{
    while (*prefix != '\0') {
        if (*text != *prefix) {
            return false;
        }
        text++;
        prefix++;
    }

    return true;
}

static bool ParseU32Token(const char **text, uint32_t *value)
{
    uint32_t result = 0U;
    bool hasDigit = false;
    const char *cursor;

    if ((text == 0) || (*text == 0) || (value == 0)) {
        return false;
    }

    cursor = *text;
    while (*cursor == ' ') {
        cursor++;
    }

    while ((*cursor >= '0') && (*cursor <= '9')) {
        uint32_t digit = (uint32_t) (*cursor - '0');

        if (result > ((0xFFFFFFFFUL - digit) / 10UL)) {
            return false;
        }
        result = (result * 10UL) + digit;
        hasDigit = true;
        cursor++;
    }

    *text = cursor;
    *value = result;
    return hasDigit;
}

static bool ParseTwoU32(const char *text, uint32_t *first, uint32_t *second)
{
    if (!ParseU32Token(&text, first) || !ParseU32Token(&text, second)) {
        return false;
    }

    while (*text == ' ') {
        text++;
    }

    return (*text == '\0');
}

static void WriteDriverLine(const char *name, const char *status,
    const char *note)
{
    Uart_WriteString("  ");
    Uart_WriteString(name);
    Uart_WriteString(": ");
    Uart_WriteString(status);
    Uart_WriteString(" - ");
    Uart_WriteString(note);
    Uart_WriteString("\r\n");
}

static void WriteStatusLine(const char *name, Periph_Status_t status)
{
    Uart_WriteString("OK ");
    Uart_WriteString(name);
    Uart_WriteString(" status=");
    Uart_WriteString(Periph_StatusText(status));
    Uart_WriteString("\r\n");
}

static void WriteP4Info(void)
{
    Uart_WriteString("OK p4 common peripheral driver matrix\r\n");
    WriteDriverLine("UART0", "READY", "PA10 TX, PA11 RX, 115200 8N1");
    WriteDriverLine("ADC0", "READY", "single input PA25 / ADC0_CH2");
    WriteDriverLine("SOFT_SPI", "READY", "PA8 SCLK, PA9 MOSI, PA12 MISO, PA13 AD9959_CS, PB5 ADS1220_CS");
    WriteDriverLine("GPIO", "READY", "LED PA0, KEY PB10, BUZZER PB11");
    WriteDriverLine("SOFT_PWM", "READY", "software PWM on LED PA0");
    WriteDriverLine("FLASH_PARAM", "READY", "last 1KB main Flash via AppParams");
    WriteDriverLine("AD9959", "READY", "external DDS control by software SPI");
    WriteDriverLine("AD9833_DUAL", "READY", "external dual DDS, CS1=PB8, CS2=PB9");
    WriteDriverLine("I2C", "NOT_CONFIGURED", "no SysConfig I2C instance/pins yet");
    WriteDriverLine("OLED_SSD1306", "NOT_CONFIGURED", "needs I2C instance first");
    WriteDriverLine("HW_PWM", "NOT_CONFIGURED", "no timer PWM output pin yet");
    WriteDriverLine("CAPTURE", "NOT_CONFIGURED", "no timer capture input pin yet");
    WriteDriverLine("ADC_DMA", "NOT_CONFIGURED", "no DMA channel/event route yet");
    WriteDriverLine("ADS1220", "READY", "24-bit ADC on shared SPI, CS=PB5");
    WriteDriverLine("DIGITAL_POT", "NOT_USED", "PB9 is reserved for AD9833 CS2 in dual-DDS mode");
}

static void WriteP4Test(void)
{
    uint16_t adcRaw = 0U;
    uint8_t tx = 0xA5U;
    uint8_t rx = 0U;
    uint32_t src[4] = {0x11223344UL, 0x55667788UL, 0xA5A55A5AUL,
        0x0000FFFFUL};
    uint32_t dst[4] = {0U, 0U, 0U, 0U};
    Periph_Status_t status;

    Uart_WriteString("OK p4test start\r\n");

    Uart_WriteString("OK uart tx=READY rx=INTERRUPT_BUFFERED\r\n");

    if (ADC_SampleBlocking(&adcRaw, 20000U)) {
        Uart_Printf("OK adc raw=%u mv=%u pin=PA25\r\n", adcRaw,
            ADC_Fml_RawToMillivolt(adcRaw));
    } else {
        Uart_WriteString("ERR adc TIMEOUT pin=PA25\r\n");
    }

    SoftSPI_Transfer(&tx, &rx, 1U);
    Uart_Printf("OK soft_spi tx=0x%02X rx=0x%02X loopback=%s\r\n", tx, rx,
        (tx == rx) ? "PASS" : "OPEN_OR_FAIL");

    status = DmaPort_MemCopy(dst, src, 4U);
    Uart_Printf("OK dma_mem status=%s verify=%s\r\n",
        Periph_StatusText(status),
        ((dst[0] == src[0]) && (dst[1] == src[1]) && (dst[2] == src[2]) &&
            (dst[3] == src[3])) ? "PASS" : "FAIL");

    Uart_Printf("OK flash_param status=READY page=0x0001FC00 cal_gain=%u\r\n",
        AppParams_Get()->adcGainPermille);
    Uart_Printf("OK ad9833 mclk_hz=%lu\r\n",
        (unsigned long) AD9833_GetMclkHz());
    Uart_WriteString("OK ad9959 status=READY command=dds test\r\n");

    WriteStatusLine("i2c", I2CBus_Init());
    WriteStatusLine("oled", OledSsd1306_Init(OLED_SSD1306_DEFAULT_ADDR));
    WriteStatusLine("hw_pwm", PwmOut_StartHardware(1000U, 500U));
    WriteStatusLine("capture", CaptureInput_Read(0));
    WriteStatusLine("adc_dma", DmaPort_AdcStart(0, 0U));

    Uart_WriteString("OK p4test done\r\n");
}

static void HandleI2cScan(void)
{
    uint8_t addresses[16];
    uint8_t foundCount = 0U;
    Periph_Status_t status;

    status = I2CBus_Scan(addresses, 16U, &foundCount);
    Uart_WriteString("OK i2c scan status=");
    Uart_WriteString(Periph_StatusText(status));
    Uart_WriteString(" found=");
    Uart_WriteU32(foundCount);
    Uart_WriteString("\r\n");

    if (status == PERIPH_STATUS_OK) {
        for (uint8_t i = 0U; i < foundCount; i++) {
            Uart_WriteString("  addr=0x");
            Uart_WriteHex8(addresses[i]);
            Uart_WriteString("\r\n");
        }
    }
}

static void HandleOledTest(void)
{
    Periph_Status_t status;

    status = OledSsd1306_Init(OLED_SSD1306_DEFAULT_ADDR);
    if (status == PERIPH_STATUS_OK) {
        status = OledSsd1306_Clear();
    }
    if (status == PERIPH_STATUS_OK) {
        status = OledSsd1306_WriteText(0U, 0U, "MSPM0G3507");
    }

    Uart_WriteString("OK oled test status=");
    Uart_WriteString(Periph_StatusText(status));
    Uart_WriteString(" addr=0x3C\r\n");
}

static void HandleSoftPwm(const char *args)
{
    uint32_t frequencyHz;
    uint32_t dutyPermille;
    Periph_Status_t status;

    if (!ParseTwoU32(args, &frequencyHz, &dutyPermille) ||
        (dutyPermille > 1000UL)) {
        Uart_WriteString("ERR soft pwm command: soft pwm 100 500\r\n");
        return;
    }

    status = PwmOut_StartSoftLed(frequencyHz, (uint16_t) dutyPermille, 1000U);
    Uart_Printf("OK soft_pwm status=%s freq_hz=%lu duty_permille=%lu pin=PA0\r\n",
        Periph_StatusText(status), (unsigned long) frequencyHz,
        (unsigned long) dutyPermille);
}

static void HandleHwPwm(const char *args)
{
    uint32_t frequencyHz;
    uint32_t dutyPermille;
    Periph_Status_t status;

    if (!ParseTwoU32(args, &frequencyHz, &dutyPermille) ||
        (dutyPermille > 1000UL)) {
        Uart_WriteString("ERR hw pwm command: hw pwm 1000 500\r\n");
        return;
    }

    status = PwmOut_StartHardware(frequencyHz, (uint16_t) dutyPermille);
    Uart_Printf("OK hw_pwm status=%s freq_hz=%lu duty_permille=%lu\r\n",
        Periph_StatusText(status), (unsigned long) frequencyHz,
        (unsigned long) dutyPermille);
}

static void HandleCapture(void)
{
    CaptureInput_Result_t result;
    Periph_Status_t status = CaptureInput_Read(&result);

    Uart_Printf("OK capture status=%s freq_hz=%lu duty_permille=%u phase_deg=%u\r\n",
        Periph_StatusText(status), (unsigned long) result.frequencyHz,
        result.dutyPermille, result.phaseDeg);
}

static void HandleDmaMem(void)
{
    uint32_t src[4] = {1UL, 2UL, 3UL, 4UL};
    uint32_t dst[4] = {0UL, 0UL, 0UL, 0UL};
    Periph_Status_t status = DmaPort_MemCopy(dst, src, 4U);

    Uart_Printf("OK dma mem status=%s data=%lu,%lu,%lu,%lu\r\n",
        Periph_StatusText(status), (unsigned long) dst[0],
        (unsigned long) dst[1], (unsigned long) dst[2],
        (unsigned long) dst[3]);
}

static void HandleDmaAdc(void)
{
    uint16_t buffer[16];
    Periph_Status_t status = DmaPort_AdcStart(buffer, 16U);

    Uart_WriteString("OK dma adc status=");
    Uart_WriteString(Periph_StatusText(status));
    Uart_WriteString(" count=16\r\n");
}

bool P4Lab_HandleCommand(const char *line)
{
    if (line == 0) {
        return false;
    }

    if (StrEquals(line, "p4")) {
        WriteP4Info();
        return true;
    }
    if (StrEquals(line, "p4test")) {
        WriteP4Test();
        return true;
    }
    if (StrEquals(line, "i2c scan")) {
        HandleI2cScan();
        return true;
    }
    if (StrEquals(line, "oled test")) {
        HandleOledTest();
        return true;
    }
    if (StrStartsWith(line, "soft pwm ")) {
        HandleSoftPwm(&line[9]);
        return true;
    }
    if (StrStartsWith(line, "hw pwm ")) {
        HandleHwPwm(&line[7]);
        return true;
    }
    if (StrEquals(line, "capture")) {
        HandleCapture();
        return true;
    }
    if (StrEquals(line, "dma mem")) {
        HandleDmaMem();
        return true;
    }
    if (StrEquals(line, "dma adc")) {
        HandleDmaAdc();
        return true;
    }

    return false;
}

void P4Lab_WriteHelp(void)
{
    Uart_WriteString("Board peripherals: p4 | p4test | soft pwm <hz> <0..1000> | dma mem; legacy placeholders: i2c scan/oled test/hw pwm/capture/dma adc\r\n");
}
