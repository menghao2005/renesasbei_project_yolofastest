/*
 * ai_center_offset.c
 *
 * 计算「显示区中心 (上半部分中心)」与「框到的水果中心」的 x/y 差值。
 */

#include "ai_center_offset.h"
#include "Uart9_Debug.h"
#include <stdio.h>
#include <string.h>

/* 显示区中心 (显示空间): 上半部分 480x640 的中心 */
#define DISPLAY_CENTER_X  ((float)(FB_CAM_W) / 2.0f)   /* = 240 */
#define DISPLAY_CENTER_Y  ((float)(FB_CAM_H) / 2.0f)   /* = 320 */

/* 全局最新结果 */
ai_center_offset_t g_ai_center_offset = {
    .dx = 0.0f, .dy = 0.0f, .box_cx = 0.0f, .box_cy = 0.0f,
    .cls = 0, .score = 0.0f, .valid = false
};

bool ai_center_offset_calc(const ai_detection_t *dets,
                           uint32_t              num_det,
                           ai_center_offset_t    *p_out)
{
    ai_center_offset_t res;
    memset(&res, 0, sizeof(res));

    if (dets == NULL || num_det == 0) {
        res.valid = false;
        g_ai_center_offset = res;
        if (p_out) {
            *p_out = res;
        }
        return false;
    }

    /* 取置信度最高的框 (一帧一个水果) */
    uint32_t best = 0;
    for (uint32_t i = 1; i < num_det; i++) {
        if (dets[i].score > dets[best].score) {
            best = i;
        }
    }

    /* VGA 原图坐标 -> 显示空间 (与 ai_draw_detections 完全一致) */
    float disp_x = (dets[best].x + dets[best].w * 0.5f) * FB_SCALE_X + FB_BOX_OFFSET_X;
    float disp_y = (dets[best].y + dets[best].h * 0.5f) * FB_SCALE_Y + FB_BOX_OFFSET_Y;

    res.box_cx = disp_x;
    res.box_cy = disp_y;
    res.dx     = disp_x - DISPLAY_CENTER_X;
    res.dy     = disp_y - DISPLAY_CENTER_Y;
    res.cls    = dets[best].cls;
    res.score  = dets[best].score;
    res.valid  = true;

    g_ai_center_offset = res;
    if (p_out) {
        *p_out = res;
    }
    return true;
}

void ai_center_offset_print(const ai_center_offset_t *p_off)
{
    const ai_center_offset_t *o = (p_off != NULL) ? p_off : &g_ai_center_offset;

    if (!o->valid) {
        DBG_LOG("[OFFSET] no detection\r\n");
        return;
    }

    const char *name = "?";
    if (o->cls < AI_OUT_NUM_CLS) {
        name = g_ai_class_names[o->cls];
    }

    /* dx>0 右, dy>0 下 */
    DBG_LOG("[OFFSET] %s score=%.2f dx=%+.1f dy=%+.1f (box_cx=%.1f box_cy=%.1f)\r\n",
           name, o->score, o->dx, o->dy, o->box_cx, o->box_cy);
}
