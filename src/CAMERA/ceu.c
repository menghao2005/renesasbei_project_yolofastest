/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/***********************************************************************************************************************
 * File Name    : ceu.c
 * Description  : Contains data structures and functions used in hal_entry.c.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2023 Renesas Electronics Corporation. All rights reserved.
 ***********************************************************************************************************************/

/*----------------------------------------------------------------------------------------------------------------------
 * 文件说明  : CEU(摄像头扩展单元)采集驱动封装。
 *             - 采集流程:ceu_init → ceu_capture_start → ceu_capture_wait(ceu_operation 一步封装);
 *             - 中断级 kick:FRAME_END 回调内立即启动下一帧并切换双缓冲(g_image_vga_sdram[2]),
 *               主循环只处理 g_ceu_completed_buf,AI 推理期间不丢帧;
 *             - 断流自愈:ceu_recover 关闭并重开 CEU 清除内部状态;
 *             - 同步极性尝试:ceu_apply_sync_try 把 HDPOL/VDPOL/HDSEL/VDSEL 组合写入 CAMCR;
 *             - 软件转换:yuv422_to_rgb888 / yuv422_to_rgb565(YUV422 → RGB,供显示/AI 使用)。
 *---------------------------------------------------------------------------------------------------------------------*/

#include "ceu.h"
//#include "ov7725.h"
#include "ov5640.h"
#include "graphics.h"
#include "mipi_dsi_ep.h"
#include "math.h"
#include "stdio.h"
#include "Uart9_Debug.h"

//#include "perf_counter/perf_counter.h"

/* External variable */

/* Global variable */

uint32_t g_image_width = RESET_VALUE;
uint32_t g_image_height = RESET_VALUE;
uint8_t * gp_image_buffer = NULL;
volatile bool g_capture_ready = false;
volatile ceu_event_t g_ceu_last_event = CEU_EVENT_NONE;
volatile uint32_t g_ceu_callback_count = 0;
volatile uint32_t g_ceu_frame_end_count = 0;
volatile uint32_t g_ceu_error_count = 0;
volatile uint32_t g_ceu_zero_event_count = 0;
volatile uint32_t g_ceu_sync_event_count = 0;
volatile uint32_t g_ceu_hd_event_count = 0;
volatile uint32_t g_ceu_vd_event_count = 0;
void YUVtoRGB565(uint8_t *yuv, uint16_t *rgb565, int width, int height);

typedef struct st_ceu_sync_try
{
    uint8_t hdpol;
    uint8_t vdpol;
    uint8_t hdsel;
    uint8_t vdsel;
} ceu_sync_try_t;

static const ceu_sync_try_t g_ceu_sync_tries[] =
{
    {0, 0, 0, 0},
    {0, 1, 0, 0},
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 0, 0, 1},
    {0, 1, 0, 1},
    {1, 0, 0, 1},
    {1, 1, 0, 1},
    {0, 0, 1, 0},
    {0, 1, 1, 0},
    {1, 0, 1, 0},
    {1, 1, 1, 0},
    {0, 0, 1, 1},
    {0, 1, 1, 1},
    {1, 0, 1, 1},
    {1, 1, 1, 1},
};

static uint32_t g_ceu_sync_try_index = 0;

/* ===== 中断级 kick（FRAME_END 回调里立即启动下一帧采集，延迟 0ms）=====
 * 主循环处理 g_ceu_completed_buf（回调刚采完的帧），下一帧采集由回调直接
 * R_CEU_CaptureStart——不再等主循环轮询 kick（55ms AI 处理期间不丢帧）。 */
extern uint8_t g_image_vga_sdram[2][VGA_WIDTH * VGA_HEIGHT * RGB565_BYTE_PER_PIXEL];  /* hal_entry.c 定义 */
static volatile uint8_t * s_capture_buf = NULL;           /* 当前正在采集的缓冲（上次 kick 的） */
volatile uint8_t * g_ceu_completed_buf = NULL;            /* 刚采完待处理的帧 */

/* 记录当前正在采集的缓冲地址(供 FRAME_END 回调做双缓冲切换),启动采集/恢复后由外部调用 */
void ceu_set_capture_buf(uint8_t * buf)
{
    s_capture_buf = buf;
}

