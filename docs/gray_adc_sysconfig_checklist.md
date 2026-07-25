# PA27 灰度 ADC 的 SysConfig 检查表

`GRAY_ADC` 用于读取八路灰度模块复用后的模拟输出。工程约定为
`ADC0 channel 0 / PA27`。

## 必须设置

| SysConfig 项目 | 设置值 |
|---|---|
| Name | `GRAY_ADC` |
| Selected Peripheral | `ADC0` |
| ADC Clock Source | `ULPCLK` |
| Sample Clock Divider | `Divide by 8` |
| Conversion Mode | `Single` |
| Conversion Starting Address | `0` |
| Enable Repeat Mode | 不勾选 |
| Sampling Mode | `Auto` |
| Trigger Source | `Software` |
| Conversion Data Format | `Binary unsigned` |
| Memory name | `GRAY_ADC_MEM` |
| Input Channel | `Channel 0` |
| Device Pin Name | `PA27` |
| Reference Voltage | `VDDA` |
| Sample Period Source | `Sampling Timer 0` |
| Conversion Resolution | `12-bits` |
| Enable FIFO Mode | 不勾选 |
| Power Down Mode | `Manual` |
| Desired Sample Time 0 | `0.025 ms`（25 us） |
| Enable Interrupts | `None` |
| DMA Trigger | 不勾选 |
| ADC12 Peripheral | `ADC0` |
| ADC12 Channel 0 Pin | `PA27/31`（界面组合名称，对应 PA27） |

## 生成代码检查

保存 `empty.syscfg` 后，生成的配置中应包含：

```c
DL_ADC12_configConversionMem(
    GRAY_ADC_INST,
    GRAY_ADC_ADCMEM_GRAY_ADC_MEM,
    DL_ADC12_INPUT_CHAN_0,
    DL_ADC12_REFERENCE_VOLTAGE_VDDA,
    DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0,
    DL_ADC12_AVERAGING_MODE_DISABLED,
    DL_ADC12_BURN_OUT_SOURCE_DISABLED,
    DL_ADC12_TRIGGER_MODE_AUTO_NEXT,
    DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
```

`PinMux` 中出现 `PA27/31` 是 SysConfig 对该模拟通道候选引脚的组合显示，
上方 `Device Pin Name` 必须明确显示 `PA27`。

## 已知电压测试

断开灰度模块的 `OUT` 后测试 ADC：

1. `PA27 -> GND` 时，12 位结果应接近 `0`。
2. `3.3V -> 4.7kΩ~10kΩ -> PA27` 时，结果应接近 `4095`。
3. 两个相同电阻分压得到约 `1.65V` 时，结果应接近 `2048`。

所有改线必须断电进行。如果 PA27 实测为 3.3V 而结果仍为 0，应先核对
开发板排针位置和 SysConfig 的 `Device Pin Name`，不要继续调整灰度阈值。
