/*
 * ai_postprocess.c
 *
 * AI 后处理: 解码 NPU 输出的 int8 P4/P5 两分支 (YOLO-fastest anchor 解码),
 * 按类别做 NMS 去重, 并把 letterbox 坐标反变换回原始相机图像坐标;
 * 另含在 GLCDC framebuffer 上绘制检测框/中心十字标记/中文类别标签的函数。
 */

#define CN_FONT_IMPLEMENT
#include "cn_font.h"
#include "ai_postprocess.h"
#include "font_8x8.h"
#include <math.h>
#include <string.h>

/* 直接操作 GLCDC framebuffer (RGB565) 画框
 * 摄像头 VGA 640x480 经 Dave2D 缩放到 480x640 铺到 framebuffer 顶部,
 * 因此 ai_detection_t 的 VGA 坐标需经过缩放映射才对应 framebuffer 像素位置 */
extern uint8_t *gp_frame_buffer;
extern uint32_t g_hstride;   /* framebuffer 每行像素步长 */

/* FB_CAM_W/H, FB_SCALE_X/Y, FB_BOX_OFFSET_X/Y 已移至 ai_postprocess.h
 * (VGA 640x480 -> framebuffer 480x640 映射), 供多模块共享。 */
#define AI_DRAW_GEOMETRY_DEBUG  (1)

/* ===== 默认 anchor (可被覆盖, 需按训练配置核对) ===== */
static const float g_anchor_p4[AI_OUT_NUM_BOX * 2] = AI_ANCHOR_P4;  /* stride 16, 20x20 */
static const float g_anchor_p5[AI_OUT_NUM_BOX * 2] = AI_ANCHOR_P5;  /* stride 32, 10x10 */

/* 类别名 (fruit_6lei: hongjiao, li, nangua, pingguo, yangcong) */
const char *g_ai_class_names[AI_OUT_NUM_CLS] = {
    "红椒", "梨", "南瓜", "苹果", "洋葱"
};

#define AI_BOX_SHRINK_RATIO          (0.85f)
#define AI_BOX_SMOOTH_OLD_WEIGHT     (0.70f)
#define AI_BOX_RELOCK_CENTER_PX      (120.0f)

static ai_detection_t g_stable_det;
static bool g_stable_det_valid = false;

/* sigmoid */
static inline float ai_sigmoid(float x)
{
    return 1.0f / (1.0f + expf(-x));
}

static inline float ai_dequantize_int8(int8_t value, float scale, int32_t zero_point)
{
    return ((float)((int32_t) value - zero_point)) * scale;
}

/* 计算两个框 (左上角 x,y,w,h) 的 IoU */
static float ai_iou(float ax, float ay, float aw, float ah,
                    float bx, float by, float bw, float bh)
{
    float ax2 = ax + aw, ay2 = ay + ah;
    float bx2 = bx + bw, by2 = by + bh;

    float ix = (ax > bx) ? ax : bx;
    float iy = (ay > by) ? ay : by;
    float ix2 = (ax2 < bx2) ? ax2 : bx2;
    float iy2 = (ay2 < by2) ? ay2 : by2;

    float iw = ix2 - ix;
    float ih = iy2 - iy;
    if (iw <= 0.0f || ih <= 0.0f) {
        return 0.0f;
    }
    float inter = iw * ih;
    float area_a = aw * ah;
    float area_b = bw * bh;
    float uni = area_a + area_b - inter;
    if (uni <= 0.0f) {
        return 0.0f;
    }
    return inter / uni;
}

/*
 * 解码单个分支
 * @param p_out      该分支输出指针
 * @param grid       该分支 grid 边长 (p4=20, p5=10)
 * @param anchor     该分支 anchor 数组 (3*2)
 * @param p_info     letterbox 信息 (用于坐标映射)
 * @param dets       输出检测数组
 * @param p_num      输入输出: 当前检测总数
 * @param max_dets   数组容量
 */
