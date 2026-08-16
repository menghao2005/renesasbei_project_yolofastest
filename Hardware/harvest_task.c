#include "harvest_task.h"

#include <stddef.h>
#include <stdbool.h>
#include "Stepping_Motor.h"
#include "ai_center_offset.h"
#include "robot_arm.h"

extern uint32_t DWT_get_count(void);
extern uint32_t DWT_count_to_us(uint32_t delta_count);

/*
 * Harvest tuning area.
 * Full sequence:
 *   ground right -> ground left -> drive 50cm ->
 *   ground right -> ground left -> drive 50cm ->
 *   tree grab.
 */
#define HARVEST_DRIVE_BETWEEN_DISTANCE_CM      (50.0f)
#define HARVEST_GROUND_DETECT_TIMEOUT_MS       (5000U)
#define HARVEST_GROUND_DETECT_STABLE_FRAMES    (3U)
#define HARVEST_TREE_DETECT_TIMEOUT_MS         (5000U)
#define HARVEST_TREE_DETECT_IGNORE_MS          (300U)
#define HARVEST_TREE_DETECT_STABLE_FRAMES      (2U)

/* Set to 1 to run the full ground-right/left + drive sequence before tree grab. */
#define HARVEST_ENABLE_GROUND_SEQUENCE         (1U)

#define HARVEST_GROUND_RIGHT_BASE_US           (1500U - 760U)
#define HARVEST_GROUND_RIGHT_UPPER_US          (1640U - 180U)
#define HARVEST_GROUND_RIGHT_FOREARM_US        (1500U + 766U)
#define HARVEST_GROUND_RIGHT_GRIPPER_US        (900U)
#define HARVEST_GROUND_RIGHT_MOVE_MS           (ROBOT_ARM_DEFAULT_MOVE_MS)

#define HARVEST_GROUND_LEFT_BASE_US            (1500U + 660U)
#define HARVEST_GROUND_LEFT_UPPER_US           (1640U - 180U)
#define HARVEST_GROUND_LEFT_FOREARM_US         (1500U + 766U)
#define HARVEST_GROUND_LEFT_GRIPPER_US         (900U)
#define HARVEST_GROUND_LEFT_MOVE_MS            (ROBOT_ARM_DEFAULT_MOVE_MS)

#define HARVEST_GROUND_HOME_BASE_US            (1500U)
#define HARVEST_GROUND_HOME_UPPER_US           (1640U)
#define HARVEST_GROUND_HOME_FOREARM_US         (2400U)
#define HARVEST_GROUND_HOME_GRIPPER_US         (900U)
#define HARVEST_GROUND_HOME_MOVE_MS            (ROBOT_ARM_DEFAULT_MOVE_MS)

#define HARVEST_TREE_UP_APPROACH_UPPER_DELTA_US       (-100)
#define HARVEST_TREE_DOWN_APPROACH_FOREARM_DELTA_US   (-100)

#define HARVEST_TREE_GRIPPER_OPEN_US                  (900U)
#define HARVEST_TREE_GRIPPER_CLOSE_US                 (1400U)
#define HARVEST_TREE_HOME_BASE_US                   (1500U)
#define HARVEST_TREE_HOME_UPPER_US                  (1640U)
#define HARVEST_TREE_HOME_FOREARM_US                (2400U)
#define HARVEST_TREE_HOME_MOVE_MS                   (4000U)
#define HARVEST_TREE_FIXED_MOVE_MS                    (ROBOT_ARM_DEFAULT_MOVE_MS)
#define HARVEST_TREE_APPROACH_MOVE_MS                 (1000U)
#define HARVEST_TREE_CLOSE_MOVE_MS                    (1500U)
#define HARVEST_TREE_RETRACT_MOVE_MS                  (2000U)
#define HARVEST_TREE_RETURN_MOVE_MS                   (1500U)
#define HARVEST_TREE_ALIGN_DEADBAND_PX                (18.0f)
#define HARVEST_TREE_ALIGN_STABLE_FRAMES              (2U)
#define HARVEST_TREE_ALIGN_STEP_US                    (5U)
#define HARVEST_TREE_ALIGN_MOVE_MS                    (180U)
#define HARVEST_TREE_GRAB_AREA_PERCENT                (45.0f)

