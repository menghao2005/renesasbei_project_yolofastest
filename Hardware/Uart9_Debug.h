#ifndef __UART9_DEBUG_H
#define __UART9_DEBUG_H

#include "hal_data.h"
#include "stdio.h"

void uart9_Init(void);

typedef enum
{
    Failure =0,
    Success =1
}Status; //先定义枚举类型

typedef struct
{
    volatile Status Send;
    volatile Status Receive;
}Flag; //定义枚举变量

#endif
