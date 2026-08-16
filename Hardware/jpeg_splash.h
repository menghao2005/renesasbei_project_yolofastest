/* jpeg_splash.h - 上电开屏海报（tjpgd 解码内置 JPEG 到 framebuffer） */
#ifndef JPEG_SPLASH_H
#define JPEG_SPLASH_H

#include <stdint.h>

/* 解码内置海报（480x800 RGB565）到整个 framebuffer。成功后海报覆盖 UI 初始画面，
 * 主循环第一帧 blit 前由 hal_entry 调 ui_control_redraw_screen() 重绘界面退场。 */
void jpeg_show_splash(void);

#endif /* JPEG_SPLASH_H */