typedef enum e_harvest_tree_grab_index
{
    HARVEST_TREE_LEFT_UP = 0,
    HARVEST_TREE_RIGHT_UP,
    HARVEST_TREE_RIGHT_DOWN,
    HARVEST_TREE_LEFT_DOWN,
    HARVEST_TREE_GRAB_COUNT,
} harvest_tree_grab_index_t;

typedef enum e_harvest_tree_pose_col
{
    HARVEST_TREE_POSE_BASE = 0,
    HARVEST_TREE_POSE_UPPER,
    HARVEST_TREE_POSE_FOREARM,
    HARVEST_TREE_POSE_WRIST_RESERVED,
    HARVEST_TREE_POSE_COL_COUNT,
} harvest_tree_pose_col_t;

typedef enum e_harvest_tree_close_delta_col
{
    HARVEST_TREE_CLOSE_DELTA_BASE = 0,
    HARVEST_TREE_CLOSE_DELTA_UPPER,
    HARVEST_TREE_CLOSE_DELTA_FOREARM,
    HARVEST_TREE_CLOSE_DELTA_COL_COUNT,
} harvest_tree_close_delta_col_t;

/* Rows: left-up, right-up, right-down, left-down. Columns: base, upper, forearm, reserved.
 * Tree wrist is always calculated from upper/forearm to keep it horizontal. */
static const uint16_t g_tree_fixed_pose_us[HARVEST_TREE_GRAB_COUNT][HARVEST_TREE_POSE_COL_COUNT] =
{
    {1500U - 640U, 1600U, 2180U, 1740U},  /* left-up */
    {1500U - 900U, 1600U, 2180U, 1740U},  /* right-up */
    {1500U - 640U, 1050U, 2400U, 1740U},  /* right-down */
    {1500U - 900U, 1050U, 2400U, 1740U},  /* left-down */
};

/* Rows: left-up, right-up, right-down, left-down. Columns: base delta, upper delta, forearm delta.
 * These deltas are added after visual alignment has already reached the grab condition. */
static const int16_t g_tree_close_delta_us[HARVEST_TREE_GRAB_COUNT][HARVEST_TREE_CLOSE_DELTA_COL_COUNT] =
{
    {0, -140, -180},  /* left-up */
    {0, -140, -180},  /* right-up */
    {0, -60, -250},  /* right-down */
    {0, -60, -250},  /* left-down */
};