/* 把 g_ceu_sync_try_index 对应的同步极性组合(HD/VD 极性 HDPOL/VDPOL 与选择位
 * HDSEL/VDSEL)写入 CAMCR 寄存器;组合变化时才打印一次,便于确认当前生效的同步配置 */
static void ceu_apply_sync_try(void)
{
    static uint32_t last_printed_index = 0xFFFFFFFFU;
    const ceu_sync_try_t * p_try = &g_ceu_sync_tries[g_ceu_sync_try_index];

    uint32_t camcr = R_CEU->CAMCR;
    camcr &= ~(R_CEU_CAMCR_HDPOL_Msk |
               R_CEU_CAMCR_VDPOL_Msk |
               R_CEU_CAMCR_HDSEL_Msk |
               R_CEU_CAMCR_VDSEL_Msk);
    camcr |= ((uint32_t) p_try->hdpol << R_CEU_CAMCR_HDPOL_Pos) & R_CEU_CAMCR_HDPOL_Msk;
    camcr |= ((uint32_t) p_try->vdpol << R_CEU_CAMCR_VDPOL_Pos) & R_CEU_CAMCR_VDPOL_Msk;
    camcr |= ((uint32_t) p_try->hdsel << R_CEU_CAMCR_HDSEL_Pos) & R_CEU_CAMCR_HDSEL_Msk;
    camcr |= ((uint32_t) p_try->vdsel << R_CEU_CAMCR_VDSEL_Pos) & R_CEU_CAMCR_VDSEL_Msk;
    R_CEU->CAMCR = camcr;

    if (last_printed_index != g_ceu_sync_try_index)
    {
        last_printed_index = g_ceu_sync_try_index;
        DBG_LOG("CEU sync try[%lu/%lu]: HDPOL=%u VDPOL=%u HDSEL=%u VDSEL=%u CAMCR=0x%08lX\r\n",
               (unsigned long) g_ceu_sync_try_index,
               (unsigned long) (sizeof(g_ceu_sync_tries) / sizeof(g_ceu_sync_tries[0])),
               p_try->hdpol,
               p_try->vdpol,
               p_try->hdsel,
               p_try->vdsel,
               (unsigned long) R_CEU->CAMCR);
    }
}

/* 同步尝试索引前进一步(循环遍历 16 种极性组合) */
static void ceu_advance_sync_try(void)
{
    g_ceu_sync_try_index++;
    if (g_ceu_sync_try_index >= (sizeof(g_ceu_sync_tries) / sizeof(g_ceu_sync_tries[0])))
    {
        g_ceu_sync_try_index = 0;
    }
}

/* 判断事件是否属于硬错误(CRAM 溢出、HD/VD 失配、VD 错误、总线防火墙) */
static bool ceu_event_is_error(ceu_event_t event)
{
    return (0U != (event & (CEU_EVENT_CRAM_OVERFLOW |
                            CEU_EVENT_HD_MISMATCH |
                            CEU_EVENT_VD_MISMATCH |
                            CEU_EVENT_VD_ERROR |
                            CEU_EVENT_FIREWALL)));
}

/* 判断事件是否属于同步缺失警告(HD/VD 丢失,此时帧可能仍在正常采集) */
static bool ceu_event_is_sync_warning(ceu_event_t event)
{
    return (0U != (event & (CEU_EVENT_HD_MISSING |
                            CEU_EVENT_VD_MISSING)));
}

/* 按位解析并打印 CEU 事件标志(超时/异常诊断用) */
static void ceu_print_event(ceu_event_t event)
{
    DBG_LOG("CEU event=0x%08lX", (uint32_t) event);

    if (event & CEU_EVENT_FRAME_END)     DBG_LOG(" FRAME_END");
    if (event & CEU_EVENT_HD)            DBG_LOG(" HD");
    if (event & CEU_EVENT_VD)            DBG_LOG(" VD");
    if (event & CEU_EVENT_CRAM_OVERFLOW) DBG_LOG(" CRAM_OVERFLOW");
    if (event & CEU_EVENT_HD_MISMATCH)   DBG_LOG(" HD_MISMATCH");
    if (event & CEU_EVENT_VD_MISMATCH)   DBG_LOG(" VD_MISMATCH");
    if (event & CEU_EVENT_VD_ERROR)      DBG_LOG(" VD_ERROR");
    if (event & CEU_EVENT_FIREWALL)      DBG_LOG(" FIREWALL");
    if (event & CEU_EVENT_HD_MISSING)    DBG_LOG(" HD_MISSING");
    if (event & CEU_EVENT_VD_MISSING)    DBG_LOG(" VD_MISSING");

    DBG_LOG("\r\n");
}

