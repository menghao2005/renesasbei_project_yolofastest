/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * graphics.c
 *
 *  Created on: Sep 5, 2023
 *      Author: a5123412
 */

#include "hal_data.h"
#include "common_utils.h"
#include "Graphics/graphics.h"

#define LCD_BITS_PER_PIXEL        (16)//32
#define LCD_XSTRIDE_PHYS          (DISPLAY_BUFFER_STRIDE_PIXELS_INPUT0)
#define LCD_XSIZE_PHYS            (DISPLAY_HSIZE_INPUT0)
#define LCD_YSIZE_PHYS            (DISPLAY_VSIZE_INPUT0)
#define LCD_NUM_FRAMEBUFFERS      (2)

//extern d2_device *d2_handle;
d2_device *d2_handle;

d2_device ** _d2_handle_user = &d2_handle;
static d2_renderbuffer * renderbuffer;

/* Variables to store resolution information */
uint16_t g_hz_size, g_vr_size;

/* Variables used for buffer usage */
uint32_t g_buffer_size, g_hstride;
uint8_t * gp_single_buffer = NULL;
uint8_t * gp_double_buffer = NULL;
uint8_t * gp_frame_buffer = NULL;

void graphics_init(void)
{
    /* Get LCDC configuration */
    g_hz_size = (g_display_cfg.input[0].hsize);// 显示屏水平分辨率
    g_vr_size = (g_display_cfg.input[0].vsize);// 显示屏垂直分辨率
    g_hstride = (g_display_cfg.input[0].hstride);// 行跨度（每行字节数/像素数，考虑内存对齐）

    /* Initialize buffer pointers */
    // 计算单帧缓冲区总大小：分辨率 × 每个像素字节数（BYTES_PER_PIXEL，如RGB565为2字节）
    g_buffer_size = (uint32_t) (g_hstride * g_vr_size * BYTES_PER_PIXEL);
    // 单缓冲区首地址：指向LCDC配置的帧缓冲物理基地址（如DDR/TCM内存）
    gp_single_buffer = (uint8_t*) g_display_cfg.input[0].p_base;

    /* Double buffer for drawing color bands with good quality */
    // 双缓冲区首地址：单缓冲区末尾 + 单帧大小（实现双缓冲，避免渲染时画面撕裂）
    gp_double_buffer = gp_single_buffer + g_buffer_size;

    // Initialize D/AVE 2D driver
    // 打开D/AVE 2D设备（0为设备编号，瑞萨MCU通常只有1个2D加速器）
    *_d2_handle_user = d2_opendevice(0);
    // 初始化2D硬件（重置加速器、配置基础寄存器）
    d2_inithw(*_d2_handle_user, 0);
    // 配置帧缓冲区：绑定物理内存、分辨率、像素格式
    d2_framebuffer(*_d2_handle_user,
                    g_display_cfg.input[0].p_base,// 帧缓冲基地址
                   LCD_XSTRIDE_PHYS,// 物理行跨度（内存对齐后的行宽）
                   LCD_XSIZE_PHYS,// 水平物理分辨率
                   LCD_YSIZE_PHYS * LCD_NUM_FRAMEBUFFERS,// 垂直总高度（双缓冲则×2）
                   d2_mode_rgb565);
    // 清空帧缓冲区（初始化为黑色：0x000000）
    d2_clear(*_d2_handle_user, 0x000000);

    // Set various D2 parameters
    // 设置混合模式：源Alpha混合（常用作透明/半透明渲染）
    d2_setblendmode(*_d2_handle_user, d2_bm_alpha, d2_bm_one_minus_alpha);
    // 设置Alpha模式：常量Alpha（全局透明度）
    d2_setalphamode(*_d2_handle_user, d2_am_constant);
    // 设置全局Alpha值：UINT8_MAX（0xFF，完全不透明）
    d2_setalpha(*_d2_handle_user, UINT8_MAX);
    // 开启抗锯齿（渲染线条/图形时边缘更平滑）
    d2_setantialiasing(*_d2_handle_user, 1);
    // 设置线条端点样式：平头（d2_lc_butt，无额外延伸）
    d2_setlinecap(*_d2_handle_user, d2_lc_butt);
    // 设置线条连接样式：斜接（d2_lj_miter，尖角连接）
    d2_setlinejoin(*_d2_handle_user, d2_lj_miter);

    renderbuffer = d2_newrenderbuffer(*_d2_handle_user, 10, 10);
}

/*******************************************************************************************************************//**
 * Start a new display list, set the framebuffer and add a clear operation
 *
 * This function will automatically prepare an empty framebuffer.
 **********************************************************************************************************************/
void graphics_draw_frame(const void * pSrc, void * pDst, int PitchSrc, int WidthSrc, int HeightSrc)
{

    /* Set the new buffer to the current draw buffer */
    // 1. 帧缓冲区映射：将pDst（目标缓冲区）设置为当前渲染的帧缓冲区
    // - pDst 大概率是 SDRAM 地址（屏显帧缓冲通常放在大容量SDRAM）
    // - LCD_XSTRIDE_PHYS/LCD_XSIZE_PHYS 是屏物理分辨率参数，关联硬件层
    d2_framebuffer(*_d2_handle_user, pDst, LCD_XSTRIDE_PHYS, LCD_XSIZE_PHYS, LCD_YSIZE_PHYS, d2_mode_rgb565);

    // 2. 选择渲染缓冲区（指定渲染操作的目标缓冲区）
    d2_selectrenderbuffer(*_d2_handle_user, renderbuffer);
    //
    // Generate render operations
    //
    // 3. 设置渲染源：将pSrc（源数据）绑定为blit操作的源缓冲区
    //- pSrc 可能是 SRAM（小容量临时数据）
    d2_setblitsrc(*_d2_handle_user, (void *) pSrc, (d2_s32) PitchSrc, WidthSrc, HeightSrc, d2_mode_rgb565);

    // 4. 核心：2D硬件加速的块拷贝（Blit）
    // - 把pSrc的图像数据渲染/拷贝到pDst的指定区域
    // - 不是单纯memcpy，支持分辨率缩放、坐标偏移、像素格式转换（此处是rgb565）
    d2_blitcopy(*_d2_handle_user,
                WidthSrc,
                HeightSrc,
                0,
                0,
                (d2_width) ((480) << 4),
                (d2_width) ((800) << 4),
                0,
                0,
                0);

    /* End the current display list */
    // 5. 执行渲染指令：将渲染缓冲区的操作提交到硬件执行
    d2_executerenderbuffer(*_d2_handle_user, renderbuffer, 0);
    // 6. 刷新帧缓冲区：确保数据写入物理内存（SDRAM），供屏显控制器读取
    d2_flushframe(*_d2_handle_user);
}