static void ai_decode_branch(const int8_t *p_out,
                             int grid,
                             const float *anchor,
                             float output_scale,
                             int32_t output_zero_point,
                             const ai_letterbox_info_t *p_info,
                             ai_detection_t *dets,
                             uint32_t *p_num,
                             uint32_t max_dets)
{
    /* 免 sigmoid 的 logit 阈值：logit(p) = ln(p/(1-p))，与 ai_postprocess.h 里的 AI_POST_*_THRESHOLD 对应 */
    const float obj_logit_th = 0.2006707f;  /* logit(0.55) */
    const float cls_logit_th = 0.4054651f;  /* logit(0.60) */

    for (int h = 0; h < grid; h++) {
        for (int w = 0; w < grid; w++) {
            for (int anc = 0; anc < AI_OUT_NUM_BOX; anc++) {
                /*
                 * p_out 现在指向 raw NPU 输出，布局 [1, 3, 10, grid, grid]
                 *   (batch, anchor, attr, y, x)，即 raw[anc][attr][y][x]。
                 * 已省略 compute_sub_0001 的 Transpose，后处理直接读 raw，
                 * 映射（已用 Python 对拍验证）：final[w][h][anc][attr] == raw[anc][attr][h][w]。
                 * 属性顺序不变：[0]=tx [1]=ty [2]=tw [3]=th [4]=obj [5..9]=cls。
                 */
                const int   g2  = grid * grid;   /* attr 维 stride = grid^2 */
                const int8_t *raw = p_out + (anc * 10 * g2 + h * grid + w);

                /* objectness：先反量化到 logit 空间比较，免 sigmoid/expf */
                float obj_logit = ai_dequantize_int8(raw[4 * g2], output_scale, output_zero_point);
                if (obj_logit < obj_logit_th) {
                    continue;
                }

                /* class：sigmoid 单调，直接在 logit 空间找 max，位置不变 */
                float best_logit = ai_dequantize_int8(raw[5 * g2], output_scale, output_zero_point);
                int   best_c = 0;
                for (int c = 1; c < AI_OUT_NUM_CLS; c++) {
                    float cls_logit = ai_dequantize_int8(raw[(5 + c) * g2], output_scale, output_zero_point);
                    if (cls_logit > best_logit) {
                        best_logit = cls_logit;
                        best_c = c;
                    }
                }
                if (best_logit < cls_logit_th) {
                    continue;
                }

                /* 通过阈值后才算 sigmoid（每帧只有少数 cell 走到这里） */
                float obj_conf = ai_sigmoid(obj_logit);
                float best_cls = ai_sigmoid(best_logit);
                float prob = best_cls * obj_conf;
                if (prob < AI_POST_THRESHOLD) {
                    continue;
                }

                float sx = ai_sigmoid(ai_dequantize_int8(raw[0 * g2], output_scale, output_zero_point));
                float sy = ai_sigmoid(ai_dequantize_int8(raw[1 * g2], output_scale, output_zero_point));
                float tw = ai_dequantize_int8(raw[2 * g2], output_scale, output_zero_point);
                float th = ai_dequantize_int8(raw[3 * g2], output_scale, output_zero_point);

                /* 解码公式与原 float 路径一致 */
                float cx_m = (sx + (float)w) / (float)grid * (float)AI_INPUT_WIDTH;
                float cy_m = (sy + (float)h) / (float)grid * (float)AI_INPUT_HEIGHT;
                float w_m  = expf(tw) * anchor[anc * 2];
                float h_m  = expf(th) * anchor[anc * 2 + 1];

                /* 类别概率 = class sigmoid * objectness */
                {
                    int c = best_c;
/* letterbox 反变换: 减 pad, 除 scale -> 原图像素 (中心, 宽高) */
                    float cx_o = (cx_m - (float)p_info->pad_x) / p_info->scale;
                    float cy_o = (cy_m - (float)p_info->pad_y) / p_info->scale;
                    float w_o  = w_m / p_info->scale;
                    float h_o  = h_m / p_info->scale;

                    /* 中心 -> 左上角 */
                    float x0 = cx_o - w_o / 2.0f;
                    float y0 = cy_o - h_o / 2.0f;

                    /* 裁剪到原图范围 */
                    if (x0 < 0.0f) { w_o += x0; x0 = 0.0f; }
                    if (y0 < 0.0f) { h_o += y0; y0 = 0.0f; }
                    if (x0 + w_o > (float)p_info->src_width) {
                        w_o = (float)p_info->src_width - x0;
                    }
                    if (y0 + h_o > (float)p_info->src_height) {
                        h_o = (float)p_info->src_height - y0;
                    }
                    if (w_o <= 0.0f || h_o <= 0.0f) {
                        continue;
                    }

                    if (*p_num < max_dets) {
                        ai_detection_t *d = &dets[*p_num];
                        d->x = x0;
                        d->y = y0;
                        d->w = w_o;
                        d->h = h_o;
                        d->cls = (uint32_t)c;
                        d->score = prob;
                        (*p_num)++;
                    }
                }
            }
        }
    }
}

