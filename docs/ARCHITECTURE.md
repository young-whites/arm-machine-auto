# STM32 机械臂自动化控制 — 架构文档

## 1. 系统概述

基于 **STM32F103RCT6** + **RT-Thread** 嵌入式操作系统，通过 **Modbus TCP** 协议控制机械臂完成药瓶取放、瓶盖拧松/拧紧、取药、摇匀出药等全流程自动化。

| 项目 | 说明 |
|------|------|
| MCU | STM32F103RCT6 (Cortex-M3, 72MHz) |
| OS | RT-Thread v5.1.0 |
| 通信协议 | Modbus TCP (经串口转网口转换器) |
| 调试接口 | USART1 (115200 8N1) → RT-Thread MSH 控制台 |
| 通信接口 | USART2 (9600 8N1) → Modbus TCP 帧 |

---

## 2. 系统架构

```
┌─────────────────────────────────────────────────────┐
│                  上位机 (STM32F103)                  │
│  RT-Thread OS · Modbus TCP Master · 7阶段状态机      │
│                                                     │
│  ┌──────────┐  ┌──────────────┐  ┌───────────────┐  │
│  │ main.c   │  │ openclaw_main│  │ modbus_tcp.c  │  │
│  │ 创建线程  │→ │ 状态机控制    │→ │ FC06写/FC03轮询│  │
│  └──────────┘  └──────────────┘  └───────────────┘  │
│       ↕              ↕                  ↕            │
│  ┌──────────┐  ┌──────────────┐  ┌───────────────┐  │
│  │debug_uart│  │  config.h    │  │   UART2       │  │
│  │ MSH命令  │  │ 参数/宏定义   │  │ 9600 8N1      │  │
│  └──────────┘  └──────────────┘  └───────────────┘  │
└─────────────────────────────────────────────────────┘
                        │ UART2 (9600 8N1)
                        ▼
              ┌──────────────────┐
              │   机械臂控制器    │
              │ Modbus TCP Slave │
              │                  │
              │ AI寄存器(100~104)│ ← 上位机写入指令
              │ AO寄存器(100~101)│ → 机械臂写入状态
              │                  │
              │ 不主动发送响应    │
              │ 等待上位机轮询    │
              └──────────────────┘
```

---

## 3. 文件结构

```
arm-machine-auto/
├── applications/              # 用户应用代码
│   ├── main.c                 # 系统入口，创建控制线程
│   ├── config.h               # 全局配置宏定义（串口、寄存器、超时、范围限制）
│   ├── openclaw_main.h        # 状态机控制接口（阶段枚举、函数声明）
│   ├── openclaw_main.c        # 7阶段状态机核心实现
│   ├── modbus_tcp.h           # Modbus TCP 封装接口
│   ├── modbus_tcp.c           # Modbus TCP 通信实现（FC06/FC03）
│   ├── debug_uart.h           # 调试串口接口
│   └── debug_uart.c           # MSH 调试命令实现
├── cubemx/                    # STM32CubeMX 生成的底层配置
├── drivers/                   # RT-Thread BSP 驱动
├── libraries/                 # STM32 HAL 库 + CMSIS
├── rt-thread/                 # RT-Thread 内核及组件
├── rtconfig.h                 # RT-Thread 编译配置
├── docs/                      # 项目文档
│   └── ARCHITECTURE.md        # 本文档
└── makefile.targets           # 编译目标配置
```

---

## 4. 通信模型：Master-Poll

本系统采用 **Master-Poll** 模式：上位机（STM32）作为 Modbus Master 主动发起读写，机械臂作为 Slave 不主动发送任何数据。

### 4.1 寄存器映射

| 方向 | 功能码 | 寄存器 | 地址 | 用途 |
|------|--------|--------|------|------|
| 上位机 → 机械臂 | FC06 | AI0 | 100 | 启动信号（1=启动） |
| 上位机 → 机械臂 | FC06 | AI1 | 101 | 药瓶编号 / 动作完成确认(ACK) |
| 上位机 → 机械臂 | FC06 | AI3 | 103 | 取药数量（1~20） |
| 上位机 → 机械臂 | FC06 | AI4 | 104 | 取药口位（1~20） |
| 机械臂 → 上位机 | FC03 | AO0 | 100 | 总程序完成标记（2=完成） |
| 机械臂 → 上位机 | FC03 | AO1 | 101 | 步骤状态（1=就绪, 2=拧松请求） |

### 4.2 通信流程

