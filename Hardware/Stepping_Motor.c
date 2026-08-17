#include "Stepping_Motor.h"
#include <stdio.h>
#include <stdint.h>

#define GPT_GTWP_RESET_VALUE        (0xA500U)
#include "Uart9_Debug.h"
#define GPT_GTIO_TOGGLE_ON_COMPARE  (0x03U)
#define GPT_GTBER_DISABLE_AB_BUFFER (0x00500000U)
#define GPT_MAX_PERIOD_COUNTS       (0xFFFFFFFFU)
#define MOTOR_MIN_PULSE_COUNTS      (2U)
#define MOTOR_MIN_DUTY_PERCENT      (1U)
#define MOTOR_MAX_DUTY_PERCENT      (99U)
#define MOTOR_TIME_BASE_HZ          (10000U)
#define MOTOR_WHEEL_DIAMETER_CM     (9.50f)
#define MOTOR_STEPS_PER_REVOLUTION  (400.0f)
#define MOTOR_PI                    (3.1415926535f)

#define LEFT_STEP_PIN               BSP_IO_PORT_01_PIN_05
#define RIGHT_STEP_PIN              BSP_IO_PORT_01_PIN_04
#define LEFT_DIR_PIN                BSP_IO_PORT_09_PIN_06
#define RIGHT_DIR_PIN               BSP_IO_PORT_05_PIN_13

int match_vary = 0;

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

static bool g_driver_opened = false;
static bool g_distance_active = false;
static uint32_t g_distance_target_steps = 0U;

static void gpt_log_error(char const * const api_name, fsp_err_t err)
{
    if (FSP_SUCCESS != err)
    {
        DBG_LOG("%s failed: %d\r\n", api_name, err);
    }
}

static uint32_t gpt_write_protect_disable(gpt_instance_ctrl_t * const p_ctrl)
{
    uint32_t previous = p_ctrl->p_reg->GTWP;
    p_ctrl->p_reg->GTWP = GPT_GTWP_RESET_VALUE;

    return previous;
}

static void gpt_write_protect_restore(gpt_instance_ctrl_t * const p_ctrl, uint32_t const previous)
{
    p_ctrl->p_reg->GTWP = previous | GPT_GTWP_RESET_VALUE;
}

static uint32_t motor_pulse_counts_clamp(int32_t pulse_counts)
{
    if (pulse_counts < (int32_t) MOTOR_MIN_PULSE_COUNTS)
    {
        return MOTOR_MIN_PULSE_COUNTS;
    }

    return (uint32_t) pulse_counts;
}

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

static uint32_t motor_low_counts_get(Motor_Number side)
{
    return motor_total_counts_get(side) - motor_high_counts_get(side);
}

static uint32_t motor_next_segment_counts_get(Motor_Number side)
{
    if (g_motor_runtime[side].output_is_high)
    {
        return motor_high_counts_get(side);
    }

    return motor_low_counts_get(side);
}

static void motor_compare_write(Motor_Number side, uint32_t compare_counts)
{
    gpt_instance_ctrl_t * const p_ctrl = &g_timer_step1_ctrl;
    uint32_t previous_wp = gpt_write_protect_disable(p_ctrl);

    p_ctrl->p_reg->GTCCR[g_motor_runtime[side].compare_match] = compare_counts - 1U;

    gpt_write_protect_restore(p_ctrl, previous_wp);
}

static void motor_direction_write(Motor_Number side, bsp_io_level_t level)
{
    FSP_PARAMETER_NOT_USED(R_IOPORT_PinWrite(&g_ioport_ctrl, g_motor_runtime[side].dir_pin, level));
}

static void motor_runtime_reset(Motor_Number side)
{
    g_motor_runtime[side].output_is_high = false;
    g_motor_runtime[side].next_compare_counts = motor_low_counts_get(side);
    motor_compare_write(side, g_motor_runtime[side].next_compare_counts);
    OC_channel[side].Sum = 0U;
}

static void motor_side_configure(Motor_Number side, int32_t pulse_counts, uint32_t duty_percent)
{
    OC_channel[side].OC_Channel_Pulse = motor_pulse_counts_clamp(pulse_counts);
    OC_channel[side].OC_Channel_Duty  = motor_duty_percent_clamp(duty_percent);
}

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

void Stepping_Motor_Go_Init(void)
{
    g_distance_active = false;
    back_flag = 0;
    Turn_Flag = 0;
    motor_motion_start(BSP_IO_LEVEL_LOW, BSP_IO_LEVEL_HIGH,
                       left_standard, right_standard,
                       50U, 50U);
}

void Stepping_Motor_Back_Init(void)
{
    back_flag = 1;
    Turn_Flag = 0;
    motor_motion_start(BSP_IO_LEVEL_HIGH, BSP_IO_LEVEL_LOW,
                       left_standard, right_standard,
                       50U, 50U);
}

void Stepping_Motor_Left_Init(void)
{
    back_flag = 0;
    Turn_Flag = 1;
    motor_motion_start(BSP_IO_LEVEL_HIGH, BSP_IO_LEVEL_HIGH,
                       90, 90,
                       30U, 30U);
}

void Stepping_Motor_Right_Init(void)
{
    back_flag = 0;
    Turn_Flag = 1;
    motor_motion_start(BSP_IO_LEVEL_LOW, BSP_IO_LEVEL_LOW,
                       90, 90,
                       30U, 30U);
}

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

void Stepping_Motor_GoDistance_Init(uint32_t distance_mm)
{
    Go_Distance_Init((float) distance_mm / 10.0f);
}

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

bool Stepping_Motor_DistanceIsBusy(void)
{
    return g_distance_active;
}

void Stepping_Motor_SetSpeed(uint32_t left_pulse, uint32_t right_pulse)
{
    left_standard  = (int) motor_pulse_counts_clamp((int32_t) left_pulse);
    right_standard = (int) motor_pulse_counts_clamp((int32_t) right_pulse);

    motor_side_configure(LeftSide, left_standard, OC_channel[LeftSide].OC_Channel_Duty);
    motor_side_configure(RightSide, right_standard, OC_channel[RightSide].OC_Channel_Duty);
}

void Stepping_Motor_SetDuty(uint32_t left_duty, uint32_t right_duty)
{
    motor_side_configure(LeftSide, (int32_t) OC_channel[LeftSide].OC_Channel_Pulse, left_duty);
    motor_side_configure(RightSide, (int32_t) OC_channel[RightSide].OC_Channel_Pulse, right_duty);
}

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
