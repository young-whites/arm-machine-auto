/*
 * openclaw_main.h - 7 阶段状态机控制接口
 */

#ifndef APPLICATIONS_OPENCLAW_MAIN_H_
#define APPLICATIONS_OPENCLAW_MAIN_H_

#include <stdint.h>

/* ========================================================================
 *  阶段枚举
 * ======================================================================== */
typedef enum {
    STAGE_STARTUP = 0,          /* 阶段1: 启动 */
    STAGE_PICK_BOTTLE,          /* 阶段2: 取药瓶 */
    STAGE_LOOSEN_CAP,           /* 阶段3: 拧松瓶盖 */
    STAGE_DISPENSE,             /* 阶段4: 取药 */
    STAGE_TIGHTEN_CAP,          /* 阶段5: 拧紧瓶盖 */
    STAGE_SHAKE_DISPENSE,       /* 阶段6: 摇匀出药 */
    STAGE_COMPLETE,             /* 阶段7: 总程序完成 */
    STAGE_MAX
} Stage_t;

/* ========================================================================
 *  函数接口
 * ======================================================================== */

/**
 * @brief  控制线程入口函数 (在 main.c 中创建线程时使用)
 * @param  parameter  未使用
 */
void openclaw_control_thread_entry(void *parameter);

/**
 * @brief  获取当前阶段名称字符串 (用于调试打印)
 * @param  stage  阶段枚举值
 * @return 阶段名称
 */
const char *stage_name(Stage_t stage);

/**
 * @brief  重置所有 AI/AO 寄存器 (调试命令 reset 使用)
 * @return 0=成功, -1=部分失败
 */
int openclaw_reset_all(void);

#endif /* APPLICATIONS_OPENCLAW_MAIN_H_ */
