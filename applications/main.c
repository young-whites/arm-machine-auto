/*
 * main.c - 系统入口
 * 创建控制线程，调试命令通过 msh (MSH_CMD_EXPORT) 自动注册
 */

#include <rtthread.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#include "config.h"
#include "openclaw_main.h"

/* 线程控制块 */
static struct rt_thread control_thread;

/* 线程栈 */
static rt_uint8_t control_thread_stack[CONTROL_THREAD_STACK_SIZE];

int main(void)
{
    rt_err_t result;

    /* 创建控制线程 */
    result = rt_thread_init(&control_thread,
                            "ctrl",
                            openclaw_control_thread_entry,
                            RT_NULL,
                            control_thread_stack,
                            sizeof(control_thread_stack),
                            CONTROL_THREAD_PRIORITY,
                            CONTROL_THREAD_TICK);
    if (result != RT_EOK) {
        LOG_E("Failed to create control thread");
        return -1;
    }
    rt_thread_startup(&control_thread);

    LOG_D("Control thread started successfully");

    return RT_EOK;
}
