/*
 * config.h - 全局配置宏定义
 * STM32 机械臂自动化控制 - 配置参数集中管理
 */

#ifndef APPLICATIONS_CONFIG_H_
#define APPLICATIONS_CONFIG_H_

/* ========================================================================
 *  串口配置
 * ======================================================================== */
#define MODBUS_UART_NAME        "uart2"         /* Modbus 通信串口 (USART2) */
#define MODBUS_UART_BAUD        BAUD_RATE_9600  /* 波特率 9600 */
#define MODBUS_UART_DATA_BITS   DATA_BITS_8
#define MODBUS_UART_STOP_BITS   STOP_BITS_1
#define MODBUS_UART_PARITY      PARITY_NONE    /* Modbus RTU 标准为 PARITY_EVEN, 8N1 需设备支持 */


/* ========================================================================
 *  Modbus TCP 参数
 * ======================================================================== */
#define MODBUS_UNIT_ID          0x01            /* 从站地址 (固定) */
#define MODBUS_FC_WRITE_SINGLE  0x06            /* FC06 写单个寄存器 */
#define MODBUS_FC_READ_HOLD     0x03            /* FC03 读保持寄存器 */
#define MODBUS_FC_READ_INPUT    0x04            /* FC04 读输入寄存器 */
#define MODBUS_PROTOCOL_ID      0x0000          /* 协议标识 (固定) */

/* ========================================================================
 *  AI 寄存器地址 (STM32 使用 FC06 写入, 机械臂读取)
 * ======================================================================== */
#define AI_ADDR_START           100             /* AI0 - 启动信号 (1=启动) */
#define AI_ADDR_SIGNAL          101             /* AI1 - 药瓶编号/动作完成信号 */
#define AI_ADDR_COUNT           103             /* AI3 - 取药数量 (1~20) */
#define AI_ADDR_PORT            104             /* AI4 - 取药口位 (1~20) */

/* ========================================================================
 *  AO 寄存器地址 (机械臂写入, STM32 使用 FC03 读取)
 * ======================================================================== */
#define AO_ADDR_COMPLETE        100             /* AO0 - 总程序完成标记 (2=完成) */
#define AO_ADDR_STEP            101             /* AO1 - 步骤状态 (1=就绪, 2=请求拧松) */

/* ========================================================================
 *  超时与重试配置
 * ======================================================================== */
#define STEP_TIMEOUT_MS         60000           /* 每个等待步骤超时时间 (ms), 适配机械臂运动耗时 */
#define MAX_RETRY_COUNT         3               /* 超时最大重试次数 */
#define MODBUS_POLL_INTERVAL_MS 50              /* AO1 轮询间隔 (ms), 平衡通信负载与响应速度 */
#define DATA_CLEAR_DELAY_MS     50              /* 数据写入后等待机械臂读取的延迟 (ms) */

/* ========================================================================
 *  取值范围限制
 * ======================================================================== */
#define BOTTLE_MIN              1               /* 药瓶编号最小值 */
#define BOTTLE_MAX              8               /* 药瓶编号最大值 */
#define COUNT_MIN               1               /* 取药数量最小值 */
#define COUNT_MAX               20              /* 取药数量最大值 */
#define PORT_MIN                1               /* 取药口位最小值 */
#define PORT_MAX                20              /* 取药口位最大值 */

/* ========================================================================
 *  模块跳过开关 (可通过调试命令动态修改)
 * ======================================================================== */
extern int g_skip_medicine;                     /* 1=跳过取药模块 */
extern int g_skip_shake;                        /* 1=跳过摇匀出药模块 */

/* ========================================================================
 *  全局运行参数
 * ======================================================================== */
typedef enum {
    MODE_LOOP = 0,                              /* 自动循环模式 (默认) */
    MODE_SINGLE = 1                             /* 单次执行模式 */
} SystemMode_t;

extern SystemMode_t g_system_mode;              /* 系统运行模式 */
extern int g_debug_verbose;                     /* 1=详细报文打印 */
extern int g_running;                           /* 1=系统运行中, 0=停止 */
extern volatile int g_stop_requested;           /* 1=收到停止请求 */

/* 调试命令可修改的参数 */
extern int g_bottle_id;                         /* 当前药瓶编号 (1~8) */
extern int g_dispense_count;                    /* 取药数量 (1~20) */
extern int g_dispense_port;                     /* 取药口位 (1~20) */

/* ========================================================================
 *  系统线程参数
 * ======================================================================== */
#define CONTROL_THREAD_STACK_SIZE   2048        /* 控制线程栈大小 */
#define CONTROL_THREAD_PRIORITY     10          /* 控制线程优先级 */
#define CONTROL_THREAD_TICK         20          /* 控制线程时间片 */


/* ========================================================================
 *  Modbus 帧长度定义
 * ======================================================================== */
#define MBAP_HEADER_LEN         6               /* 事务ID(2) + 协议ID(2) + 长度(2) */
#define MB_FRAME_HEADER_LEN     7               /* MBAP + 单元ID(1) */
#define FC06_FRAME_LEN          12              /* FC06 完整帧长度 */
#define FC03_FRAME_LEN          12              /* FC03 请求帧长度 */
#define FC03_RESP_MIN_LEN       11              /* FC03 响应最小长度 */
#define MODBUS_RX_BUF_SIZE      64              /* 接收缓冲区大小 */

#endif /* APPLICATIONS_CONFIG_H_ */
