/*
 * openclaw_main.c - 7 阶段状态机控制核心
 * 实现完整的机械臂自动化控制流程
 *
 * 通信模型: 上位机主动轮询机械臂 AO 寄存器获取状态，
 *           机械臂不主动上传，上位机根据轮询结果决定下一步动作。
 *
 * 流程: 上位机 FC06 写 AI → 机械臂执行 → 机械臂 FC16 写 AO → 上位机 FC03 轮询检测
 */

#include "openclaw_main.h"
#include "modbus_tcp.h"
#include "config.h"
#include "debug_uart.h"

#include <rtthread.h>
#include <string.h>

/* ========================================================================
 *  全局变量定义
 * ======================================================================== */
int g_skip_medicine = 0;
int g_skip_shake = 0;
SystemMode_t g_system_mode = MODE_LOOP;
int g_debug_verbose = 0;
int g_running = 0;
volatile int g_stop_requested = 0;

int g_bottle_id = 1;
int g_dispense_count = 1;
int g_dispense_port = 1;

static uint32_t g_total_cycles = 0;

/* ========================================================================
 *  阶段名称
 * ======================================================================== */
const char *stage_name(Stage_t stage)
{
    static const char *names[] = {
        "Startup", "PickBottle", "LoosenCap",
        "Dispense", "TightenCap", "ShakeDispense", "Complete"
    };
    if (stage < STAGE_MAX) return names[stage];
    return "Unknown";
}

/* ========================================================================
 *  核心辅助: 轮询等待机械臂 AO1 信号（任意非零值即有效）
 *
 *  机械臂每到达一个关键位置或需要上位机操作时，会通过 FC16 写 AO1 为非零值
 *  然后立即清零。上位机轮询检测到非零值即表示"机械臂有信号"。
 *  信号含义由当前阶段上下文决定。
 *
 *  @return 检测到的 AO1 值 (>0), 或 -1=超时/失败
 * ======================================================================== */
static int wait_robot_signal(uint32_t timeout_ms, const char *desc)
{
    rt_tick_t start_tick = rt_tick_get();
    rt_tick_t timeout_tick = rt_tick_from_millisecond(timeout_ms);
    uint16_t val;
    int ret;

    while (1) {
        if ((rt_tick_get() - start_tick) > timeout_tick) {
            ERROR_PRINT("[%s] poll timeout (%dms)\n", desc, (int)timeout_ms);
            return -1;  /* 超时: 设备未发信号 */
        }
        if (g_stop_requested) return -1;

        ret = modbus_read_holding_register(AO_ADDR_STEP, &val);
        if (ret == 0 && val > 0) {
            DEBUG_PRINT("[%s] AO1=%d detected\n", desc, val);
            return (int)val;
        }
        if (ret == -1) {
            ERROR_PRINT("[%s] FC03 send failed\n", desc);
            return -2;
        }
        rt_thread_mdelay(MODBUS_POLL_INTERVAL_MS);
    }
}

/* ------------------------------------------------------------ */
/* 业务帮助函数：写寄存器后轮询目标 AO/线圈直至等于期望值 */
int write_then_poll(uint16_t write_addr, uint16_t write_val,
                          uint16_t poll_addr, uint16_t target_val,
                          uint32_t timeout_ms, uint32_t poll_interval)
{
    int ret = modbus_write_and_clear(write_addr, write_val);
    if (ret != 0) return ret;                     /* 写失败直接返回 */
    rt_thread_mdelay(DATA_CLEAR_DELAY_MS);        /* 给机械臂读取时间 */
    return modbus_poll_until_equal(poll_addr, target_val,
                                   timeout_ms, poll_interval);
}

/* ------------------------------------------------------------ */
/* 等待机械臂先写指定 AO 值，然后发送 ACK（写 AI1=1） */
int wait_robot_request_and_ack(uint16_t expected_ao, const char *stage)
{
    int ret = modbus_poll_until_equal(AO_ADDR_STEP, expected_ao,
                                      STEP_TIMEOUT_MS,
                                      MODBUS_POLL_INTERVAL_MS);
    if (ret != 0) {
        ERROR_PRINT("%s: robot did not send expected request AO=%d\n", stage, expected_ao);
        return -1;
    }
    INFO_PRINT("%s: robot request AO=%d detected\n", stage, expected_ao);
    /* 发送 ACK (AI1=1) 并清零 */
    ret = modbus_write_and_clear(AI_ADDR_SIGNAL, 1);
    if (ret != 0) {
        ERROR_PRINT("%s: failed to ACK robot\n", stage);
        return -1;
    }
    return 0;
}
/* ========================================================================
 *  辅助: 发送 AI 信号并清零
 *  上位机完成操作后，写 AI1=1 通知机械臂，短延迟后清零
 * ======================================================================== */
