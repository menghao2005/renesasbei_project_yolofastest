/*
 * robot_arm.c
 *
 *  Created on: 2026�?�?7�?
 *      Author: menghao2005
 *
 *  机械臂舵机控制。参�?STM32 �?Core/Src/robot_arm.c 的函数结构，
 *  底层用瑞�?RA8P1 �?GPT 产生 50Hz(20ms) PWM 驱动舵机�?
 *
 *  GPT 周期�?ra_gen 中固定为 0.02s（period_counts = 0x4C4B40 = 5,000,000），
 *  因此脉冲宽度(pulse_us) -> 比较计数�?的换算为�?
 *      compare_counts = period_counts * pulse_us / 20000
 *
 *  【非阻塞实现】RobotArm_MoveTo/MoveToTime 仅记�?start/target/steps 并立即返回；
 *  RobotArm_Update() �?DWT 周期计数器测时，�?20ms 步长推进插值，不调用任�?
 *  阻塞延时，因此不会卡�?AI 主循环。只需在主循环里周期性调�?RobotArm_Update()�?
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

/* 每个舵机对应�?GPT 实例与输出引脚（与硬件接线对应，按需修改�?*/
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

/* 当前各舵机脉冲宽�?us)，初始化为中�?*/
static volatile uint16_t robot_arm_current_us[ROBOT_ARM_SERVO_NUM] = {
    1500U, 1640U, 2400U, 1740U, 900U
};

/* GPT 一个周期的计数值，Init 时通过 R_GPT_InfoGet 取得 */
static uint32_t robot_arm_period_counts = 0U;

/* 非阻塞插值状�?*/
static volatile bool robot_arm_moving = false;
static volatile bool robot_arm_paused  = false;
static uint16_t robot_arm_start_us[ROBOT_ARM_SERVO_NUM];
static uint16_t robot_arm_target_us[ROBOT_ARM_SERVO_NUM];
static uint16_t robot_arm_steps        = 1U;
static uint16_t robot_arm_step         = 0U;
static uint32_t robot_arm_last_cycles  = 0U;
static volatile bool robot_arm_wrist_down_mode = false;
static bool     robot_arm_next_move_wrist_down = false;
/* 每步对应�?CPU 周期数（�?SystemCoreClock 推算�?*/
static uint32_t robot_arm_step_cycles  = 1U;

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

static robot_arm_tree_grab_state_t robot_arm_tree_grab_state = ROBOT_ARM_TREE_GRAB_IDLE;
static robot_arm_tree_grab_config_t robot_arm_tree_grab_config;
static robot_arm_pose_t robot_arm_tree_grab_base_pose;
static robot_arm_pose_t robot_arm_tree_grab_align_pose;
static robot_arm_pose_t robot_arm_tree_grab_current_pose;
static uint32_t robot_arm_tree_grab_aligned_frames = 0U;
static bool robot_arm_tree_grab_active = false;
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

static int32_t RobotArm_Abs32(int32_t value)
{
    return (value < 0) ? -value : value;
}

static int32_t RobotArm_DivRound(int32_t value, int32_t divisor)
{
    if (value >= 0)
    {
        return (value + divisor / 2) / divisor;
    }

    return (value - divisor / 2) / divisor;
}

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

static void RobotArm_EnableCycleCounter(void)
{
    if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0U)
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    }
    DWT->CYCCNT = 0U;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

/* 将脉冲宽�?us)换算�?GPT 比较计数值并写入对应通道 */
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
        /* 兜底：按 20ms / 5,000,000 计数换算（compare = pulse_us * 250�?*/
        compare_counts = (uint32_t) pulse_us * 250U;
    }

    R_GPT_DutyCycleSet(robot_arm_channels[index].p_ctrl, compare_counts, robot_arm_channels[index].pin);
    robot_arm_current_us[index] = pulse_us;
}

