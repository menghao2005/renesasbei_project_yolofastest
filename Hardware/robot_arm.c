/*
 * robot_arm.c
 *
 *  Created on: 2026年7月
 *      Author: menghao2005
 *
 *  机械臂舵机控制。参考 STM32 的 Core/Src/robot_arm.c 的函数结构，
 *  底层用瑞萨 RA8P1 的 GPT 产生 50Hz(20ms) PWM 驱动舵机。
 *
 *  GPT 周期在 ra_gen 中固定为 0.02s（period_counts = 0x4C4B40 = 5,000,000），
 *  因此脉冲宽度(pulse_us) -> 比较计数值的换算为：
 *      compare_counts = period_counts * pulse_us / 20000
 *
 *  【非阻塞实现】RobotArm_MoveTo/MoveToTime 仅记录 start/target/steps 并立即返回；
 *  RobotArm_Update() 用 DWT 周期计数器测时，按 20ms 步长推进插值，不调用任何
 *  阻塞延时，因此不会卡住 AI 主循环。只需在主循环里周期性调用 RobotArm_Update()。
 */

#include "robot_arm.h"
#include "hal_data.h"
#include "r_gpt.h"
#include "ai_center_offset.h"
#include <math.h>
#include <stdbool.h>

#define ROBOT_ARM_SERVO_NUM           5U
#define ROBOT_ARM_PWM_PERIOD_US       20000U /* GPT 周期 = 20ms */
#define ROBOT_ARM_UPDATE_PERIOD_US    ((ROBOT_ARM_UPDATE_PERIOD_MS) * 1000U)

/* 每个舵机对应的 GPT 实例与输出引脚（与硬件接线对应，按需修改） */
#define ROBOT_ARM_UPPER_VERTICAL_US        1640
#define ROBOT_ARM_UPPER_SERVO_DEG          180
#define ROBOT_ARM_FOREARM_VERTICAL_US      1500
#define ROBOT_ARM_FOREARM_SERVO_DEG        270
#define ROBOT_ARM_WRIST_UP_US              1440
#define ROBOT_ARM_WRIST_SERVO_DEG          270
#define ROBOT_ARM_WRIST_DOWN_DEG           180
#define ROBOT_ARM_WRIST_FORWARD_DEG        90
#define ROBOT_ARM_WRIST_MANUAL_OFFSET_US   -100
#define ROBOT_ARM_SERVO_PULSE_RANGE_US     2000
#define ROBOT_ARM_TREE_Y_UPPER_PERCENT     50U
#define ROBOT_ARM_TREE_Y_FOREARM_PERCENT   (100U - ROBOT_ARM_TREE_Y_UPPER_PERCENT)


typedef struct
{
    gpt_instance_ctrl_t *p_ctrl;
    gpt_io_pin_t         pin;
} RobotArmServoChannel;

static const RobotArmServoChannel robot_arm_channels[ROBOT_ARM_SERVO_NUM] = {
    {&g_timer_servo_01_ctrl, GPT_IO_PIN_GTIOCA}, /* base      */
    {&g_timer_servo_01_ctrl, GPT_IO_PIN_GTIOCB}, /* upper_arm */
    {&g_timer_servo_23_ctrl, GPT_IO_PIN_GTIOCA}, /* forearm   */
    {&g_timer_servo_23_ctrl, GPT_IO_PIN_GTIOCB}, /* wrist     */
    {&g_timer_servo_4_ctrl,  GPT_IO_PIN_GTIOCA}, /* gripper   */
};

/* 当前各舵机脉冲宽度(us)，初始化为中位 */
static volatile uint16_t robot_arm_current_us[ROBOT_ARM_SERVO_NUM] = {
    1500U, 1640U, 2400U, 1740U, 900U
};

/* GPT 一个周期的计数值，Init 时通过 R_GPT_InfoGet 取得 */
static uint32_t robot_arm_period_counts = 0U;

/* 非阻塞插值状态 */
static volatile bool robot_arm_moving = false;
static volatile bool robot_arm_paused  = false;
static uint16_t robot_arm_start_us[ROBOT_ARM_SERVO_NUM];
static uint16_t robot_arm_target_us[ROBOT_ARM_SERVO_NUM];
static uint16_t robot_arm_steps        = 1U;
static uint16_t robot_arm_step         = 0U;
static uint32_t robot_arm_last_cycles  = 0U;
static volatile bool robot_arm_wrist_down_mode = false;
static bool     robot_arm_next_move_wrist_down = false;
static bool     robot_arm_wrist_smooth_once = false;   /* mode switch: wrist interpolates this move */
static bool     robot_arm_prev_wrist_down_mode = false;
/* 每步对应的 CPU 周期数（由 SystemCoreClock 推算） */
static uint32_t robot_arm_step_cycles  = 1U;

/*
 * 地面抓取状态机：
 * IDLE -> MOVE_TO_HOME 移到起始位 -> ALIGN 视觉对准并逐级下降 -> APPROACH 最终逼近
 * -> CLOSE 闭合夹爪 -> RETRACT 抬臂收回 -> RETURN 底座转回正前 -> RELEASE 松爪 -> IDLE
 */
typedef enum
{
    ROBOT_ARM_GRAB_IDLE = 0,
    ROBOT_ARM_GRAB_MOVE_TO_HOME,
    ROBOT_ARM_GRAB_ALIGN,
    ROBOT_ARM_GRAB_APPROACH,
    ROBOT_ARM_GRAB_CLOSE,
    ROBOT_ARM_GRAB_RETRACT,
    ROBOT_ARM_GRAB_RETURN,
    ROBOT_ARM_GRAB_RELEASE,
} robot_arm_grab_state_t;

/* 地面抓取状态机运行变量 */
static robot_arm_grab_state_t robot_arm_grab_state = ROBOT_ARM_GRAB_IDLE;
static uint16_t robot_arm_grab_home_base = 0U;
static uint16_t robot_arm_grab_home_upper = 0U;
static uint16_t robot_arm_grab_home_forearm = 0U;
static uint16_t robot_arm_grab_home_gripper = 0U;
static uint16_t robot_arm_grab_current_base = 0U;
static uint16_t robot_arm_grab_current_upper = 0U;
static uint16_t robot_arm_grab_current_forearm = 0U;
static uint16_t robot_arm_grab_close_gripper = 1350U;
static uint32_t robot_arm_grab_aligned_frames = 0U;
static bool     robot_arm_grab_active = false;

/*
 * 树上抓取状态机：
 * IDLE -> MOVE_TO_FIXED 移到当前采摘位固定位姿 -> ALIGN_AND_APPROACH 视觉对准+逐步逼近
 * -> CLOSE 闭爪 -> RETRACT 抬臂收回 -> RETURN 转回正前 -> RELEASE 松爪 -> IDLE
 */
typedef enum
{
    ROBOT_ARM_TREE_GRAB_IDLE = 0,
    ROBOT_ARM_TREE_GRAB_MOVE_TO_FIXED,
    ROBOT_ARM_TREE_GRAB_ALIGN_AND_APPROACH,
    ROBOT_ARM_TREE_GRAB_CLOSE,
    ROBOT_ARM_TREE_GRAB_RETRACT,
    ROBOT_ARM_TREE_GRAB_RETURN,
    ROBOT_ARM_TREE_GRAB_RELEASE,
} robot_arm_tree_grab_state_t;

