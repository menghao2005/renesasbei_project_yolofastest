#include "Stepping_Motor.h"
/*
 * Stepping_Motor.c — 步进电机(差速底盘左右驱动轮)控制。
 * 用一个 GPT 定时器的 A/B 两路比较匹配翻转产生左右轮 STEP 脉冲，
 * DIR 引脚控制转向；支持按距离行驶（统计 STEP 沿数换算里程，到点自动停车）。
 * STEP 高/低电平宽度按占空比在比较中断里逐段安排，实现可调速脉冲。
 */
#include <stdio.h>
#include <stdint.h>

#define GPT_GTWP_RESET_VALUE        (0xA500U)
#include "Uart9_Debug.h"
#define GPT_GTIO_TOGGLE_ON_COMPARE  (0x03U)
#define GPT_GTBER_DISABLE_AB_BUFFER (0x00500000U)
#define GPT_MAX_PERIOD_COUNTS       (0xFFFFFFFFU)
/* 脉冲个数下限（防止 0 节拍导致定时器异常） */
#define MOTOR_MIN_PULSE_COUNTS      (2U)
#define MOTOR_MIN_DUTY_PERCENT      (1U)
#define MOTOR_MAX_DUTY_PERCENT      (99U)
/* 逻辑时基 10kHz：1 个逻辑脉冲单位 = 0.1ms（见 “PWM周期 = 0.1*standard” 注释） */
#define MOTOR_TIME_BASE_HZ          (10000U)
/* 车轮直径(cm)与每转步数（含细分），用于行驶距离 <-> 步数换算 */
#define MOTOR_WHEEL_DIAMETER_CM     (9.50f)
#define MOTOR_STEPS_PER_REVOLUTION  (400.0f)
#define MOTOR_PI                    (3.1415926535f)

/* 左右轮 STEP(脉冲)与 DIR(方向)引脚 */
#define LEFT_STEP_PIN               BSP_IO_PORT_01_PIN_05
#define RIGHT_STEP_PIN              BSP_IO_PORT_01_PIN_04
#define LEFT_DIR_PIN                BSP_IO_PORT_09_PIN_06
#define RIGHT_DIR_PIN               BSP_IO_PORT_05_PIN_13

int match_vary = 0;

/* GPT 定时器信息与两路输出通道运行参数（Pulse/单位 0.1ms 节拍、Duty/百分比、Sum/累计 STEP 沿数） */
timer_info_t time_info;
OC_Channel   OC_channel[NumMotors];

int left_standard  = 50;//PWM周期 = 0.1*left_standard 越小越快
int right_standard = 50;//PWM周期 = 0.1*right_standard
int Turn_Flag      = 0;
int back_flag      = 0;

typedef struct st_motor_runtime
{
    timer_compare_match_t compare_match;
    bsp_io_port_pin_t     step_pin;
    bsp_io_port_pin_t     dir_pin;
    uint32_t              next_compare_counts;
    bool                  output_is_high;
} motor_runtime_t;

/* 每侧电机的运行状态：所用比较通道、STEP/DIR 引脚、下一段比较值、当前输出电平 */
static motor_runtime_t g_motor_runtime[NumMotors] =
{
    [LeftSide] =
    {
        .compare_match = TIMER_COMPARE_MATCH_A,
        .step_pin = LEFT_STEP_PIN,
        .dir_pin = LEFT_DIR_PIN,
        .next_compare_counts = 0U,
        .output_is_high = false,
    },
    [RightSide] =
    {
        .compare_match = TIMER_COMPARE_MATCH_B,
        .step_pin = RIGHT_STEP_PIN,
        .dir_pin = RIGHT_DIR_PIN,
        .next_compare_counts = 0U,
        .output_is_high = false,
    },
};

/* 驱动惰性初始化标志；按距离行驶的目标步数与进行中标志 */
static bool g_driver_opened = false;
static bool g_distance_active = false;
static uint32_t g_distance_target_steps = 0U;

/* GPT API 出错时打印错误码（成功则静默） */
static void gpt_log_error(char const * const api_name, fsp_err_t err)
{
    if (FSP_SUCCESS != err)
    {
        DBG_LOG("%s failed: %d\r\n", api_name, err);
    }
}

/* 解除 GPT 寄存器写保护(GTWP)，返回先前值以便恢复 */
static uint32_t gpt_write_protect_disable(gpt_instance_ctrl_t * const p_ctrl)
{
    uint32_t previous = p_ctrl->p_reg->GTWP;
    p_ctrl->p_reg->GTWP = GPT_GTWP_RESET_VALUE;

    return previous;
}