/* 恢复流程核心:关闭再重新打开 CEU 以清除内部卡死状态;
 * Open 会用配置默认值重写 CAMCR,故之后必须重新应用同步极性(见函数内注释) */
static void ceu_reopen_after_timeout(void)
{
    static uint32_t s_recover_count = 0U;
    fsp_err_t close_err = R_CEU_Close(&g_ceu_vga_ctrl);
    fsp_err_t open_err = FSP_ERR_NOT_OPEN;
    s_recover_count++;
    /* 打印降频：前 3 次全打（启动诊断），之后每 10 次打一次（防刷屏） */
    bool print_recover = (s_recover_count <= 3U) || (0U == (s_recover_count % 10U));
    if (print_recover)
    {
        DBG_LOG("CEU recover: close=%d\r\n", close_err);
    }

    if ((FSP_SUCCESS == close_err) || (FSP_ERR_NOT_OPEN == close_err))
    {
        open_err = R_CEU_Open(&g_ceu_vga_ctrl, &g_ceu_vga_cfg);
        if (print_recover)
        {
            DBG_LOG("CEU recover: open=%d\r\n", open_err);
        }
        /* Open 会用 cfg 默认 CAMCR 覆盖同步极性——必须重新应用 index 0
         * （实测有效极性），否则 recover 后同步错乱 → 帧不来 → 循环 recover 刷屏 */
        ceu_apply_sync_try();
    }
}

/* 断流自愈：主循环检测到长时间无帧时调用（只重开 CEU 清状态，不换同步极性——
 * index 0 是实测有效极性，换极性会掉帧率） */
void ceu_recover(void)
{
    ceu_reopen_after_timeout();
}
/*******************************************************************************************************************//**
 *  @brief      ceu vga callback function
 *  @param[in]  p_args
 *  @retval     None
 **********************************************************************************************************************/
void g_ceu_vga_callback (capture_callback_args_t * p_args)
{
    ceu_event_t event = (ceu_event_t) p_args->event;

    g_ceu_callback_count++;
    if (CEU_EVENT_NONE == event)
    {
        g_ceu_zero_event_count++;
    }
    else if (CEU_EVENT_HD == event)
    {
        /* Line sync diagnostic event. Keep the count only. */
        g_ceu_sync_event_count++;
        g_ceu_hd_event_count++;
    }
    else if (CEU_EVENT_VD == event)
    {
        /* Frame sync diagnostic event. Keep the count only. */
        g_ceu_sync_event_count++;
        g_ceu_vd_event_count++;
    }
    else
    {
        g_ceu_last_event = event;

        if (0U != (event & CEU_EVENT_FRAME_END))
        {
            g_ceu_frame_end_count++;
            g_capture_ready = true;
            /* 中断级 kick：FRAME_END 瞬间立即启动下一帧采集（延迟 0，赶上传感器
             * VSYNC——AUTO 55ms AI 处理不再导致 kick 延迟错过帧 → 满速 7.2fps）。
             * 双缓冲切换：completed = 刚采完的帧，下一块 = 另一块。 */
            g_ceu_completed_buf = s_capture_buf;
            s_capture_buf = (s_capture_buf == (volatile uint8_t *) &g_image_vga_sdram[0][0])
                            ? (volatile uint8_t *) &g_image_vga_sdram[1][0]
                            : (volatile uint8_t *) &g_image_vga_sdram[0][0];
            __DSB();
            (void) R_CEU_CaptureStart(&g_ceu_vga_ctrl, (uint8_t *) s_capture_buf);
        }
        else if (ceu_event_is_sync_warning(event))
        {
            /* Missing-HD/VD warnings can occur while a frame is still captured. */
            g_ceu_error_count++;
        }
        else if (ceu_event_is_error(event))
        {
            g_ceu_error_count++;
            g_capture_ready = true;
        }
    }
}

