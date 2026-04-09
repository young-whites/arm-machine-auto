/*
 * debug_uart.h - 串口1 调试交互接口
 */

#ifndef APPLICATIONS_DEBUG_UART_H_
#define APPLICATIONS_DEBUG_UART_H_

#include <stdint.h>

/* ========================================================================
 *  调试输出宏
 * ======================================================================== */

/**
 * @brief  调试打印 (仅在 g_debug_verbose=1 时输出详细报文)
 */
#define DEBUG_PRINT(fmt, ...)   do { \
    debug_printf("[DBG] " fmt, ##__VA_ARGS__); \
} while(0)

/**
 * @brief  信息打印 (始终输出)
 */
#define INFO_PRINT(fmt, ...)    do { \
    debug_printf("[INF] " fmt, ##__VA_ARGS__); \
} while(0)

/**
 * @brief  错误打印 (始终输出)
 */
#define ERROR_PRINT(fmt, ...)   do { \
    debug_printf("[ERR] " fmt, ##__VA_ARGS__); \
} while(0)

/**
 * @brief  报文打印 (仅详细模式)
 */
#define FRAME_PRINT(label, buf, len)  do { \
    if (g_debug_verbose) { \
        debug_printf("[FRM] %s (%d): ", label, len); \
        debug_print_hex(buf, len); \
    } \
} while(0)

/* ========================================================================
 *  函数接口
 * ======================================================================== */

/**
 * @brief  格式化打印 (输出到 msh 控制台)
 */
void debug_printf(const char *fmt, ...);

/**
 * @brief  以十六进制打印缓冲区内容
 */
void debug_print_hex(const uint8_t *buf, uint16_t len);

#endif /* APPLICATIONS_DEBUG_UART_H_ */
