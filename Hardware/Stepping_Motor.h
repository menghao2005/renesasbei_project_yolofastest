#ifndef STEPPING_MOTOR_H
#define STEPPING_MOTOR_H

#include "hal_data.h"
#include <stdbool.h>

#define left  (0)
#define right (1)

typedef struct st_oc_channel
{
    uint32_t OC_Channel_Pulse;
    uint32_t OC_Channel_Duty;
    volatile uint32_t Sum;
} OC_Channel;

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
void Stepping_Motor_Go_Init(void);
void Stepping_Motor_Left_Init(void);
void Stepping_Motor_Right_Init(void);
void Stepping_Motor_Back_Init(void);
void Stepping_Motor_Stopping_Init(void);
void Go_Distance_Init(float distance_cm);
void Stepping_Motor_GoDistance_Init(uint32_t distance_mm);
bool Stepping_Motor_DistanceService(void);
bool Stepping_Motor_DistanceIsBusy(void);
void Stepping_Motor_SetSpeed(uint32_t left_pulse, uint32_t right_pulse);
void Stepping_Motor_SetDuty(uint32_t left_duty, uint32_t right_duty);
void Direction_correction(int32_t correction);

#endif
