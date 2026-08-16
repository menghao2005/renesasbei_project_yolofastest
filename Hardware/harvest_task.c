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

/* Set to 1 to run the full ground-right/left + drive sequence before tree grab. */
#define HARVEST_ENABLE_GROUND_SEQUENCE         (0U)

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

static const robot_arm_tree_grab_config_t g_tree_fruit_grab_config =
{
    .fixed_pose    = {1500U - 660U, 1600U,       2150U,        1740U, 900U,  ROBOT_ARM_DEFAULT_MOVE_MS},
    .approach_pose = {1500U - 760U, 1500U,       2200U,        1600U, 900U,  1000U},
    .close_pose    = {.gripper = 1400U, .move_ms = 1500U},
    .retract_pose  = {1500U - 760U, 1640U,       2400U,        1740U, 1400U, 1000U},
    .return_pose   = {1500U,        1640U,       2400U,        1740U, 1350U, 1500U},
    .release_pose  = {1500U,        1640U,       2400U,        1740U, 900U,  1500U},
    .align_deadband_px = 18.0f,
    .align_stable_frames = 2U,
    .align_step_us = 5U,
    .align_move_ms = 150U,
    .grab_area_percent = 65.0f,
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
    HARVEST_TASK_TREE_GRAB_START,
    HARVEST_TASK_TREE_GRABBING,
    HARVEST_TASK_DONE,
} harvest_task_state_t;

static harvest_task_state_t g_harvest_task_state;
static uint32_t g_detect_wait_start_count;
static uint32_t g_detect_valid_frames;

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
    g_harvest_task_state = HARVEST_TASK_TREE_GRAB_START;
#endif
    g_detect_wait_start_count = DWT_get_count();
    g_detect_valid_frames = 0U;
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
                g_harvest_task_state = HARVEST_TASK_TREE_GRAB_START;
            }
            break;

        case HARVEST_TASK_TREE_GRAB_START:
            RobotArm_TreeGrabStart(&g_tree_fruit_grab_config);
            g_harvest_task_state = HARVEST_TASK_TREE_GRABBING;
            break;

        case HARVEST_TASK_TREE_GRABBING:
            RobotArm_TreeGrabService(&g_ai_center_offset,
                                     harvest_detection_screen_area_percent(p_dets, num_dets));
            if (!RobotArm_TreeGrabIsBusy())
            {
                g_harvest_task_state = HARVEST_TASK_DONE;
            }
            break;

        case HARVEST_TASK_DONE:
        default:
            break;
    }
}