/*******************************************************************************************************************//**
 *  @brief      ceu init function
 *  @param[in]  p_instance : ceu instance pointer
 *  @param[in]  p_buffer : image buffer pointer
 *  @param[in]  width : width of image
 *  @param[in]  height : height of image
 *  @retval     FSP_SUCCESS   Upon successful operation
 *  @retval     Any Other Error code apart from FSP_SUCCES
 **********************************************************************************************************************/
fsp_err_t ceu_init(uint8_t * const p_buffer, uint32_t width, uint32_t height)
{
    /* Initialize CEU module with the configuration specified by CEU instance pointer */
    R_CEU_Open(&g_ceu_vga_ctrl, &g_ceu_vga_cfg);
    g_ceu_sync_try_index = 0U;
    ceu_apply_sync_try();


    /* Clean image buffer */
    memset(p_buffer, RESET_VALUE, width * height * RGB565_BYTE_PER_PIXEL);

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 *  @brief      轮询等待一帧采集完成
 *  @param[out] used_ms    : 实际等待的毫秒数(可为 NULL)
 *  @param[in]  timeout_ms : 超时时间(毫秒)
 *  @retval     FSP_SUCCESS       收到 FRAME_END,正常收帧
 *  @retval     FSP_ERR_TIMEOUT   超时无回调,打印诊断信息并重开 CEU 恢复
 *  @retval     FSP_ERR_ABORTED   被非 FRAME_END 事件终止
 **********************************************************************************************************************/
static fsp_err_t ceu_wait_for_capture_complete(uint32_t *used_ms, uint32_t timeout_ms)
{
    uint32_t wait_budget_ms = timeout_ms;

    while (true != g_capture_ready)
    {
        R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);
        if (0U == wait_budget_ms)
        {
            capture_status_t status = {0};
            fsp_err_t status_err = R_CEU_StatusGet(&g_ceu_vga_ctrl, &status);

            DBG_LOG(" ** CEU Callback event not received ** \r\n");
            DBG_LOG("CEU callbacks: frame_end=%lu errors=%lu\r\n",
                   (unsigned long) g_ceu_frame_end_count,
                   (unsigned long) g_ceu_error_count);
            DBG_LOG("CEU zero events: %lu\r\n", (unsigned long) g_ceu_zero_event_count);
            DBG_LOG("CEU sync events: %lu\r\n", (unsigned long) g_ceu_sync_event_count);
            DBG_LOG("CEU hd/vd events: hd=%lu vd=%lu\r\n",
                   (unsigned long) g_ceu_hd_event_count,
                   (unsigned long) g_ceu_vd_event_count);
            DBG_LOG("CEU status: err=%d state=%d data_size=%lu buffer=0x%08lX\r\n",
                   status_err,
                   status.state,
                   (unsigned long) status.data_size,
                   (unsigned long) status.p_buffer);
            DBG_LOG("CEU regs: CAMCR=0x%08lX CAPSR=0x%08lX CSTSR=0x%08lX CEIER=0x%08lX CETCR=0x%08lX\r\n",
                   (unsigned long) R_CEU->CAMCR,
                   (unsigned long) R_CEU->CAPSR,
                   (unsigned long) R_CEU->CSTSR,
                   (unsigned long) R_CEU->CEIER,
                   (unsigned long) R_CEU->CETCR);
            DBG_LOG("CEU size: CMCYR=0x%08lX CAPWR=0x%08lX CDWDR=0x%08lX CDSSR=0x%08lX\r\n",
                   (unsigned long) R_CEU->CMCYR,
                   (unsigned long) R_CEU->CAPWR,
                   (unsigned long) R_CEU->CDWDR,
                   (unsigned long) R_CEU->CDSSR);
            ceu_print_event(g_ceu_last_event);
            /* 不 advance 极性：index 0（HDPOL=0 VDPOL=0）是实测有效极性（队友基线 7.2fps），
             * 换到 index 1+ 会导致出帧率掉到 3.15fps（VDPOL=1 同步慢）。只 reopen 清状态。 */
            ceu_reopen_after_timeout();
            if (NULL != used_ms)
            {
                *used_ms = timeout_ms;
            }
            return FSP_ERR_TIMEOUT;
        }

        wait_budget_ms--;
    }

    if (0U == (g_ceu_last_event & CEU_EVENT_FRAME_END))
    {
        DBG_LOG(" ** CEU capture stopped by non-frame event ** \r\n");
        ceu_print_event(g_ceu_last_event);
        DBG_LOG("CEU callbacks: hd=%lu vd=%lu zero=%lu frame_end=%lu errors=%lu\r\n",
               (unsigned long) g_ceu_hd_event_count,
               (unsigned long) g_ceu_vd_event_count,
               (unsigned long) g_ceu_zero_event_count,
               (unsigned long) g_ceu_frame_end_count,
               (unsigned long) g_ceu_error_count);
        g_capture_ready = false;
        /* 保持 index 0（有效极性），不 advance */
        if (NULL != used_ms)
        {
            *used_ms = timeout_ms - wait_budget_ms;
        }
        return FSP_ERR_ABORTED;
    }

    if (NULL != used_ms)
    {
        *used_ms = timeout_ms - wait_budget_ms;
    }

    g_capture_ready = false;
    return FSP_SUCCESS;
}