```
上位机                              机械臂
  │                                   │
  │── FC06 写 AIx=值 ────────────────→│  发送指令
  │                                   │  读取并执行动作
  │                                   │  写 AO1=状态
  │←── FC03 轮询 AO1 ────────────────│  主动查询
  │── FC03 读到 AO1=目标值 ──────────→│  确认完成
  │                                   │
  │── FC06 写 AI1=1 (ACK) ──────────→│  发送确认
  │── FC06 写 AI1=0 (清零) ──────────→│  清除信号
  ▼                                   ▼
```

### 4.3 Modbus TCP 帧格式

**FC06 — 写单个寄存器（12字节）**

```
事务ID(2B) | 协议ID(2B) | 长度(2B) | 单元ID(1B) | 功能码(1B) | 地址(2B) | 值(2B)
  0x0001   |   0x0000   |  0x0006  |    0x01    |    0x06    |  0x0064  | 0x0001
```

**FC03 — 读保持寄存器（请求12字节，响应≥11字节）**

```
请求帧:
事务ID(2B) | 协议ID(2B) | 长度(2B) | 单元ID(1B) | 功能码(1B) | 地址(2B) | 数量(2B)
  0x0001   |   0x0000   |  0x0006  |    0x01    |    0x03    |  0x0065  | 0x0001

响应帧:
事务ID(2B) | 协议ID(2B) | 长度(2B) | 单元ID(1B) | 功能码(1B) | 字节数(1B) | 数据(2B)
  0x0001   |   0x0000   |  0x0005  |    0x01    |    0x03    |    0x02    | 0x0001
```

---

## 5. 7 阶段状态机

### 5.1 流程总览

```
┌─────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐
│ Stage 1  │────→│ Stage 2  │────→│ Stage 3  │────→│ Stage 4  │
│ Startup  │     │PickBottle│     │LoosenCap │     │ Dispense │
│ 写AI0=1  │     │ 写AI1=瓶号│     │ 轮询AO=2 │     │ 写AI3/AI4│
│ 轮询AO=1 │     │ 轮询确认  │     │ ACK AI1=1│     │ 循环N次  │
└─────────┘     └──────────┘     └──────────┘     └──────────┘
                                                         │
                                                         ▼
┌─────────┐     ┌──────────┐     ┌──────────┐
│ Stage 7  │←───│ Stage 6  │←───│ Stage 5  │
│ Complete │     │ShakeDisp │     │TightenCap│
│ 轮询AO=2 │     │ 3次交互  │     │ 2次交互  │
│ 循环计数+1│     │ 夹→摇→松 │     │ 紧→松    │
└─────────┘     └──────────┘     └──────────┘
```

### 5.2 各阶段详细说明

#### Stage 1 — 启动 (Startup)

| 步骤 | 上位机操作 | 机械臂行为 |
|------|-----------|-----------|
| 1 | FC06 写 AI0=1 | WaitAI(AI0>0) 检测到 |
| 2 | FC06 写 AI0=0 (清零) | 读取 AI0 并开始执行 |
| 3 | FC03 轮询 AO1=1 | 写 AO1=1 表示就绪 |

```c
static int stage_startup(void)
{
    return write_then_poll(AI_ADDR_START, 1,
                           AO_ADDR_STEP, 1,
                           STEP_TIMEOUT_MS,
                           MODBUS_POLL_INTERVAL_MS);
}
```

#### Stage 2 — 取药瓶 (PickBottle)

| 步骤 | 上位机操作 | 机械臂行为 |
|------|-----------|-----------|
| 1 | FC06 写 AI1=瓶号 | WaitAI(AI1>0) 读取瓶号 |
| 2 | FC06 写 AI1=0 (清零) | 执行取瓶动作 |
| 3 | (可选) 轮询 AO1 → ACK | 反瓶检测（夹紧/换位） |

#### Stage 3 — 拧松瓶盖 (LoosenCap)

| 步骤 | 上位机操作 | 机械臂行为 |
|------|-----------|-----------|
| 1 | FC03 轮询 AO1=2 | 写 AO1=2 表示拧松请求 |
| 2 | FC06 写 AI1=1 (ACK) | 收到确认 |
| 3 | FC06 写 AI1=0 (清零) | 继续下一步 |

```c
static int stage_loosen_cap(void)
{
    return wait_robot_request_and_ack(2, "LoosenCap");
}
```

#### Stage 4 — 取药 (Dispense)

| 步骤 | 上位机操作 | 机械臂行为 |
|------|-----------|-----------|
| 1 | FC06 写 AI3=数量 | 读取取药数量 |
| 2 | FC06 写 AI4=口位 | 读取取药口位 |
| 3 | 循环 N 次：轮询 AO1=1 → ACK | 到位 → 等待确认 → 下一个口位 |

#### Stage 5 — 拧紧瓶盖 (TightenCap)

