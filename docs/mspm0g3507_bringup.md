# MSPM0G3507 小车传感器调试步骤

本文面向地猛星 MSPM0G3507、ICM42688、亚博智能八路灰度寻线模块绿光款和 CCS 环境。

## 1. 接线核对

### 八路灰度模块

| 模块信号 | MSPM0G3507 引脚 | 用途 |
| --- | --- | --- |
| 3V3 | 3.3V | 传感器电源，避免 5V 信号进入 MSPM0 IO |
| GND | GND | 共地 |
| AD0 | PA0 | 八选一地址 bit0，GPIO 输出 |
| AD1 | PA1 | 八选一地址 bit1，GPIO 输出 |
| AD2 | PA2 | 八选一地址 bit2，GPIO 输出 |
| OUT | PA27 | 当前通道灰度输出，配置为 ADC 输入 |

灰度模块按 `AD2:AD1:AD0` 选择 0~7 路探头，切换地址后读取 `OUT/PA27` 的 ADC 值。

### ICM42688

| 模块信号 | MSPM0G3507 引脚 | 当前工程用途 |
| --- | --- | --- |
| 3V3 | 3.3V | IMU 电源 |
| GND | GND | 共地 |
| SCL | PA15 | 软件 I2C 时钟 |
| SDA | PA14 | 软件 I2C 数据 |
| CS | PA13 | 拉高选择 I2C 模式 |
| AD0 | PA16 | 拉低选择地址 0x68 |

当前工程已经包含 ICM42688 的软件 I2C 读写与六轴数据读取示例。

## 2. CCS/SysConfig 配置建议

1. 打开 `hui du/empty.syscfg`。
2. 保留 OLED 的 I2C1：PB2=SCL、PB3=SDA。
3. 保留调试串口 UART0：PA28=TX、PA31=RX，波特率 115200。
4. 保留 ICM42688 软件 I2C GPIO：PA13、PA14、PA15、PA16。
5. 新增灰度地址 GPIO：PA0、PA1、PA2，全部配置为输出，默认低电平。
6. 新增 ADC 输入：PA27，对应灰度 `OUT`。建议先使用单次采样模式，12 bit 分辨率。

如果 SysConfig 生成的宏名不同，可以在 `gray.c` 顶部把 `GRAY_AD0_PORT/GRAY_AD0_PIN`、`GRAY_AD1_PORT/GRAY_AD1_PIN`、`GRAY_AD2_PORT/GRAY_AD2_PIN` 改成实际宏名。

## 3. 灰度调试顺序

1. 只读 PA27 ADC，并通过串口打印原始值。
2. 手动遮挡灰度模块，确认黑线和白底的 ADC 值确实不同。
3. 调用 `Gray_ReadAll()` 循环读取 8 路值。
4. 根据实测黑白值计算阈值，例如 `(black + white) / 2`。
5. 调用 `Gray_CalcPositionError()` 得到循迹误差。

## 4. 推荐主循环

```c
uint16_t gray[GRAY_CHANNEL_COUNT];
Gray_ReadAll(gray);
int16_t error = Gray_CalcPositionError(gray, threshold, blackIsHigh);

turn = kp * error + kd * (error - lastError);
leftPwm = basePwm - turn;
rightPwm = basePwm + turn;
lastError = error;
```

如果小车修正方向相反，把 `turn` 的符号反过来，或交换左右轮加减关系。

## 5. ICM42688 调试顺序

1. 先读取 WHO_AM_I，正确值应为 `0x47`。
2. 再读取加速度和角速度原始值。
3. 车静止时采样 500~1000 次，计算陀螺仪零偏。
4. 循迹稳定后，再把 `gyro_z` 加入转向抑制：`turn = grayPid - kGyro * gyroZ`。

## 6. 常见问题

- ADC 值一直为 0：检查 PA27 是否配置为 ADC 输入，灰度模块是否使用 3.3V 供电，GND 是否共地。
- 8 路数据完全一样：检查 PA0/PA1/PA2 是否真的输出，地址线顺序是否与模块一致。
- ICM42688 读不到 ID：检查 CS 是否拉高、AD0 是否拉低、SCL/SDA 是否接反、是否有上拉。
- 串口乱码：检查波特率是否为 115200，TX/RX 是否交叉连接。