/* 树上抓取默认配置（调用方未传配置时使用） */
static const robot_arm_tree_grab_config_t s_tree_grab_default_config =
{
    .fixed_pose   = {1500U, 1640U, 2400U, 1740U, 900U,  ROBOT_ARM_DEFAULT_MOVE_MS},
    .approach_pose = {1500U, 1500U, 2200U, 1600U, 900U, 1000U},
    .close_pose   = {1500U, 1500U, 2200U, 1600U, 1350U, 500U},
    .retract_pose = {1500U, 1640U, 2400U, 1740U, 1350U, 1000U},
    .return_pose  = {1500U, 1640U, 2400U, 1740U, 1350U, 1000U},
    .release_pose = {1500U, 1640U, 2400U, 1740U, 900U, 500U},
    .align_deadband_px = 18.0f,
    .align_stable_frames = 3U,
    .align_step_us = 5U,
    .align_move_ms = 120U,
    .grab_area_percent = 8.0f,
    .close_base_delta_us = 0,
    .close_upper_delta_us = 0,
    .close_forearm_delta_us = 0,
    .lower_grab_mode = false,
};

/* 树上抓取状态机运行变量 */
static robot_arm_tree_grab_state_t robot_arm_tree_grab_state = ROBOT_ARM_TREE_GRAB_IDLE;
static robot_arm_tree_grab_config_t robot_arm_tree_grab_config;
static robot_arm_pose_t robot_arm_tree_grab_base_pose;
static robot_arm_pose_t robot_arm_tree_grab_align_pose;
static robot_arm_pose_t robot_arm_tree_grab_current_pose;
static uint32_t robot_arm_tree_grab_aligned_frames = 0U;
static bool robot_arm_tree_grab_active = false;
/* 将脉冲宽度夹紧到舵机安全范围 [ROBOT_ARM_SERVO_MIN_US, ROBOT_ARM_SERVO_MAX_US] */
static uint16_t RobotArm_ClampPulse(uint16_t pulse_us)
{
    if (pulse_us < ROBOT_ARM_SERVO_MIN_US)
    {
        return ROBOT_ARM_SERVO_MIN_US;
    }
    if (pulse_us > ROBOT_ARM_SERVO_MAX_US)
    {
        return ROBOT_ARM_SERVO_MAX_US;
    }
    return pulse_us;
}

/* 32 位整数绝对值 */
static int32_t RobotArm_Abs32(int32_t value)
{
    return (value < 0) ? -value : value;
}

/* 四舍五入整除（负数同样就近取整），用于脉冲与角度间的换算 */
static int32_t RobotArm_DivRound(int32_t value, int32_t divisor)
{
    if (value >= 0)
    {
        return (value + divisor / 2) / divisor;
    }

    return (value - divisor / 2) / divisor;
}

/*
 * 「手腕下压」模式：由大臂/小臂脉冲推算手腕脉冲，使末端保持竖直向下。
 * 角度映射（单位 0.1°，2000us 脉冲行程对应舵机全程角度）：
 *   upper_angle   = (UPPER_VERTICAL_US - upper_arm) * 大臂全程角 / 2000us
 *   forearm_angle = (forearm - FOREARM_VERTICAL_US) * 小臂全程角 / 2000us
 *   wrist_angle   = WRIST_DOWN_DEG*10 - upper_angle - forearm_angle（反向补偿两关节偏转）
 * 再把手腕角度换算回脉冲：WRIST_UP_US + wrist_angle*2000us/手腕全程角 + 手动微调，
 * 结果夹紧到安全范围。p_*_angle_tenth 可选返回各关节角度(0.1°)，传 NULL 忽略。
 */
static uint16_t RobotArm_CalcWristDownDetail(uint16_t upper_arm,
                                             uint16_t forearm,
                                             int32_t * p_upper_angle_tenth,
                                             int32_t * p_forearm_angle_tenth,
                                             int32_t * p_wrist_angle_tenth)
{
    int32_t upper_angle_tenth;
    int32_t forearm_angle_tenth;
    int32_t wrist_angle_tenth;
    int32_t wrist_delta_us;
    int32_t wrist_us;

    upper_angle_tenth =
        RobotArm_DivRound(((int32_t) ROBOT_ARM_UPPER_VERTICAL_US - (int32_t) upper_arm) *
                          ROBOT_ARM_UPPER_SERVO_DEG * 10,
                          ROBOT_ARM_SERVO_PULSE_RANGE_US);
    forearm_angle_tenth =
        RobotArm_DivRound(((int32_t) forearm - (int32_t) ROBOT_ARM_FOREARM_VERTICAL_US) *
                          ROBOT_ARM_FOREARM_SERVO_DEG * 10,
                          ROBOT_ARM_SERVO_PULSE_RANGE_US);

    wrist_angle_tenth = (ROBOT_ARM_WRIST_DOWN_DEG * 10) - upper_angle_tenth - forearm_angle_tenth;
    wrist_delta_us = RobotArm_DivRound(wrist_angle_tenth * ROBOT_ARM_SERVO_PULSE_RANGE_US,
                                       ROBOT_ARM_WRIST_SERVO_DEG * 10);
    wrist_us = (int32_t) ROBOT_ARM_WRIST_UP_US + wrist_delta_us + ROBOT_ARM_WRIST_MANUAL_OFFSET_US;

    if (wrist_us < (int32_t) ROBOT_ARM_SERVO_MIN_US)
    {
        wrist_us = ROBOT_ARM_SERVO_MIN_US;
    }
    if (wrist_us > (int32_t) ROBOT_ARM_SERVO_MAX_US)
    {
        wrist_us = ROBOT_ARM_SERVO_MAX_US;
    }

    if (p_upper_angle_tenth != NULL)
    {
        *p_upper_angle_tenth = upper_angle_tenth;
    }
    if (p_forearm_angle_tenth != NULL)
    {
        *p_forearm_angle_tenth = forearm_angle_tenth;
    }
    if (p_wrist_angle_tenth != NULL)
    {
        *p_wrist_angle_tenth = wrist_angle_tenth;
    }

    return (uint16_t) wrist_us;
}


/*
 * 「手腕前伸」模式：角度映射与 RobotArm_CalcWristDownDetail 相同，
 * 仅目标角改为 WRIST_FORWARD_DEG(90°)，使末端保持水平前指。
 */
static uint16_t RobotArm_CalcWristForwardDetail(uint16_t upper_arm,
                                                uint16_t forearm,
                                                int32_t * p_upper_angle_tenth,
                                                int32_t * p_forearm_angle_tenth,
                                                int32_t * p_wrist_angle_tenth)
{
    int32_t upper_angle_tenth;
    int32_t forearm_angle_tenth;
    int32_t wrist_angle_tenth;
    int32_t wrist_delta_us;
    int32_t wrist_us;

    upper_angle_tenth =
        RobotArm_DivRound(((int32_t) ROBOT_ARM_UPPER_VERTICAL_US - (int32_t) upper_arm) *
                          ROBOT_ARM_UPPER_SERVO_DEG * 10,
                          ROBOT_ARM_SERVO_PULSE_RANGE_US);
    forearm_angle_tenth =
        RobotArm_DivRound(((int32_t) forearm - (int32_t) ROBOT_ARM_FOREARM_VERTICAL_US) *
                          ROBOT_ARM_FOREARM_SERVO_DEG * 10,
                          ROBOT_ARM_SERVO_PULSE_RANGE_US);

    wrist_angle_tenth = (ROBOT_ARM_WRIST_FORWARD_DEG * 10) - upper_angle_tenth - forearm_angle_tenth;
    wrist_delta_us = RobotArm_DivRound(wrist_angle_tenth * ROBOT_ARM_SERVO_PULSE_RANGE_US,
                                       ROBOT_ARM_WRIST_SERVO_DEG * 10);
    wrist_us = (int32_t) ROBOT_ARM_WRIST_UP_US + wrist_delta_us + ROBOT_ARM_WRIST_MANUAL_OFFSET_US;

    if (wrist_us < (int32_t) ROBOT_ARM_SERVO_MIN_US)
    {
        wrist_us = ROBOT_ARM_SERVO_MIN_US;
    }
    if (wrist_us > (int32_t) ROBOT_ARM_SERVO_MAX_US)
    {
        wrist_us = ROBOT_ARM_SERVO_MAX_US;
    }

    if (p_upper_angle_tenth != NULL)
    {
        *p_upper_angle_tenth = upper_angle_tenth;
    }
    if (p_forearm_angle_tenth != NULL)
    {
        *p_forearm_angle_tenth = forearm_angle_tenth;
    }
    if (p_wrist_angle_tenth != NULL)
    {
        *p_wrist_angle_tenth = wrist_angle_tenth;
    }

    return (uint16_t) wrist_us;
}
/* 读取 Cortex-M DWT 周期计数器（用于非阻塞计时） */
static uint32_t RobotArm_CycleCount(void)
{
    return DWT->CYCCNT;
}