/* 启动一次采集:清完成标志、按当前索引应用同步极性,再调用底层启动 API */
fsp_err_t ceu_capture_start(uint8_t * const p_buffer)
{
    g_capture_ready = false;
    g_ceu_last_event = CEU_EVENT_NONE;
    /* 每帧轮换同步极性（队友基线行为，实测 8fps）：
     * v3 曾改为固定 index 0 → 出帧率掉到 3.65fps（待验证是否极性所致）。
     * 轮换让 CEU 每帧重新配置同步极性，与队友 (6).zip 行为一致。 */
    ceu_apply_sync_try();

    return R_CEU_CaptureStart(&g_ceu_vga_ctrl, p_buffer);
}

/* 阻塞等待当前采集完成(封装内部轮询函数) */
fsp_err_t ceu_capture_wait(uint32_t *used_ms, uint32_t timeout_ms)
{
    return ceu_wait_for_capture_complete(used_ms, timeout_ms);
}

/*******************************************************************************************************************//**
 *  @brief      ceu operation function
 *  @param[in]  p_instance : ceu instance pointer
 *  @param[in]  p_buffer : image buffer pointer
 *  @param[in]  width : width of image
 *  @param[in]  height : height of image
 *  @retval     FSP_SUCCESS   Upon successful operation
 *  @retval     Any Other Error code apart from FSP_SUCCES
 **********************************************************************************************************************/
//fsp_err_t ceu_operation (uint8_t * const p_buffer, uint32_t width, uint32_t height)
fsp_err_t ceu_operation (uint8_t * const p_buffer, uint32_t *used_ms)
{
    fsp_err_t err = FSP_SUCCESS;
    uint32_t callback_count_before = g_ceu_callback_count;

    /* Print capture operation start */
//    APP_PRINT("\r\nImage Capturing Operation started\r\n");

    /* Start capture image and store it in the buffer specified by image buffer pointer */
//    ceu_apply_sync_try();

    err = ceu_capture_start(p_buffer);
    if (FSP_SUCCESS != err)
    {
        DBG_LOG(" ** R_CEU_CaptureStart API FAILED: %d ** \r\n", err);
        return err;
    }
    err = ceu_capture_wait(used_ms, 2000U);
    if (FSP_SUCCESS != err)
    {
        DBG_LOG("CEU callbacks: before=%lu now=%lu frame_end=%lu errors=%lu\r\n",
               (unsigned long) callback_count_before,
               (unsigned long) g_ceu_callback_count,
               (unsigned long) g_ceu_frame_end_count,
               (unsigned long) g_ceu_error_count);
    }

    return err;
}

// YUV422 non-swapped data format : Y0 U0 Y1 V2 Y2 U2 Y3 V4 Y4 U4 Y5 V6 Y6 U6 Y7…
// YUV422 swapped data format     : U0 Y0 V0 Y1 U2 Y2 V2 Y3 U4 Y4 V4 Y5 U6 Y6 V6…

