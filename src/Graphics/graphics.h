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

extern uint8_t * gp_single_buffer;
extern uint8_t * gp_double_buffer;
extern uint8_t * gp_frame_buffer;
extern uint16_t g_hz_size, g_vr_size;
extern uint32_t g_buffer_size, g_hstride;

void graphics_init(void);
void graphics_draw_frame(const void * pSrc, void * pDst, int PitchSrc, int WidthSrc, int HeightSrc);

#endif /* GRAPHICS_GRAPHICS_H_ */