/* 按置信度降序排序 (插入排序, 检测数 < 20 足够快) */
static void ai_sort_by_score(ai_detection_t *dets, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        for (uint32_t j = i + 1; j < n; j++) {
            if (dets[j].score > dets[i].score) {
                ai_detection_t tmp = dets[i];
                dets[i] = dets[j];
                dets[j] = tmp;
            }
        }
    }
}

/* 非极大值抑制 (先按得分排序, 再按类别分别处理) */
static void ai_nms(ai_detection_t *dets, uint32_t *p_num)
{
    uint32_t n = *p_num;
    if (n > AI_POST_MAX_DETECT) {
        n = AI_POST_MAX_DETECT;
    }
    if (n <= 1) return;

    /* 1. 按置信度降序排序 */
    ai_sort_by_score(dets, n);

    /* 2. 用数组上标记计数做 NMS */
    uint8_t marks[AI_POST_MAX_DETECT];
    memset(marks, 0, n);

    for (uint32_t i = 0; i < n; i++) {
        if (marks[i]) continue;
        for (uint32_t j = i + 1; j < n; j++) {
            if (marks[j]) continue;
            if (dets[i].cls != dets[j].cls) continue;
            float iou = ai_iou(dets[i].x, dets[i].y, dets[i].w, dets[i].h,
                               dets[j].x, dets[j].y, dets[j].w, dets[j].h);
            if (iou > AI_POST_NMS_IOU) {
                marks[j] = 1;
            }
        }
    }

    /* 3. 压缩掉被抑制的框 (按得分排序保持优先) */
    uint32_t w = 0;
    for (uint32_t i = 0; i < n; i++) {
        if (!marks[i]) {
            if (w != i) {
                dets[w] = dets[i];
            }
            w++;
        }
    }
    *p_num = w;
}

static void ai_shrink_box_to_center(ai_detection_t *p_det)
{
    float cx;
    float cy;
    float new_w;
    float new_h;

    if (NULL == p_det) {
        return;
    }

    cx = p_det->x + p_det->w * 0.5f;
    cy = p_det->y + p_det->h * 0.5f;
    new_w = p_det->w * AI_BOX_SHRINK_RATIO;
    new_h = p_det->h * AI_BOX_SHRINK_RATIO;

    p_det->x = cx - new_w * 0.5f;
    p_det->y = cy - new_h * 0.5f;
    p_det->w = new_w;
    p_det->h = new_h;
}

static void ai_clip_box(ai_detection_t *p_det, const ai_letterbox_info_t *p_info)
{
    if ((NULL == p_det) || (NULL == p_info)) {
        return;
    }

    if (p_det->x < 0.0f) {
        p_det->w += p_det->x;
        p_det->x = 0.0f;
    }
    if (p_det->y < 0.0f) {
        p_det->h += p_det->y;
        p_det->y = 0.0f;
    }
    if ((p_det->x + p_det->w) > (float)p_info->src_width) {
        p_det->w = (float)p_info->src_width - p_det->x;
    }
    if ((p_det->y + p_det->h) > (float)p_info->src_height) {
        p_det->h = (float)p_info->src_height - p_det->y;
    }
}