/* 恢复 GPT 写保护（保留先前的密码位） */
static void gpt_write_protect_restore(gpt_instance_ctrl_t * const p_ctrl, uint32_t const previous)
{
    p_ctrl->p_reg->GTWP = previous | GPT_GTWP_RESET_VALUE;
}

/* 脉冲个数下限保护 */
static uint32_t motor_pulse_counts_clamp(int32_t pulse_counts)
{
    if (pulse_counts < (int32_t) MOTOR_MIN_PULSE_COUNTS)
    {
        return MOTOR_MIN_PULSE_COUNTS;
    }

    return (uint32_t) pulse_counts;
}

/* 占空比限制在 [1%, 99%]，保证高低电平段都存在 */
static uint32_t motor_duty_percent_clamp(uint32_t duty_percent)
{
    if (duty_percent < MOTOR_MIN_DUTY_PERCENT)
    {
        return MOTOR_MIN_DUTY_PERCENT;
    }

    if (duty_percent > MOTOR_MAX_DUTY_PERCENT)
    {
        return MOTOR_MAX_DUTY_PERCENT;
    }

    return duty_percent;
}

/* 一个脉冲周期对应的定时器计数：逻辑节拍(0.1ms)换算为实际计数 */
static uint32_t motor_total_counts_get(Motor_Number side)
{
    uint32_t logical_pulse = OC_channel[side].OC_Channel_Pulse;
    uint32_t counts_per_pulse;
    uint32_t total_counts;

    counts_per_pulse = time_info.clock_frequency / MOTOR_TIME_BASE_HZ;
    if (0U == counts_per_pulse)
    {
        counts_per_pulse = 1U;
    }

    total_counts = logical_pulse * counts_per_pulse;

    if (total_counts < MOTOR_MIN_PULSE_COUNTS)
    {
        total_counts = MOTOR_MIN_PULSE_COUNTS;
    }

    return total_counts;
}

/* 脉冲周期内高电平段计数（按占空比换算，强制留出低电平段） */
static uint32_t motor_high_counts_get(Motor_Number side)
{
    uint32_t pulse_counts = motor_total_counts_get(side);
    uint32_t duty_percent = motor_duty_percent_clamp(OC_channel[side].OC_Channel_Duty);
    uint32_t high_counts  = (pulse_counts * duty_percent) / 100U;

    if (0U == high_counts)
    {
        high_counts = 1U;
    }

    if (high_counts >= pulse_counts)
    {
        high_counts = pulse_counts - 1U;
    }

    return high_counts;
}

/* 脉冲周期内低电平段计数 = 周期 - 高电平段 */
static uint32_t motor_low_counts_get(Motor_Number side)
{
    return motor_total_counts_get(side) - motor_high_counts_get(side);
}

/* 取下一段（高或低电平）应持续的计数，由当前输出电平决定 */
static uint32_t motor_next_segment_counts_get(Motor_Number side)
{
    if (g_motor_runtime[side].output_is_high)
    {
        return motor_high_counts_get(side);
    }

    return motor_low_counts_get(side);
}

/* 写入指定通道的比较寄存器（GTCCRA/GTCCRB），自动处理写保护 */
static void motor_compare_write(Motor_Number side, uint32_t compare_counts)
{
    gpt_instance_ctrl_t * const p_ctrl = &g_timer_step1_ctrl;
    uint32_t previous_wp = gpt_write_protect_disable(p_ctrl);

    p_ctrl->p_reg->GTCCR[g_motor_runtime[side].compare_match] = compare_counts - 1U;

    gpt_write_protect_restore(p_ctrl, previous_wp);
}

/* 写 DIR 引脚电平决定该侧旋转方向 */
static void motor_direction_write(Motor_Number side, bsp_io_level_t level)
{
    FSP_PARAMETER_NOT_USED(R_IOPORT_PinWrite(&g_ioport_ctrl, g_motor_runtime[side].dir_pin, level));
}

/* 复位该侧波形相位：从低电平段开始，比较值按段累加，沿计数清零 */
static void motor_runtime_reset(Motor_Number side)
{
    g_motor_runtime[side].output_is_high = false;
    g_motor_runtime[side].next_compare_counts = motor_low_counts_get(side);
    motor_compare_write(side, g_motor_runtime[side].next_compare_counts);
    OC_channel[side].Sum = 0U;
}