void RobotArm_Init(void)
{
    timer_info_t info;

    RobotArm_EnableCycleCounter();

    /* 每步周期�?= (SystemCoreClock/1000) * UPDATE_PERIOD_MS */
    robot_arm_step_cycles = (SystemCoreClock / 1000U) * ROBOT_ARM_UPDATE_PERIOD_MS;
    if (robot_arm_step_cycles == 0U)
    {
        robot_arm_step_cycles = 1U;
    }

    /* 打开并启动三�?GPT 定时�?*/
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

uint16_t RobotArm_CalcWristDownUs(uint16_t upper_arm, uint16_t forearm)
{
    return RobotArm_CalcWristDownDetail(upper_arm, forearm, NULL, NULL, NULL);
}

uint16_t RobotArm_CalcWristForwardUs(uint16_t upper_arm, uint16_t forearm)
{
    return RobotArm_CalcWristForwardDetail(upper_arm, forearm, NULL, NULL, NULL);
}

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

void RobotArm_MoveToTime(uint16_t base,
                         uint16_t upper_arm,
                         uint16_t forearm,
                         uint16_t wrist,
                         uint16_t gripper,
                         uint16_t time_ms)
{
    robot_arm_wrist_down_mode = robot_arm_next_move_wrist_down;
    robot_arm_next_move_wrist_down = false;

    for (uint8_t i = 0U; i < ROBOT_ARM_SERVO_NUM; i++)
    {
        robot_arm_target_us[i] = RobotArm_ClampPulse(
            (i == 0U) ? base : (i == 1U) ? upper_arm : (i == 2U) ? forearm : (i == 3U) ? wrist : gripper);
        robot_arm_start_us[i] = robot_arm_current_us[i];
    }

    /* 时间极短或不需要缓动：直接到位，立即返�?*/
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

bool RobotArm_IsMoving(void)
{
    return robot_arm_moving;
}

static float RobotArm_AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

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

bool RobotArm_GrabIsBusy(void)
{
    return robot_arm_grab_active;
}

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
        case ROBOT_ARM_GRAB_MOVE_TO_HOME:
            robot_arm_grab_state = ROBOT_ARM_GRAB_ALIGN;
            robot_arm_grab_aligned_frames = 0U;
            return;

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

        case ROBOT_ARM_GRAB_APPROACH:
            robot_arm_grab_state = ROBOT_ARM_GRAB_CLOSE;
            /* Close gripper: gripper moves to 1350us after final approach. */
            RobotArm_MoveToWristDownTime(robot_arm_grab_current_base,
                                         robot_arm_grab_current_upper,
                                         robot_arm_grab_current_forearm,
                                         robot_arm_grab_close_gripper,
                                         500U);
            return;

        case ROBOT_ARM_GRAB_CLOSE:
            robot_arm_grab_state = ROBOT_ARM_GRAB_RETRACT;
            /* Retract upward: base keeps the grasp side value, upper arm moves to 1640us, forearm moves to 2400us. */
            RobotArm_MoveToWristDownTime(robot_arm_grab_home_base,
                                         1640U,
                                         2400U,
                                         robot_arm_grab_close_gripper,
                                         1500U);
            return;

        case ROBOT_ARM_GRAB_RETRACT:
            robot_arm_grab_state = ROBOT_ARM_GRAB_RETURN;
            /* Return base to front: base moves to 1500us before releasing fruit. */
            RobotArm_MoveToWristDownTime(1500U,
                                         1640U,
                                         2400U,
                                         robot_arm_grab_close_gripper,
                                         1500U);
            return;

        case ROBOT_ARM_GRAB_RETURN:
            robot_arm_grab_state = ROBOT_ARM_GRAB_RELEASE;
            /* Release fruit at front: base stays at 1500us, gripper returns to the value passed into RobotArm_GrabStart(). */
            RobotArm_MoveToWristDownTime(1500U,
                                         1640U,
                                         2400U,
                                         robot_arm_grab_home_gripper,
                                         500U);
            return;

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

static uint16_t RobotArm_MoveTimeOrDefault(uint16_t move_ms)
{
    return (move_ms == 0U) ? ROBOT_ARM_DEFAULT_MOVE_MS : move_ms;
}

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



static robot_arm_pose_t RobotArm_TreeGrabPoseWithForwardWrist(const robot_arm_pose_t * p_pose)
{
    robot_arm_pose_t pose = *p_pose;
    pose.wrist = RobotArm_CalcWristForwardUs(pose.upper_arm, pose.forearm);
    return pose;
}

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

static void RobotArm_TreeGrabStop(void)
{
    robot_arm_tree_grab_state = ROBOT_ARM_TREE_GRAB_IDLE;
    robot_arm_tree_grab_active = false;
    robot_arm_tree_grab_aligned_frames = 0U;
}

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

void RobotArm_TreeGrabStartDefault(void)
{
    RobotArm_TreeGrabStart(&s_tree_grab_default_config);
}

bool RobotArm_TreeGrabIsBusy(void)
{
    return robot_arm_tree_grab_active;
}

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
        case ROBOT_ARM_TREE_GRAB_MOVE_TO_FIXED:
            RobotArm_TreeGrabCaptureCurrentAsBase();
            robot_arm_tree_grab_state = ROBOT_ARM_TREE_GRAB_ALIGN_AND_APPROACH;
            robot_arm_tree_grab_aligned_frames = 0U;
            return;

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

        case ROBOT_ARM_TREE_GRAB_CLOSE:
        {
            robot_arm_pose_t retract_pose = robot_arm_tree_grab_config.retract_pose;

            retract_pose.base = robot_arm_tree_grab_current_pose.base;
            robot_arm_tree_grab_current_pose = retract_pose;
            robot_arm_tree_grab_state = ROBOT_ARM_TREE_GRAB_RETRACT;
            RobotArm_TreeGrabMovePoseWithDownWrist(&retract_pose);
            return;
        }

        case ROBOT_ARM_TREE_GRAB_RETRACT:
            robot_arm_tree_grab_state = ROBOT_ARM_TREE_GRAB_RETURN;
            robot_arm_tree_grab_current_pose = robot_arm_tree_grab_config.return_pose;
            RobotArm_TreeGrabMovePoseWithDownWrist(&robot_arm_tree_grab_config.return_pose);
            return;

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

            if (RobotArm_Abs32((int32_t) robot_arm_target_us[i] - pulse) <= 1)
            {
                pulse = (int32_t) robot_arm_target_us[i];
            }

            step_pulse_us[i] = (uint16_t) pulse;
        }

        if (robot_arm_wrist_down_mode)
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
        if (robot_arm_wrist_down_mode)
        {
            robot_arm_target_us[3] = RobotArm_CalcWristDownUs(robot_arm_target_us[1], robot_arm_target_us[2]);
        }

        for (uint8_t i = 0U; i < ROBOT_ARM_SERVO_NUM; i++)
        {
            RobotArm_WriteServo(i, robot_arm_target_us[i]);
        }
        robot_arm_moving = false;
    }
}
