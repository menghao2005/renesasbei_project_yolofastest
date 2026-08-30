/*
 * bottom_banner.h
 *
 * AUTO 界面底部 banner 绘制接口（480x160，固定画在 y640-800）。
 * 实现与素材存储（点阵字库/logo 图数据）见 bottom_banner.c。
 */

#ifndef BOTTOM_BANNER_H_
#define BOTTOM_BANNER_H_

#include <stdint.h>

/* 绘制底部 banner 到指定帧缓冲。
 * 参数 fb —— RGB565 帧缓冲首地址；stride —— 行跨度（像素数）。无返回值。 */
void bottom_banner_draw(uint16_t *fb, uint32_t stride);

#endif