/* 配置该侧的脉冲个数(逻辑节拍)与占空比(%) */
static void motor_side_configure(Motor_Number side, int32_t pulse_counts, uint32_t duty_percent)
{
    OC_channel[side].OC_Channel_Pulse = motor_pulse_counts_clamp(pulse_counts);
    OC_channel[side].OC_Channel_Duty  = motor_duty_percent_clamp(duty_percent);
}

/* 手工配置 GPT 输出：关闭 A/B 缓冲，两路 STEP 引脚在比较匹配时翻转并使能 A/B 中断 */
static void gpt_oc_hw_configure(void)
{
    gpt_instance_ctrl_t * const p_ctrl = &g_timer_step1_ctrl;
    uint32_t previous_wp = gpt_write_protect_disable(p_ctrl);

    /* Disable A/B buffering so ISR writes to GTCCRA/GTCCRB take effect immediately. */
    p_ctrl->p_reg->GTBER = GPT_GTBER_DISABLE_AB_BUFFER;

    /* Left STEP starts low and toggles on GTCCRA compare match. */
    p_ctrl->p_reg->GTIOR_b.GTIOA  = GPT_GTIO_TOGGLE_ON_COMPARE;
    p_ctrl->p_reg->GTIOR_b.OADFLT = GPT_PIN_LEVEL_LOW;
    p_ctrl->p_reg->GTIOR_b.OAHLD  = 1U;
    p_ctrl->p_reg->GTIOR_b.OAE    = 1U;

    /* Right STEP starts low and toggles on GTCCRB compare match. */
    p_ctrl->p_reg->GTIOR_b.GTIOB  = GPT_GTIO_TOGGLE_ON_COMPARE;
    p_ctrl->p_reg->GTIOR_b.OBDFLT = GPT_PIN_LEVEL_LOW;
    p_ctrl->p_reg->GTIOR_b.OBHLD  = 1U;
    p_ctrl->p_reg->GTIOR_b.OBE    = 1U;

    /* Enable A/B compare interrupts and dispatch both sides in one callback. */
    p_ctrl->p_reg->GTINTAD_b.GTINTA = 1U;
    p_ctrl->p_reg->GTINTAD_b.GTINTB = 1U;

    gpt_write_protect_restore(p_ctrl, previous_wp);
}

/*
 * 惰性初始化 GPT：打开定时器、周期设为最大(自由计数)并手工配置比较翻转输出。
 * 周期设为最大值后 STEP 脉冲完全由比较匹配中断逐段安排，不受自动周期影响。
 */
static fsp_err_t motor_driver_open_if_needed(void)
{
    fsp_err_t err = FSP_SUCCESS;

    if (g_driver_opened)
    {
        return FSP_SUCCESS;
    }

    err = R_GPT_Open(&g_timer_step1_ctrl, &g_timer_step1_cfg);
    gpt_log_error("R_GPT_Open", err);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = R_GPT_InfoGet(&g_timer_step1_ctrl, &time_info);
    gpt_log_error("R_GPT_InfoGet", err);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = R_GPT_PeriodSet(&g_timer_step1_ctrl, GPT_MAX_PERIOD_COUNTS);
    gpt_log_error("R_GPT_PeriodSet", err);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    gpt_oc_hw_configure();
    time_info.period_counts = GPT_MAX_PERIOD_COUNTS;
    g_driver_opened = true;

    return FSP_SUCCESS;
}

/*
 * 启动一段运动：配置两侧脉冲/占空比，停车并关闭输出后写 DIR 电平、
 * 复位波形相位，再重新使能输出并启动 GPT。
 */
static void motor_motion_start(bsp_io_level_t left_dir,
                               bsp_io_level_t right_dir,
                               int32_t        left_pulse,
                               int32_t        right_pulse,
                               uint32_t       left_duty,
                               uint32_t       right_duty)
{
    fsp_err_t err = motor_driver_open_if_needed();
    if (FSP_SUCCESS != err)
    {
        return;
    }

    motor_side_configure(LeftSide, left_pulse, left_duty);
    motor_side_configure(RightSide, right_pulse, right_duty);

    err = R_GPT_Stop(&g_timer_step1_ctrl);
    gpt_log_error("R_GPT_Stop", err);
    if (FSP_SUCCESS != err)
    {
        return;
    }

    err = R_GPT_OutputDisable(&g_timer_step1_ctrl, GPT_IO_PIN_GTIOCA_AND_GTIOCB);
    gpt_log_error("R_GPT_OutputDisable", err);
    if (FSP_SUCCESS != err)
    {
        return;
    }

    motor_direction_write(LeftSide, left_dir);
    motor_direction_write(RightSide, right_dir);

    motor_runtime_reset(LeftSide);
    motor_runtime_reset(RightSide);

    err = R_GPT_Reset(&g_timer_step1_ctrl);
    gpt_log_error("R_GPT_Reset", err);
    if (FSP_SUCCESS != err)
    {
        return;
    }

    err = R_GPT_OutputEnable(&g_timer_step1_ctrl, GPT_IO_PIN_GTIOCA_AND_GTIOCB);
    gpt_log_error("R_GPT_OutputEnable", err);
    if (FSP_SUCCESS != err)
    {
        return;
    }

    err = R_GPT_Start(&g_timer_step1_ctrl);
    gpt_log_error("R_GPT_Start", err);
    if (FSP_SUCCESS != err)
    {
        return;
    }
}

