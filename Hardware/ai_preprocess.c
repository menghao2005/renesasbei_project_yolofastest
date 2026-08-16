/*
 * ai_preprocess.c
 *
 * RGB565 camera image -> letterbox resize -> RGB int8 NHWC [1,320,320,3]
 * Quantization for the current TFLite int8 model is simply q = pixel - 128.
 */

#include "ai_preprocess.h"
#include <stddef.h>
#include <string.h>

static inline uint8_t r5_to_r8(uint8_t v5)
{
    return (uint8_t) ((v5 * 527U + 23U) >> 6);
}

static inline uint8_t g6_to_g8(uint8_t v6)
{
    return (uint8_t) ((v6 * 259U + 33U) >> 6);
}

static inline int8_t u8_to_model_q(uint8_t v)
{
    return (int8_t) ((int16_t) v - 128);
}

static const int8_t * ai_preprocess_r5_table(void)
{
    static int8_t s_r5_q[32];
    static bool s_initialized = false;

    if (!s_initialized)
    {
        for (uint32_t i = 0; i < 32U; i++)
        {
            s_r5_q[i] = u8_to_model_q(r5_to_r8((uint8_t) i));
        }
        s_initialized = true;
    }

    return s_r5_q;
}

static const int8_t * ai_preprocess_g6_table(void)
{
    static int8_t s_g6_q[64];
    static bool s_initialized = false;

    if (!s_initialized)
    {
        for (uint32_t i = 0; i < 64U; i++)
        {
            s_g6_q[i] = u8_to_model_q(g6_to_g8((uint8_t) i));
        }
        s_initialized = true;
    }

    return s_g6_q;
}

static void ai_preprocess_rgb565_vga_fast(const uint8_t * p_rgb565,
                                          int8_t        * p_model_in,
                                          ai_letterbox_info_t * p_info)
{
    const int8_t * p_r5_q = ai_preprocess_r5_table();
    const int8_t * p_g6_q = ai_preprocess_g6_table();
    const int8_t pad_val = u8_to_model_q((uint8_t) AI_LETTERBOX_PAD);
    const uint32_t pad_rows = 40U;
    const uint32_t dst_row_bytes = AI_INPUT_WIDTH * AI_INPUT_CHANNELS;

    if (NULL != p_info)
    {
        p_info->scale = 0.5f;
        p_info->pad_x = 0U;
        p_info->pad_y = pad_rows;
        p_info->src_width = 640U;
        p_info->src_height = 480U;
    }

    memset(p_model_in, (uint8_t) pad_val, dst_row_bytes * pad_rows);
    memset(p_model_in + (dst_row_bytes * (pad_rows + 240U)),
           (uint8_t) pad_val,
           dst_row_bytes * pad_rows);

    for (uint32_t dy = 0; dy < 240U; dy++)
    {
        const uint16_t * p_src_row =
            (const uint16_t *) (const void *) (p_rgb565 + (dy * 2U * 640U * 2U));
        int8_t * p_dst = p_model_in + (dst_row_bytes * (dy + pad_rows));

        for (uint32_t dx = 0; dx < 320U; dx++)
        {
            uint16_t pixel = p_src_row[dx * 2U];

            p_dst[0] = p_r5_q[(pixel >> 11) & 0x1FU];
            p_dst[1] = p_g6_q[(pixel >> 5) & 0x3FU];
            p_dst[2] = p_r5_q[pixel & 0x1FU];
            p_dst += AI_INPUT_CHANNELS;
        }
    }
}

void ai_preprocess_rgb565(const uint8_t * p_rgb565,
                          uint32_t        src_width,
                          uint32_t        src_height,
                          int8_t        * p_model_in,
                          ai_letterbox_info_t * p_info)
{
    const uint32_t dst_w = AI_INPUT_WIDTH;
    const uint32_t dst_h = AI_INPUT_HEIGHT;

    if ((NULL == p_rgb565) || (NULL == p_model_in) || (0U == src_width) || (0U == src_height))
    {
        return;
    }

    if ((640U == src_width) && (480U == src_height))
    {
        ai_preprocess_rgb565_vga_fast(p_rgb565, p_model_in, p_info);
        return;
    }

    {
        float scale_w = (float) dst_w / (float) src_width;
        float scale_h = (float) dst_h / (float) src_height;
        float scale = (scale_w < scale_h) ? scale_w : scale_h;
        uint32_t scaled_w = (uint32_t) (((float) src_width) * scale + 0.5f);
        uint32_t scaled_h = (uint32_t) (((float) src_height) * scale + 0.5f);
        uint32_t pad_x;
        uint32_t pad_y;
        uint32_t inv_scale_q16;
        uint16_t x_map[AI_INPUT_WIDTH];
        const int8_t * p_r5_q = ai_preprocess_r5_table();
        const int8_t * p_g6_q = ai_preprocess_g6_table();
        const int8_t pad_val = u8_to_model_q((uint8_t) AI_LETTERBOX_PAD);

        if (scaled_w > dst_w)
        {
            scaled_w = dst_w;
        }
        if (scaled_h > dst_h)
        {
            scaled_h = dst_h;
        }

        pad_x = (dst_w - scaled_w) / 2U;
        pad_y = (dst_h - scaled_h) / 2U;

        if (NULL != p_info)
        {
            p_info->scale = scale;
            p_info->pad_x = pad_x;
            p_info->pad_y = pad_y;
            p_info->src_width = src_width;
            p_info->src_height = src_height;
        }

        inv_scale_q16 = (uint32_t) (65536.0f / scale + 0.5f);
        memset(p_model_in, (uint8_t) pad_val, AI_INPUT_COUNT);

        for (uint32_t dx = 0; dx < scaled_w; dx++)
        {
            uint32_t sx = (dx * inv_scale_q16 + 32768U) >> 16;
            if (sx >= src_width)
            {
                sx = src_width - 1U;
            }
            x_map[dx] = (uint16_t) sx;
        }

        for (uint32_t dy = 0; dy < scaled_h; dy++)
        {
            uint32_t sy = (dy * inv_scale_q16 + 32768U) >> 16;
            const uint16_t * p_src_row;
            int8_t * p_dst;

            if (sy >= src_height)
            {
                sy = src_height - 1U;
            }

            p_src_row = (const uint16_t *) (const void *) (p_rgb565 + (sy * src_width * 2U));
            p_dst = p_model_in + ((((dy + pad_y) * dst_w) + pad_x) * AI_INPUT_CHANNELS);

            for (uint32_t dx = 0; dx < scaled_w; dx++)
            {
                uint16_t pixel = p_src_row[x_map[dx]];

                p_dst[0] = p_r5_q[(pixel >> 11) & 0x1FU];
                p_dst[1] = p_g6_q[(pixel >> 5) & 0x3FU];
                p_dst[2] = p_r5_q[pixel & 0x1FU];
                p_dst += AI_INPUT_CHANNELS;
            }
        }
    }
}
