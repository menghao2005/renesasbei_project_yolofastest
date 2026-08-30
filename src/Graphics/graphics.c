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
/*----------------------------------------------------------------------------------------------------------------------
 * 文件说明  : D/AVE 2D(Dave2D)硬件加速图形封装。
 *             - graphics_init:解析 GLCDC 分辨率配置并初始化 2D 引擎(帧缓冲绑定、混合/抗锯齿参数);
 *             - graphics_draw_frame / graphics_blit_scale / graphics_blit_scale_region:
 *               硬件 blit(拷贝/缩放,16.4 定点尺寸),用于相机帧 → 显示帧的缩放搬运。
 *---------------------------------------------------------------------------------------------------------------------*/

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
uint8_t * gp_frame_buffer = NULL;

/* Dave2D 初始化:解析分辨率/缓冲配置,打开 2D 设备并绑定帧缓冲、配置渲染参数 */
void graphics_init(void)
{
    /* Get LCDC configuration */
    g_hz_size = (g_display_cfg.input[0].hsize);// 显示屏水平分辨率
    g_vr_size = (g_display_cfg.input[0].vsize);// 显示屏垂直分辨率
    g_hstride = (g_display_cfg.input[0].hstride);// 行跨度（每行字节数，考虑内存对齐）

    /* Initialize buffer pointers */
    // 计算单帧缓冲区总大小：分辨率 × 每个像素字节数（BYTES_PER_PIXEL，如RGB565为2字节）
    g_buffer_size = (uint32_t) (g_hstride * g_vr_size * BYTES_PER_PIXEL);
    // 单缓冲区首地址：指向LCDC配置的帧缓冲物理基地址（如DDR/TCM内存）
    gp_frame_buffer = (uint8_t *) g_display_cfg.input[0].p_base;

    /* Double buffer for drawing color bands with good quality */
    // 双缓冲区首地址：单缓冲区末尾 + 单帧大小（实现双缓冲，避免渲染时画面撕裂）

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

/*******************************************************************************************************************//**
 * Dave2D 硬件缩放 Blit: SrcWidth×SrcHeight → DstWidth×DstHeight, 写入 pDst.
 * pSrc 在 SRAM, pDst 在 SDRAM. Dave2D 使用独立总线主接口完成缩放+拷贝.
 **********************************************************************************************************************/
void graphics_blit_scale(const void * pSrc, int SrcWidth, int SrcHeight,
                          void * pDst, int DstWidth, int DstHeight,
                          int DstStride)
{
    /* 设置目标帧缓冲 (SDRAM) */
    d2_framebuffer(*_d2_handle_user, pDst, (d2_s32)DstStride, (d2_s32)DstWidth, (d2_s32)DstHeight, d2_mode_rgb565);

    /* 选择渲染缓冲区 */
    d2_selectrenderbuffer(*_d2_handle_user, renderbuffer);

    /* 设置源 (SRAM) */
    d2_setblitsrc(*_d2_handle_user, (void *)pSrc, (d2_s32)SrcWidth, (d2_s32)SrcWidth, (d2_s32)SrcHeight, d2_mode_rgb565);

    /* 硬件缩放 Blit: 源尺寸 → 目标尺寸 (16.4 定点数) */
    d2_blitcopy(*_d2_handle_user,
                (d2_s32)SrcWidth, (d2_s32)SrcHeight,
                0, 0,
                (d2_width)((uint32_t)DstWidth  << 4),
                (d2_width)((uint32_t)DstHeight << 4),
                0, 0, 0);

    /* 执行并刷新 */
    d2_executerenderbuffer(*_d2_handle_user, renderbuffer, 0);
    d2_flushframe(*_d2_handle_user);
}

/* 区域缩放 Blit:源图 (src_x,src_y,src_w,src_h) → 目标 (dst_x,dst_y,dst_w,dst_h),
 * 坐标均相对整屏帧缓冲;尺寸/偏移以 16.4 定点数传给硬件 */
void graphics_blit_scale_region(const void * pSrc, int SrcWidth, int SrcHeight,
                                int src_x, int src_y, int src_w, int src_h,
                                void * pDst, int DstStride, int DstWidth, int DstHeight,
                                int dst_x, int dst_y, int dst_w, int dst_h)
{
    /* 目标 framebuffer 基地址（全屏坐标系，偏移由 d2_blitcopy 给出） */
    d2_framebuffer(*_d2_handle_user, pDst, (d2_s32)DstStride, (d2_s32)DstWidth, (d2_s32)DstHeight, d2_mode_rgb565);
    d2_selectrenderbuffer(*_d2_handle_user, renderbuffer);
    d2_setblitsrc(*_d2_handle_user, (void *)pSrc, (d2_s32)SrcWidth, (d2_s32)SrcWidth, (d2_s32)SrcHeight, d2_mode_rgb565);

    /* 源区域 → 目标区域（16.4 定点） */
    d2_blitcopy(*_d2_handle_user,
                (d2_s32)src_w, (d2_s32)src_h,
                (d2_s32)src_x, (d2_s32)src_y,
                (d2_width)((uint32_t)dst_w << 4), (d2_width)((uint32_t)dst_h << 4),
                (d2_width)((uint32_t)dst_x << 4), (d2_width)((uint32_t)dst_y << 4), 0);

    d2_executerenderbuffer(*_d2_handle_user, renderbuffer, 0);
    d2_flushframe(*_d2_handle_user);
}


