/*
 * modbus_tcp.c - Modbus TCP 通信实现
 * 通过 USART2 串口发送/接收 Modbus TCP 帧
 */

#include "modbus_tcp.h"
#include "config.h"
#include "debug_uart.h"

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>

/* ========================================================================
 *  内部变量
 * ======================================================================== */
static rt_device_t s_uart_dev = RT_NULL;        /* 串口设备句柄 */
static uint16_t s_trans_id = 0;                 /* 事务ID, 每次发送自增 */
static struct rt_semaphore s_rx_sem;            /* 接收信号量 */
static uint8_t s_rx_buf[MODBUS_RX_BUF_SIZE];    /* 接收缓冲区 */

/* ========================================================================
 *  CRC16 (Modbus 标准多项式 0xA001)
 * ======================================================================== */
uint16_t modbus_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint16_t i, j;

    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/* ========================================================================
 *  串口接收回调
 * ======================================================================== */
static rt_err_t uart_rx_indicate(rt_device_t dev, rt_size_t size)
{
    /* 收到数据后释放信号量, 唤醒等待线程 */
    if (size > 0) {
        rt_sem_release(&s_rx_sem);
    }
    return RT_EOK;
}

/* ========================================================================
 *  Modbus 初始化
 * ======================================================================== */
int modbus_init(void)
{
    /* 查找串口设备 */
    s_uart_dev = rt_device_find(MODBUS_UART_NAME);
    if (s_uart_dev == RT_NULL) {
        ERROR_PRINT("Modbus: cannot find %s\n", MODBUS_UART_NAME);
        return -1;
    }

    /* 配置串口参数: 9600, 8N1, 非阻塞接收 */
    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;
    cfg.baud_rate = MODBUS_UART_BAUD;
    cfg.data_bits = MODBUS_UART_DATA_BITS;
    cfg.stop_bits = MODBUS_UART_STOP_BITS;
    cfg.parity    = MODBUS_UART_PARITY;

    rt_device_control(s_uart_dev, RT_DEVICE_CTRL_CONFIG, &cfg);

    /* 创建接收信号量 */
    rt_sem_init(&s_rx_sem, "mbrx", 0, RT_IPC_FLAG_FIFO);

    /* 打开设备: 中断接收模式 */
    if (rt_device_open(s_uart_dev, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        ERROR_PRINT("Modbus: failed to open %s\n", MODBUS_UART_NAME);
        return -1;
    }

    /* 设置接收回调 */
    rt_device_set_rx_indicate(s_uart_dev, uart_rx_indicate);

    INFO_PRINT("Modbus: %s initialized (9600 8N1)\n", MODBUS_UART_NAME);
    return 0;
}

void modbus_deinit(void)
{
    if (s_uart_dev != RT_NULL) {
        rt_device_close(s_uart_dev);
        s_uart_dev = RT_NULL;
    }
    rt_sem_detach(&s_rx_sem);
}

/* ========================================================================
 *  事务ID 自增
 * ======================================================================== */
static uint16_t get_next_trans_id(void)
{
    uint16_t id = s_trans_id;
    s_trans_id++;
    return id;
}

/* ========================================================================
 *  内部函数: 清空接收缓冲
 * ======================================================================== */
static void flush_rx_buffer(void)
{
    uint8_t tmp;
    while (rt_device_read(s_uart_dev, 0, &tmp, 1) == 1) {
        /* 丢弃所有残留数据 */
    }
}

/* ========================================================================
 *  内部函数: 发送帧数据
 * ======================================================================== */
static int send_frame(const uint8_t *frame, uint16_t len)
{
    rt_size_t written;

    flush_rx_buffer();
    written = rt_device_write(s_uart_dev, 0, frame, len);

    FRAME_PRINT("TX", frame, len);

    if (written != len) {
        ERROR_PRINT("Modbus TX: sent %d/%d bytes\n", (int)written, len);
        return -1;
    }
    return 0;
}

/* ========================================================================
 *  内部函数: 等待并接收响应
 * ======================================================================== */
static int receive_response(uint8_t *buf, uint16_t buf_size,
                            uint16_t *p_recv_len, uint32_t timeout_ms)
{
    rt_tick_t start_tick = rt_tick_get();
    rt_tick_t timeout_tick = rt_tick_from_millisecond(timeout_ms);
    uint16_t total_received = 0;

    /* 等待接收信号量 (带超时) */
    if (rt_sem_take(&s_rx_sem, timeout_tick) != RT_EOK) {
        return -2; /* 超时 */
    }

    /* 读取所有可用数据 */
    while (total_received < buf_size) {
        rt_size_t n = rt_device_read(s_uart_dev, 0,
                                     buf + total_received,
                                     buf_size - total_received);
        if (n == 0) {
            /* 短暂等待更多数据 */
            rt_thread_mdelay(10);
            n = rt_device_read(s_uart_dev, 0,
                               buf + total_received,
                               buf_size - total_received);
            if (n == 0) break;
        }
        total_received += n;

        /* 检查是否超时 */
        if ((rt_tick_get() - start_tick) > timeout_tick) {
            break;
        }
    }

    *p_recv_len = total_received;
    FRAME_PRINT("RX", buf, total_received);

    return (total_received > 0) ? 0 : -2;
}

/* ========================================================================
 *  FC06 - 写单个寄存器
 *  帧格式: 事务ID(2B) 00 00 00 06 01 06 寄存器地址(2B) 值(2B)
 *  总长度: 12 字节
 * ======================================================================== */
int modbus_write_single_register(uint16_t reg_addr, uint16_t value)
{
    uint8_t frame[FC06_FRAME_LEN];
    uint16_t tid = get_next_trans_id();
    int ret;

    /* 组装 Modbus TCP 帧 */
    frame[0]  = (uint8_t)(tid >> 8);            /* 事务ID 高字节 */
    frame[1]  = (uint8_t)(tid & 0xFF);          /* 事务ID 低字节 */
    frame[2]  = 0x00;                           /* 协议ID 高字节 */
    frame[3]  = 0x00;                           /* 协议ID 低字节 */
    frame[4]  = 0x00;                           /* 长度 高字节 */
    frame[5]  = 0x06;                           /* 长度 低字节 (6字节后续) */
    frame[6]  = MODBUS_UNIT_ID;                 /* 单元ID */
    frame[7]  = MODBUS_FC_WRITE_SINGLE;         /* 功能码 0x06 */
    frame[8]  = (uint8_t)(reg_addr >> 8);       /* 寄存器地址 高字节 */
    frame[9]  = (uint8_t)(reg_addr & 0xFF);     /* 寄存器地址 低字节 */
    frame[10] = (uint8_t)(value >> 8);          /* 值 高字节 */
    frame[11] = (uint8_t)(value & 0xFF);        /* 值 低字节 */

    /* 发送 */
    ret = send_frame(frame, FC06_FRAME_LEN);
    if (ret != 0) return -1;

    if (g_debug_verbose) {
        DEBUG_PRINT("FC06 TID=%04X: write reg[%d] = %d\n", tid, reg_addr, value);
    }

    /* 在本系统中，机械臂不会返回响应，只需确保帧已发送成功。
       因此这里不等待任何回包，直接返回成功。 */
    rt_thread_mdelay(5); // 短延迟让 UART 完成发送
    return 0;
}

/* ========================================================================
 *  FC03 - 读保持寄存器
 *  请求帧: 事务ID(2B) 00 00 00 06 01 03 起始地址(2B) 数量(2B)
 *  响应帧: 事务ID(2B) 00 00 00 05 01 03 02 值高 值低 (读1个寄存器)
 * ======================================================================== */
int modbus_read_holding_register(uint16_t reg_addr, uint16_t *p_value)
{
    uint8_t frame[FC03_FRAME_LEN];
    uint16_t tid = get_next_trans_id();
    int ret;

    /* 组装 FC03 请求帧 */
    frame[0]  = (uint8_t)(tid >> 8);            /* 事务ID 高字节 */
    frame[1]  = (uint8_t)(tid & 0xFF);          /* 事务ID 低字节 */
    frame[2]  = 0x00;                           /* 协议ID */
    frame[3]  = 0x00;
    frame[4]  = 0x00;                           /* 长度 */
    frame[5]  = 0x06;
    frame[6]  = MODBUS_UNIT_ID;                 /* 单元ID */
    frame[7]  = MODBUS_FC_READ_HOLD;            /* 功能码 0x03 */
    frame[8]  = (uint8_t)(reg_addr >> 8);       /* 起始地址 高字节 */
    frame[9]  = (uint8_t)(reg_addr & 0xFF);     /* 起始地址 低字节 */
    frame[10] = 0x00;                           /* 数量 高字节 */
    frame[11] = 0x01;                           /* 数量 低字节 (读1个) */

    /* 发送 */
    ret = send_frame(frame, FC03_FRAME_LEN);
    if (ret != 0) return -1;

    /* 等待响应 (超时30秒, 适配机械臂运动耗时) */
    uint16_t recv_len = 0;
    ret = receive_response(s_rx_buf, sizeof(s_rx_buf), &recv_len, 30000);

    if (ret != 0) {
        ERROR_PRINT("FC03 TID=%04X: no response (timeout)\n", tid);
        return -2;
    }

    /* 解析响应: 期望至少 11 字节 (MBAP 7B + FC 1B + ByteCount 1B + Data 2B) */
    if (recv_len >= 11 && s_rx_buf[7] == MODBUS_FC_READ_HOLD) {
        uint8_t byte_count = s_rx_buf[8];
        if (byte_count >= 2) {
            *p_value = ((uint16_t)s_rx_buf[9] << 8) | s_rx_buf[10];

            if (g_debug_verbose) {
                DEBUG_PRINT("FC03 TID=%04X: reg[%d] = %d\n", tid, reg_addr, *p_value);
            }
            return 0;
        }
    }

    /* 检查异常响应 */
    if (recv_len >= 9 && (s_rx_buf[7] & 0x80)) {
        ERROR_PRINT("FC03 TID=%04X: exception code %d\n", tid, s_rx_buf[8]);
        flush_rx_buffer();
        return -2;
    }

    ERROR_PRINT("FC03 TID=%04X: unexpected response len=%d\n", tid, recv_len);
    flush_rx_buffer();
    return -2;
}

/* ========================================================================
 *  写入后立即清零
 * ======================================================================== */
int modbus_write_and_clear(uint16_t reg_addr, uint16_t value)
{
    int ret;

    /* 写入目标值 */
    ret = modbus_write_single_register(reg_addr, value);
    if (ret != 0) {
        ERROR_PRINT("write_and_clear: failed to write %d to reg[%d]\n", value, reg_addr);
        return ret;
    }

    /* 短暂延迟, 确保从站已读取该值 */
    rt_thread_mdelay(DATA_CLEAR_DELAY_MS);

    /* 清零 */
    ret = modbus_write_single_register(reg_addr, 0);
    if (ret != 0) {
        ERROR_PRINT("write_and_clear: failed to clear reg[%d]\n", reg_addr);
        return ret;
    }

    return 0;
}

/* ========================================================================
 *  轮询等待 AO 值等于目标值
 * ======================================================================== */
int modbus_poll_until_equal(uint16_t reg_addr, uint16_t target_value,
                            uint32_t timeout_ms, uint32_t poll_interval)
{
    rt_tick_t start_tick = rt_tick_get();
    rt_tick_t timeout_tick = rt_tick_from_millisecond(timeout_ms);
    uint16_t read_value;
    int ret;

    while (1) {
        /* 检查超时 */
        if ((rt_tick_get() - start_tick) > timeout_tick) {
            ERROR_PRINT("poll: timeout waiting reg[%d]==%d\n", reg_addr, target_value);
            return -1;
        }

        /* 检查停止请求 */
        if (g_stop_requested) {
            INFO_PRINT("poll: stop requested\n");
            return -1;
        }

        /* 读取寄存器 */
        ret = modbus_read_holding_register(reg_addr, &read_value);
        if (ret == 0 && read_value == target_value) {
            DEBUG_PRINT("poll: reg[%d] == %d (matched)\n", reg_addr, target_value);
            return 0;
        }
        if (ret == 0) {
            /* 读取成功但值不匹配，打印当前值 */
            INFO_PRINT("poll: reg[%d] = %d (target=%d)\n", reg_addr, read_value, target_value);
        } else {
            /* 通信失败 */
            INFO_PRINT("poll: read reg[%d] failed (ret=%d)\n", reg_addr, ret);
        }

        /* 轮询间隔 (统一一次延迟, 修复双重延迟 bug) */
        rt_thread_mdelay(poll_interval);
    }
}