/* 使能 DWT 周期计数器（DEMCR.TRCENA + CTRL.CYCCNTENA），供非阻塞计时使用 */
static void RobotArm_EnableCycleCounter(void)
{
    if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0U)
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    }
    DWT->CYCCNT = 0U;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

/* 将脉冲宽度(us)换算为 GPT 比较计数值并写入对应通道 */
static void RobotArm_WriteServo(uint8_t index, uint16_t pulse_us)
{
    uint32_t compare_counts;

    if (index >= ROBOT_ARM_SERVO_NUM)
    {
        return;
    }

    pulse_us = RobotArm_ClampPulse(pulse_us);

    if (robot_arm_period_counts != 0U)
    {
        compare_counts =
            (uint32_t)(((uint64_t) robot_arm_period_counts * pulse_us + (ROBOT_ARM_PWM_PERIOD_US / 2U)) /
                       ROBOT_ARM_PWM_PERIOD_US);
    }
    else
    {
        /* 兜底：按 20ms / 5,000,000 计数换算（compare = pulse_us * 250） */
        compare_counts = (uint32_t) pulse_us * 250U;
    }

    R_GPT_DutyCycleSet(robot_arm_channels[index].p_ctrl, compare_counts, robot_arm_channels[index].pin);
    robot_arm_current_us[index] = pulse_us;
}

/* 初始化：使能 DWT 计时，打开并启动 3 个 GPT，读取周期计数值，输出各舵机初始(中位)脉冲 */
void RobotArm_Init(void)
{
    timer_info_t info;

    RobotArm_EnableCycleCounter();

    /* 每步周期数 = (SystemCoreClock/1000) * UPDATE_PERIOD_MS */
    robot_arm_step_cycles = (SystemCoreClock / 1000U) * ROBOT_ARM_UPDATE_PERIOD_MS;
    if (robot_arm_step_cycles == 0U)
    {
        robot_arm_step_cycles = 1U;
    }

    /* 打开并启动三个 GPT 定时器 */
    R_GPT_Open(&g_timer_servo_01_ctrl, &g_timer_servo_01_cfg);
    R_GPT_Open(&g_timer_servo_23_ctrl, &g_timer_servo_23_cfg);
    R_GPT_Open(&g_timer_servo_4_ctrl,  &g_timer_servo_4_cfg);

    R_GPT_Start(&g_timer_servo_01_ctrl);
    R_GPT_Start(&g_timer_servo_23_ctrl);
    R_GPT_Start(&g_timer_servo_4_ctrl);

    /* 取得周期计数值，用于 us -> counts 换算 */
    if (R_GPT_InfoGet(&g_timer_servo_01_ctrl, &info) == FSP_SUCCESS)
    {
        robot_arm_period_counts = info.period_counts;
    }

    /* 输出初始位置（中位） */
    for (uint8_t i = 0U; i < ROBOT_ARM_SERVO_NUM; i++)
    {
        RobotArm_WriteServo(i, robot_arm_current_us[i]);
    }

    robot_arm_moving = false;
}

/* 以默认时长(ROBOT_ARM_DEFAULT_MOVE_MS)缓动到目标姿态 */
void RobotArm_MoveTo(uint16_t base,
                     uint16_t upper_arm,
                     uint16_t forearm,
                     uint16_t wrist,
                     uint16_t gripper)
{
    RobotArm_MoveToTime(base,
                        upper_arm,
                        forearm,
                        wrist,
                        gripper,
                        ROBOT_ARM_DEFAULT_MOVE_MS);
}

/* 简化封装：仅返回推算出的手腕脉冲宽度(us) */
uint16_t RobotArm_CalcWristDownUs(uint16_t upper_arm, uint16_t forearm)
{
    return RobotArm_CalcWristDownDetail(upper_arm, forearm, NULL, NULL, NULL);
}

/* 简化封装：仅返回推算出的手腕脉冲宽度(us) */
uint16_t RobotArm_CalcWristForwardUs(uint16_t upper_arm, uint16_t forearm)
{
    return RobotArm_CalcWristForwardDetail(upper_arm, forearm, NULL, NULL, NULL);
}

/* 同 RobotArm_MoveTo，但手腕脉冲由大臂/小臂自动推算（默认时长） */
void RobotArm_MoveToWristDown(uint16_t base,
                              uint16_t upper_arm,
                              uint16_t forearm,
                              uint16_t gripper)
{
    RobotArm_MoveToWristDownTime(base,
                                 upper_arm,
                                 forearm,
                                 gripper,
                                 ROBOT_ARM_DEFAULT_MOVE_MS);
}

/* 同 RobotArm_MoveToTime，但手腕脉冲不直接给定：按「下压」模式由大臂/小臂实时推算 */
void RobotArm_MoveToWristDownTime(uint16_t base,
                                  uint16_t upper_arm,
                                  uint16_t forearm,
                                  uint16_t gripper,
                                  uint16_t time_ms)
{
    uint16_t wrist;

    wrist = RobotArm_CalcWristDownDetail(upper_arm,
                                         forearm,
                                         NULL,
                                         NULL,
                                         NULL);

    robot_arm_next_move_wrist_down = true;
    RobotArm_MoveToTime(base,
                        upper_arm,
                        forearm,
                        wrist,
                        gripper,
                        time_ms);
}

/*
 * 非阻塞缓动入口：记录起点/目标/步数后立即返回，插值由 RobotArm_Update() 推进。
 * time_ms 不足一个刷新周期时直接写目标脉冲并结束（无需缓动）。
 * 手腕下压模式经 robot_arm_next_move_wrist_down 标志传入；模式发生切换的那次移动
 * (robot_arm_wrist_smooth_once)按调用者给定的手腕脉冲插值一次，避免手腕角度跳变。
 */
