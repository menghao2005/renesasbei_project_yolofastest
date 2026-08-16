#include "bottom_banner.h"

#define BOTTOM_LOGO_IMPLEMENT
#include "bottom_logo_data.h"
#define FONT_8X8_IMPLEMENT
#include "font_8x8.h"
#define RENESAS_LOGO_IMPLEMENT
#include "renesas_logo_data.h"

#define BOTTOM_BANNER_WIDTH  (480U)
#define BOTTOM_BANNER_HEIGHT (160U)

/* Draw one scaled 8x8 ASCII character on an RGB565 framebuffer. */
static void fb_draw_char(uint16_t *fb, uint32_t stride,
                         int px, int py, char ch,
                         uint16_t fg, int scale)
{
    if (((uint8_t) ch < 32U) || ((uint8_t) ch > 126U))
    {
        return;
    }

    if (scale < 1)
    {
        scale = 1;
    }

    const uint8_t *glyph = &font_8x8[((uint8_t) ch - 32U) * 8U];
    for (int r = 0; r < 8; r++)
    {
        uint8_t bits = glyph[r];
        for (int c = 0; c < 8; c++)
        {
            if (bits & 0x80U)
            {
                for (int sy = 0; sy < scale; sy++)
                {
                    for (int sx = 0; sx < scale; sx++)
                    {
                        uint32_t x = (uint32_t) (px + c * scale + sx);
                        uint32_t y = (uint32_t) (py + r * scale + sy);
                        fb[y * stride + x] = fg;
                    }
                }
            }

            bits <<= 1;
        }
    }
}

/* Draw a scaled ASCII string on an RGB565 framebuffer. */
static void fb_draw_string(uint16_t *fb, uint32_t stride,
                           int px, int py, const char *str,
                           uint16_t fg, int scale)
{
    while (*str)
    {
        fb_draw_char(fb, stride, px, py, *str, fg, scale);
        px += 8 * scale;
        str++;
    }
}

static uint16_t blend_rgb565(uint16_t a, uint16_t b, uint32_t mix)
{
    uint32_t ar = (a >> 11) & 0x1FU;
    uint32_t ag = (a >> 5) & 0x3FU;
    uint32_t ab = a & 0x1FU;
    uint32_t br = (b >> 11) & 0x1FU;
    uint32_t bg = (b >> 5) & 0x3FU;
    uint32_t bb = b & 0x1FU;
    uint32_t r = (ar * (255U - mix) + br * mix) / 255U;
    uint32_t g = (ag * (255U - mix) + bg * mix) / 255U;
    uint32_t blue = (ab * (255U - mix) + bb * mix) / 255U;

    return (uint16_t) ((r << 11) | (g << 5) | blue);
}

static void fb_draw_image_rgb565(uint16_t *fb, uint32_t stride,
                                 int px, int py,
                                 const uint16_t *pixels,
                                 uint32_t width,
                                 uint32_t height,
                                 uint16_t transparent_color)
{
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            uint16_t color = pixels[y * width + x];
            if (transparent_color != color)
            {
                fb[((uint32_t) py + y) * stride + (uint32_t) px + x] = color;
            }
        }
    }
}

void bottom_banner_draw(uint16_t *fb, uint32_t stride)
{
    const uint32_t bottom_top = 640U;
    const uint16_t top_color = 0x1908;
    const uint16_t bottom_color = 0x3A4B;
    const uint16_t snow_color = 0xEFFF;

    for (uint32_t y = 0; y < BOTTOM_BANNER_HEIGHT; y++)
    {
        uint16_t row_color = blend_rgb565(top_color, bottom_color,
                                          (y * 255U) / (BOTTOM_BANNER_HEIGHT - 1U));
        for (uint32_t x = 0; x < BOTTOM_BANNER_WIDTH; x++)
        {
            uint16_t color = row_color;
            if (((x * 17U + y * 29U) % 211U) == 0U)
            {
                color = snow_color;
            }

            fb[(bottom_top + y) * stride + x] = color;
        }
    }

    const uint16_t shadow = 0x1082;
    const uint16_t text = 0xFFFF;
    const int brand_scale = 4;
    const int model_scale = 3;
    const int brand_h = 8 * brand_scale;
    const int gap = 12;
    const int block_top = (int) bottom_top + 84;

    const int brand_x = 8;
    const int brand_y = block_top;
    fb_draw_string(fb, stride, brand_x + 2, brand_y + 2, "RENESAS", shadow, brand_scale);
    fb_draw_string(fb, stride, brand_x, brand_y, "RENESAS", text, brand_scale);

    const int model_x = 8;
    const int model_y = block_top + brand_h + gap;
    fb_draw_string(fb, stride, model_x + 2, model_y + 2, "MODEL:YOLOFASTEST", shadow, model_scale);
    fb_draw_string(fb, stride, model_x, model_y, "MODEL:YOLOFASTEST", text, model_scale);

    fb_draw_image_rgb565(fb, stride,
                         8,
                         (int) bottom_top + 6,
                         g_renesas_logo_pixels,
                         RENESAS_LOGO_WIDTH,
                         RENESAS_LOGO_HEIGHT,
                         RENESAS_LOGO_TRANSPARENT_RGB565);

    const int logo_x = (int) BOTTOM_BANNER_WIDTH - (int) BOTTOM_LOGO_WIDTH - 8;
    const int logo_y = (int) bottom_top + 4;
    fb_draw_image_rgb565(fb, stride,
                         logo_x,
                         logo_y,
                         g_bottom_logo_pixels,
                         BOTTOM_LOGO_WIDTH,
                         BOTTOM_LOGO_HEIGHT,
                         BOTTOM_LOGO_TRANSPARENT_RGB565);
}
