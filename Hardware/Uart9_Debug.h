#ifndef __UART9_DEBUG_H
#define __UART9_DEBUG_H

#include "hal_data.h"
#include "stdio.h"

/* === Debug printf 控制 ===
 * DEBUG_EN=1: 启用所有 DBG_LOG 输出（调试用）
 * DEBUG_EN=0: 关闭所有 DBG_LOG（比赛/正式版本）
 */
#ifndef DEBUG_EN
#define DEBUG_EN  (0)
#endif

#if DEBUG_EN
  #define DBG_LOG(...) printf(__VA_ARGS__)
#else
  #define DBG_LOG(...) do { } while (0)
#endif

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