void RobotArm_MoveToTime(uint16_t base,
                         uint16_t upper_arm,
                         uint16_t forearm,
                         uint16_t wrist,
                         uint16_t gripper,
                         uint16_t time_ms)
{
    robot_arm_wrist_down_mode = robot_arm_next_move_wrist_down;
    robot_arm_next_move_wrist_down = false;

    /* mode switch: this move interpolates wrist from actual current to target (no jump) */
    robot_arm_wrist_smooth_once = (robot_arm_wrist_down_mode != robot_arm_prev_wrist_down_mode);
    robot_arm_prev_wrist_down_mode = robot_arm_wrist_down_mode;

    for (uint8_t i = 0U; i < ROBOT_ARM_SERVO_NUM; i++)
    {
        robot_arm_target_us[i] = RobotArm_ClampPulse(
            (i == 0U) ? base : (i == 1U) ? upper_arm : (i == 2U) ? forearm : (i == 3U) ? wrist : gripper);
        robot_arm_start_us[i] = robot_arm_current_us[i];
    }

    /* 时间极短或不需要缓动：直接到位，立即返回 */
    if (time_ms < ROBOT_ARM_UPDATE_PERIOD_MS)
    {
        for (uint8_t i = 0U; i < ROBOT_ARM_SERVO_NUM; i++)
        {
            RobotArm_WriteServo(i, robot_arm_target_us[i]);
        }
        robot_arm_moving = false;
        return;
    }

    robot_arm_steps       = (uint16_t) (time_ms / ROBOT_ARM_UPDATE_PERIOD_MS);
    if (robot_arm_steps == 0U)
    {
        robot_arm_steps = 1U;
    }
    robot_arm_step        = 0U;
    robot_arm_last_cycles = RobotArm_CycleCount();
    robot_arm_moving      = true;
}

/* 是否有缓动插值正在进行 */
bool RobotArm_IsMoving(void)
{
    return robot_arm_moving;
}

/* 浮点绝对值 */
static float RobotArm_AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

/* 当前脉冲 + 增量后夹紧到 [min_us, max_us]，用于视觉对准微调 */
static uint16_t RobotArm_AddClamped(uint16_t current_us,
                                    int16_t delta_us,
                                    uint16_t min_us,
                                    uint16_t max_us)
{
    int32_t next_us = (int32_t) current_us + (int32_t) delta_us;

    if (next_us < (int32_t) min_us)
    {
        next_us = (int32_t) min_us;
    }
    if (next_us > (int32_t) max_us)
    {
        next_us = (int32_t) max_us;
    }

    return (uint16_t) next_us;
}

/*
 * 启动地面抓取状态机（非阻塞）：先移动到参数给定的起始位姿（手腕自动按「下压」推算），
 * 之后由 RobotArm_GrabService() 依据视觉偏移驱动后续状态。
 */
void RobotArm_GrabStart(uint16_t base,
                        uint16_t upper_arm,
                        uint16_t forearm,
                        uint16_t gripper)
{
    if (robot_arm_grab_active || robot_arm_moving)
    {
        return;
    }

    robot_arm_grab_home_base = base;
    robot_arm_grab_home_upper = upper_arm;
    robot_arm_grab_home_forearm = forearm;
    robot_arm_grab_home_gripper = gripper;

    robot_arm_grab_current_base = base;
    robot_arm_grab_current_upper = upper_arm;
    robot_arm_grab_current_forearm = forearm;
    robot_arm_grab_close_gripper = 1350U;  /* Close gripper pulse, change this value to tune gripping force. */
    robot_arm_grab_aligned_frames = 0U;
    robot_arm_grab_state = ROBOT_ARM_GRAB_MOVE_TO_HOME;
    robot_arm_grab_active = true;

    /* Move to the user selected start pose: base/upper/forearm/gripper come from RobotArm_GrabStart(). */
    RobotArm_MoveToWristDownTime(base,
                                 upper_arm,
                                 forearm,
                                 gripper,
                                 ROBOT_ARM_DEFAULT_MOVE_MS);
}

/* 地面抓取状态机是否仍在运行 */
bool RobotArm_GrabIsBusy(void)
{
    return robot_arm_grab_active;
}

/*
 * 地面抓取状态机服务：须在主循环中周期性调用。
 * p_offset 为 AI 给出的目标相对画面中心的像素偏移（为空或无效时原地等待）；
 * 仅当上一段插值移动完成后才推进一个状态。
 */
