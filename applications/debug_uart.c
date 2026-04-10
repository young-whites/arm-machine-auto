/*
 * debug_uart.c - 调试命令集成到 RT-Thread msh
 * 所有命令通过 MSH_CMD_EXPORT 注册，输出通过 rt_kprintf
 */

#include "debug_uart.h"
#include "config.h"
#include "openclaw_main.h"
#include "modbus_tcp.h"

#include <rtthread.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ========================================================================
 *  格式化打印 (宏 INFO_PRINT / ERROR_PRINT / DEBUG_PRINT 底层调用)
 * ======================================================================== */
void debug_printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    int len;

    va_start(args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len > 0) {
        rt_kprintf("%s", buf);
    }
}

/* ========================================================================
 *  十六进制打印 (FRAME_PRINT 宏调用)
 * ======================================================================== */
void debug_print_hex(const uint8_t *buf, uint16_t len)
{
    uint16_t i;
    for (i = 0; i < len; i++) {
        rt_kprintf("%02X ", buf[i]);
    }
    rt_kprintf("\r\n");
}

/* ========================================================================
 *  msh 命令实现
 * ======================================================================== */

/* start - 启动系统 */
static void cmd_start(int argc, char **argv)
{
    (void)argc; (void)argv;
    if (g_running) {
        INFO_PRINT("System already running\n");
    } else {
        g_running = 1;
        g_stop_requested = 0;
        INFO_PRINT("System STARTED\n");
    }
}
MSH_CMD_EXPORT(cmd_start, Start system);

/* stop - 停止系统 */
static void cmd_stop(int argc, char **argv)
{
    (void)argc; (void)argv;
    if (!g_running) {
        INFO_PRINT("System already stopped\n");
    } else {
        g_stop_requested = 1;
        INFO_PRINT("Stop requested\n");
    }
}
MSH_CMD_EXPORT(cmd_stop, Stop system);

/* bottle N - 设置药瓶编号 */
static void cmd_bottle(int argc, char **argv)
{
    if (argc < 2) {
        ERROR_PRINT("Usage: cmd_bottle <1~8>\n");
        return;
    }
    int val = atoi(argv[1]);
    if (val >= BOTTLE_MIN && val <= BOTTLE_MAX) {
        g_bottle_id = val;
        INFO_PRINT("Bottle ID set to %d\n", val);
    } else {
        ERROR_PRINT("Invalid bottle ID (1~8)\n");
    }
}
MSH_CMD_EXPORT(cmd_bottle, Set bottle ID);

/* count N - 设置取药数量 */
static void cmd_count(int argc, char **argv)
{
    if (argc < 2) {
        ERROR_PRINT("Usage: cmd_count <1~20>\n");
        return;
    }
    int val = atoi(argv[1]);
    if (val >= COUNT_MIN && val <= COUNT_MAX) {
        g_dispense_count = val;
        INFO_PRINT("Dispense count set to %d\n", val);
    } else {
        ERROR_PRINT("Invalid count (1~20)\n");
    }
}
MSH_CMD_EXPORT(cmd_count, Set dispense count);

/* port N - 设置取药口位 */
static void cmd_port(int argc, char **argv)
{
    if (argc < 2) {
        ERROR_PRINT("Usage: cmd_port <1~20>\n");
        return;
    }
    int val = atoi(argv[1]);
    if (val >= PORT_MIN && val <= PORT_MAX) {
        g_dispense_port = val;
        INFO_PRINT("Dispense port set to %d\n", val);
    } else {
        ERROR_PRINT("Invalid port (1~20)\n");
    }
}
MSH_CMD_EXPORT(cmd_port, Set dispense port);

/* debug on/off - 切换详细输出 */
static void cmd_debug(int argc, char **argv)
{
    if (argc < 2) {
        INFO_PRINT("Debug: %s (usage: cmd_debug on/off)\n", g_debug_verbose ? "ON" : "OFF");
        return;
    }
    if (strcmp(argv[1], "on") == 0) {
        g_debug_verbose = 1;
        INFO_PRINT("Debug verbose ON\n");
    } else if (strcmp(argv[1], "off") == 0) {
        g_debug_verbose = 0;
        INFO_PRINT("Debug verbose OFF\n");
    } else {
        ERROR_PRINT("Usage: cmd_debug on/off\n");
    }
}
MSH_CMD_EXPORT(cmd_debug, Toggle verbose output);

/* reset - 重置所有寄存器 */
static void cmd_reset(int argc, char **argv)
{
    (void)argc; (void)argv;
    g_stop_requested = 1;
    rt_thread_mdelay(200);
    openclaw_reset_all();
    g_stop_requested = 0;
    INFO_PRINT("System RESET complete\n");
}
MSH_CMD_EXPORT(cmd_reset, Reset all registers);

/* help - 显示帮助 */
static void cmd_help(int argc, char **argv)
{
    (void)argc; (void)argv;
    INFO_PRINT("Available commands:\n");
    INFO_PRINT("  cmd_start        - Start system\n");
    INFO_PRINT("  cmd_stop         - Stop system\n");
    INFO_PRINT("  cmd_bottle N     - Set bottle ID (1~8)\n");
    INFO_PRINT("  cmd_count N      - Set dispense count (1~20)\n");
    INFO_PRINT("  cmd_port N       - Set dispense port (1~20)\n");
    INFO_PRINT("  cmd_debug on/off - Toggle verbose output\n");
    INFO_PRINT("  cmd_reset        - Reset all registers\n");
    INFO_PRINT("  cmd_mode loop/single - Set mode\n");
    INFO_PRINT("  cmd_skip_med on/off  - Skip dispense stage\n");
    INFO_PRINT("  cmd_skip_shake on/off - Skip shake stage\n");
    INFO_PRINT("  cmd_write_ai1 N  - Write AI1 register (test)\n");
    INFO_PRINT("  cmd_status       - Show system status\n");
}
MSH_CMD_EXPORT(cmd_help, Show available commands);

