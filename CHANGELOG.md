# 修改日志

## 2026-04-11

### 17:40 — 阶段2和阶段3之间添加机械臂移动等待
- **文件**: `openclaw_main.c`
- **内容**: 
  - 阶段2完成后添加 5 秒延迟，等待机械臂移动到拧开瓶位置
  - 避免阶段3轮询过早，机械臂还没到达位置

### 17:33 — 阶段3恢复AO1=2并使用快速轮询
- **文件**: `openclaw_main.c`
- **内容**: 
  - 阶段3（拧松瓶盖）恢复等待 AO1=2（机械臂实际写入值）
  - 新增 `wait_robot_request_and_ack_fast()` 函数，使用 10ms 快速轮询
  - 阶段3使用快速轮询，适配 500ms 短窗口信号

### 17:23 — 阶段2添加机械臂动作等待延迟
- **文件**: `openclaw_main.c`
- **内容**: 
  - 发送瓶号后添加 3 秒延迟，等待机械臂启动取瓶动作
  - 避免轮询过早导致收到脏数据（unexpected response len=4）
  - 解决机械臂移动期间串口噪声干扰

### 17:16 — 修复阶段3信号值不匹配
- **文件**: `openclaw_main.c`
- **内容**: 
  - 阶段3（拧松瓶盖）期望值从 AO1=2 改为 AO1=1
  - 与机械臂Lua代码 `WriteAO(AO1=1)` 对应
  - 更新注释说明

### 16:30 — 正式业务代码 AO 寄存器读取统一使用 FC04
- **文件**: `openclaw_main.c`, `modbus_tcp.c`
- **内容**: 
  - 修改 `wait_robot_signal()` 函数，使用 `modbus_read_input_register` (FC04) 读取 AO1
  - 修改 `modbus_poll_until_equal()` 函数，使用 `modbus_read_input_register` (FC04)
  - 统一所有 AO 寄存器读取都使用 FC04
  - 更新错误日志，FC03 → FC04

### 16:19 — 规范 AO 寄存器读取使用 FC04
- **文件**: `debug_uart.c`
- **内容**: 
  - 修改 `cmd_read_ao1` 命令，使用 `modbus_read_input_register` (FC04) 读取 AO1
  - 统一 AO 和 AI 寄存器都使用 FC04 读取
  - 更新函数注释，明确使用 FC04

### 15:30 — 新增 MSH 命令 cmd_read_ai1 (验证写入)
- **文件**: `debug_uart.c`, `modbus_tcp.h`
- **内容**: 
  - 新增 `cmd_read_ai1` 命令，使用 FC04 读取 AI1 寄存器（地址 101）
  - 用于验证 AI1 写入是否成功
  - 在 modbus_tcp.h 中添加 `modbus_read_input_register()` 函数声明
  - 更新帮助信息，添加 cmd_read_ai1 命令说明

## 2026-04-10

### 18:42 — 恢复 AO 寄存器使用 FC03 读取
- **文件**: `debug_uart.c`, `config.h`
- **内容**: 
  - 将 `cmd_read_ao1` 改回使用 `modbus_read_holding_register` (FC03)
  - 更新 config.h 注释，明确 AO 是保持寄存器，支持 0x03/0x06/0x16
  - 用户反馈：AO 是模拟输出（保持寄存器），不是输入寄存器

### 18:40 — 修复 FC04 超时后缓冲区残留问题
- **文件**: `modbus_tcp.c`
- **内容**: 
  - 在 `modbus_read_input_register()` 超时错误处理中添加 `flush_rx_buffer()`
  - 避免残留数据影响下次读取
  - 解决第一次读取成功后后续读取失败的问题

### 18:35 — 添加 FC04 读输入寄存器函数
- **文件**: `modbus_tcp.c`, `debug_uart.c`, `config.h`
- **内容**: 
  - 新增 `modbus_read_input_register()` 函数，使用 FC04 读取输入寄存器
  - 修改 `cmd_read_ao1` 命令，使用 FC04 读取 AO1
  - 添加 `MODBUS_FC_READ_INPUT` 宏定义
  - AO 寄存器属于输入寄存器，应使用 FC04 读取