void RobotArm_GrabService(const ai_center_offset_t * p_offset)
{
    bool x_aligned;
    bool y_aligned;
    uint16_t align_step_us;

    if (!robot_arm_grab_active)
    {
        return;
    }

    if (robot_arm_moving)
    {
        return;
    }

    switch (robot_arm_grab_state)
    {
        /* 起始位移动完成 -> 进入视觉对准 */
        case ROBOT_ARM_GRAB_MOVE_TO_HOME:
            robot_arm_grab_state = ROBOT_ARM_GRAB_ALIGN;
            robot_arm_grab_aligned_frames = 0U;
            return;

        /*
         * 视觉对准：dx/dy 进入 ±18px 死区并连续稳定 3 帧后下降一级(upper -= 50us)；
         * 降到 1000us 后做最终逼近(upper -= 200us, forearm -= 125us)转入闭爪阶段。
         * 未对准则按偏移方向以 align_step_us(近处 2us / 远处 5us)微调 base 与 upper/forearm。
         */
        case ROBOT_ARM_GRAB_ALIGN:
            if ((NULL == p_offset) || (!p_offset->valid))
            {
                robot_arm_grab_aligned_frames = 0U;
                return;
            }

            x_aligned = (RobotArm_AbsFloat(p_offset->dx) <= 18.0f);  /* X alignment deadband in pixels. */
            y_aligned = (RobotArm_AbsFloat(p_offset->dy) <= 18.0f);  /* Y alignment deadband in pixels. */
            align_step_us = (robot_arm_grab_current_upper <=
                             (1000U + 150U)) ?
                            2U :     /* Fine correction step near the target. */
                            5U;      /* Normal correction step before getting close. */

            if (x_aligned && y_aligned)
            {
                if (robot_arm_grab_aligned_frames < 3U)
                {
                    robot_arm_grab_aligned_frames++;
                }

                if ((robot_arm_grab_aligned_frames >= 3U) &&
                    (robot_arm_grab_current_upper > 1000U))
                {
                    robot_arm_grab_current_upper =
                        RobotArm_AddClamped(robot_arm_grab_current_upper,
                                            (int16_t) -50,
                                            ROBOT_ARM_SERVO_MIN_US,
                                            ROBOT_ARM_SERVO_MAX_US);
                    if (robot_arm_grab_current_upper < 1000U)
                    {
                        robot_arm_grab_current_upper = 1000U;
                    }

                    robot_arm_grab_aligned_frames = 0U;
                    /* Descend one step: upper arm decreases by 50us until it reaches 1000us. */
                    RobotArm_MoveToWristDownTime(robot_arm_grab_current_base,
                                                 robot_arm_grab_current_upper,
                                                 robot_arm_grab_current_forearm,
                                                 robot_arm_grab_home_gripper,
                                                 300U);
                    return;
                }

                if ((robot_arm_grab_aligned_frames >= 3U) &&
                    (robot_arm_grab_current_upper <= 1000U))
                {
                    robot_arm_grab_aligned_frames = 0U;
                    robot_arm_grab_state = ROBOT_ARM_GRAB_APPROACH;
                    robot_arm_grab_current_upper =
                        RobotArm_AddClamped(robot_arm_grab_current_upper,
                                            (int16_t) -200,
                                            ROBOT_ARM_SERVO_MIN_US,
                                            ROBOT_ARM_SERVO_MAX_US);
                    robot_arm_grab_current_forearm =
                        RobotArm_AddClamped(robot_arm_grab_current_forearm,
                                            (int16_t) -125,
                                            ROBOT_ARM_SERVO_MIN_US,
                                            ROBOT_ARM_SERVO_MAX_US);
                    /* Final approach: upper arm decreases by 200us, forearm decreases by 125us. */
                    RobotArm_MoveToWristDownTime(robot_arm_grab_current_base,
                                                 robot_arm_grab_current_upper,
                                                 robot_arm_grab_current_forearm,
                                                 robot_arm_grab_home_gripper,
                                                 1500U);
                    return;
                }

                return;
            }

            robot_arm_grab_aligned_frames = 0U;

            if (p_offset->dx > 18.0f)
            {
                robot_arm_grab_current_base = RobotArm_AddClamped(robot_arm_grab_current_base,
                                                                  (int16_t) (-(int32_t) align_step_us),
                                                                  ROBOT_ARM_SERVO_MIN_US,
                                                                  ROBOT_ARM_SERVO_MAX_US);
            }
            else if (p_offset->dx < -18.0f)
            {
                robot_arm_grab_current_base = RobotArm_AddClamped(robot_arm_grab_current_base,
                                                                  (int16_t) align_step_us,
                                                                  ROBOT_ARM_SERVO_MIN_US,
                                                                  ROBOT_ARM_SERVO_MAX_US);
            }

            if (p_offset->dy > 18.0f)
            {
                robot_arm_grab_current_upper = RobotArm_AddClamped(robot_arm_grab_current_upper,
                                                                   (int16_t) align_step_us,
                                                                   ROBOT_ARM_SERVO_MIN_US,
                                                                   ROBOT_ARM_SERVO_MAX_US);
                robot_arm_grab_current_forearm = RobotArm_AddClamped(robot_arm_grab_current_forearm,
                                                                     (int16_t) align_step_us,
                                                                     ROBOT_ARM_SERVO_MIN_US,
                                                                     ROBOT_ARM_SERVO_MAX_US);
            }
            else if (p_offset->dy < -18.0f)
            {
                robot_arm_grab_current_upper = RobotArm_AddClamped(robot_arm_grab_current_upper,
                                                                   (int16_t) (-(int32_t) align_step_us),
                                                                   ROBOT_ARM_SERVO_MIN_US,
                                                                   ROBOT_ARM_SERVO_MAX_US);
                robot_arm_grab_current_forearm = RobotArm_AddClamped(robot_arm_grab_current_forearm,
                                                                     (int16_t) (-(int32_t) align_step_us),
                                                                     ROBOT_ARM_SERVO_MIN_US,
                                                                     ROBOT_ARM_SERVO_MAX_US);
            }

            /* Visual correction: base, upper arm and forearm are adjusted by 5us or 2us each time. */
            RobotArm_MoveToWristDownTime(robot_arm_grab_current_base,
                                         robot_arm_grab_current_upper,
                                         robot_arm_grab_current_forearm,
                                         robot_arm_grab_home_gripper,
                                         120U);
            return;

        /* 最终逼近完成 -> 闭合夹爪 */
        case ROBOT_ARM_GRAB_APPROACH:
            robot_arm_grab_state = ROBOT_ARM_GRAB_CLOSE;
            /* Close gripper: gripper moves to 1350us after final approach. */
            RobotArm_MoveToWristDownTime(robot_arm_grab_current_base,
                                         robot_arm_grab_current_upper,
                                         robot_arm_grab_current_forearm,
                                         robot_arm_grab_close_gripper,
                                         500U);
            return;

        /* 夹爪已闭合 -> 抬臂收回 */
        case ROBOT_ARM_GRAB_CLOSE:
            robot_arm_grab_state = ROBOT_ARM_GRAB_RETRACT;
            /* Retract upward: base keeps the grasp side value, upper arm moves to 1640us, forearm moves to 2400us. */
            RobotArm_MoveToWristDownTime(robot_arm_grab_home_base,
                                         1640U,
                                         2400U,
                                         robot_arm_grab_close_gripper,
                                         1500U);
            return;

        /* 抬臂完成 -> 底座转回正前方 */
        case ROBOT_ARM_GRAB_RETRACT:
            robot_arm_grab_state = ROBOT_ARM_GRAB_RETURN;
            /* Return base to front: base moves to 1500us before releasing fruit. */
            RobotArm_MoveToWristDownTime(1500U,
                                         1640U,
                                         2400U,
                                         robot_arm_grab_close_gripper,
                                         1500U);
            return;

        /* 底座转正完成 -> 松爪释放 */
        case ROBOT_ARM_GRAB_RETURN:
            robot_arm_grab_state = ROBOT_ARM_GRAB_RELEASE;
            /* Release fruit at front: base stays at 1500us, gripper returns to the value passed into RobotArm_GrabStart(). */
            RobotArm_MoveToWristDownTime(1500U,
                                         1640U,
                                         2400U,
                                         robot_arm_grab_home_gripper,
                                         500U);
            return;

        /* 释放完成 -> 回到空闲 */
        case ROBOT_ARM_GRAB_RELEASE:
            robot_arm_grab_state = ROBOT_ARM_GRAB_IDLE;
            robot_arm_grab_active = false;
            robot_arm_grab_aligned_frames = 0U;
            return;

        case ROBOT_ARM_GRAB_IDLE:
        default:
            robot_arm_grab_active = false;
            return;
    }
}

/* move_ms 为 0 时回退到默认动作时长 */
static uint16_t RobotArm_MoveTimeOrDefault(uint16_t move_ms)
{
    return (move_ms == 0U) ? ROBOT_ARM_DEFAULT_MOVE_MS : move_ms;
}

/* 按位姿结构体(五个关节脉冲 + 时长)移动 */
static void RobotArm_MoveToPose(const robot_arm_pose_t * p_pose)
{
    if (NULL == p_pose)
    {
        return;
    }

    RobotArm_MoveToTime(p_pose->base,
                        p_pose->upper_arm,
                        p_pose->forearm,
                        p_pose->wrist,
                        p_pose->gripper,
                        RobotArm_MoveTimeOrDefault(p_pose->move_ms));
}



/* 复制位姿并把手腕替换为「水平前伸」自动推算值（树上抓取全程保持手腕水平） */
static robot_arm_pose_t RobotArm_TreeGrabPoseWithForwardWrist(const robot_arm_pose_t * p_pose)
{
    robot_arm_pose_t pose = *p_pose;
    pose.wrist = RobotArm_CalcWristForwardUs(pose.upper_arm, pose.forearm);
    return pose;
}

/* 以水平手腕方式移动到指定位姿 */
static void RobotArm_TreeGrabMovePose(const robot_arm_pose_t * p_pose)
{
    robot_arm_pose_t pose;

    if (NULL == p_pose)
    {
        return;
    }

    pose = RobotArm_TreeGrabPoseWithForwardWrist(p_pose);
    RobotArm_MoveToPose(&pose);
}

/* 以竖直下压手腕方式移动到指定位姿（收回/返回/释放阶段使用） */
static void RobotArm_TreeGrabMovePoseWithDownWrist(const robot_arm_pose_t * p_pose)
{
    uint16_t wrist;

    if (NULL == p_pose)
    {
        return;
    }

    wrist = RobotArm_CalcWristDownUs(p_pose->upper_arm, p_pose->forearm);
    RobotArm_MoveToTime(p_pose->base,
                        p_pose->upper_arm,
                        p_pose->forearm,
                        wrist,
                        p_pose->gripper,
                        RobotArm_MoveTimeOrDefault(p_pose->move_ms));
}