static int ack_robot(const char *desc)
{
    int ret;

    ret = modbus_write_and_clear(AI_ADDR_SIGNAL, 1);
    if (ret != 0) {
        ERROR_PRINT("[%s] ack failed\n", desc);
        return -1;
    }
    DEBUG_PRINT("[%s] AI1=1 ack sent\n", desc);
    return 0;
}

/* ========================================================================
 *  辅助: 轮询等待 AO0=2 (总程序完成)
 * ======================================================================== */
static int wait_ao0_complete(void)
{
    int retry;
    uint16_t val;

    for (retry = 0; retry < MAX_RETRY_COUNT; retry++) {
        int ret = modbus_poll_until_equal(AO_ADDR_COMPLETE, 2,
                                          STEP_TIMEOUT_MS * 3,
                                          MODBUS_POLL_INTERVAL_MS * 5);
        if (ret == 0) return 0;

        ERROR_PRINT("[Complete] timeout (attempt %d/%d)\n", retry + 1, MAX_RETRY_COUNT);
        if (g_stop_requested) return -1;
    }
    ERROR_PRINT("[Complete] FAILED after %d retries\n", MAX_RETRY_COUNT);
    return -1;
}

/* ========================================================================
 *  阶段1: 启动
 *  上位机写 AI0=1 → 机械臂 WaitAI(AI0>0) 检测到 → 开始执行
 *  上位机随后清零 AI0
 * ======================================================================== */
static int stage_startup(void)
{
    INFO_PRINT("=== Stage 1: Startup ===\n");
    return write_then_poll(AI_ADDR_START, 1,
                           AO_ADDR_STEP, 1,
                           STEP_TIMEOUT_MS,
                           MODBUS_POLL_INTERVAL_MS);
}

/* ========================================================================
 *  阶段2: 取药瓶
 *  上位机写 AI1=药瓶编号 → 机械臂 WaitAI(AI1>0) → 读取编号 → 执行取瓶
 *
 *  反瓶流程（机械臂需要上位机配合夹紧/换位）:
 *    机械臂写 AO1=1 → 上位机轮询检测 → 上位机执行夹紧 → 写 AI1=1 确认 → 清零
 *    机械臂写 AO1=1 → 上位机轮询检测 → 上位机执行换位 → 写 AI1=1 确认 → 清零
 *  （最多两次 AO1 信号交换）
 * ======================================================================== */
static int stage_pick_bottle(void)
{
    int ret;

    INFO_PRINT("=== Stage 2: PickBottle (bottle=%d) ===\n", g_bottle_id);

    /* 发送药瓶编号 */
    ret = modbus_write_and_clear(AI_ADDR_SIGNAL, (uint16_t)g_bottle_id);
    if (ret != 0) {
        ERROR_PRINT("PickBottle: failed to send bottle ID\n");
        return -1;
    }

    INFO_PRINT("PickBottle: bottle ID %d sent, waiting for robot...\n", g_bottle_id);

    /* 反瓶检测: 轮询 AO1，最多处理两次信号（夹紧+换位） */
    {
        int sub_step;
        int ao_val;

        for (sub_step = 0; sub_step < 2; sub_step++) {
            ao_val = wait_robot_signal(STEP_TIMEOUT_MS, "PickBottle");
            if (ao_val == -1) {
                /* 超时 = 没有反瓶信号，正瓶流程直接继续 */
                INFO_PRINT("PickBottle: no reverse-bottle signal (normal flow)\n");
                break;
            }
            if (ao_val == -2) {
                ERROR_PRINT("PickBottle: comm failure\n");
                return -1;
            }

            INFO_PRINT("PickBottle: AO1=%d detected (sub-step %d)\n", ao_val, sub_step + 1);

            /* TODO: 上位机执行夹紧/换位操作 */
            rt_thread_mdelay(500);

            /* 确认完成 */
            ret = ack_robot("PickBottle");
            if (ret != 0) return -1;

            INFO_PRINT("PickBottle: sub-step %d completed\n", sub_step + 1);
        }
    }

    INFO_PRINT("PickBottle: completed\n");
    return 0;
}

/* ========================================================================
 *  阶段3: 拧松瓶盖
 *  机械臂写 AO1=2（拧松请求）→ 上位机轮询检测 → 执行拧松 → 写 AI1=1 → 清零
 * ======================================================================== */
static int stage_loosen_cap(void)
{
    INFO_PRINT("=== Stage 3: LoosenCap ===\n");
    return wait_robot_request_and_ack(2, "LoosenCap");
}

