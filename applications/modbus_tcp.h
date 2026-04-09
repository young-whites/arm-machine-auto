/*
 * modbus_tcp.h - Modbus TCP 通信封装接口
 * 通过串口发送/接收 Modbus TCP 帧 (经串口转网口转换器)
 */

#ifndef APPLICATIONS_MODBUS_TCP_H_
#define APPLICATIONS_MODBUS_TCP_H_

#include <stdint.h>

/* ========================================================================
 *  初始化与通信
 * ======================================================================== */

/**
 * @brief  初始化 Modbus 通信 (打开 USART2, 配置 9600 8N1)
 * @return 0=成功, -1=失败
 */
int modbus_init(void);

/**
 * @brief  反初始化 Modbus 通信 (关闭串口)
 */
void modbus_deinit(void);

/* ========================================================================
 *  FC06 - 写单个寄存器 (写 AI)
 * ======================================================================== */

/**
 * @brief  使用 FC06 写单个寄存器
 * @param  reg_addr  寄存器地址 (如 AI_ADDR_START=100)
 * @param  value     写入值
 * @return 0=成功, -1=发送失败, -2=响应异常
 */
int modbus_write_single_register(uint16_t reg_addr, uint16_t value);

/* ========================================================================
 *  FC03 - 读保持寄存器 (读 AO)
 * ======================================================================== */

/**
 * @brief  使用 FC03 读单个保持寄存器
 * @param  reg_addr  寄存器地址 (如 AO_ADDR_STEP=101)
 * @param  p_value   输出: 读取到的寄存器值
 * @return 0=成功, -1=发送失败, -2=响应异常/超时
 */
int modbus_read_holding_register(uint16_t reg_addr, uint16_t *p_value);

/* ========================================================================
 *  高级封装
 * ======================================================================== */

/**
 * @brief  写入寄存器后立即清零 (数据写入的标准操作)
 * @param  reg_addr  寄存器地址
 * @param  value     写入值
 * @return 0=成功, -1=失败
 */
int modbus_write_and_clear(uint16_t reg_addr, uint16_t value);

/**
 * @brief  轮询等待 AO 寄存器值等于目标值
 * @param  reg_addr      寄存器地址
 * @param  target_value  目标值
 * @param  timeout_ms    超时时间 (ms)
 * @param  poll_interval 轮询间隔 (ms)
 * @return 0=成功等到, -1=超时, -2=通信错误
 */
int modbus_poll_until_equal(uint16_t reg_addr, uint16_t target_value,
                            uint32_t timeout_ms, uint32_t poll_interval);

/* ========================================================================
 *  CRC16 工具 (供调试使用)
 * ======================================================================== */

/**
 * @brief  计算 CRC16 (Modbus 标准)
 * @param  data  数据指针
 * @param  len   数据长度
 * @return CRC16 值
 */
uint16_t modbus_crc16(const uint8_t *data, uint16_t len);

#endif /* APPLICATIONS_MODBUS_TCP_H_ */