/* 比较匹配中断里调用：翻转电平并安排下一段高低电平宽度，同时累计 STEP 沿数 */
static void motor_side_service(Motor_Number side)
{
    uint32_t segment_counts;

    OC_channel[side].Sum++;

    /* The pin has toggled when the ISR fires, so mirror the current output level first. */
    g_motor_runtime[side].output_is_high = !g_motor_runtime[side].output_is_high;

    /* Match the STM32 timing: high segment is duty*Pulse, low segment is Pulse-high. */
    segment_counts = motor_next_segment_counts_get(side);
    g_motor_runtime[side].next_compare_counts += segment_counts;
    motor_compare_write(side, g_motor_runtime[side].next_compare_counts);
}

/* 双轮持续前进（左 DIR=LOW / 右 DIR=HIGH，两电机镜像安装故同向行走） */
void Stepping_Motor_Go_Init(void)
{
    g_distance_active = false;
    back_flag = 0;
    Turn_Flag = 0;
    motor_motion_start(BSP_IO_LEVEL_LOW, BSP_IO_LEVEL_HIGH,
                       left_standard, right_standard,
                       50U, 50U);
}

/* 双轮后退（DIR 电平与前进相反） */
void Stepping_Motor_Back_Init(void)
{
    back_flag = 1;
    Turn_Flag = 0;
    motor_motion_start(BSP_IO_LEVEL_HIGH, BSP_IO_LEVEL_LOW,
                       left_standard, right_standard,
                       50U, 50U);
}

/* 原地左转（两侧 DIR 同电平、镜像安装 => 两轮反向旋转） */
void Stepping_Motor_Left_Init(void)
{
    back_flag = 0;
    Turn_Flag = 1;
    motor_motion_start(BSP_IO_LEVEL_HIGH, BSP_IO_LEVEL_HIGH,
                       90, 90,
                       30U, 30U);
}

/* 原地右转 */
void Stepping_Motor_Right_Init(void)
{
    back_flag = 0;
    Turn_Flag = 1;
    motor_motion_start(BSP_IO_LEVEL_LOW, BSP_IO_LEVEL_LOW,
                       90, 90,
                       30U, 30U);
}

/* 立即停止：GPT 停止计数并关闭 STEP 输出引脚 */
void Stepping_Motor_Stopping_Init(void)
{
    fsp_err_t err;

    g_distance_active = false;

    if (!g_driver_opened)
    {
        return;
    }

    err = R_GPT_Stop(&g_timer_step1_ctrl);
    gpt_log_error("R_GPT_Stop", err);
    if (FSP_SUCCESS != err)
    {
        return;
    }

    err = R_GPT_OutputDisable(&g_timer_step1_ctrl, GPT_IO_PIN_GTIOCA_AND_GTIOCB);
    gpt_log_error("R_GPT_OutputDisable", err);
    if (FSP_SUCCESS != err)
    {
        return;
    }

    g_motor_runtime[LeftSide].output_is_high = false;
    g_motor_runtime[RightSide].output_is_high = false;

    DBG_LOG("STEP stop\r\n");
}

/*
 * 按距离启动前进：steps = distance / (π*车轮直径) * 每转步数（四舍五入）。
 * distance_cm<=0 或换算步数为 0 时直接停车并结束。
 */