/* ========================================================================
 *  阶段4: 取药
 *  1. 机械臂写 AO1=1（就绪信号）→ 上位机轮询检测
 *  2. 上位机写 AI3=数量, AI4=口位 → 机械臂读取
 *  3. 循环 N 次:
 *     机械臂到达口位 → 写 AO1=1 → 上位机轮询检测 → 执行取药 → 写 AI1=1 → 清零
 * ======================================================================== */
static int stage_dispense(void)
{
    int ret, i;
    int ao_val;

    INFO_PRINT("=== Stage 4: Dispense (count=%d, port=%d) ===\n",
               g_dispense_count, g_dispense_port);

    /* 等待机械臂就绪 */
    ao_val = wait_robot_signal(STEP_TIMEOUT_MS, "Dispense-Ready");
    if (ao_val < 0) return -1;

    INFO_PRINT("Dispense: robot ready, sending parameters\n");

    /* 写入取药数量 */
    ret = modbus_write_and_clear(AI_ADDR_COUNT, (uint16_t)g_dispense_count);
    if (ret != 0) return -1;

    /* 写入取药口位 */
    ret = modbus_write_and_clear(AI_ADDR_PORT, (uint16_t)g_dispense_port);
    if (ret != 0) return -1;

    INFO_PRINT("Dispense: parameters sent, starting loop\n");

    /* 循环取药 */
    for (i = 0; i < g_dispense_count; i++) {
        if (g_stop_requested) {
            INFO_PRINT("Dispense: stop requested at cycle %d/%d\n", i + 1, g_dispense_count);
            return -1;
        }

        INFO_PRINT("Dispense: waiting for robot at port (cycle %d/%d)\n", i + 1, g_dispense_count);

        /* 等待机械臂到达口位（AO1=1） */
        ao_val = wait_robot_signal(STEP_TIMEOUT_MS, "Dispense-Arrive");
        if (ao_val < 0) return -1;

        INFO_PRINT("Dispense: robot arrived, dispensing\n");

        /* TODO: 上位机取药控制逻辑 */
        rt_thread_mdelay(300);

        /* 确认取药完成 */
        ret = ack_robot("Dispense-Done");
        if (ret != 0) return -1;

        INFO_PRINT("Dispense: cycle %d/%d completed\n", i + 1, g_dispense_count);
    }

    INFO_PRINT("Dispense: all %d cycles completed\n", g_dispense_count);
    return 0;
}

/* ========================================================================
 *  阶段5: 拧紧瓶盖
 *  两次信号交换:
 *    机械臂写 AO1=1 → 上位机闭合夹爪并拧紧 → 写 AI1=1 → 清零
 *    机械臂写 AO1=1 → 上位机松开夹爪 → 写 AI1=1 → 清零
 * ======================================================================== */
static int stage_tighten_cap(void)
{
    INFO_PRINT("=== Stage 5: TightenCap ===\n");
    for (int i = 0; i < 2; i++) {
        if (g_stop_requested) return -1;
        if (wait_robot_request_and_ack(1, "TightenCap") != 0) return -1;
        if (i == 0) {
            INFO_PRINT("TightenCap: closing gripper and tightening (simulated)\n");
            rt_thread_mdelay(500);
        } else {
            INFO_PRINT("TightenCap: releasing gripper (simulated)\n");
            rt_thread_mdelay(300);
        }
    }
    INFO_PRINT("TightenCap: completed\n");
    return 0;
}

/* ========================================================================
 *  阶段6: 摇匀出药
 *  三次信号交换:
 *    AO1=1 → 上位机闭合夹爪 → AI1=1 确认
 *    AO1=1 → 上位机执行摇匀 → AI1=1 确认
 *    AO1=1 → 上位机松开夹爪 → AI1=1 确认
 * ======================================================================== */
static int stage_shake_dispense(void)
{
    INFO_PRINT("=== Stage 6: ShakeDispense ===\n");
    for (int step = 0; step < 3; step++) {
        if (g_stop_requested) return -1;
        if (wait_robot_request_and_ack(1, "ShakeDispense") != 0) return -1;
        if (step == 0) {
            INFO_PRINT("ShakeDispense: closing gripper to hold bottle (simulated)\n");
            rt_thread_mdelay(300);
        } else if (step == 1) {
            INFO_PRINT("ShakeDispense: executing shake action (simulated)\n");
            rt_thread_mdelay(1000);
        } else {
            INFO_PRINT("ShakeDispense: releasing gripper (simulated)\n");
            rt_thread_mdelay(300);
        }
    }
    INFO_PRINT("ShakeDispense: completed\n");
    return 0;
}

/* ========================================================================
 *  阶段7: 总程序完成
 *  轮询 AO0=2
 * ======================================================================== */