static void ai_stabilize_top_detection(ai_detection_t *p_det, const ai_letterbox_info_t *p_info)
{
    float prev_cx;
    float prev_cy;
    float curr_cx;
    float curr_cy;
    float center_dx;
    float center_dy;
    float new_weight;

    if ((NULL == p_det) || (NULL == p_info)) {
        return;
    }

    ai_shrink_box_to_center(p_det);
    ai_clip_box(p_det, p_info);

    if (!g_stable_det_valid || (g_stable_det.cls != p_det->cls)) {
        g_stable_det = *p_det;
        g_stable_det_valid = true;
        return;
    }

    prev_cx = g_stable_det.x + g_stable_det.w * 0.5f;
    prev_cy = g_stable_det.y + g_stable_det.h * 0.5f;
    curr_cx = p_det->x + p_det->w * 0.5f;
    curr_cy = p_det->y + p_det->h * 0.5f;
    center_dx = curr_cx - prev_cx;
    center_dy = curr_cy - prev_cy;

    if ((fabsf(center_dx) > AI_BOX_RELOCK_CENTER_PX) ||
        (fabsf(center_dy) > AI_BOX_RELOCK_CENTER_PX)) {
        g_stable_det = *p_det;
        return;
    }

    new_weight = 1.0f - AI_BOX_SMOOTH_OLD_WEIGHT;
    g_stable_det.x = g_stable_det.x * AI_BOX_SMOOTH_OLD_WEIGHT + p_det->x * new_weight;
    g_stable_det.y = g_stable_det.y * AI_BOX_SMOOTH_OLD_WEIGHT + p_det->y * new_weight;
    g_stable_det.w = g_stable_det.w * AI_BOX_SMOOTH_OLD_WEIGHT + p_det->w * new_weight;
    g_stable_det.h = g_stable_det.h * AI_BOX_SMOOTH_OLD_WEIGHT + p_det->h * new_weight;
    g_stable_det.cls = p_det->cls;
    g_stable_det.score = p_det->score;

    ai_clip_box(&g_stable_det, p_info);
    *p_det = g_stable_det;
}

bool ai_postprocess(const int8_t *p4,
                    const int8_t *p5,
                    const ai_letterbox_info_t *p_info,
                    ai_detection_t *p_dets,
                    uint32_t max_dets,
                    uint32_t *p_num_det)
{
    if (p4 == NULL || p5 == NULL || p_info == NULL ||
        p_dets == NULL || p_num_det == NULL) {
        return false;
    }
    if (max_dets == 0) {
        *p_num_det = 0;
        return false;
    }

    uint32_t num = 0;

    /* p4: 20x20 分支 (stride 16, 较小目标) */
    ai_decode_branch(p4, AI_OUT_P4_GRID, g_anchor_p4,
                     0.26804847f, 67,
                     p_info, p_dets, &num, max_dets);

    /* p5: 10x10 分支 (stride 32, 较大目标) */
    ai_decode_branch(p5, AI_OUT_P5_GRID, g_anchor_p5,
                     0.24737312f, 69,
                     p_info, p_dets, &num, max_dets);

    /* NMS */
    if (num > 1) {
        ai_nms(p_dets, &num);
    }

    /* 只保留 Top-K 个最高分的框 (一帧只有一个水果) */
    if (num > AI_POST_TOP_K) {
        ai_sort_by_score(p_dets, num);
        num = AI_POST_TOP_K;
    }

    if (num > 0) {
        ai_stabilize_top_detection(&p_dets[0], p_info);
    }
    else {
        g_stable_det_valid = false;
    }

    *p_num_det = num;
    return true;
}

/*
 * 8x8 字体 (ASCII 32-126) 由公共头 font_8x8.h 提供单一定义, 此处不再重复。
 */

/* 在 fb 上画一个 8x8 字符, 支持 2x 缩放 */
static void ai_fb_char(uint16_t *fb, uint32_t stride,
                        int px, int py, char ch,
                        uint16_t fg, uint16_t bg, int scale)
{
    if ((uint8_t)ch < 32 || (uint8_t)ch > 126) return;
    if (scale < 1) scale = 1;
    const uint8_t *glyph = &font_8x8[((uint8_t)ch - 32) * 8];
    for (int r = 0; r < 8; r++) {
        uint8_t bits = glyph[r];
        for (int c = 0; c < 8; c++) {
            uint16_t color = (bits & 0x80) ? fg : bg;
            for (int sy = 0; sy < scale; sy++) {
                int fy = py + r * scale + sy;
                if (fy < 0) continue;
                for (int sx = 0; sx < scale; sx++) {
                    int fx = px + c * scale + sx;
                    if (fx < 0) continue;
                    fb[(uint32_t)fy * stride + (uint32_t)fx] = color;
                }
            }
            bits <<= 1;
        }
    }
}

