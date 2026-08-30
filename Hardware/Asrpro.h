/*
 * Asrpro.h
 *
 * ASRPRO 离线语音模块串口协议定义：
 *   帧格式为 6 字节固定长度（帧头 AA 55 + 命令 CMD + 数据 DATA + 帧尾 0D 0A），
 *   下方 asr_cmd_t 为语音命令码表。收发/解析/执行实现见 Asrpro.c。
 */
#ifndef ASRPRO_H
#define ASRPRO_H

#include "hal_data.h"
#include "r_sci_b_uart.h"

/* ============ 帧格式 ============
 * 帧头(2) + 命令(1) + 数据(1) + 帧尾(2) = 6 字节
 * AA 55 | CMD | DATA | 0D 0A
 * =================================== */
#define ASR_FRAME_HEADER1  0xAA
#define ASR_FRAME_HEADER2  0x55
#define ASR_FRAME_TAIL1    0x0D
#define ASR_FRAME_TAIL2    0x0A

/* ============ 命令定义 ============ */
typedef enum
{
    ASR_CMD_NONE      = 0x00,
    ASR_CMD_START     = 0x01,   /* 开始采摘 */
    ASR_CMD_STOP      = 0x02,   /* 停止当前动作 */
    ASR_CMD_RESUME    = 0x03,   /* 继续当前动作 */
    ASR_CMD_MODE      = 0x04,   /* 切换模式 */
    ASR_CMD_AUTOGRAB  = 0x05,   /* 执行下抓 */
    ASR_CMD_GRIP      = 0x06,   /* 执行抓取 */
    ASR_CMD_OPEN      = 0x07,   /* 松开 */
    ASR_CMD_LIGHT_ON  = 0x08,   /* 开灯 */
    ASR_CMD_LIGHT_OFF = 0x09,   /* 关灯 */
} asr_cmd_t;

/* ============ 全局变量 ============ */
extern volatile asr_cmd_t  g_asr_cmd;
extern volatile uint8_t    g_asr_cmd_data;
extern volatile uint8_t    g_asr_cmd_ready;

/* 接口说明：
 * ASRPRO_Init()        打开 g_uart0_asrpro 并复位接收状态机/命令缓存（上电调用一次）
 * ASRPRO_SendMessage() 向语音模块发送字符串（阻塞写）
 * ASRPRO_RxByte()      逐字节喂入帧解析状态机（仅限 UART 中断回调中调用）
 * ASRPRO_Process()     主循环调用：取出完整命令并执行对应 UI 动作（见 Asrpro.c） */
void ASRPRO_Init(void);
void ASRPRO_SendMessage(const char * p_message);
void ASRPRO_RxByte(uint8_t byte);
void ASRPRO_Process(void);

#endif