static int stage_complete(void)
{
    int ret;

    INFO_PRINT("=== Stage 7: Complete ===\n");

    ret = wait_ao0_complete();
    if (ret != 0) {
        ERROR_PRINT("Complete: failed to detect completion flag\n");
        return -1;
    }

    g_total_cycles++;
    INFO_PRINT("Complete: cycle #%d finished!\n", g_total_cycles);
    return 0;
}

/* ========================================================================
 *  单次完整流程执行
 * ======================================================================== */
static int run_single_cycle(void)
{
    int ret;
    uint32_t cycle_start_tick = rt_tick_get();

    INFO_PRINT(">>> Cycle start (bottle=%d, count=%d, port=%d) <<<\n",
               g_bottle_id, g_dispense_count, g_dispense_port);

    /* 阶段1: 启动 */
    if (g_stop_requested) return -1;
    ret = stage_startup();
    if (ret != 0) return ret;

    /* 阶段2: 取药瓶 */
    if (g_stop_requested) return -1;
    ret = stage_pick_bottle();
    if (ret != 0) return ret;

    /* 阶段3: 拧松瓶盖 */
    if (g_stop_requested) return -1;
    ret = stage_loosen_cap();
    if (ret != 0) return ret;

    /* 阶段4: 取药 (可跳过) */
    if (g_stop_requested) return -1;
    if (!g_skip_medicine) {
        ret = stage_dispense();
        if (ret != 0) return ret;
    } else {
        INFO_PRINT(">>> Dispense stage SKIPPED <<<\n");
    }

    /* 阶段5: 拧紧瓶盖 */
    if (g_stop_requested) return -1;
    ret = stage_tighten_cap();
    if (ret != 0) return ret;

    /* 阶段6: 摇匀出药 (可跳过) */
    if (g_stop_requested) return -1;
    if (!g_skip_shake) {
        ret = stage_shake_dispense();
        if (ret != 0) return ret;
    } else {
        INFO_PRINT(">>> ShakeDispense stage SKIPPED <<<\n");
    }

    /* 阶段7: 总程序完成 */
    if (g_stop_requested) return -1;
    ret = stage_complete();
    if (ret != 0) return ret;

    {
        uint32_t elapsed_ms = (rt_tick_get() - cycle_start_tick) * 1000 / RT_TICK_PER_SECOND;
        INFO_PRINT(">>> Cycle completed in %d.%03d seconds <<<\n",
                   (int)(elapsed_ms / 1000), (int)(elapsed_ms % 1000));
    }

    return 0;
}

/* ========================================================================
 *  重置所有寄存器
 * ======================================================================== */
int openclaw_reset_all(void)
{
    int ret = 0;

    INFO_PRINT("Resetting all registers...\n");

    if (modbus_write_single_register(AI_ADDR_START, 0) != 0) ret = -1;
    if (modbus_write_single_register(AI_ADDR_SIGNAL, 0) != 0) ret = -1;
    if (modbus_write_single_register(AI_ADDR_COUNT, 0) != 0) ret = -1;
    if (modbus_write_single_register(AI_ADDR_PORT, 0) != 0) ret = -1;

    if (ret == 0) {
        INFO_PRINT("All AI registers cleared\n");
    } else {
        ERROR_PRINT("Some register resets failed\n");
    }

    return ret;
}

/* ========================================================================
 *  控制线程主入口
 * ======================================================================== */
void openclaw_control_thread_entry(void *parameter)
{
    int ret;

    (void)parameter;

    /* 初始化 Modbus */
    ret = modbus_init();
    if (ret != 0) {
        ERROR_PRINT("Modbus init failed, control thread exiting\n");
        return;
    }

    INFO_PRINT("Control thread started. Mode: %s\n",
               (g_system_mode == MODE_LOOP) ? "LOOP" : "SINGLE");

    /* 主循环 */
    while (1) {
        /* 等待启动命令 */
        while (!g_running && !g_stop_requested) {
            rt_thread_mdelay(500);
        }

        if (g_stop_requested) {
            g_stop_requested = 0;
            g_running = 0;
            openclaw_reset_all();
            INFO_PRINT("System stopped\n");
            continue;
        }

        /* 执行单次流程 */
        g_stop_requested = 0;
        ret = run_single_cycle();

        if (ret != 0) {
            ERROR_PRINT("Cycle failed, stopping\n");
            g_running = 0;
            openclaw_reset_all();
            continue;
        }

        /* 单次模式: 完成后自动停止 */
        if (g_system_mode == MODE_SINGLE) {
            INFO_PRINT("Single mode: cycle completed, stopping\n");
            g_running = 0;
            continue;
        }

        /* 循环模式 */
        INFO_PRINT("Loop mode: starting next cycle in 1 second...\n");
        rt_thread_mdelay(1000);
    }
}
