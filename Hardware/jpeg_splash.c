/* jpeg_splash.c - 上电开屏海报：tjpgd 解码 Hardware/splash_jpeg_data.c 的内置 JPEG
 * （480x800 baseline RGB565）到 framebuffer。工作区 4KB SRAM，解码 ~100-300ms。 */
#include "jpeg_splash.h"
#include "tjpgd.h"
#include "hal_data.h"
#include "graphics.h"
#include <string.h>
#include <stdio.h>

extern const uint8_t  g_splash_jpg[];
extern const uint32_t g_splash_jpg_size;

typedef struct
{
    const uint8_t * data;
    uint32_t        pos;
    uint32_t        size;
} jpg_stream_t;

/* tjpgd 输入回调：从内存数组读 */
static unsigned int jpg_in_func(JDEC * jd, uint8_t * buf, unsigned int ndata)
{
    jpg_stream_t * st = (jpg_stream_t *) jd->device;
    uint32_t remain = st->size - st->pos;
    uint32_t n = (ndata < remain) ? (uint32_t) ndata : remain;
    if (buf != NULL)
    {
        memcpy(buf, st->data + st->pos, n);
    }
    st->pos += n;
    return n;
}

static uint16_t * g_splash_fb;
static uint32_t   g_splash_stride;
static uint32_t   g_splash_w;
static uint32_t   g_splash_h;

/* tjpgd 输出回调：MCU 块（RGB565）写入 framebuffer 对应区域。
 * 防御：rect 越界（JPEG 尺寸异常）时中断解码，绝不写坏 framebuffer 外内存 */
static int jpg_out_func(JDEC * jd, void * bitmap, JRECT * rect)
{
    (void) jd;
    if ((uint32_t) rect->right >= g_splash_w || (uint32_t) rect->bottom >= g_splash_h)
    {
        printf("[SPLASH] rect OOB: L%d R%d T%d B%d\r\n",
               (int) rect->left, (int) rect->right, (int) rect->top, (int) rect->bottom);
        return 0;   /* 中断解码 */
    }
    uint16_t * src = (uint16_t *) bitmap;
    uint32_t w = (uint32_t) rect->right - (uint32_t) rect->left + 1U;
    for (uint32_t y = rect->top; y <= (uint32_t) rect->bottom; y++)
    {
        uint16_t * dst = g_splash_fb + y * g_splash_stride + (uint32_t) rect->left;
        memcpy(dst, src, w * 2U);
        src += w;
    }
    return 1;   /* 继续解码 */
}

void jpeg_show_splash(void)
{
    /* 解码工作区（SRAM 静态分配）：流输入缓冲(JD_SZBUF=4096) + Huffman/量化表/IDCT/MCU 缓冲
     * 都从 pool 分配，4096 会溢出写坏相邻全局变量（g_hstride 在 s_work 附近）→ 必须给足 */
    static uint8_t s_work[32768];
    JDEC jdec;
    jpg_stream_t stream = { g_splash_jpg, 0U, g_splash_jpg_size };

    g_splash_fb     = (uint16_t *) gp_frame_buffer;
    g_splash_stride = (uint32_t) g_hstride;
    g_splash_w      = 480U;
    g_splash_h      = 800U;

    JRESULT r = jd_prepare(&jdec, jpg_in_func, s_work, sizeof(s_work), &stream);
    if (JDR_OK == r)
    {
        printf("[SPLASH] jpg=%u B %ux%u\r\n",
               (unsigned) g_splash_jpg_size, (unsigned) jdec.width, (unsigned) jdec.height);
        r = jd_decomp(&jdec, jpg_out_func, 0);
    }
    printf("[SPLASH] decode=%d\r\n", (int) r);
    __DSB();
}