/* 当前脉冲 + 增量后夹紧到舵机安全范围 */
static uint16_t RobotArm_AddDeltaClamped(uint16_t current_us, int32_t delta_us)
{
    int32_t next_us = (int32_t) current_us + delta_us;

    if (next_us < (int32_t) ROBOT_ARM_SERVO_MIN_US)
    {
        next_us = (int32_t) ROBOT_ARM_SERVO_MIN_US;
    }
    if (next_us > (int32_t) ROBOT_ARM_SERVO_MAX_US)
    {
        next_us = (int32_t) ROBOT_ARM_SERVO_MAX_US;
    }

    return (uint16_t) next_us;
}



/* 以不超过 step_us 的步长向 target_us 逼近（一步内可达则直接到位） */
static uint16_t RobotArm_MoveValueToward(uint16_t current_us, uint16_t target_us, uint16_t step_us)
{
    if (current_us < target_us)
    {
        uint16_t delta = (uint16_t) (target_us - current_us);
        return (delta <= step_us) ? target_us : (uint16_t) (current_us + step_us);
    }

    if (current_us > target_us)
    {
        uint16_t delta = (uint16_t) (current_us - target_us);
        return (delta <= step_us) ? target_us : (uint16_t) (current_us - step_us);
    }

    return current_us;
}

static robot_arm_pose_t RobotArm_TreeGrabApplyCorrection(const robot_arm_pose_t * p_pose);

/*
 * 沿「fixed -> approach」方向推进一步：
 * 先以 step_us 步长逼近修正后的对准位 corrected_approach_us，
 * 抵达后沿同一方向继续越过该位（保持对果实的压近量）。
 */
static uint16_t RobotArm_TreeGrabStepPastApproach(uint16_t current_us,
                                                  uint16_t fixed_us,
                                                  uint16_t approach_us,
                                                  uint16_t corrected_approach_us,
                                                  uint16_t step_us)
{
    if (approach_us > fixed_us)
    {
        if (current_us < corrected_approach_us)
        {
            return RobotArm_MoveValueToward(current_us, corrected_approach_us, step_us);
        }

        return RobotArm_AddDeltaClamped(current_us, (int32_t) step_us);
    }

    if (approach_us < fixed_us)
    {
        if (current_us > corrected_approach_us)
        {
            return RobotArm_MoveValueToward(current_us, corrected_approach_us, step_us);
        }

        return RobotArm_AddDeltaClamped(current_us, -(int32_t) step_us);
    }

    return current_us;
}

/* 对准稳定后向 approach 位推进一步：低位果实(lower_grab_mode)动小臂，高位动大臂 */
static void RobotArm_TreeGrabStepTowardApproach(uint16_t step_us)
{
    robot_arm_pose_t target = RobotArm_TreeGrabApplyCorrection(&robot_arm_tree_grab_config.approach_pose);

    if (robot_arm_tree_grab_config.lower_grab_mode)
    {
        robot_arm_tree_grab_current_pose.forearm =
            RobotArm_TreeGrabStepPastApproach(robot_arm_tree_grab_current_pose.forearm,
                                              robot_arm_tree_grab_config.fixed_pose.forearm,
                                              robot_arm_tree_grab_config.approach_pose.forearm,
                                              target.forearm,
                                              step_us);
    }
    else
    {
        robot_arm_tree_grab_current_pose.upper_arm =
            RobotArm_TreeGrabStepPastApproach(robot_arm_tree_grab_current_pose.upper_arm,
                                              robot_arm_tree_grab_config.fixed_pose.upper_arm,
                                              robot_arm_tree_grab_config.approach_pose.upper_arm,
                                              target.upper_arm,
                                              step_us);
    }
}
/* 记录当前实际脉冲为基准位姿；对准阶段累计的偏移量均相对该基准 */
static void RobotArm_TreeGrabCaptureCurrentAsBase(void)
{
    robot_arm_tree_grab_base_pose.base = robot_arm_current_us[0];
    robot_arm_tree_grab_base_pose.upper_arm = robot_arm_current_us[1];
    robot_arm_tree_grab_base_pose.forearm = robot_arm_current_us[2];
    robot_arm_tree_grab_base_pose.wrist = robot_arm_current_us[3];
    robot_arm_tree_grab_base_pose.gripper = robot_arm_current_us[4];
    robot_arm_tree_grab_base_pose.move_ms = robot_arm_tree_grab_config.align_move_ms;

    robot_arm_tree_grab_align_pose = robot_arm_tree_grab_base_pose;
    robot_arm_tree_grab_current_pose = robot_arm_tree_grab_base_pose;
}
/* 把对准阶段累计的 base/upper/forearm 偏移量叠加到位姿上，使其对准当前目标 */
static robot_arm_pose_t RobotArm_TreeGrabApplyCorrection(const robot_arm_pose_t * p_pose)
{
    robot_arm_pose_t corrected = *p_pose;
    int32_t base_delta = (int32_t) robot_arm_tree_grab_align_pose.base -
                         (int32_t) robot_arm_tree_grab_base_pose.base;
    int32_t upper_delta = (int32_t) robot_arm_tree_grab_align_pose.upper_arm -
                          (int32_t) robot_arm_tree_grab_base_pose.upper_arm;
    int32_t forearm_delta = (int32_t) robot_arm_tree_grab_align_pose.forearm -
                            (int32_t) robot_arm_tree_grab_base_pose.forearm;

    corrected.base = RobotArm_AddDeltaClamped(corrected.base, base_delta);
    corrected.upper_arm = RobotArm_AddDeltaClamped(corrected.upper_arm, upper_delta);
    corrected.forearm = RobotArm_AddDeltaClamped(corrected.forearm, forearm_delta);

    return corrected;
}

/* 从当前对准位姿闭爪：各关节叠加 close_*_delta_us 微调，夹爪收拢到 close_pose 值 */
static void RobotArm_TreeGrabMoveCloseFromCurrent(void)
{
    robot_arm_pose_t close_pose = robot_arm_tree_grab_current_pose;

    close_pose.base = RobotArm_AddDeltaClamped(close_pose.base,
                                               robot_arm_tree_grab_config.close_base_delta_us);
    close_pose.upper_arm = RobotArm_AddDeltaClamped(close_pose.upper_arm,
                                                    robot_arm_tree_grab_config.close_upper_delta_us);
    close_pose.forearm = RobotArm_AddDeltaClamped(close_pose.forearm,
                                                  robot_arm_tree_grab_config.close_forearm_delta_us);
    close_pose.gripper = robot_arm_tree_grab_config.close_pose.gripper;
    close_pose.move_ms = RobotArm_MoveTimeOrDefault(robot_arm_tree_grab_config.close_pose.move_ms);

    robot_arm_tree_grab_current_pose = close_pose;
    RobotArm_TreeGrabMovePose(&close_pose);
}

/* 树上抓取状态机复位到空闲 */
static void RobotArm_TreeGrabStop(void)
{
    robot_arm_tree_grab_state = ROBOT_ARM_TREE_GRAB_IDLE;
    robot_arm_tree_grab_active = false;
    robot_arm_tree_grab_aligned_frames = 0U;
}

/*
 * 启动树上抓取状态机（非阻塞）：p_config 为空时用默认配置，关键参数为 0 时回退默认值。
 * 先移动到 fixed_pose，之后由 RobotArm_TreeGrabService() 依据视觉偏移与目标面积占比推进。
 */
