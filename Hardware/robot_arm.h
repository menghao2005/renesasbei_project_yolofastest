/*
 * robot_arm.h
 *
 *  Created on: 2026�?�?7�?
 *      Author: menghao2005
 *
 *  机械臂舵机控制接口（参�?STM32 �?Core/Src/robot_arm.c 的函数形态，
 *  底层改为瑞萨 RA8P1 �?GPT-PWM 实现）�?
 *
 *  【非阻塞】RobotArm_MoveTo / RobotArm_MoveToTime 仅登记一次动作并立即返回�?
 *  实际插值由主循环周期性调�?RobotArm_Update() 推进，期间不阻塞 AI 推理�?
 *
 *  角度�?脉冲宽度(us)"表示：标准舵�?0.5ms(500us) ~ 2.5ms(2500us)�?
 *  中位 1.5ms(1500us)�?
 */

#ifndef __ROBOT_ARM_H__
#define __ROBOT_ARM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "ai_center_offset.h"

/* 舵机脉冲宽度范围(us) */
#define ROBOT_ARM_SERVO_MIN_US      500U
#define ROBOT_ARM_SERVO_MAX_US      2500U
/* 默认动作时间(ms) */
#define ROBOT_ARM_DEFAULT_MOVE_MS   1500U
/* 插值刷新步�?ms) */
#define ROBOT_ARM_UPDATE_PERIOD_MS  20U

/* 关节顺序：base / upper_arm / forearm / wrist / gripper */
void RobotArm_Init(void);
void RobotArm_MoveTo(uint16_t base,
                     uint16_t upper_arm,
                     uint16_t forearm,
                     uint16_t wrist,
                     uint16_t gripper);
void RobotArm_MoveToTime(uint16_t base,
                         uint16_t upper_arm,
                         uint16_t forearm,
                         uint16_t wrist,
                         uint16_t gripper,
                         uint16_t time_ms);
/* 非阻塞推进：须在main循环里周期性调用（建议每帧一次） */
uint16_t RobotArm_CalcWristDownUs(uint16_t upper_arm, uint16_t forearm);
uint16_t RobotArm_CalcWristForwardUs(uint16_t upper_arm, uint16_t forearm);
void RobotArm_MoveToWristDown(uint16_t base,
                              uint16_t upper_arm,
                              uint16_t forearm,
                              uint16_t gripper);
void RobotArm_MoveToWristDownTime(uint16_t base,
                                  uint16_t upper_arm,
                                  uint16_t forearm,
                                  uint16_t gripper,
                                  uint16_t time_ms);
bool RobotArm_IsMoving(void);

/* 非阻塞抓取状态机：先移动到指定初始位，再由主循环周期性服务�?*/
void RobotArm_GrabStart(uint16_t base,
                        uint16_t upper_arm,
                        uint16_t forearm,
                        uint16_t gripper);
void RobotArm_GrabService(const ai_center_offset_t * p_offset);
bool RobotArm_GrabIsBusy(void);

typedef struct st_robot_arm_pose
{
    uint16_t base;
    uint16_t upper_arm;
    uint16_t forearm;
    uint16_t wrist;
    uint16_t gripper;
    uint16_t move_ms;
} robot_arm_pose_t;

typedef struct st_robot_arm_tree_grab_config
{
    robot_arm_pose_t fixed_pose;
    robot_arm_pose_t approach_pose;
    robot_arm_pose_t close_pose;
    robot_arm_pose_t retract_pose;
    robot_arm_pose_t return_pose;
    robot_arm_pose_t release_pose;
    float            align_deadband_px;
    uint32_t         align_stable_frames;
    uint16_t         align_step_us;
    uint16_t         align_move_ms;
    float            grab_area_percent;
} robot_arm_tree_grab_config_t;

/* Tree-fruit grab task. RobotArm_Grab* remains the ground-object task. */
void RobotArm_TreeGrabStart(const robot_arm_tree_grab_config_t * p_config);
void RobotArm_TreeGrabStartDefault(void);
void RobotArm_TreeGrabService(const ai_center_offset_t * p_offset, float target_area_percent);
bool RobotArm_TreeGrabIsBusy(void);
void RobotArm_Update(void);

/* 冻结/恢复机械臂：冻结时插值暂停（舵机停在当前位置），恢复时丢弃暂停期间的
 * 积压时间继续运动，不会跳变。由屏幕锁定(开关)功能调用。 */
void RobotArm_SetPaused(bool paused);

#ifdef __cplusplus
}
#endif

#endif /* __ROBOT_ARM_H__ */
