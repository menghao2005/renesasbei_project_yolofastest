#include "Asrpro.h"
#include "string.h"
#include "ui_control.h"

/* ============ 状态机常量 ============ */
#define ASR_RX_IDLE  0
#define ASR_RX_H1    1
#define ASR_RX_H2    2
#define ASR_RX_CMD   3
#define ASR_RX_DATA  4
#define ASR_RX_T1    5
#define ASR_RX_T2    6

static uint8_t            asr_rx_state   = ASR_RX_IDLE;
static uint8_t            asr_rx_buf[2]; /* [0]CMD [1]DATA */
static uint8_t            asr_rx_idx     = 0;

volatile asr_cmd_t  g_asr_cmd      = ASR_CMD_NONE;
volatile uint8_t    g_asr_cmd_data = 0;
volatile uint8_t    g_asr_cmd_ready = 0;



/* ---- UART 初始化（使用 RA8P1 的 g_uart0_asrpro） ---- */
void ASRPRO_Init(void)
{
    R_SCI_B_UART_Open(&g_uart0_asrpro_ctrl, &g_uart0_asrpro_cfg);
    asr_rx_state   = ASR_RX_IDLE;
    asr_rx_idx     = 0;
    g_asr_cmd      = ASR_CMD_NONE;
    g_asr_cmd_data = 0;
    g_asr_cmd_ready = 0;
}

void ASRPRO_SendMessage(const char * p_message)
{
    R_SCI_B_UART_Write(&g_uart0_asrpro_ctrl, (uint8_t *)p_message, strlen(p_message));
}

/* ---- 单字节状态机（在 UART 中断回调中调用） ---- */
void ASRPRO_RxByte(uint8_t byte)
{
    if (!ui_control_get_voice_enabled())
    {
        asr_rx_state = ASR_RX_IDLE;
        asr_rx_idx = 0;
        return;
    }

    switch (asr_rx_state)
    {
        case ASR_RX_IDLE:
            if (byte == ASR_FRAME_HEADER1) asr_rx_state = ASR_RX_H1;
            break;
        case ASR_RX_H1:
            if (byte == ASR_FRAME_HEADER2) { asr_rx_state = ASR_RX_H2; asr_rx_idx = 0; }
            else if (byte == ASR_FRAME_HEADER1) { asr_rx_state = ASR_RX_H1; }
            else { asr_rx_state = ASR_RX_IDLE; }
            break;
        case ASR_RX_H2:   asr_rx_buf[asr_rx_idx++] = byte; asr_rx_state = ASR_RX_CMD;  break;
        case ASR_RX_CMD:  asr_rx_buf[asr_rx_idx++] = byte; asr_rx_state = ASR_RX_DATA; break;
        case ASR_RX_DATA:
            asr_rx_state = (byte == ASR_FRAME_TAIL1) ? ASR_RX_T1 : ASR_RX_IDLE;
            break;
        case ASR_RX_T1:
            if (byte == ASR_FRAME_TAIL2)
            {
                g_asr_cmd       = (asr_cmd_t)asr_rx_buf[0];
                g_asr_cmd_data  = asr_rx_buf[1];
                g_asr_cmd_ready = 1;
            }
            asr_rx_state = ASR_RX_IDLE;
            break;
        default:
            asr_rx_state = ASR_RX_IDLE;
            break;
    }
}

/* ---- 主循环调用：解析命令并执行对应 UI 动作 ---- */
void ASRPRO_Process(void)
{
    if (!g_asr_cmd_ready) return;
    if (!ui_control_get_voice_enabled())
    {
        /* 关闭期间的命令和半包均不能在重新开启后补执行。 */
        g_asr_cmd_ready = 0;
        g_asr_cmd = ASR_CMD_NONE;
        asr_rx_state = ASR_RX_IDLE;
        asr_rx_idx = 0;
        return;
    }
    g_asr_cmd_ready = 0;

    switch (g_asr_cmd)
    {
        case ASR_CMD_START:
            if (ui_control_get_mode() == UI_MODE_AUTO && ui_control_get_power() == UI_POWER_OFF)
            {
                extern void ui_toggle_power(void);
                ui_toggle_power();
            }
            break;

        case ASR_CMD_STOP:
            if (ui_control_get_power() == UI_POWER_ON)
            {
                extern void ui_toggle_power(void);
                ui_toggle_power();
            }
            break;

        case ASR_CMD_RESUME:
            if (ui_control_get_power() == UI_POWER_LOCKED)
            {
                extern void ui_toggle_power(void);
                ui_toggle_power();
            }
            break;

        case ASR_CMD_MODE:
            {
                extern void ui_toggle_mode(void);
                ui_toggle_mode();
            }
            break;

        case ASR_CMD_AUTOGRAB:
            if (ui_control_get_mode() == UI_MODE_REMOTE)
            {
                ui_autograb_start();
            }
            break;

        case ASR_CMD_GRIP:
            if (ui_control_get_mode() == UI_MODE_REMOTE)
            {
                ui_gripper_grasp_voice();
            }
            break;

        case ASR_CMD_OPEN:
            if (ui_control_get_mode() == UI_MODE_REMOTE)
            {
                ui_gripper_open_voice();
            }
            break;

        case ASR_CMD_LIGHT_ON:
            /* 任意模式（AUTO/REMOTE）均可语音控制灯光 */
            if (!ui_control_get_light_on())
            {
                ui_light_toggle();
            }
            break;

        case ASR_CMD_LIGHT_OFF:
            /* 任意模式（AUTO/REMOTE）均可语音控制灯光 */
            if (ui_control_get_light_on())
            {
                ui_light_toggle();
            }
            break;

        default:
            break;
    }

    g_asr_cmd = ASR_CMD_NONE;
}

/* ---- UART 中断回调 ---- */
void g_uart0_asrpro_callback(uart_callback_args_t * p_args)
{
    if (p_args->event == UART_EVENT_RX_CHAR)
    {
        ASRPRO_RxByte((uint8_t)(p_args->data));
    }
}