| 次数 | 上位机操作 | 机械臂行为 |
|------|-----------|-----------|
| 1 | 轮询 AO1=1 → ACK | 闭合夹爪并拧紧 |
| 2 | 轮询 AO1=1 → ACK | 松开夹爪 |

#### Stage 6 — 摇匀出药 (ShakeDispense)

| 次数 | 上位机操作 | 机械臂行为 |
|------|-----------|-----------|
| 1 | 轮询 AO1=1 → ACK | 闭合夹爪固定药瓶 |
| 2 | 轮询 AO1=1 → ACK | 执行摇匀动作 |
| 3 | 轮询 AO1=1 → ACK | 松开夹爪 |

#### Stage 7 — 总程序完成 (Complete)

| 步骤 | 上位机操作 | 机械臂行为 |
|------|-----------|-----------|
| 1 | FC03 轮询 AO0=2 | 写 AO0=2 表示全部完成 |
| 2 | 循环计数 g_total_cycles++ | — |

---

## 6. 核心函数说明

### 6.1 Modbus 通信层 (`modbus_tcp.c`)

| 函数 | 功能 |
|------|------|
| `modbus_init()` | 初始化 UART2，配置 9600 8N1，创建接收信号量 |
| `modbus_write_single_register(addr, val)` | 组装并发送 FC06 帧（不等待响应） |
| `modbus_read_holding_register(addr, &val)` | 组装 FC03 帧，发送后轮询等待响应 |
| `modbus_write_and_clear(addr, val)` | 写寄存器 → 延时(50ms) → 清零 |
| `modbus_poll_until_equal(addr, target, timeout, interval)` | 循环轮询寄存器直至等于目标值 |

### 6.2 业务帮助层 (`openclaw_main.c`)

| 函数 | 功能 |
|------|------|
| `write_then_poll(write_addr, write_val, poll_addr, target_val, timeout, interval)` | 写寄存器 → 延时 → 轮询目标值（通用封装） |
| `wait_robot_request_and_ack(expected_ao, stage)` | 轮询等待机械臂写 AO → 发送 ACK（通用封装） |
| `wait_robot_signal(timeout, desc)` | 轮询 AO1 任意非零值（用于不确定具体值的场景） |
| `ack_robot(desc)` | 发送 AI1=1 并清零（通用 ACK） |
| `run_single_cycle()` | 依次执行 7 个阶段 |

### 6.3 调试命令层 (`debug_uart.c`)

| 命令 | 功能 | 参数 |
|------|------|------|
| `cmd_start` | 启动系统 | 无 |
| `cmd_stop` | 停止系统 | 无 |
| `cmd_bottle` | 设置药瓶编号 | 1~8 |
| `cmd_count` | 设置取药数量 | 1~20 |
| `cmd_port` | 设置取药口位 | 1~20 |
| `cmd_mode` | 设置运行模式 | loop / single |
| `cmd_debug` | 开关详细报文 | on / off |
| `cmd_skip_med` | 跳过取药阶段 | on / off |
| `cmd_skip_shake` | 跳过摇匀阶段 | on / off |
| `cmd_reset` | 重置所有寄存器 | 无 |
| `cmd_status` | 显示系统状态 | 无 |
| `cmd_help` | 显示帮助信息 | 无 |

---

## 7. 全局配置参数 (`config.h`)

### 7.1 串口配置

| 宏定义 | 值 | 说明 |
|--------|-----|------|
| `MODBUS_UART_NAME` | "uart2" | Modbus 通信串口 |
| `MODBUS_UART_BAUD` | 9600 | 波特率 |
| `MODBUS_UART_DATA_BITS` | DATA_BITS_8 | 数据位 |
| `MODBUS_UART_STOP_BITS` | STOP_BITS_1 | 停止位 |
| `MODBUS_UART_PARITY` | PARITY_NONE | 校验位 |

### 7.2 超时与重试

| 宏定义 | 值 | 说明 |
|--------|-----|------|
| `STEP_TIMEOUT_MS` | 10000 | 单步等待超时时间 |
| `MAX_RETRY_COUNT` | 3 | 超时最大重试次数 |
| `MODBUS_POLL_INTERVAL_MS` | 100 | AO1 轮询间隔（≤200ms） |
| `DATA_CLEAR_DELAY_MS` | 50 | 写入后等待机械臂读取的延迟 |

### 7.3 参数范围限制

| 宏定义 | 最小值 | 最大值 | 说明 |
|--------|--------|--------|------|
| `BOTTLE_MIN/MAX` | 1 | 8 | 药瓶编号范围 |
| `COUNT_MIN/MAX` | 1 | 20 | 取药数量范围 |
| `PORT_MIN/MAX` | 1 | 20 | 取药口位范围 |

### 7.4 线程配置