void Go_Distance_Init(float distance_cm)
{
    float target_steps_f;
    uint32_t target_steps;

    if (distance_cm <= 0.0f)
    {
        g_distance_target_steps = 0U;
        g_distance_active = false;
        Stepping_Motor_Stopping_Init();
        return;
    }

    target_steps_f = (distance_cm / (MOTOR_WHEEL_DIAMETER_CM * MOTOR_PI)) * MOTOR_STEPS_PER_REVOLUTION;
    if (target_steps_f > (float) UINT32_MAX)
    {
        target_steps = UINT32_MAX;
    }
    else
    {
        target_steps = (uint32_t) (target_steps_f + 0.5f);
    }

    Stepping_Motor_Go_Init();
    g_distance_target_steps = target_steps;
    g_distance_active = (g_distance_target_steps > 0U);

    if (!g_distance_active)
    {
        Stepping_Motor_Stopping_Init();
    }
}

/* 以毫米为单位设定行驶距离（内部换算为 cm 调用 Go_Distance_Init） */
void Stepping_Motor_GoDistance_Init(uint32_t distance_mm)
{
    Go_Distance_Init((float) distance_mm / 10.0f);
}

/*
 * 按距离行驶服务：须周期性调用。取左右轮累计 STEP 沿数的平均并换算为步数
 * （两个沿对应一个完整脉冲），达到目标步数即停车并返回 true，未到则返回 false。
 */
bool Stepping_Motor_DistanceService(void)
{
    uint32_t left_edges;
    uint32_t right_edges;
    uint32_t forward_steps;

    if (!g_distance_active)
    {
        return false;
    }

    left_edges = OC_channel[LeftSide].Sum;
    right_edges = OC_channel[RightSide].Sum;

    /* Match the STM32 Go_Distance() idea: average both sides and convert two STEP edges to one pulse. */
    forward_steps = (uint32_t) (((uint64_t) left_edges + (uint64_t) right_edges) / 4U);

    if (forward_steps < g_distance_target_steps)
    {
        return false;
    }

    Stepping_Motor_Stopping_Init();
    return true;
}

/* 是否正在按距离行驶 */
bool Stepping_Motor_DistanceIsBusy(void)
{
    return g_distance_active;
}

/* 设置左右轮速度基准（逻辑节拍数，决定脉冲周期：值越大周期越长、越慢） */
void Stepping_Motor_SetSpeed(uint32_t left_pulse, uint32_t right_pulse)
{
    left_standard  = (int) motor_pulse_counts_clamp((int32_t) left_pulse);
    right_standard = (int) motor_pulse_counts_clamp((int32_t) right_pulse);

    motor_side_configure(LeftSide, left_standard, OC_channel[LeftSide].OC_Channel_Duty);
    motor_side_configure(RightSide, right_standard, OC_channel[RightSide].OC_Channel_Duty);
}

/* 设置左右轮输出占空比(%)（影响驱动电流/扭矩） */
void Stepping_Motor_SetDuty(uint32_t left_duty, uint32_t right_duty)
{
    motor_side_configure(LeftSide, (int32_t) OC_channel[LeftSide].OC_Channel_Pulse, left_duty);
    motor_side_configure(RightSide, (int32_t) OC_channel[RightSide].OC_Channel_Pulse, right_duty);
}

/*
 * 直行纠偏：按行驶方向对左右轮速度基准做一加一减微调
 * （correction>0 时前进向右偏/后退向左偏补偿），原地转向(Turn_Flag)时忽略。
 */
void Direction_correction(int32_t correction)
{
    if (Turn_Flag)
    {
        return;
    }

    if (back_flag)
    {
        motor_side_configure(LeftSide, left_standard - correction, OC_channel[LeftSide].OC_Channel_Duty);
        motor_side_configure(RightSide, right_standard + correction, OC_channel[RightSide].OC_Channel_Duty);
    }
    else
    {
        motor_side_configure(LeftSide, left_standard + correction, OC_channel[LeftSide].OC_Channel_Duty);
        motor_side_configure(RightSide, right_standard - correction, OC_channel[RightSide].OC_Channel_Duty);
    }
}

/* GPT A/B 比较匹配共享中断回调：按事件类型分发给对应侧的脉冲服务 */
void G_Timer_DelayElapsedCallback(timer_callback_args_t * p_args)
{
    if (NULL == p_args)
    {
        return;
    }

    /* When compare_match_status is disabled, the shared interrupt is usually reported as CAPTURE_A/B. */
    //实测用的CAPTURE_A/B
    if ((TIMER_EVENT_CAPTURE_A == p_args->event) || (TIMER_EVENT_COMPARE_A == p_args->event))
    {
        motor_side_service(LeftSide);
        return;
    }

    if ((TIMER_EVENT_CAPTURE_B == p_args->event) || (TIMER_EVENT_COMPARE_B == p_args->event))
    {
        motor_side_service(RightSide);
        return;
    }
}
