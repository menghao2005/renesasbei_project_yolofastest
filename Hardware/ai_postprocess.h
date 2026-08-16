/*
 * ai_postprocess.h
 *
 * Postprocess for the current RUHMI/TFLite int8 model (5-class):
 *   P4 output [1,20,20,3,10], scale = 0.26804847, zero_point = 67
 *   P5 output [1,10,10,3,10], scale = 0.24737312, zero_point = 69
 *
 * Channel order per anchor (10 channels):
 *   [0]=tx [1]=ty [2]=tw [3]=th [4]=objectness [5..9]=class scores
 */

#ifndef __AI_POSTPROCESS_H
#define __AI_POSTPROCESS_H

#include <stdint.h>
#include <stdbool.h>
#include "ai_preprocess.h"

#define AI_OUT_P4_GRID   (20)
#define AI_OUT_P5_GRID   (10)
#define AI_OUT_NUM_BOX   (3)
#define AI_OUT_NUM_CLS   (5)
#define AI_OUT_CHAN      (AI_OUT_NUM_BOX * (5 + AI_OUT_NUM_CLS))

#define AI_OUT_P4_STRIDE (16)
#define AI_OUT_P5_STRIDE (32)

#ifndef AI_ANCHOR_P4
#define AI_ANCHOR_P4  { 12, 18,  37, 49,  52, 132 }
#endif
#ifndef AI_ANCHOR_P5
#define AI_ANCHOR_P5  { 115, 73,  119, 199, 242, 238 }
#endif

#define AI_POST_THRESHOLD     (0.50f) // 总置信度阈值
#define AI_POST_OBJ_THRESHOLD (0.55f) // 物体置信度(objectness)阈值
#define AI_POST_CLS_THRESHOLD (0.60f) // 类别分数阈值
#define AI_POST_NMS_IOU       (0.45f) // NMS 去重阈值（一般不用动）
#define AI_POST_MAX_DETECT    (20)
#define AI_POST_TOP_K         (1)

#define AI_RGB565_RED         (0x1F << 11)
#define AI_RGB565_GREEN       (0x3F << 5)
#define AI_RGB565_BLUE        (0x1F << 0)

typedef struct {
    float x;
    float y;
    float w;
    float h;
    uint32_t cls;
    float score;
} ai_detection_t;

extern const char *g_ai_class_names[AI_OUT_NUM_CLS];

bool ai_postprocess(const int8_t           *p4,
                    const int8_t           *p5,
                    const ai_letterbox_info_t *p_info,
                    ai_detection_t        *p_dets,
                    uint32_t               max_dets,
                    uint32_t              *p_num_det);

void ai_draw_detections(const ai_detection_t *dets, uint32_t num_det);

#define FB_CAM_W          (480)
#define FB_CAM_H          (640)
#define FB_SCALE_X        ((float)FB_CAM_W / 640.0f)
#define FB_SCALE_Y        ((float)FB_CAM_H / 480.0f)
#define FB_BOX_OFFSET_X   (0)
#define FB_BOX_OFFSET_Y   (0)

#endif /* __AI_POSTPROCESS_H */
