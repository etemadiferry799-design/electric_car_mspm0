# electric_car_mspm0

MSPM0G3507 电赛小车传感器调试工程。

## 当前内容

- `hui du/`：CCS 工程，已有 OLED、串口、ICM42688 软件 I2C 示例。
- `hui du/gray.h`、`hui du/gray.c`：亚博智能八路灰度模块通道选择、八路扫描、循迹误差计算代码。
- `docs/mspm0g3507_bringup.md`：从接线、SysConfig、灰度、ICM42688 到 PID 的调试步骤。
- `docs/student_tasks.md`：适合初学者逐项执行的上电、编译、烧录、串口、ICM42688、灰度和电机接入任务单。

## 推荐调试顺序

1. 在 CCS 中导入 `hui du` 工程。
2. 确认 UART0 PA28/PA31 能以 115200 波特率打印。
3. 配置 PA0、PA1、PA2 为灰度地址 GPIO 输出，PA27 为 ADC 输入。
4. 用 `Gray_ReadAll()` 打印 8 路灰度原始值。
5. 调通 ICM42688 的 `WHO_AM_I` 和六轴原始数据。
6. 最后把灰度误差和 `gyro_z` 接入电机 PID。

如果你要立刻开始操作，先看 `docs/student_tasks.md`；如果你要理解整体原理，再看 `docs/mspm0g3507_bringup.md`。