/* 在 fb 上画字符串, 支持 2x 缩放 */
static void ai_fb_string(uint16_t *fb, uint32_t stride,
                          int px, int py, const char *str,
                          uint16_t fg, uint16_t bg, int scale)
{
    while (*str) {
        ai_fb_char(fb, stride, px, py, *str, fg, bg, scale);
        px += 8 * scale;
        str++;
    }
}

/* 在 GLCDC framebuffer (RGB565) 上画检测框 + 类别名 */
/* 16x16 Chinese char (cn_font.h) */
static void ai_fb_cn_char(uint16_t *fb, uint32_t stride, int px, int py,
                          uint16_t glyph_idx, uint16_t fg, uint16_t bg, int scale)
{
    const uint8_t *g = &g_cn_glyphs[glyph_idx * CN_FONT_BYTES];
    for (int gy = 0; gy < CN_FONT_H; gy++)
    {
        uint8_t hi = g[gy * 2];
        uint8_t lo = g[gy * 2 + 1];
        for (int gx = 0; gx < CN_FONT_W; gx++)
        {
            uint8_t bit = (gx < 8) ? (uint8_t) ((hi >> (7 - gx)) & 1U)
                                   : (uint8_t) ((lo >> (15 - gx)) & 1U);
            for (int dy = 0; dy < scale; dy++)
            {
                for (int dx = 0; dx < scale; dx++)
                {
                    int x = px + gx * scale + dx;
                    int y = py + gy * scale + dy;
                    if ((x < 0) || (x >= FB_CAM_W) || (y < 0) || (y >= FB_CAM_H))
                    {
                        continue;
                    }
                    fb[(uint32_t) y * stride + (uint32_t) x] = bit ? fg : bg;
                }
            }
        }
    }
}

/* Chinese UTF-8 string */
static void ai_fb_cn_string(uint16_t *fb, uint32_t stride, int px, int py,
                            const char *str, uint16_t fg, uint16_t bg, int scale)
{
    while (*str)
    {
        if ((uint8_t) *str < 0x80U)
        {
            str++;
            continue;
        }
        uint8_t b0 = (uint8_t) str[0];
        uint8_t b1 = (uint8_t) str[1];
        uint8_t b2 = (uint8_t) str[2];
        uint16_t idx = 0xFFFFU;
        for (uint16_t i = 0U; i < CN_FONT_COUNT; i++)
        {
            if ((g_cn_font_index[i].utf8[0] == b0) &&
                (g_cn_font_index[i].utf8[1] == b1) &&
                (g_cn_font_index[i].utf8[2] == b2))
            {
                idx = g_cn_font_index[i].idx;
                break;
            }
        }
        if (0xFFFFU != idx)
        {
            ai_fb_cn_char(fb, stride, px, py, idx, fg, bg, scale);
        }
        px += CN_FONT_W * scale;
        str += 3;
    }
}

