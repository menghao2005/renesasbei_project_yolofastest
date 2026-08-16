/*
* Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * graphics.h
 *
 *  Created on: Sep 5, 2023
 *      Author: a5123412
 */

#ifndef GRAPHICS_GRAPHICS_H_
#define GRAPHICS_GRAPHICS_H_

#define BYTES_PER_PIXEL                              (2)
#define COLOR_BAND_COUNT                             (8)

extern uint8_t * gp_frame_buffer;
extern uint16_t g_hz_size, g_vr_size;
extern uint32_t g_buffer_size, g_hstride;

void graphics_init(void);
void graphics_draw_frame(const void * pSrc, void * pDst, int PitchSrc, int WidthSrc, int HeightSrc);
void graphics_blit_scale(const void * pSrc, int SrcWidth, int SrcHeight,
                          void * pDst, int DstWidth, int DstHeight,
                          int DstStride);
/* 分块缩放 blit：源区域 (src_x,src_y,src_w,src_h) → 目标区域 (dst_x,dst_y,dst_w,dst_h)。
 * pDst 为 framebuffer 基地址，目标偏移由 d2_blitcopy 的 dst 参数给出。
 * 用于 AUTO 界面分块显示相机画面（跳过按钮矩形，按钮不被覆盖 → 不闪）。 */
void graphics_blit_scale_region(const void * pSrc, int SrcWidth, int SrcHeight,
                                int src_x, int src_y, int src_w, int src_h,
                                void * pDst, int DstStride, int DstWidth, int DstHeight,
                                int dst_x, int dst_y, int dst_w, int dst_h);

#endif /* GRAPHICS_GRAPHICS_H_ */
