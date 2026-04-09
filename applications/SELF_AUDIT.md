# STM32 机械臂自动化控制 - 自审报告

## 生成时间
2026-03-31

## 文件清单
| 文件 | 大小 | 用途 |
|------|------|------|
| config.h | 5.7KB | 全局配置宏定义 |
| modbus_tcp.h | 2.8KB | Modbus TCP 封装接口 |
| modbus_tcp.c | 12.6KB | Modbus TCP 通信实现 |
| openclaw_main.h | 1.5KB | 状态机控制接口 |
| openclaw_main.c | 17.5KB | 7阶段状态机核心 |
| debug_uart.h | 1.7KB | 调试串口接口 |
| debug_uart.c | 9.9KB | 调试串口交互实现 |
| main.c | 1.9KB | 系统入口 |

## 合规性检查

### ✅ 通信配置
- USART2: 9600, 8N1, Modbus 通信 ✓
- USART1: 115200, 8N1, 调试输出 ✓
- STM32 作为 Modbus Master ✓

### ✅ 寄存器地址
- AI0 (100): 启动信号 ✓
- AI1 (101): 药瓶编号/动作完成 ✓
- AI3 (103): 取药数量 ✓
- AI4 (104): 取药口位 ✓
- AO0 (100): 总程序完成标记 ✓
- AO1 (101): 步骤状态 ✓

### ✅ 7阶段流程完整性
1. 启动: 写 AI0=1 → 清0 ✓
2. 取药瓶: 写 AI1=瓶号 → 清0, 含反瓶检测 ✓
3. 拧松瓶盖: 轮询 AO1=2 → 写 AI1=1 → 清0 ✓
4. 取药: 写 AI3/AI4 → 循环N次到位信号 ✓
5. 拧紧瓶盖: 两次等待 AI1 ✓
6. 摇匀出药: 三次信号 + 等待 ✓
7. 总程序完成: 轮询 AO0=2 ✓

### ✅ 关键规则
- 所有数据写入后立即清零 (modbus_write_and_clear) ✓
- 超时时间可配置 (config.h: STEP_TIMEOUT_MS=10000) ✓
- 超时重试最多3次 (MAX_RETRY_COUNT=3) ✓
- Modbus轮询间隔100ms (≤200ms要求) ✓

### ✅ FC06帧格式
```
事务ID(2B) 00 00 00 06 01 06 寄存器地址(2B) 值(2B)
```
与参考文档完全一致 ✓

### ✅ FC03帧格式
```
事务ID(2B) 00 00 00 06 01 03 起始地址(2B) 数量(2B)
```
与参考文档完全一致 ✓

### ✅ 调试命令
- start / stop ✓
- bottle N ✓
- count N ✓
- port N ✓
- debug on/off ✓
- reset ✓
- 额外: help, mode, skip, status ✓

### ✅ 模块跳过开关
- g_skip_medicine: 跳过取药 ✓
- g_skip_shake: 跳过摇匀 ✓

### ✅ 跨模块引用
- 所有 extern 变量在 config.h 中声明 ✓
- 所有 extern 变量在 openclaw_main.c 中定义 ✓

## 已知限制
1. 上位机动作 (拧松/拧紧/取药/摇匀) 使用 `rt_thread_mdelay()` 模拟，实际部署需替换为真实控制逻辑
2. CRC16 函数已实现但 Modbus TCP 帧本身不附带CRC (TCP帧用长度字段)，函数供扩展使用
3. 反瓶流程检测使用单次读取而非带超时轮询，可能需要根据实际机械臂响应速度调整

## 建议
1. 首次部署时降低 STEP_TIMEOUT_MS 至 5000ms 以快速发现通信问题
2. 使用 debug on 模式观察完整报文交换
3. 先使用 mode single 单步验证每个阶段