void RobotArm_TreeGrabStart(const robot_arm_tree_grab_config_t * p_config)
{
    if (robot_arm_tree_grab_active || robot_arm_moving || robot_arm_grab_active)
    {
        return;
    }

    if (NULL == p_config)
    {
        robot_arm_tree_grab_config = s_tree_grab_default_config;
    }
    else
    {
        robot_arm_tree_grab_config = *p_config;
    }

    if (robot_arm_tree_grab_config.align_deadband_px <= 0.0f)
    {
        robot_arm_tree_grab_config.align_deadband_px = 18.0f;
    }
    if (0U == robot_arm_tree_grab_config.align_stable_frames)
    {
        robot_arm_tree_grab_config.align_stable_frames = 3U;
    }
    if (0U == robot_arm_tree_grab_config.align_step_us)
    {
        robot_arm_tree_grab_config.align_step_us = 5U;
    }
    if (0U == robot_arm_tree_grab_config.align_move_ms)
    {
        robot_arm_tree_grab_config.align_move_ms = 120U;
    }
    if (robot_arm_tree_grab_config.grab_area_percent <= 0.0f)
    {
        robot_arm_tree_grab_config.grab_area_percent = 8.0f;
    }
    robot_arm_tree_grab_base_pose = robot_arm_tree_grab_config.fixed_pose;

    robot_arm_tree_grab_align_pose = robot_arm_tree_grab_config.fixed_pose;
    robot_arm_tree_grab_current_pose = robot_arm_tree_grab_config.fixed_pose;
    robot_arm_tree_grab_aligned_frames = 0U;
    robot_arm_tree_grab_state = ROBOT_ARM_TREE_GRAB_MOVE_TO_FIXED;
    robot_arm_tree_grab_active = true;

    RobotArm_TreeGrabMovePose(&robot_arm_tree_grab_config.fixed_pose);
}

/* 以默认配置启动树上抓取 */
void RobotArm_TreeGrabStartDefault(void)
{
    RobotArm_TreeGrabStart(&s_tree_grab_default_config);
}

/* 树上抓取状态机是否仍在运行 */
bool RobotArm_TreeGrabIsBusy(void)
{
    return robot_arm_tree_grab_active;
}

/*
 * 树上抓取状态机服务：须在主循环中周期性调用。
 * p_offset 为目标像素偏移；target_area_percent 为检测框占屏幕面积百分比，
 * 达到 grab_area_percent 即认为足够靠近，对准后直接闭爪。
 */
void RobotArm_TreeGrabService(const ai_center_offset_t * p_offset, float target_area_percent)
{
    float deadband;
    uint16_t step_us;
    bool x_aligned;
    bool y_aligned;

    uint16_t y_step_us;
    if (!robot_arm_tree_grab_active)
    {
        return;
    }

    if (robot_arm_moving)
    {
        return;
    }

    switch (robot_arm_tree_grab_state)
    {
        /* 到达固定位姿：记录基准位姿后进入对准/逼近 */
        case ROBOT_ARM_TREE_GRAB_MOVE_TO_FIXED:
            RobotArm_TreeGrabCaptureCurrentAsBase();
            robot_arm_tree_grab_state = ROBOT_ARM_TREE_GRAB_ALIGN_AND_APPROACH;
            robot_arm_tree_grab_aligned_frames = 0U;
            return;

        /*
         * 视觉对准+逼近：按 dx/dy 死区微调底座与升降关节，对准连续稳定后
         * 向 approach 位逐步推进；目标面积达标且已对准时直接闭爪。
         */
        case ROBOT_ARM_TREE_GRAB_ALIGN_AND_APPROACH:
            if ((NULL == p_offset) || (!p_offset->valid))
            {
                robot_arm_tree_grab_aligned_frames = 0U;
                return;
            }

            deadband = robot_arm_tree_grab_config.align_deadband_px;
            step_us = robot_arm_tree_grab_config.align_step_us;
            y_step_us = step_us;
            if (0U == y_step_us)
            {
                y_step_us = 1U;
            }

            x_aligned = (RobotArm_AbsFloat(p_offset->dx) <= deadband);
            y_aligned = (RobotArm_AbsFloat(p_offset->dy) <= deadband);

            /* 目标面积已达阈值且对准完成：认为足够靠近，直接闭爪 */
            if ((target_area_percent >= robot_arm_tree_grab_config.grab_area_percent) &&
                x_aligned &&
                y_aligned)
            {
                robot_arm_tree_grab_state = ROBOT_ARM_TREE_GRAB_CLOSE;
                RobotArm_TreeGrabMoveCloseFromCurrent();
                return;
            }

            if (x_aligned && y_aligned)
            {
                if (robot_arm_tree_grab_aligned_frames < robot_arm_tree_grab_config.align_stable_frames)
                {
                    robot_arm_tree_grab_aligned_frames++;
                }

                if (robot_arm_tree_grab_aligned_frames >= robot_arm_tree_grab_config.align_stable_frames)
                {
                    robot_arm_tree_grab_aligned_frames = 0U;
                    RobotArm_TreeGrabStepTowardApproach(step_us);
                    robot_arm_tree_grab_current_pose.move_ms = robot_arm_tree_grab_config.align_move_ms;
                    RobotArm_TreeGrabMovePose(&robot_arm_tree_grab_current_pose);
                }

                return;
            }

            robot_arm_tree_grab_aligned_frames = 0U;

            /* 未对准：按偏移方向微调，同时更新基准位姿与当前位姿（低位动 upper，高位动 forearm） */
            if (p_offset->dx > deadband)
            {
                robot_arm_tree_grab_align_pose.base = RobotArm_AddClamped(robot_arm_tree_grab_align_pose.base,
                                                                            (int16_t) (-(int32_t) step_us),
                                                                            ROBOT_ARM_SERVO_MIN_US,
                                                                            ROBOT_ARM_SERVO_MAX_US);
                robot_arm_tree_grab_current_pose.base = RobotArm_AddClamped(robot_arm_tree_grab_current_pose.base,
                                                                              (int16_t) (-(int32_t) step_us),
                                                                              ROBOT_ARM_SERVO_MIN_US,
                                                                              ROBOT_ARM_SERVO_MAX_US);
            }
            else if (p_offset->dx < -deadband)
            {
                robot_arm_tree_grab_align_pose.base = RobotArm_AddClamped(robot_arm_tree_grab_align_pose.base,
                                                                            (int16_t) step_us,
                                                                            ROBOT_ARM_SERVO_MIN_US,
                                                                            ROBOT_ARM_SERVO_MAX_US);
                robot_arm_tree_grab_current_pose.base = RobotArm_AddClamped(robot_arm_tree_grab_current_pose.base,
                                                                              (int16_t) step_us,
                                                                              ROBOT_ARM_SERVO_MIN_US,
                                                                              ROBOT_ARM_SERVO_MAX_US);
            }

            if (p_offset->dy > deadband)
            {
                if (robot_arm_tree_grab_config.lower_grab_mode)
                {
                    robot_arm_tree_grab_align_pose.upper_arm = RobotArm_AddClamped(robot_arm_tree_grab_align_pose.upper_arm,
                                                                                     (int16_t) (-(int32_t) y_step_us),
                                                                                     ROBOT_ARM_SERVO_MIN_US,
                                                                                     ROBOT_ARM_SERVO_MAX_US);
                    robot_arm_tree_grab_current_pose.upper_arm = RobotArm_AddClamped(robot_arm_tree_grab_current_pose.upper_arm,
                                                                                       (int16_t) (-(int32_t) y_step_us),
                                                                                       ROBOT_ARM_SERVO_MIN_US,
                                                                                       ROBOT_ARM_SERVO_MAX_US);
                }
                else
                {
                    robot_arm_tree_grab_align_pose.forearm = RobotArm_AddClamped(robot_arm_tree_grab_align_pose.forearm,
                                                                                   (int16_t) y_step_us,
                                                                                   ROBOT_ARM_SERVO_MIN_US,
                                                                                   ROBOT_ARM_SERVO_MAX_US);
                    robot_arm_tree_grab_current_pose.forearm = RobotArm_AddClamped(robot_arm_tree_grab_current_pose.forearm,
                                                                                     (int16_t) y_step_us,
                                                                                     ROBOT_ARM_SERVO_MIN_US,
                                                                                     ROBOT_ARM_SERVO_MAX_US);
                }
            }
            else if (p_offset->dy < -deadband)
            {
                if (robot_arm_tree_grab_config.lower_grab_mode)
                {
                    robot_arm_tree_grab_align_pose.upper_arm = RobotArm_AddClamped(robot_arm_tree_grab_align_pose.upper_arm,
                                                                                     (int16_t) y_step_us,
                                                                                     ROBOT_ARM_SERVO_MIN_US,
                                                                                     ROBOT_ARM_SERVO_MAX_US);
                    robot_arm_tree_grab_current_pose.upper_arm = RobotArm_AddClamped(robot_arm_tree_grab_current_pose.upper_arm,
                                                                                       (int16_t) y_step_us,
                                                                                       ROBOT_ARM_SERVO_MIN_US,
                                                                                       ROBOT_ARM_SERVO_MAX_US);
                }
                else
                {
                    robot_arm_tree_grab_align_pose.forearm = RobotArm_AddClamped(robot_arm_tree_grab_align_pose.forearm,
                                                                                   (int16_t) (-(int32_t) y_step_us),
                                                                                   ROBOT_ARM_SERVO_MIN_US,
                                                                                   ROBOT_ARM_SERVO_MAX_US);
                    robot_arm_tree_grab_current_pose.forearm = RobotArm_AddClamped(robot_arm_tree_grab_current_pose.forearm,
                                                                                     (int16_t) (-(int32_t) y_step_us),
                                                                                     ROBOT_ARM_SERVO_MIN_US,
                                                                                     ROBOT_ARM_SERVO_MAX_US);
                }
            }

            robot_arm_tree_grab_current_pose.move_ms = robot_arm_tree_grab_config.align_move_ms;
            RobotArm_TreeGrabMovePose(&robot_arm_tree_grab_current_pose);
            return;

        /* 闭爪完成 -> 抬臂收回（base 保持抓取侧角度） */
        case ROBOT_ARM_TREE_GRAB_CLOSE:
        {
            robot_arm_pose_t retract_pose = robot_arm_tree_grab_config.retract_pose;

            retract_pose.base = robot_arm_tree_grab_current_pose.base;
            robot_arm_tree_grab_current_pose = retract_pose;
            robot_arm_tree_grab_state = ROBOT_ARM_TREE_GRAB_RETRACT;
            RobotArm_TreeGrabMovePoseWithDownWrist(&retract_pose);
            return;
        }

        /* 收回完成 -> 转回正前方 */
        case ROBOT_ARM_TREE_GRAB_RETRACT:
            robot_arm_tree_grab_state = ROBOT_ARM_TREE_GRAB_RETURN;
            robot_arm_tree_grab_current_pose = robot_arm_tree_grab_config.return_pose;
            RobotArm_TreeGrabMovePoseWithDownWrist(&robot_arm_tree_grab_config.return_pose);
            return;

        /* 转正完成 -> 松爪释放 */
        case ROBOT_ARM_TREE_GRAB_RETURN:
            robot_arm_tree_grab_state = ROBOT_ARM_TREE_GRAB_RELEASE;
            robot_arm_tree_grab_current_pose = robot_arm_tree_grab_config.release_pose;
            RobotArm_TreeGrabMovePoseWithDownWrist(&robot_arm_tree_grab_config.release_pose);
            return;

        case ROBOT_ARM_TREE_GRAB_RELEASE:
            RobotArm_TreeGrabStop();
            return;

        case ROBOT_ARM_TREE_GRAB_IDLE:
        default:
            RobotArm_TreeGrabStop();
            return;
    }
}
/* 暂停/恢复插值推进（POWER 关闭等场合调用，防止恢复瞬间机械臂急跳） */
void RobotArm_SetPaused(bool paused)
{
    if (paused == robot_arm_paused)
    {
        return;
    }

    robot_arm_paused = paused;

    if (!paused)
    {
        /* 恢复时丢弃累积周期计数，防止机械臂瞬间跳到目标姿态 */
        robot_arm_last_cycles = RobotArm_CycleCount();
    }
}