### 18:29 — 新增 MSH 命令 cmd_read_ao1
- **文件**: `debug_uart.c`
- **内容**: 
  - 新增 `cmd_read_ao1` 命令，用于读取 AO1 寄存器（地址 101）
  - 打印读取到的数值，便于调试机械臂状态
  - 更新帮助信息

### 18:25 — 减少 FC03 超时错误刷屏
- **文件**: `modbus_tcp.c`
- **内容**: 
  - 添加静态变量 `timeout_error_printed` 控制超时错误打印
  - 连续超时只打印第一次，成功读取后重置标志
  - 减少 "FC03 TID=XXXX: no response (timeout)" 刷屏

### 18:22 — 调整日志级别，减少通信错误刷屏
- **文件**: `modbus_tcp.c`
- **内容**: 
  - 将 "unexpected response len" 错误改为 DEBUG_PRINT
  - 将轮询通信失败日志改为 DEBUG_PRINT
  - 将读取值不匹配日志改为 DEBUG_PRINT
  - 保留阶段步骤、超时、成功读取AO寄存器等关键INFO_PRINT

### 18:10 — 调整轮询间隔为 50ms
- **文件**: `config.h`
- **内容**: 
  - `MODBUS_POLL_INTERVAL_MS`: 10ms → **50ms**
  - 平衡通信负载与响应速度，减少串口噪声干扰

### 18:07 — 增强 FC03 响应解析容错
- **文件**: `modbus_tcp.c`
- **内容**: 
  - 修改 `modbus_read_holding_register()` 函数，增加原始帧调试打印
  - 响应解析条件从 `recv_len >= 11` 放宽至 `recv_len >= 9`\  - 当字节数合理且接收长度足够时尝试解析数据
  - 畸形响应不清空缓冲区，让上层重试
  - 解决 "unexpected response len" 错误导致的通信失败

### 17:50 — 增强轮询容错，解决 ACK 未发送问题
- **文件**: `modbus_tcp.c`
- **内容**: 
  - 修改 `modbus_poll_until_equal()` 函数，增加错误计数器
  - 通信失败（ret != 0）时不再立即返回，而是继续重试直至超时
  - 每次通信失败记录错误次数，前3次及每10次打印日志
  - 解决因 FC03 超时/畸形响应导致 ACK 未发送的问题

### 16:42 — 增大串口接收缓冲区
- **文件**: `rtconfig.h`
- **内容**: 
  - `RT_SERIAL_RB_BUFSZ`: 64 → **256**
  - 解决 MSH 终端警告："no enough buffer for saving data"
  - 适配 Modbus 通信大量数据接收

### 16:24 — 新增 MSH 测试命令 cmd_write_ai1
- **文件**: `debug_uart.c`
- **内容**: 
  - 新增 `cmd_write_ai1 <value>` 命令，用于直接写入 AI1 寄存器（地址 101）
  - 便于调试 Modbus 通信，测试上位机写入功能
  - 更新帮助信息

### 12:08 — 调整 FC03 响应超时为 60s
- **文件**: `modbus_tcp.c`
- **内容**: 
  - FC03 响应超时: 100ms → **60s**（适配机械臂长耗时运动）
  - 保留轮询间隔 10ms（快速重试）
  - 平衡快速检测与长运动时间

### 12:06 — 快速轮询适配机械臂 500ms 清除
- **文件**: `config.h`, `modbus_tcp.c`
- **内容**: 
  - 轮询间隔: 100ms → **10ms**（加快检测频率）
  - FC03 响应超时: 30s → **100ms**（快速失败重试）
  - 适配机械臂 AO1=2 仅保持 500ms 的时序
  - **注意**：其他阶段可能因超时过短而失败，需观察