/* mode loop/single - 设置运行模式 */
static void cmd_mode(int argc, char **argv)
{
    if (argc < 2) {
        INFO_PRINT("Mode: %s (usage: cmd_mode loop/single)\n",
                   (g_system_mode == MODE_LOOP) ? "LOOP" : "SINGLE");
        return;
    }
    if (strcmp(argv[1], "loop") == 0) {
        g_system_mode = MODE_LOOP;
        INFO_PRINT("Mode set to LOOP\n");
    } else if (strcmp(argv[1], "single") == 0) {
        g_system_mode = MODE_SINGLE;
        INFO_PRINT("Mode set to SINGLE\n");
    } else {
        ERROR_PRINT("Usage: cmd_mode loop/single\n");
    }
}
MSH_CMD_EXPORT(cmd_mode, Set loop/single mode);

/* skip_med on/off - 跳过取药阶段 */
static void cmd_skip_med(int argc, char **argv)
{
    if (argc < 2) {
        INFO_PRINT("Skip Medicine: %s\n", g_skip_medicine ? "YES" : "NO");
        return;
    }
    if (strcmp(argv[1], "on") == 0) {
        g_skip_medicine = 1;
        INFO_PRINT("Dispense stage will be SKIPPED\n");
    } else if (strcmp(argv[1], "off") == 0) {
        g_skip_medicine = 0;
        INFO_PRINT("Dispense stage ENABLED\n");
    } else {
        ERROR_PRINT("Usage: cmd_skip_med on/off\n");
    }
}
MSH_CMD_EXPORT(cmd_skip_med, Skip dispense stage);

/* skip_shake on/off - 跳过摇匀阶段 */
static void cmd_skip_shake(int argc, char **argv)
{
    if (argc < 2) {
        INFO_PRINT("Skip Shake: %s\n", g_skip_shake ? "YES" : "NO");
        return;
    }
    if (strcmp(argv[1], "on") == 0) {
        g_skip_shake = 1;
        INFO_PRINT("Shake stage will be SKIPPED\n");
    } else if (strcmp(argv[1], "off") == 0) {
        g_skip_shake = 0;
        INFO_PRINT("Shake stage ENABLED\n");
    } else {
        ERROR_PRINT("Usage: cmd_skip_shake on/off\n");
    }
}
MSH_CMD_EXPORT(cmd_skip_shake, Skip shake stage);

/* write_ai1 N - 写 AI1 寄存器值 */
static void cmd_write_ai1(int argc, char **argv)
{
    if (argc < 2) {
        ERROR_PRINT("Usage: cmd_write_ai1 <value>\n");
        return;
    }
    int val = atoi(argv[1]);
    int ret = modbus_write_single_register(AI_ADDR_SIGNAL, (uint16_t)val);
    if (ret == 0) {
        INFO_PRINT("AI1 (reg 101) written to %d\n", val);
    } else {
        ERROR_PRINT("Failed to write AI1 (ret=%d)\n", ret);
    }
}
MSH_CMD_EXPORT(cmd_write_ai1, Write AI1 register);

/* status - 显示系统状态 */
static void cmd_status(int argc, char **argv)
{
    (void)argc; (void)argv;
    INFO_PRINT("=== System Status ===\n");
    INFO_PRINT("  Running: %s\n", g_running ? "YES" : "NO");
    INFO_PRINT("  Mode: %s\n", (g_system_mode == MODE_LOOP) ? "LOOP" : "SINGLE");
    INFO_PRINT("  Bottle: %d\n", g_bottle_id);
    INFO_PRINT("  Count: %d\n", g_dispense_count);
    INFO_PRINT("  Port: %d\n", g_dispense_port);
    INFO_PRINT("  Skip Medicine: %s\n", g_skip_medicine ? "YES" : "NO");
    INFO_PRINT("  Skip Shake: %s\n", g_skip_shake ? "YES" : "NO");
    INFO_PRINT("  Debug: %s\n", g_debug_verbose ? "ON" : "OFF");
    INFO_PRINT("====================\n");
}
MSH_CMD_EXPORT(cmd_status, Show system status);

/* ========================================================================
 *  初始化 (打印启动 banner, 通过 INIT_APP_EXPORT 自动调用)
 * ======================================================================== */
static int debug_uart_init(void)
{
    INFO_PRINT("\n");
    INFO_PRINT("========================================\n");
    INFO_PRINT("  STM32 Robotic Arm Controller v1.0\n");
    INFO_PRINT("  RT-Thread | STM32F103RCT6\n");
    INFO_PRINT("  Modbus TCP over USART2 (9600)\n");
    INFO_PRINT("  Debug via msh console (uart1)\n");
    INFO_PRINT("========================================\n");
    INFO_PRINT("Type 'cmd_help' for available commands\n\n");
    return 0;
}
INIT_APP_EXPORT(debug_uart_init);
