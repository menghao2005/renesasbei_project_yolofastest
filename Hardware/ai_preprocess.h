/*
 * ai_preprocess.h
 *
 * Preprocess for the current RUHMI/TFLite int8 model:
 *   RGB565 camera image -> letterbox resize -> RGB int8 NHWC [1,320,320,3]
 *
 * Input quantization:
 *   scale = 0.00392157, zero_point = -128
 *   q = round(pixel / 255 / scale) + zero_point = pixel - 128
 */

#ifndef __AI_PREPROCESS_H
#define __AI_PREPROCESS_H

#include <stdint.h>
#include <stdbool.h>

#define AI_INPUT_WIDTH      (320)
#define AI_INPUT_HEIGHT     (320)
#define AI_INPUT_CHANNELS   (3)
#define AI_INPUT_COUNT      (AI_INPUT_CHANNELS * AI_INPUT_HEIGHT * AI_INPUT_WIDTH)

#define AI_LETTERBOX_PAD    (114)

typedef struct {
    float    scale;
    uint32_t pad_x;
    uint32_t pad_y;
    uint32_t src_width;
    uint32_t src_height;
} ai_letterbox_info_t;

/*
 * Convert RGB565 image to the model input buffer in int8 NHWC layout.
 *
 * @param[in]  p_rgb565    Input RGB565 image buffer, 2 bytes per pixel.
 * @param[in]  src_width   Input image width, for example 640.
 * @param[in]  src_height  Input image height, for example 480.
 * @param[out] p_model_in  Model input buffer returned by
 *                         GetModelInputPtr_serving_default_images_0().
 * @param[out] p_info      Letterbox transform info for postprocess mapping.
 */
void ai_preprocess_rgb565(const uint8_t *p_rgb565,
                          uint32_t       src_width,
                          uint32_t       src_height,
                          int8_t        *p_model_in,
                          ai_letterbox_info_t *p_info);

#endif /* __AI_PREPROCESS_H */