### 11:07 — 增加轮询调试信息
- **文件**: `modbus_tcp.c`
- **内容**: 
  - 在 `modbus_poll_until_equal` 中增加每次读取的调试打印
  - 读取成功但值不匹配时打印当前值
  - 通信失败时打印错误码
  - 便于排查机械臂是否写入 AO1=2 及通信状态

### 10:46 — 增大 FC03 响应超时
- **文件**: `modbus_tcp.c`
- **内容**: 
  - FC03 响应超时: 10s → 30s（解决机械臂运动期间 Modbus 从站无响应问题）
  - 适配机械臂执行 `Lin(拧开瓶盖到位)` 等长耗时运动时的通信等待

## 2026-04-09

### 17:22 — 引入 Master-Poll 架构
- **文件**: `modbus_tcp.c`, `openclaw_main.c`, `openclaw_main.h`
- **内容**: 
  - FC06 写寄存器不再等待响应（机械臂不回包），发送成功即返回
  - 新增 `write_then_poll()` — 写寄存器后轮询目标AO直至等于期望值
  - 新增 `wait_robot_request_and_ack()` — 等待机械臂先写AO然后ACK
  - 阶段1/3/5/6 使用新函数统一"写→轮询→确认"流程

### 17:42 — 修复静态声明冲突
- **文件**: `openclaw_main.h`, `openclaw_main.c`
- **内容**: 头文件中移除 `write_then_poll` / `wait_robot_request_and_ack` 的原型声明（内部函数不应暴露），.c 中保留非static实现

### 18:18 — 修复阶段1逻辑错误
- **文件**: `openclaw_main.c`
- **内容**: 
  - 阶段1（Startup）写完AI0=1后不再轮询AO1
  - 根据机械臂Lua代码(总程序.lua)，机械臂启动后直接进入阶段2等待AI1（瓶号），不会写AO1
  - 阶段1改为：写AI0=1 → 清零 → 直接返回成功

### 18:37 — 修复阶段5/6循环交互次数不匹配
- **文件**: `openclaw_main.c`
- **内容**:
  - **阶段5（拧紧瓶盖）**: 机械臂第2次只执行 `WaitAI(AI1>0)` 无 `WriteAO`，修复为第2次直接ACK不轮询AO1
  - **阶段6（摇匀出药）**: 机械臂只写了2次AO1但有3次WaitAI，修复为第3次直接ACK不轮询AO1

### 18:37 — 增大超时时间
- **文件**: `config.h`, `modbus_tcp.c`
- **内容**:
  - `STEP_TIMEOUT_MS`: 10s → 60s（适配机械臂Lin/PTP运动耗时）
  - FC03 响应超时: 2s → 10s（机械臂运动时响应较慢）

---

## 通信协议备忘

| 方向 | 功能码 | 寄存器 | 地址 | 说明 |
|------|--------|--------|------|------|
| 上位机→机械臂 | FC06 | AI0 | 100 | 启动信号 |
| 上位机→机械臂 | FC06 | AI1 | 101 | 药瓶编号 / 动作确认ACK |
| 上位机→机械臂 | FC06 | AI3 | 103 | 取药数量 |
| 上位机→机械臂 | FC06 | AI4 | 104 | 取药口位 |
| 上位机→机械臂 | FC04 | AI1 | 101 | 读取AI1验证写入 (新增) |
| 上位机→机械臂 | FC04 | AO0 | 100 | 读取总程序完成(2=完成) |
| 上位机→机械臂 | FC04 | AO1 | 101 | 读取步骤状态(1=就绪, 2=拧松) |

**机械臂Lua API对应关系**:
- `ModbusSlaveWaitAI(AIx,0,0,-1)` → 机械臂轮询AI直到值>0 → 上位机用FC06写AI
- `ModbusSlaveReadAI(AIx,1)` → 机械臂读取AI值 → 上位机写入的具体数值
- `ModbusSlaveWriteAO(AOx,1,{val})` → 机械臂写AO → 上位机用FC03轮询读取
