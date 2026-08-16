/*
 * ai_center_offset.h
 *
 * 计算「显示区中心」(上半部分中心) 与「框到的水果中心」的 x/y 坐标差值。
 *
 * 坐标空间: framebuffer 显示空间 (与 ai_draw_detections 画框时相同的映射)。
 *   上半部分显示区尺寸 = FB_CAM_W(480) x FB_CAM_H(640) (见 ai_postprocess.c)
 *   显示区中心 = (FB_CAM_W/2, FB_CAM_H/2) = (240, 320)
 *
 *   检测框中心先由 VGA 原图坐标经 FB_SCALE_X/Y 缩放并加 FB_BOX_OFFSET 映射到
 *   显示空间, 再减显示区中心得到差值:
 *     dx = box_center_x_display - display_center_x
 *     dy = box_center_y_display - display_center_y
 *
 *   dx > 0  -> 水果在中心右侧
 *   dy > 0  -> 水果在中心下方 (竖屏, y 向下为正)
 *
 * 依赖: ai_postprocess.h (ai_detection_t / FB_CAM_* / FB_SCALE_* / FB_BOX_OFFSET_*)
 */

#ifndef __AI_CENTER_OFFSET_H
#define __AI_CENTER_OFFSET_H

#include <stdint.h>
#include <stdbool.h>
#include "ai_postprocess.h"

/* 最近一次计算得到的中心差值 (显示空间像素, 浮点便于做 PID/控制) */
typedef struct {
    float   dx;             /* 水果中心 x - 显示区中心 x (右为正) */
    float   dy;             /* 水果中心 y - 显示区中心 y (下为正) */
    float   box_cx;         /* 水果框中心 x (显示空间) */
    float   box_cy;         /* 水果框中心 y (显示空间) */
    uint32_t cls;           /* 该水果类别 */
    float   score;          /* 该水果置信度 */
    bool    valid;          /* 本次是否有有效检测 (无检测时为 false) */
} ai_center_offset_t;

/* 最新一帧的差值结果 (供主循环/其他模块直接读取) */
extern ai_center_offset_t g_ai_center_offset;

/*
 * 根据检测结果计算「显示区中心 - 水果框中心」差值。
 * 取 num_det 中置信度最高的框 (与 AI_POST_TOP_K=1 一致, 一帧一个水果)。
 *
 * @param[in]  dets      ai_postprocess 输出 (VGA 原图坐标)
 * @param[in]  num_det   检测数量
 * @param[out] p_out     结果 (可为 NULL, 仅更新全局 g_ai_center_offset)
 * @return     true  有有效检测并计算成功; false 无检测 (p_out->valid=false)
 */
bool ai_center_offset_calc(const ai_detection_t *dets,
                           uint32_t              num_det,
                           ai_center_offset_t    *p_out);

/*
 * 把当前差值通过 printf 输出 (串口)。
 * 无检测时打印 "no detection"。
 */
void ai_center_offset_print(const ai_center_offset_t *p_off);

#endif /* __AI_CENTER_OFFSET_H */