//*****************************//
// Pixel Number | Pixel Values //
//      0       | 0Y0V0        //
//      1       | U0Y1V0       //
//      2       | U2Y2V2       //
//      3       | U2Y3V2       //
//      4       | U4Y4V4       //
//     ...      |  ...         //
//*****************************//
#define RANGE_LIMIT(x)        (x > 255 ? 255 : (x < 0 ? 0 : x))

/* YUV422(swapped 格式 U0 Y0 V0 Y1...)→ RGB888(32 位)软件转换。
 * 系数用移位近似浮点:103/256≈0.403、88/256≈0.344、183/256≈0.714、198/256≈0.770,
 * 即 R=Y+1.403V'、G=Y-0.344U'-0.714V'、B=Y+1.770U'。U/V 为相邻两像素共用,
 * 故每处理完两个像素 U/V 指针才各前进 4 字节(columns 奇数时)。 */
void yuv422_to_rgb888(const void* inbuf, void* outbuf, uint16_t width, uint16_t height)
{
    uint32_t rows, columns;
    int32_t  y, u, v;

    int32_t  r8, g8, b8;
    uint8_t  *yuv_buf;
    uint32_t *rgb_buf = (uint32_t *) outbuf;
    uint32_t y_pos,u_pos,v_pos;

    yuv_buf = (uint8_t *)inbuf;
    uint32_t x_start, y_start;
    int32_t temp;

    uint32_t rgb888_pixel_data = 0;


    x_start = 0;
    y_start = 0;

    // YUV422 swapped data format : U0 Y0 V0 Y1 U2 Y2 V2 Y3 U4 Y4 V4 Y5 U6 Y6 V6…
    y_pos = 1;
    u_pos = 0;
    v_pos = 2;

    for (rows = 0; rows < height; rows++)
    {
        for (columns = 0; columns < width; columns++)
        {
            // Extract pixel Y U V byte from buffer
            y = yuv_buf[y_pos];
            u = yuv_buf[u_pos] - 128;
            v = yuv_buf[v_pos] - 128;

            //   Formula to Convert YUV422 to RGB888
            //   R = Y + 1.403V'
            //   G = Y - 0.344U' - 0.714V'
            //   R = Y + 1.770U'

            // R conversion
            temp = (int32_t) ( y + v + ( (v * 103) >> 8 ) ) ;
            r8 = (int32_t) RANGE_LIMIT( temp );

            // G Conversion
            temp = (int32_t) ( y - ( (u * 88) >> 8 ) - ( (v * 183) >> 8 ) );
            g8 = (int32_t) RANGE_LIMIT( temp );

            // B Conversion
            temp = (int32_t)  ( y + u + ( (u * 198) >> 8 ) );
            b8 = (int32_t) RANGE_LIMIT( temp );

            // RGB rearrange & merge back into RGB888 pixel
            rgb888_pixel_data = (uint32_t) ( ( r8 << 16 ) | ( g8 << 8 ) | ( b8 ) );

            // Display pixel directly into the screen working buffer
            rgb_buf[ ( ( rows + y_start ) * width ) + ( columns + x_start) ] = rgb888_pixel_data;

            rgb888_pixel_data = 0;
            // Move to next pixel
            y_pos += 2;

            // Move to next set of UV
            if (columns & 0x01)
            {
                u_pos += 4;
                v_pos += 4;
            }
        }
    }

}

/* YUV422(swapped)→ RGB565:转换公式与上方 RGB888 版本相同,
 * 仅最后按 R5/G6/B5 位域打包为 16 位像素(GLCDC 显存格式) */