typedef enum e_harvest_task_state
{
    HARVEST_TASK_GROUND1_RIGHT_MOVE_START = 0,
    HARVEST_TASK_GROUND1_RIGHT_MOVING,
    HARVEST_TASK_GROUND1_RIGHT_WAIT_DETECT,
    HARVEST_TASK_GROUND1_RIGHT_GRAB_START,
    HARVEST_TASK_GROUND1_RIGHT_GRABBING,
    HARVEST_TASK_GROUND1_LEFT_MOVE_START,
    HARVEST_TASK_GROUND1_LEFT_MOVING,
    HARVEST_TASK_GROUND1_LEFT_WAIT_DETECT,
    HARVEST_TASK_GROUND1_LEFT_GRAB_START,
    HARVEST_TASK_GROUND1_LEFT_GRABBING,
    HARVEST_TASK_GROUND1_HOME_START,
    HARVEST_TASK_GROUND1_HOMING,
    HARVEST_TASK_DRIVE1_START,
    HARVEST_TASK_DRIVE1_DRIVING,
    HARVEST_TASK_GROUND2_RIGHT_MOVE_START,
    HARVEST_TASK_GROUND2_RIGHT_MOVING,
    HARVEST_TASK_GROUND2_RIGHT_WAIT_DETECT,
    HARVEST_TASK_GROUND2_RIGHT_GRAB_START,
    HARVEST_TASK_GROUND2_RIGHT_GRABBING,
    HARVEST_TASK_GROUND2_LEFT_MOVE_START,
    HARVEST_TASK_GROUND2_LEFT_MOVING,
    HARVEST_TASK_GROUND2_LEFT_WAIT_DETECT,
    HARVEST_TASK_GROUND2_LEFT_GRAB_START,
    HARVEST_TASK_GROUND2_LEFT_GRABBING,
    HARVEST_TASK_GROUND2_HOME_START,
    HARVEST_TASK_GROUND2_HOMING,
    HARVEST_TASK_DRIVE2_START,
    HARVEST_TASK_DRIVE2_DRIVING,
    HARVEST_TASK_TREE_MOVE_START,
    HARVEST_TASK_TREE_MOVING,
    HARVEST_TASK_TREE_WAIT_DETECT,
    HARVEST_TASK_TREE_GRAB_START,
    HARVEST_TASK_TREE_GRABBING,
    HARVEST_TASK_DONE,
} harvest_task_state_t;

static harvest_task_state_t g_harvest_task_state;
static uint32_t g_detect_wait_start_count;
static uint32_t g_detect_valid_frames;
static uint32_t g_tree_grab_index;
static bool     g_harvest_done_homed;   /* home move registered once */
static robot_arm_tree_grab_config_t g_tree_grab_config_work;