| 宏定义 | 值 | 说明 |
|--------|-----|------|
| `CONTROL_THREAD_STACK_SIZE` | 2048 | 控制线程栈大小 |
| `CONTROL_THREAD_PRIORITY` | 10 | 控制线程优先级 |
| `CONTROL_THREAD_TICK` | 20 | 控制线程时间片 |

---

## 8. 全局变量

| 变量 | 类型 | 说明 |
|------|------|------|
| `g_running` | int | 1=系统运行中, 0=停止 |
| `g_stop_requested` | volatile int | 1=收到停止请求 |
| `g_system_mode` | SystemMode_t | MODE_LOOP / MODE_SINGLE |
| `g_bottle_id` | int | 当前药瓶编号 (1~8) |
| `g_dispense_count` | int | 取药数量 (1~20) |
| `g_dispense_port` | int | 取药口位 (1~20) |
| `g_skip_medicine` | int | 1=跳过取药阶段 |
| `g_skip_shake` | int | 1=跳过摇匀阶段 |
| `g_debug_verbose` | int | 1=详细报文打印 |
| `g_total_cycles` | uint32_t | 总完成循环计数 |

---

## 9. 运行模式

### 9.1 单次模式 (MODE_SINGLE)

```
cmd_start → 执行1个完整循环 → 自动停止
```

适用于：**调试验证**、**单步测试**。

### 9.2 循环模式 (MODE_LOOP)

```
cmd_start → 执行1个循环 → 等待1秒 → 下一个循环 → ...
```

适用于：**批量生产**、**连续运行**。

---

## 10. 异常处理

| 异常场景 | 处理方式 |
|----------|----------|
| FC06 发送失败 | 返回 -1，状态机停止，打印错误日志 |
| FC03 轮询超时 | 返回 -1（超时）或 -2（通信错误），状态机停止 |
| 机械臂未在超时时间内响应 | `STEP_TIMEOUT_MS` 到期后停止当前阶段 |
| 收到停止请求 (`g_stop_requested`) | 立即退出当前轮询/等待循环，重置寄存器 |
| 循环模式下单次失败 | 停止系统，重置所有寄存器 |

---

## 11. 使用流程

### 11.1 首次运行

```bash
# 1. 编译并烧录固件
make clean && make && make flash

# 2. 连接调试串口 (USART1, 115200)
# 3. 打开详细模式
msh > cmd_debug on

# 4. 设置参数
msh > cmd_mode single      # 单次模式（推荐首次使用）
msh > cmd_bottle 1         # 药瓶编号 1
msh > cmd_count 1          # 取药数量 1
msh > cmd_port 1           # 取药口位 1

# 5. 启动
msh > cmd_start

# 6. 观察日志输出，确认每个阶段正常完成
```

### 11.2 正常运行

```bash
msh > cmd_mode loop         # 循环模式
msh > cmd_bottle 3
msh > cmd_count 5
msh > cmd_port 10
msh > cmd_start             # 开始连续运行

# 随时查看状态
msh > cmd_status

# 随时停止
msh > cmd_stop
```

---

## 12. 扩展说明

### 12.1 添加新的调试命令

在 `debug_uart.c` 中添加：

```c
static void cmd_xxx(int argc, char **argv)
{
    // 命令逻辑
}
MSH_CMD_EXPORT(cmd_xxx, 描述字符串);
```

### 12.2 替换模拟延时为实际控制

将各阶段中的 `rt_thread_mdelay()` 替换为实际硬件控制：

```c
// 替换前（模拟）
rt_thread_mdelay(500);

// 替换后（实际控制）
set_gripper(true);          // 闭合夹爪
set_motor_speed(1000);      // 设置电机速度
wait_encoder_reached(500);  // 等待编码器到位
```

### 12.3 添加错误重试机制

在 `write_then_poll` 和 `wait_robot_request_and_ack` 中加入 `MAX_RETRY_COUNT` 循环：

```c
int write_then_poll_with_retry(uint16_t write_addr, uint16_t write_val,
                               uint16_t poll_addr, uint16_t target_val,
                               uint32_t timeout_ms, uint32_t poll_interval,
                               int max_retry)
{
    for (int i = 0; i < max_retry; i++) {
        if (write_then_poll(write_addr, write_val, poll_addr, target_val,
                            timeout_ms, poll_interval) == 0) {
            return 0;
        }
        ERROR_PRINT("Retry %d/%d\n", i + 1, max_retry);
    }
    return -1;
}
```

---

## 13. 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| v1.0 | 2026-03-31 | 初始版本，7阶段状态机 + Modbus TCP |
| v1.1 | 2026-04-09 | 修复 FC06 响应超时问题，引入 Master-Poll 架构，添加 write_then_poll / wait_robot_request_and_ack 业务帮助函数 |