void yuv422_to_rgb565(const void* inbuf, void* outbuf, uint16_t width, uint16_t height)
{
    uint32_t rows, columns;
    int32_t  y, u, v, r, g, b;
    uint8_t  *yuv_buf;
    uint16_t *rgb_buf = (uint16_t *) outbuf;
    uint32_t y_pos,u_pos,v_pos;

    yuv_buf = (uint8_t *)inbuf;
    uint32_t x_start, y_start;
    int32_t temp;
    uint16_t pixel_data;


    x_start = 0;
    y_start = 0;

    y_pos = 1;//1; // 0 1
    u_pos = 0;//0; // 1 0
    v_pos = 2;//2; // 3 2

    for (rows = 0; rows < height; rows++)
    {
        for (columns = 0; columns < width; columns++)
        {
            // Extract pixel Y U V byte from buffer
            y = yuv_buf[y_pos];
            u = yuv_buf[u_pos] - 128;
            v = yuv_buf[v_pos] - 128;

            //   Formula to Convert YUV422 to RGB888
            //   R = Y + 1.403V'
            //   G = Y - 0.344U' - 0.714V'
            //   R = Y + 1.770U'

            // R conversion
            temp = (int32_t) ( y + v + ( (v * 103) >> 8 ) ) ;
            r = (int32_t) RANGE_LIMIT( temp );

            // G Conversion
            temp = (int32_t) ( y - ( (u * 88) >> 8 ) - ( (v * 183) >> 8 ) );
            g = (int32_t) RANGE_LIMIT( temp );

            // B Conversion
            temp = (int32_t)  ( y + u + ( (u * 198) >> 8 ) );
            b = (int32_t) RANGE_LIMIT( temp );

            // RGB rearrange & merge back into RGB565 pixel
            pixel_data = (uint16_t) ( ( (r & 0xF8) << 8 ) | ( (g & 0xFC) << 3 ) | ( (b & 0xF8) >> 3 ) );

            // Display pixel directly into the screen working buffer
            rgb_buf[ ( ( rows + y_start ) * width ) + ( columns + x_start) ] = pixel_data;

            // Move to next pixel
            y_pos += 2;

            // Move to next set of UV
            if (columns & 0x01)
            {
                u_pos += 4;
                v_pos += 4;
            }
        }
    }

}
/* 另一版 YUV422→RGB565:BT.601 有限范围公式(Y 先减 16、U/V 减 128 后乘系数),
 * 一次处理相邻两个像素(共用一对 U/V),结果为 RGB565 */
void YUVtoRGB565(uint8_t *yuv, uint16_t *rgb565, int width, int height) {
    int i, j;
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j += 2) {
                // 从YUV数据中提取每个像素的Y、U和V值
                uint8_t y0 = yuv[i * width * 2 + j * 2];
                uint8_t u = yuv[i * width * 2 + j * 2 + 1];
                uint8_t y1 = yuv[i * width * 2 + j * 2 + 2];
                uint8_t v = yuv[i * width * 2 + j * 2 + 3];

                // YUV转换为RGB
                int c0 = y0 - 16;
                int c1 = y1 - 16;
                int d = u - 128;
                int e = v - 128;

                uint16_t r0 = (298 * c0 + 409 * e + 128) >> 8;
                uint16_t g0 = (298 * c0 - 100 * d - 208 * e + 128) >> 8;
                uint16_t b0 = (298 * c0 + 516 * d + 128) >> 8;

                uint16_t r1 = (298 * c1 + 409 * e + 128) >> 8;
                uint16_t g1 = (298 * c1 - 100 * d - 208 * e + 128) >> 8;
                uint16_t b1 = (298 * c1 + 516 * d + 128) >> 8;

                // Clamp values
                if (r0 > 0x1F) r0 = 0x1F;
                if (g0 > 0x3F) g0 = 0x3F;
                if (b0 > 0x1F) b0 = 0x1F;

                if (r1 > 0x1F) r1 = 0x1F;
                if (g1 > 0x3F) g1 = 0x3F;
                if (b1 > 0x1F) b1 = 0x1F;

                // 将RGB值组合成RGB565格式并保存
                rgb565[i * width + j] = (r0 << 11) | (g0 << 5) | b0;
                rgb565[i * width + j + 1] = (r1 << 11) | (g1 << 5) | b1;
            }
        }
}


/* 摄像头信号探针:GPIO 轮询 20000 次统计 VIO_CLK(P414)/HD(P415)/VD(P708)的高电平
 * 占比与翻转次数,并读取引脚 PFS 复用配置,用于排查排线接触/引脚复用等硬件问题 */