static uint16_t harvest_tree_add_delta_us(uint16_t pulse_us, int32_t delta_us)
{
    int32_t next_us = (int32_t) pulse_us + delta_us;

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

static robot_arm_pose_t harvest_tree_pose_from_table(uint32_t index, uint16_t gripper_us, uint16_t move_ms)
{
    robot_arm_pose_t pose;

    if (index >= (uint32_t) HARVEST_TREE_GRAB_COUNT)
    {
        index = 0U;
    }

    pose.base = g_tree_fixed_pose_us[index][HARVEST_TREE_POSE_BASE];
    pose.upper_arm = g_tree_fixed_pose_us[index][HARVEST_TREE_POSE_UPPER];
    pose.forearm = g_tree_fixed_pose_us[index][HARVEST_TREE_POSE_FOREARM];
    pose.wrist = RobotArm_CalcWristForwardUs(pose.upper_arm, pose.forearm);
    pose.gripper = gripper_us;
    pose.move_ms = move_ms;

    return pose;
}

static void harvest_tree_build_config(uint32_t index, robot_arm_tree_grab_config_t * p_config)
{
    bool lower_mode;

    if (NULL == p_config)
    {
        return;
    }

    if (index >= (uint32_t) HARVEST_TREE_GRAB_COUNT)
    {
        index = 0U;
    }

    lower_mode = (index >= (uint32_t) HARVEST_TREE_RIGHT_DOWN);

    p_config->fixed_pose = harvest_tree_pose_from_table(index,
                                                        HARVEST_TREE_GRIPPER_OPEN_US,
                                                        HARVEST_TREE_FIXED_MOVE_MS);
    p_config->approach_pose = p_config->fixed_pose;
    p_config->approach_pose.move_ms = HARVEST_TREE_APPROACH_MOVE_MS;
    if (lower_mode)
    {
        p_config->approach_pose.forearm =
            harvest_tree_add_delta_us(p_config->approach_pose.forearm,
                                      HARVEST_TREE_DOWN_APPROACH_FOREARM_DELTA_US);
    }
    else
    {
        p_config->approach_pose.upper_arm =
            harvest_tree_add_delta_us(p_config->approach_pose.upper_arm,
                                      HARVEST_TREE_UP_APPROACH_UPPER_DELTA_US);
    }

    p_config->close_pose = p_config->fixed_pose;
    p_config->close_pose.gripper = HARVEST_TREE_GRIPPER_CLOSE_US;
    p_config->close_pose.move_ms = HARVEST_TREE_CLOSE_MOVE_MS;

    p_config->retract_pose.base = p_config->fixed_pose.base;
    p_config->retract_pose.upper_arm = 1640U;
    p_config->retract_pose.forearm = 2400U;
    p_config->retract_pose.wrist = 1740U;
    p_config->retract_pose.gripper = HARVEST_TREE_GRIPPER_CLOSE_US;
    p_config->retract_pose.move_ms = HARVEST_TREE_RETRACT_MOVE_MS;

    p_config->return_pose.base = 1500U;
    p_config->return_pose.upper_arm = 1640U;
    p_config->return_pose.forearm = 2400U;
    p_config->return_pose.wrist = 1740U;
    p_config->return_pose.gripper = 1350U;
    p_config->return_pose.move_ms = HARVEST_TREE_RETURN_MOVE_MS;

    p_config->release_pose = p_config->return_pose;
    p_config->release_pose.gripper = HARVEST_TREE_GRIPPER_OPEN_US;

    p_config->align_deadband_px = HARVEST_TREE_ALIGN_DEADBAND_PX;
    p_config->align_stable_frames = HARVEST_TREE_ALIGN_STABLE_FRAMES;
    p_config->align_step_us = HARVEST_TREE_ALIGN_STEP_US;
    p_config->align_move_ms = HARVEST_TREE_ALIGN_MOVE_MS;
    p_config->grab_area_percent = HARVEST_TREE_GRAB_AREA_PERCENT;
    p_config->close_base_delta_us = g_tree_close_delta_us[index][HARVEST_TREE_CLOSE_DELTA_BASE];
    p_config->close_upper_delta_us = g_tree_close_delta_us[index][HARVEST_TREE_CLOSE_DELTA_UPPER];
    p_config->close_forearm_delta_us = g_tree_close_delta_us[index][HARVEST_TREE_CLOSE_DELTA_FOREARM];
    p_config->lower_grab_mode = lower_mode;
}

static void harvest_detect_timeout_start(void)
{
    g_detect_wait_start_count = DWT_get_count();
    g_detect_valid_frames = 0U;
}

static bool harvest_detect_timeout_expired(void)
{
    uint32_t elapsed_us = DWT_count_to_us((uint32_t) (DWT_get_count() - g_detect_wait_start_count));

    return (elapsed_us / 1000U) >= HARVEST_GROUND_DETECT_TIMEOUT_MS;
}

static bool harvest_target_is_stable(void)
{
    if (g_ai_center_offset.valid)
    {
        if (g_detect_valid_frames < HARVEST_GROUND_DETECT_STABLE_FRAMES)
        {
            g_detect_valid_frames++;
        }
    }
    else
    {
        g_detect_valid_frames = 0U;
    }

    return g_detect_valid_frames >= HARVEST_GROUND_DETECT_STABLE_FRAMES;
}

static bool harvest_tree_target_is_stable(void)
{
    if (g_ai_center_offset.valid)
    {
        if (g_detect_valid_frames < HARVEST_TREE_DETECT_STABLE_FRAMES)
        {
            g_detect_valid_frames++;
        }
    }
    else
    {
        g_detect_valid_frames = 0U;
    }

    return g_detect_valid_frames >= HARVEST_TREE_DETECT_STABLE_FRAMES;
}

static bool harvest_tree_detect_timeout_expired(void)
{
    uint32_t elapsed_us = DWT_count_to_us((uint32_t) (DWT_get_count() - g_detect_wait_start_count));

    return (elapsed_us / 1000U) >= HARVEST_TREE_DETECT_TIMEOUT_MS;
}

static bool harvest_tree_detect_ignore_elapsed(void)
{
    uint32_t elapsed_us = DWT_count_to_us((uint32_t) (DWT_get_count() - g_detect_wait_start_count));

    return (elapsed_us / 1000U) >= HARVEST_TREE_DETECT_IGNORE_MS;
}

static void harvest_tree_next_position(void)
{
    g_tree_grab_index++;
    g_harvest_task_state =
        (g_tree_grab_index < (uint32_t) HARVEST_TREE_GRAB_COUNT) ?
        HARVEST_TASK_TREE_MOVE_START :
        HARVEST_TASK_DONE;
}

static void harvest_tree_move_to_current_fixed_pose(void)
{
    robot_arm_pose_t fixed_pose = harvest_tree_pose_from_table(g_tree_grab_index,
                                                               HARVEST_TREE_GRIPPER_OPEN_US,
                                                               HARVEST_TREE_FIXED_MOVE_MS);

    RobotArm_MoveToTime(fixed_pose.base,
                        fixed_pose.upper_arm,
                        fixed_pose.forearm,
                        fixed_pose.wrist,
                        fixed_pose.gripper,
                        fixed_pose.move_ms);
}

static void harvest_ground_move_right_search(void)
{
    RobotArm_MoveToWristDownTime(HARVEST_GROUND_RIGHT_BASE_US,
                                 HARVEST_GROUND_RIGHT_UPPER_US,
                                 HARVEST_GROUND_RIGHT_FOREARM_US,
                                 HARVEST_GROUND_RIGHT_GRIPPER_US,
                                 HARVEST_GROUND_RIGHT_MOVE_MS);
}

static void harvest_ground_move_left_search(void)
{
    RobotArm_MoveToWristDownTime(HARVEST_GROUND_LEFT_BASE_US,
                                 HARVEST_GROUND_LEFT_UPPER_US,
                                 HARVEST_GROUND_LEFT_FOREARM_US,
                                 HARVEST_GROUND_LEFT_GRIPPER_US,
                                 HARVEST_GROUND_LEFT_MOVE_MS);
}

static void harvest_ground_move_home(void)
{
    RobotArm_MoveToWristDownTime(HARVEST_GROUND_HOME_BASE_US,
                                 HARVEST_GROUND_HOME_UPPER_US,
                                 HARVEST_GROUND_HOME_FOREARM_US,
                                 HARVEST_GROUND_HOME_GRIPPER_US,
                                 HARVEST_GROUND_HOME_MOVE_MS);
}

static void harvest_ground_grab_right_start(void)
{
    RobotArm_GrabStart(HARVEST_GROUND_RIGHT_BASE_US,
                       HARVEST_GROUND_RIGHT_UPPER_US,
                       HARVEST_GROUND_RIGHT_FOREARM_US,
                       HARVEST_GROUND_RIGHT_GRIPPER_US);
}

static void harvest_ground_grab_left_start(void)
{
    RobotArm_GrabStart(HARVEST_GROUND_LEFT_BASE_US,
                       HARVEST_GROUND_LEFT_UPPER_US,
                       HARVEST_GROUND_LEFT_FOREARM_US,
                       HARVEST_GROUND_LEFT_GRIPPER_US);
}

static float harvest_detection_screen_area_percent(const ai_detection_t * p_dets, uint32_t num_dets)
{
    float best_area = 0.0f;
    const float screen_area = (float) FB_CAM_W * (float) FB_CAM_H;

    if ((NULL == p_dets) || (0U == num_dets))
    {
        return 0.0f;
    }

    for (uint32_t i = 0U; i < num_dets; i++)
    {
        float x0 = p_dets[i].x * FB_SCALE_X + FB_BOX_OFFSET_X;
        float y0 = p_dets[i].y * FB_SCALE_Y + FB_BOX_OFFSET_Y;
        float x1 = (p_dets[i].x + p_dets[i].w) * FB_SCALE_X + FB_BOX_OFFSET_X;
        float y1 = (p_dets[i].y + p_dets[i].h) * FB_SCALE_Y + FB_BOX_OFFSET_Y;
        float area;

        if (x0 < 0.0f)
        {
            x0 = 0.0f;
        }
        if (y0 < 0.0f)
        {
            y0 = 0.0f;
        }
        if (x1 > (float) FB_CAM_W)
        {
            x1 = (float) FB_CAM_W;
        }
        if (y1 > (float) FB_CAM_H)
        {
            y1 = (float) FB_CAM_H;
        }
        if ((x1 <= x0) || (y1 <= y0))
        {
            continue;
        }

        area = (x1 - x0) * (y1 - y0);
        if (area > best_area)
        {
            best_area = area;
        }
    }

    return (best_area * 100.0f) / screen_area;
}

void HarvestTask_Init(void)
{
#if HARVEST_ENABLE_GROUND_SEQUENCE
    g_harvest_task_state = HARVEST_TASK_GROUND1_RIGHT_MOVE_START;
#else
    g_harvest_task_state = HARVEST_TASK_TREE_MOVE_START;
#endif
    g_detect_wait_start_count = DWT_get_count();
    g_detect_valid_frames = 0U;
    g_tree_grab_index = 0U;
    g_harvest_done_homed = false;
}

void HarvestTask_Service(const ai_detection_t * p_dets, uint32_t num_dets)
{
    ai_center_offset_calc(p_dets, num_dets, NULL);

    switch (g_harvest_task_state)
    {
        case HARVEST_TASK_GROUND1_RIGHT_MOVE_START:
        case HARVEST_TASK_GROUND2_RIGHT_MOVE_START:
            harvest_ground_move_right_search();
            g_harvest_task_state =
                (HARVEST_TASK_GROUND1_RIGHT_MOVE_START == g_harvest_task_state) ?
                HARVEST_TASK_GROUND1_RIGHT_MOVING :
                HARVEST_TASK_GROUND2_RIGHT_MOVING;
            break;

        case HARVEST_TASK_GROUND1_RIGHT_MOVING:
        case HARVEST_TASK_GROUND2_RIGHT_MOVING:
            if (!RobotArm_IsMoving())
            {
                harvest_detect_timeout_start();
                g_harvest_task_state =
                    (HARVEST_TASK_GROUND1_RIGHT_MOVING == g_harvest_task_state) ?
                    HARVEST_TASK_GROUND1_RIGHT_WAIT_DETECT :
                    HARVEST_TASK_GROUND2_RIGHT_WAIT_DETECT;
            }
            break;

        case HARVEST_TASK_GROUND1_RIGHT_WAIT_DETECT:
        case HARVEST_TASK_GROUND2_RIGHT_WAIT_DETECT:
            if (harvest_target_is_stable())
            {
                g_harvest_task_state =
                    (HARVEST_TASK_GROUND1_RIGHT_WAIT_DETECT == g_harvest_task_state) ?
                    HARVEST_TASK_GROUND1_RIGHT_GRAB_START :
                    HARVEST_TASK_GROUND2_RIGHT_GRAB_START;
            }
            else if (harvest_detect_timeout_expired())
            {
                g_harvest_task_state =
                    (HARVEST_TASK_GROUND1_RIGHT_WAIT_DETECT == g_harvest_task_state) ?
                    HARVEST_TASK_GROUND1_LEFT_MOVE_START :
                    HARVEST_TASK_GROUND2_LEFT_MOVE_START;
            }
            break;

        case HARVEST_TASK_GROUND1_RIGHT_GRAB_START:
        case HARVEST_TASK_GROUND2_RIGHT_GRAB_START:
            harvest_ground_grab_right_start();
            g_harvest_task_state =
                (HARVEST_TASK_GROUND1_RIGHT_GRAB_START == g_harvest_task_state) ?
                HARVEST_TASK_GROUND1_RIGHT_GRABBING :
                HARVEST_TASK_GROUND2_RIGHT_GRABBING;
            break;

        case HARVEST_TASK_GROUND1_RIGHT_GRABBING:
        case HARVEST_TASK_GROUND2_RIGHT_GRABBING:
            RobotArm_GrabService(&g_ai_center_offset);
            if (!RobotArm_GrabIsBusy())
            {
                g_harvest_task_state =
                    (HARVEST_TASK_GROUND1_RIGHT_GRABBING == g_harvest_task_state) ?
                    HARVEST_TASK_GROUND1_LEFT_MOVE_START :
                    HARVEST_TASK_GROUND2_LEFT_MOVE_START;
            }
            break;

        case HARVEST_TASK_GROUND1_LEFT_MOVE_START:
        case HARVEST_TASK_GROUND2_LEFT_MOVE_START:
            harvest_ground_move_left_search();
            g_harvest_task_state =
                (HARVEST_TASK_GROUND1_LEFT_MOVE_START == g_harvest_task_state) ?
                HARVEST_TASK_GROUND1_LEFT_MOVING :
                HARVEST_TASK_GROUND2_LEFT_MOVING;
            break;

        case HARVEST_TASK_GROUND1_LEFT_MOVING:
        case HARVEST_TASK_GROUND2_LEFT_MOVING:
            if (!RobotArm_IsMoving())
            {
                harvest_detect_timeout_start();
                g_harvest_task_state =
                    (HARVEST_TASK_GROUND1_LEFT_MOVING == g_harvest_task_state) ?
                    HARVEST_TASK_GROUND1_LEFT_WAIT_DETECT :
                    HARVEST_TASK_GROUND2_LEFT_WAIT_DETECT;
            }
            break;

        case HARVEST_TASK_GROUND1_LEFT_WAIT_DETECT:
        case HARVEST_TASK_GROUND2_LEFT_WAIT_DETECT:
            if (harvest_target_is_stable())
            {
                g_harvest_task_state =
                    (HARVEST_TASK_GROUND1_LEFT_WAIT_DETECT == g_harvest_task_state) ?
                    HARVEST_TASK_GROUND1_LEFT_GRAB_START :
                    HARVEST_TASK_GROUND2_LEFT_GRAB_START;
            }
            else if (harvest_detect_timeout_expired())
            {
                g_harvest_task_state =
                    (HARVEST_TASK_GROUND1_LEFT_WAIT_DETECT == g_harvest_task_state) ?
                    HARVEST_TASK_GROUND1_HOME_START :
                    HARVEST_TASK_GROUND2_HOME_START;
            }
            break;

        case HARVEST_TASK_GROUND1_LEFT_GRAB_START:
        case HARVEST_TASK_GROUND2_LEFT_GRAB_START:
            harvest_ground_grab_left_start();
            g_harvest_task_state =
                (HARVEST_TASK_GROUND1_LEFT_GRAB_START == g_harvest_task_state) ?
                HARVEST_TASK_GROUND1_LEFT_GRABBING :
                HARVEST_TASK_GROUND2_LEFT_GRABBING;
            break;

        case HARVEST_TASK_GROUND1_LEFT_GRABBING:
        case HARVEST_TASK_GROUND2_LEFT_GRABBING:
            RobotArm_GrabService(&g_ai_center_offset);
            if (!RobotArm_GrabIsBusy())
            {
                g_harvest_task_state =
                    (HARVEST_TASK_GROUND1_LEFT_GRABBING == g_harvest_task_state) ?
                    HARVEST_TASK_GROUND1_HOME_START :
                    HARVEST_TASK_GROUND2_HOME_START;
            }
            break;

        case HARVEST_TASK_GROUND1_HOME_START:
        case HARVEST_TASK_GROUND2_HOME_START:
            harvest_ground_move_home();
            g_harvest_task_state =
                (HARVEST_TASK_GROUND1_HOME_START == g_harvest_task_state) ?
                HARVEST_TASK_GROUND1_HOMING :
                HARVEST_TASK_GROUND2_HOMING;
            break;

        case HARVEST_TASK_GROUND1_HOMING:
        case HARVEST_TASK_GROUND2_HOMING:
            if (!RobotArm_IsMoving())
            {
                g_harvest_task_state =
                    (HARVEST_TASK_GROUND1_HOMING == g_harvest_task_state) ?
                    HARVEST_TASK_DRIVE1_START :
                    HARVEST_TASK_DRIVE2_START;
            }
            break;

        case HARVEST_TASK_DRIVE1_START:
        case HARVEST_TASK_DRIVE2_START:
            Go_Distance_Init(HARVEST_DRIVE_BETWEEN_DISTANCE_CM);
            g_harvest_task_state =
                (HARVEST_TASK_DRIVE1_START == g_harvest_task_state) ?
                HARVEST_TASK_DRIVE1_DRIVING :
                HARVEST_TASK_DRIVE2_DRIVING;
            break;

        case HARVEST_TASK_DRIVE1_DRIVING:
            if (Stepping_Motor_DistanceService())
            {
                g_harvest_task_state = HARVEST_TASK_GROUND2_RIGHT_MOVE_START;
            }
            break;

        case HARVEST_TASK_DRIVE2_DRIVING:
            if (Stepping_Motor_DistanceService())
            {
                g_harvest_task_state = HARVEST_TASK_TREE_MOVE_START;
            }
            break;

        case HARVEST_TASK_TREE_MOVE_START:
            if (g_tree_grab_index < HARVEST_TREE_GRAB_COUNT)
            {
                harvest_tree_build_config(g_tree_grab_index, &g_tree_grab_config_work);
                harvest_tree_move_to_current_fixed_pose();
                g_harvest_task_state = HARVEST_TASK_TREE_MOVING;
            }
            else
            {
                g_harvest_task_state = HARVEST_TASK_DONE;
            }
            break;

        case HARVEST_TASK_TREE_MOVING:
            if (!RobotArm_IsMoving())
            {
                harvest_detect_timeout_start();
                g_harvest_task_state = HARVEST_TASK_TREE_WAIT_DETECT;
            }
            break;

        case HARVEST_TASK_TREE_WAIT_DETECT:
            if ((!harvest_tree_detect_ignore_elapsed()) && g_ai_center_offset.valid)
            {
                g_detect_valid_frames = 0U;
            }
            else if (harvest_tree_target_is_stable())
            {
                g_harvest_task_state = HARVEST_TASK_TREE_GRAB_START;
            }
            else if (harvest_tree_detect_timeout_expired())
            {
                harvest_tree_next_position();
            }
            break;

        case HARVEST_TASK_TREE_GRAB_START:
            RobotArm_TreeGrabStart(&g_tree_grab_config_work);
            g_harvest_task_state = HARVEST_TASK_TREE_GRABBING;
            break;

        case HARVEST_TASK_TREE_GRABBING:
            RobotArm_TreeGrabService(&g_ai_center_offset,
                                     harvest_detection_screen_area_percent(p_dets, num_dets));
            if (!RobotArm_TreeGrabIsBusy())
            {
                harvest_tree_next_position();
            }
            break;

        case HARVEST_TASK_DONE:
            /* 4 个位置抓完：机械臂归位（上电默认姿态，4s 平滑，爪子松开） */
            if (!g_harvest_done_homed)
            {
                g_harvest_done_homed = true;
                RobotArm_MoveToWristDownTime(HARVEST_TREE_HOME_BASE_US,
                                             HARVEST_TREE_HOME_UPPER_US,
                                             HARVEST_TREE_HOME_FOREARM_US,
                                             HARVEST_TREE_GRIPPER_OPEN_US,
                                             HARVEST_TREE_HOME_MOVE_MS);
            }
            break;

        default:
            break;
    }
}