void ai_draw_detections(const ai_detection_t *dets, uint32_t num_det)
{
    if (dets == NULL || gp_frame_buffer == NULL) {
        return;
    }

    uint16_t *fb = (uint16_t *)gp_frame_buffer;
    uint32_t stride = g_hstride;

    const uint16_t box_color[AI_OUT_NUM_CLS] = {
        AI_RGB565_RED, AI_RGB565_GREEN, AI_RGB565_BLUE,
        0xFFE0, 0x07FF   /* yellow, cyan for extra 2 classes */
    };

    const int line = 2;  /* 线宽(像素) */

#if AI_DRAW_GEOMETRY_DEBUG
    /* 屏幕/相机显示区中心标记: 480x600 中心的黄色十字 (几何校准用) */
    {
        const int cx = FB_CAM_W / 2;
        const int cy = FB_CAM_H / 2;
        const uint16_t marker = 0xFFE0;
        for (int d = -20; d <= 20; d++) {
            int x = cx + d;
            int y = cy + d;
            if (x >= 0 && x < FB_CAM_W) fb[(uint32_t)cy * stride + (uint32_t)x] = marker;
            if (y >= 0 && y < FB_CAM_H) fb[(uint32_t)y * stride + (uint32_t)cx] = marker;
        }
    }
#endif

    for (uint32_t i = 0; i < num_det; i++) {
        /* VGA 640x480 到 framebuffer 480x600 坐标映射 */
        float fx0 = dets[i].x * FB_SCALE_X;
        float fy0 = dets[i].y * FB_SCALE_Y;
        float fw  = dets[i].w * FB_SCALE_X;
        float fh  = dets[i].h * FB_SCALE_Y;
        int x0 = (int)(fx0 + 0.5f) + FB_BOX_OFFSET_X;
        int y0 = (int)(fy0 + 0.5f) + FB_BOX_OFFSET_Y;
        int x1 = (int)(fx0 + fw + 0.5f) + FB_BOX_OFFSET_X;
        int y1 = (int)(fy0 + fh + 0.5f) + FB_BOX_OFFSET_Y;

        /* 裁剪到 fb 上摄像头有效显示区域 (0~480, 0~600) */
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 > FB_CAM_W) x1 = FB_CAM_W;
        if (y1 > FB_CAM_H) y1 = FB_CAM_H;
        /* 控制条顶 (y600) 之上才显示相机：检测框不画进控制条 */
        if (y1 > 600) y1 = 600;
        if (x1 <= x0 || y1 <= y0) continue;

        uint16_t color = box_color[dets[i].cls % AI_OUT_NUM_CLS];

#if AI_DRAW_GEOMETRY_DEBUG
        /* 检测框中心标记: 品红色十字 (几何校准用) */
        {
            int cx = (x0 + x1) / 2;
            int cy = (y0 + y1) / 2;
            const uint16_t marker = 0xF81F;
            for (int d = -8; d <= 8; d++) {
                int x = cx + d;
                int y = cy + d;
                if (x >= 0 && x < FB_CAM_W) fb[(uint32_t)cy * stride + (uint32_t)x] = marker;
                if (y >= 0 && y < FB_CAM_H) fb[(uint32_t)y * stride + (uint32_t)cx] = marker;
            }
        }
#endif

        for (int t = 0; t < line; t++) {
            /* 上边 / 下边 */
            for (int x = x0; x < x1; x++) {
                fb[(y0 + t) * stride + x] = color;
                fb[(y1 - t - 1) * stride + x] = color;
            }
            /* 左边 / 右边 */
            for (int y = y0; y < y1; y++) {
                fb[y * stride + (x0 + t)] = color;
                fb[y * stride + (x1 - t - 1)] = color;
            }
        }

                /* 框上方标签 (中文 16x16 字模, 2x 放大, 黑底白字) */
        {
            const char *name = g_ai_class_names[dets[i].cls % AI_OUT_NUM_CLS];
            int name_w = 0;
            for (const char *p = name; *p; p += ((uint8_t) *p >= 0x80U) ? 3 : 1) name_w++;
            int label_w = name_w * 16 * 2;   /* 2x, 32px per char */
            int label_h = 16 * 2;             /* 32px high */
            int lx = x0;
            int ly = y0 - label_h - 2;       /* 2px above box */
            if (ly < 0) ly = 0;
            int fill_x = (lx < 0) ? 0 : lx;
            int fill_w = (lx + label_w > FB_CAM_W) ? (FB_CAM_W - fill_x) : (label_w - (fill_x - lx));
            if (fill_w > 0) {
                for (int dy = 0; dy < label_h; dy++) {
                    int fy = ly + dy;
                    if (fy < 0 || fy >= FB_CAM_H) continue;
                    memset(&fb[(uint32_t)fy * stride + (uint32_t)fill_x], 0, (size_t)fill_w * 2);
                }
            }
            ai_fb_cn_string(fb, stride, lx, ly, name, 0xFFFF, 0x0000, 2);
        }
    }
}