void camera_signal_probe(void)
{
    enum { SAMPLE_COUNT = 20000 };

    bsp_io_level_t clk = BSP_IO_LEVEL_LOW;
    bsp_io_level_t hd = BSP_IO_LEVEL_LOW;
    bsp_io_level_t vd = BSP_IO_LEVEL_LOW;
    bsp_io_level_t last_clk = BSP_IO_LEVEL_LOW;
    bsp_io_level_t last_hd = BSP_IO_LEVEL_LOW;
    bsp_io_level_t last_vd = BSP_IO_LEVEL_LOW;
    uint32_t clk_high = 0;
    uint32_t hd_high = 0;
    uint32_t vd_high = 0;
    uint32_t clk_toggle = 0;
    uint32_t hd_toggle = 0;
    uint32_t vd_toggle = 0;

    R_IOPORT_PinRead(&g_ioport_ctrl, BSP_IO_PORT_04_PIN_14, &last_clk); /* CEU VIO_CLK */
    R_IOPORT_PinRead(&g_ioport_ctrl, BSP_IO_PORT_04_PIN_15, &last_hd);  /* CEU VIO_HD  */
    R_IOPORT_PinRead(&g_ioport_ctrl, BSP_IO_PORT_07_PIN_08, &last_vd);  /* CEU VIO_VD  */

    for (uint32_t i = 0; i < SAMPLE_COUNT; i++)
    {
        R_IOPORT_PinRead(&g_ioport_ctrl, BSP_IO_PORT_04_PIN_14, &clk);
        R_IOPORT_PinRead(&g_ioport_ctrl, BSP_IO_PORT_04_PIN_15, &hd);
        R_IOPORT_PinRead(&g_ioport_ctrl, BSP_IO_PORT_07_PIN_08, &vd);

        clk_high += (BSP_IO_LEVEL_HIGH == clk);
        hd_high += (BSP_IO_LEVEL_HIGH == hd);
        vd_high += (BSP_IO_LEVEL_HIGH == vd);

        if (clk != last_clk)
        {
            clk_toggle++;
            last_clk = clk;
        }

        if (hd != last_hd)
        {
            hd_toggle++;
            last_hd = hd;
        }

        if (vd != last_vd)
        {
            vd_toggle++;
            last_vd = vd;
        }

        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
    }

    DBG_LOG("Camera signal probe: samples=%u\r\n", SAMPLE_COUNT);
    DBG_LOG("  P414 VIO_CLK: high=%lu toggle=%lu\r\n", (unsigned long) clk_high, (unsigned long) clk_toggle);
    DBG_LOG("  P415 VIO_HD : high=%lu toggle=%lu\r\n", (unsigned long) hd_high, (unsigned long) hd_toggle);
    DBG_LOG("  P708 VIO_VD : high=%lu toggle=%lu\r\n", (unsigned long) vd_high, (unsigned long) vd_toggle);

    uint32_t p414_pfs = R_PFS->PORT[4].PIN[14].PmnPFS;
    uint32_t p415_pfs = R_PFS->PORT[4].PIN[15].PmnPFS;
    uint32_t p708_pfs = R_PFS->PORT[7].PIN[8].PmnPFS;
    DBG_LOG("  P414 PFS=0x%08lX PMR=%lu PSEL=0x%02lX PIDR=%lu\r\n",
           (unsigned long) p414_pfs,
           (unsigned long) R_PFS->PORT[4].PIN[14].PmnPFS_b.PMR,
           (unsigned long) R_PFS->PORT[4].PIN[14].PmnPFS_b.PSEL,
           (unsigned long) R_PFS->PORT[4].PIN[14].PmnPFS_b.PIDR);
    DBG_LOG("  P415 PFS=0x%08lX PMR=%lu PSEL=0x%02lX PIDR=%lu\r\n",
           (unsigned long) p415_pfs,
           (unsigned long) R_PFS->PORT[4].PIN[15].PmnPFS_b.PMR,
           (unsigned long) R_PFS->PORT[4].PIN[15].PmnPFS_b.PSEL,
           (unsigned long) R_PFS->PORT[4].PIN[15].PmnPFS_b.PIDR);
    DBG_LOG("  P708 PFS=0x%08lX PMR=%lu PSEL=0x%02lX PIDR=%lu\r\n",
           (unsigned long) p708_pfs,
           (unsigned long) R_PFS->PORT[7].PIN[8].PmnPFS_b.PMR,
           (unsigned long) R_PFS->PORT[7].PIN[8].PmnPFS_b.PSEL,
           (unsigned long) R_PFS->PORT[7].PIN[8].PmnPFS_b.PIDR);
}
