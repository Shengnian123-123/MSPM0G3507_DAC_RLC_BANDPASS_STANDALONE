# MSPM0G3507 DAC RLC Band-Pass Output

This is a standalone Keil project for MSPM0G3507 DAC output.

Open:

`MDK-ARM/LP_MSPM0G3507_ADC_SPI_TIMER.uvprojx`

Runtime behavior:

- Initializes SysConfig peripherals.
- Starts DAC12 DMA output on PA15/DAC_OUT.
- Main loop sleeps with `__WFI()`.
- No ADC/UART/DDS/application task is executed by `main.c`.

Output waveform:

- Source model: 50 kHz rectangular wave, 35% duty.
- Filter model: MATLAB RLC band-pass, R = 1 kOhm, C = 1 nF, L = 1 mH.
- RLC center frequency: about 159.155 kHz.
- DAC update rate: 1 MSPS.
- Samples per 50 kHz period: 20.

DAC table:

```text
2658, 3982, 3346, 2349, 1812, 1757, 1900, 1580, 171, 709,
1689, 2256, 2348, 2206, 2063, 2004, 2007, 2026, 2042, 2071
```

Wiring:

- Oscilloscope probe: PA15/DAC_OUT.
- Oscilloscope ground: board GND.
- No external RLC/band-pass circuit is required.