/*
 * 非阻塞插值推进：由主循环周期性调用。
 * 用 DWT 周期计数差衡量时间，每满一个步长周期推进一次线性插值
 * （robot_arm_last_cycles 按步长累加扣除，uint32 回绕安全）；
 * 下压模式下每步按大臂/小臂重算手腕脉冲。全部步数走完后写最终目标并结束。
 */
void RobotArm_Update(void)
{
    uint32_t now;
    uint32_t elapsed;

    if (robot_arm_paused)
    {
        return;
    }

    if (!robot_arm_moving)
    {
        return;
    }

    now     = RobotArm_CycleCount();
    elapsed = now - robot_arm_last_cycles;   /* uint32 回绕安全 */

    while ((elapsed >= robot_arm_step_cycles) && (robot_arm_step < robot_arm_steps))
    {
        uint16_t step_pulse_us[ROBOT_ARM_SERVO_NUM];

        robot_arm_step++;

        for (uint8_t i = 0U; i < ROBOT_ARM_SERVO_NUM; i++)
        {
            int32_t delta = (int32_t) robot_arm_target_us[i] - (int32_t) robot_arm_start_us[i];
            int32_t pulse = (int32_t) robot_arm_start_us[i] +
                            (delta * (int32_t) robot_arm_step) / (int32_t) robot_arm_steps;

            /* 距目标不足 1us 时直接贴合目标，消除插值残差 */
            if (RobotArm_Abs32((int32_t) robot_arm_target_us[i] - pulse) <= 1)
            {
                pulse = (int32_t) robot_arm_target_us[i];
            }

            step_pulse_us[i] = (uint16_t) pulse;
        }

        /* 下压模式：该步手腕脉冲按大臂/小臂实时重算（模式切换后的首次移动除外） */
        if (robot_arm_wrist_down_mode && !robot_arm_wrist_smooth_once)
        {
            step_pulse_us[3] = RobotArm_CalcWristDownUs(step_pulse_us[1], step_pulse_us[2]);
        }

        for (uint8_t i = 0U; i < ROBOT_ARM_SERVO_NUM; i++)
        {
            RobotArm_WriteServo(i, step_pulse_us[i]);
        }

        robot_arm_last_cycles += robot_arm_step_cycles;
        elapsed               -= robot_arm_step_cycles;
    }

    if (robot_arm_step >= robot_arm_steps)
    {
        if (robot_arm_wrist_down_mode && !robot_arm_wrist_smooth_once)
        {
            robot_arm_target_us[3] = RobotArm_CalcWristDownUs(robot_arm_target_us[1], robot_arm_target_us[2]);
        }

        for (uint8_t i = 0U; i < ROBOT_ARM_SERVO_NUM; i++)
        {
            RobotArm_WriteServo(i, robot_arm_target_us[i]);
        }
        robot_arm_wrist_smooth_once = false;
        robot_arm_moving = false;
    }
}
