# MSPM0G3507 DAC RLC Band-Pass Standalone

基于 TI MSPM0G3507 的独立 DAC 波形输出 Keil/uVision 工程。

工程将 MATLAB RLC band-pass 模型得到的 20 点波形表，通过 MSPM0G3507 的 DAC12 和 DMA 以 1 MSPS 循环输出。

## 主要参数

- 输入模型：50 kHz、35% 占空比矩形波
- RLC 模型：`R = 1 kOhm`、`C = 1 nF`、`L = 1 mH`
- RLC 中心频率：约 159.155 kHz
- DAC 更新率：1 MSPS
- 每个 50 kHz 周期：20 个 DAC 点
- DAC 输出引脚：PA15 / `DAC_OUT`
- DMA：DAC 使用 `DMA_CH1`
- 主循环：初始化后使用 `__WFI()` 休眠

当前 DAC 波形表：

```text
2658, 3982, 3346, 2349, 1812, 1757, 1900, 1580, 171, 709,
1689, 2256, 2348, 2206, 2063, 2004, 2007, 2026, 2042, 2071
```

## 工程入口

使用 Keil MDK-ARM 打开：

```text
MSPM0G3507_DAC_RLC_BANDPASS_STANDALONE/MDK-ARM/LP_MSPM0G3507_ADC_SPI_TIMER.uvprojx
```

SysConfig 文件：

```text
MSPM0G3507_DAC_RLC_BANDPASS_STANDALONE/adc_spi_timer.syscfg
```

核心波形实现：

```text
MSPM0G3507_DAC_RLC_BANDPASS_STANDALONE/application/dac_waveform.c
MSPM0G3507_DAC_RLC_BANDPASS_STANDALONE/application/dac_waveform.h
```

SysConfig 生成文件：

```text
MSPM0G3507_DAC_RLC_BANDPASS_STANDALONE/ti_msp_dl_config.c
MSPM0G3507_DAC_RLC_BANDPASS_STANDALONE/ti_msp_dl_config.h
```

修改外设、DMA、时钟或引脚时，应修改 `.syscfg`，再通过 SysConfig 重新生成配置文件。

## 接线

本工程不要求外接实际 RLC 带通电路：

1. 示波器探头接 `PA15/DAC_OUT`。
2. 示波器地夹接开发板 `GND`。
3. 示波器输入建议使用高阻档。

DAC 输出是接近 `0 V` 到 `VDDA` 的单极性信号，不是以 0 V 为中心的双极性信号。当前波形表已经包含 DAC 中点附近的偏置。

## 目录说明

- `application/dac_waveform.c`：DAC 波形表和 DMA 输出启动
- `Core/`：Keil 工程入口
- `MDK-ARM/`：Keil 工程、启动文件和 scatter 文件
- `adc_spi_timer.syscfg`：SysConfig 外设配置
- `DAC_FILTER_IMPLEMENTATION.md`：带通模型和实现说明
- `ti_msp_dl_config.c/h`：SysConfig 生成文件

## 注意事项

- MSPM0G3507 IO 为 3.3 V 逻辑，PA15 输出不要连接到超过器件允许范围的外部电路。
- DAC 输出幅度和中点会受到 VDDA、示波器负载和板级电路影响。
- 1 MSPS、每周期 20 点会限制输出波形的细节，这是当前离散化实现的固有结果。
- 工程已完成 SysConfig 静态检查；最终频率、幅度和波形仍需使用示波器验证。
