/*
 * Stepping_Motor.h — 步进电机(差速底盘左右驱动轮)驱动接口。
 * 提供前进/后退/原地转向/停车，以及按距离行驶(Go_Distance_Init + DistanceService 轮询)。
 */
#ifndef STEPPING_MOTOR_H
#define STEPPING_MOTOR_H

#include "hal_data.h"
#include <stdbool.h>

/* 电机侧别序号（与 Motor_Number 枚举值对应） */
#define left  (0)
#define right (1)

/* 单侧电机输出通道运行参数：
 * OC_Channel_Pulse —— 脉冲周期(逻辑节拍数, 1 节拍 = 0.1ms)；
 * OC_Channel_Duty  —— 占空比(%)；
 * Sum              —— 累计 STEP 沿数（中断里累加，用于里程统计）。 */
typedef struct st_oc_channel
{
    uint32_t OC_Channel_Pulse;
    uint32_t OC_Channel_Duty;
    volatile uint32_t Sum;
} OC_Channel;

/* 电机侧别：左轮 / 右轮 */
typedef enum e_motor_number
{
    LeftSide = 0,
    RightSide,
    NumMotors
} Motor_Number;

extern timer_info_t time_info;
extern OC_Channel   OC_channel[NumMotors];
extern int          left_standard;
extern int          right_standard;
extern int          Turn_Flag;
extern int          back_flag;
extern int match_vary;
/* 双轮持续前进/后退/原地左转/原地右转，Stopping 立即停止输出 */
void Stepping_Motor_Go_Init(void);
void Stepping_Motor_Left_Init(void);
void Stepping_Motor_Right_Init(void);
void Stepping_Motor_Back_Init(void);
void Stepping_Motor_Stopping_Init(void);
/* 设定行驶距离(cm)并启动前进；配合 DistanceService 轮询到点停车 */
void Go_Distance_Init(float distance_cm);
void Stepping_Motor_GoDistance_Init(uint32_t distance_mm);
/* 距离行驶轮询：到达目标步数自动停车并返回 true（须周期性调用） */
bool Stepping_Motor_DistanceService(void);
bool Stepping_Motor_DistanceIsBusy(void);
/* 运行速度/占空比调节与直行纠偏（左右轮基准一加一减） */
void Stepping_Motor_SetSpeed(uint32_t left_pulse, uint32_t right_pulse);
void Stepping_Motor_SetDuty(uint32_t left_duty, uint32_t right_duty);
void Direction_correction(int32_t correction);

#endif
