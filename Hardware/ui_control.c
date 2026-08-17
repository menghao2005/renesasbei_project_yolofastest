/*
 * ui_control.c
 *
 * 双界面屏幕 UI（无 LVGL，直接写帧缓冲）：
 *   AUTO 界面   原工程画面（相机全屏 + 检测框 + 原 banner 渐变 + 瑞萨/电赛 logo）
 *               顶部品牌条（RENESAS CUP 2026 + NUEDC），画面区浮 MODE/START 两个按钮
 *   REMOTE 界面 品牌条 + 相机小窗（480x360）+ 深色控制面板（BACK/LOCK/状态 +
 *               D-pad 十字键：FWD/BWD 前伸缩回、LEFT/RIGHT 底座转向、STOP 锁定）
 *
 * 状态机：
 *   POWER OFF（上电待机，机械臂车库位不动）
 *     --按 START/LOCK--> ON（AUTO 启动自动抓取 / REMOTE 响应遥控）
 *     --再按--> LOCKED（机械臂冻结、底盘停车、流程暂停）
 *     --再按--> ON（恢复继续）
 *   MODE 键在任意状态切换 AUTO <-> REMOTE 界面（整屏重绘）。
 *
 * 触摸依赖 src/gt911.c 的 st7123_touch_irq_print_task()（主循环每帧调用），
 * 读取 g_touch_x/g_touch_y/g_touch_updated/g_touch_count 全局变量。
 *
 * 屏幕坐标系：480x800 竖屏。
 * 相机画面每帧由 hal_entry 按 ui_control_get_camera_rect() blit，
 * 覆盖在相机区上的 UI（品牌条/按钮/HUD）由本文件每帧重绘。
 */

#include "ui_control.h"

#include "graphics.h"
#include <math.h>
#include "bottom_banner.h"
#include "font_8x8.h"
#include "cn_font.h"
#include "bottom_logo_data.h"
#include "renesas_logo_data.h"
#include "gt911.h"
#include "robot_arm.h"
#include "harvest_task.h"
#include "hal_data.h"
#include "Stepping_Motor.h"
#include "renesas_logo_data.h"

#include <string.h>

/* ------------------------------------------------------------------------- */
/* 屏幕布局                                                                   */
/* ------------------------------------------------------------------------- */
#define UI_SCREEN_W         (480)
#define UI_SCREEN_H         (800)

/* 无品牌条：相机画面从 y0 开始 */
#define AUTO_CAM_Y          (0)              /* AUTO 相机画面起点 */
#define AUTO_CAM_H          (600)            /* AUTO 相机画面高度（到控制条上沿） */
#define AUTO_BTN_MODE_X     (12)
#define AUTO_BTN_MODE_Y     (600)
#define AUTO_BTN_MODE_W     (144)
#define AUTO_BTN_MODE_H     (40)
#define AUTO_BTN_START_X    (168)
#define AUTO_BTN_START_Y    (600)
#define AUTO_BTN_START_W    (144)
#define AUTO_BTN_START_H    (40)

/* --- REMOTE 界面 --- */
#define RMT_CAM_Y           (0)              /* 相机小窗 y 0-392（直达面板上沿） */
#define RMT_CAM_H           (392)
#define RMT_PANEL_Y         (392)            /* 控制面板 y 392-800 */
#define RMT_PANEL_H         (UI_SCREEN_H - RMT_PANEL_Y)

#define RMT_BACK_X          (8)
#define RMT_BACK_Y          (RMT_PANEL_Y + 8)
#define RMT_BACK_W          (104)
#define RMT_BACK_H          (40)

#define RMT_LOCK_X          (UI_SCREEN_W - 8 - 104)
#define RMT_LOCK_Y          (RMT_BACK_Y)
#define RMT_LOCK_W          (104)
#define RMT_LOCK_H          (40)

#define RMT_STATE_X         (120)
#define RMT_STATE_Y         (RMT_BACK_Y + 8)
#define RMT_STATE_W         (UI_SCREEN_W - 120 - 120)
#define RMT_STATE_H         (24)

/* 虚拟摇杆（左半屏，控制机械臂前后左右） */
#define RMT_JOY_CX          (140)
#define RMT_JOY_CY          (620)
#define RMT_JOY_R           (88)             /* 底座半径 */
#define RMT_JOY_KNOB_R      (34)             /* 摇杆头半径 */
#define RMT_JOY_DEAD        (12)             /* 死区（px），以内不动作 */

/* 抓取 / 打开爪子按钮（右半屏） */
#define RMT_GRASP_X         (308)
#define RMT_GRASP_Y         (540)
#define RMT_GRASP_W         (164)
#define RMT_GRASP_H         (64)
/* 自动抓取按钮（原 OPEN 位置）：一键下探→闭合→缩回 */
#define RMT_AUTOGRAB_X      (308)
#define RMT_AUTOGRAB_Y      (636)
#define RMT_AUTOGRAB_W      (164)
#define RMT_AUTOGRAB_H      (64)

#define RMT_HUD_X           (14)
#define RMT_HUD_Y           (12)

/* ------------------------------------------------------------------------- */
/* 颜色（RGB565，均为 16 位正确值）——浅蓝灰精致科技风                         */
/* 注：旧版部分宏写的是 24 位值，赋给 uint16_t 时被截断成怪色（如"绿"实际     */
/*     是青绿），此处统一改为正确 RGB565。                                     */
/* ------------------------------------------------------------------------- */
#define UI_COLOR_BG         (0xDF3EU)  /* 浅蓝灰背景 (216,228,242) */
#define UI_COLOR_BG2        (0xC6BDU)  /* 深一档蓝灰（分区底色）(198,214,234) */
#define UI_COLOR_CARD       (0xFFFFU)  /* 白色卡片 */
#define UI_COLOR_PANEL_L    (0xD71EU)  /* 浅蓝灰按钮底 (208,224,240) */
#define UI_COLOR_SHADOW     (0xAE3BU)  /* 按钮投影 (168,196,216) */
#define UI_COLOR_BORDER     (0xADFAU)  /* 蓝灰边框 (168,188,208) */
#define UI_COLOR_DARK_TEXT  (0x1189U)  /* 深蓝灰文字 (16,51,72) */
#define UI_COLOR_GRAY       (0x8494U)  /* 中灰 (128,148,160) */
#define UI_COLOR_GOLD       (0xFBE0U)  /* 品牌金字（深色品牌条上用）(248,220,0) */
#define UI_COLOR_BRAND_BG   (0x0907U)  /* 深藏青品牌条底 (8,32,56) */
#define UI_COLOR_ACCENT     (0x2C5DU)  /* 主蓝 (40,136,232) */
#define UI_COLOR_GREEN      (0x1EAAU)  /* (24,212,80) */
#define UI_COLOR_GREEN_D    (0x0C46U)  /* (8,136,48) */
#define UI_COLOR_RED        (0xF208U)  /* (240,64,64) */
#define UI_COLOR_RED_D      (0x98C3U)  /* (152,24,24) */
#define UI_COLOR_BLUE       (0x3D3FU)  /* (56,164,248) */
#define UI_COLOR_BLUE_D     (0x1B7AU)  /* (24,108,208) */
#define UI_COLOR_YELLOW     (0xFE27U)  /* (248,196,56) */
#define UI_COLOR_YELLOW_D   (0xB3E1U)  /* (176,124,8) */
#define UI_COLOR_ORANGE     (0xFC65U)  /* (248,140,40) */
#define UI_COLOR_PRESS_BG   (0xCE79U)
#define UI_COLOR_PRESS_TXT  (0x0000U)

/* 圆角半径（px） */
#define UI_RADIUS           (10)

/* ------------------------------------------------------------------------- */
/* 遥控步进参数（脉冲宽度 us）                                                 */
/* ------------------------------------------------------------------------- */
#define UI_REMOTE_BASE_MIN      (700U)
#define UI_REMOTE_BASE_MAX      (2300U)
#define UI_REMOTE_UPPER_MIN     (1350U)
#define UI_REMOTE_UPPER_MAX     (1720U)
#define UI_REMOTE_FOREARM_MIN   (1980U)
#define UI_REMOTE_FOREARM_MAX   (2460U)
#define UI_REMOTE_GRIPPER_US    (900U)
#define UI_REMOTE_STEP_BASE     (10)    /* 分辨率再减半：更细节 */
#define UI_REMOTE_STEP_UPPER    (6)
#define UI_REMOTE_STEP_FOREARM  (8)
#define UI_REMOTE_MOVE_MS       (80U)   /* 节流间隔 + 插值时长 */

/* 补光灯按钮（P109 高电平亮） */
#define AUTO_LIGHT_X       (324)
#define AUTO_LIGHT_Y       (600)
#define AUTO_LIGHT_W       (144)
#define AUTO_LIGHT_H       (40)

/* AUTO 控制条（y600-640）：三按钮同排深色底，整行不显示相机 */
#define AUTO_CTRL_Y        (600)
#define AUTO_CTRL_H        (40)
#define UI_COLOR_CTRL_TOP  (0x29A9U)  /* (46,52,74) 控制条顶 */
#define UI_COLOR_CTRL_BOT  (0x1906U)  /* (30,34,50) 控制条底 */
#define RMT_LIGHT_X        (308)
#define RMT_LIGHT_Y        (455)
#define RMT_LIGHT_W        (164)
#define RMT_LIGHT_H        (48)

/* 车库位（与 RobotArm_Init 的初始脉冲一致） */
#define UI_REMOTE_HOME_BASE     (1500U)
#define UI_REMOTE_HOME_UPPER    (1640U)
#define UI_REMOTE_HOME_FOREARM  (2400U)

/* ------------------------------------------------------------------------- */
/* 可命中的 UI 元素                                                            */
/* ------------------------------------------------------------------------- */
typedef enum e_ui_hit
{
    UI_HIT_NONE = 0,
    UI_HIT_MODE,        /* AUTO 界面右上 */
    UI_HIT_START,       /* AUTO 界面中右 */
    UI_HIT_BACK,        /* REMOTE 左上 */
    UI_HIT_LOCK,        /* REMOTE 右上 */
    UI_HIT_JOY,         /* REMOTE 摇杆区（按住持续控制） */
    UI_HIT_GRASP,       /* 抓取（爪子闭合） */
    UI_HIT_AUTOGRAB,    /* 自动下探抓取 */
    UI_HIT_LIGHT        /* 补光灯开关 */
} ui_hit_t;

static void ui_light_toggle(void);
static void ui_draw_light_btn(int x, int y, int w, int h);
static void ui_draw_auto_ctrl_bar(void);
static void ui_draw_hit_btn(ui_hit_t h, bool pressed);

static ui_mode_t  g_mode  = UI_MODE_AUTO;
static ui_power_t g_power = UI_POWER_OFF;

static bool     g_harvest_started = false;
static bool     g_light_on       = false;   /* 补光灯状态 */
static ui_hit_t g_touch_last      = UI_HIT_NONE;


/* 摇杆状态（REMOTE 界面） */
static int g_joy_x = RMT_JOY_CX;       /* 摇杆头当前 x（屏幕坐标） */
static int g_joy_y = RMT_JOY_CY;

/* 遥控目标姿态（机械臂无当前位置 getter，自己维护目标并持续外推） */
static uint16_t g_remote_base    = UI_REMOTE_HOME_BASE;
static uint16_t g_remote_upper   = UI_REMOTE_HOME_UPPER;
static uint16_t g_remote_forearm = UI_REMOTE_HOME_FOREARM;

/* 爪子状态：false=开(900us), true=合(1350us) */
static bool g_gripper_closed = false;

/* 自动抓取状态机：下探→闭合→缩回 home */
typedef enum e_ui_autograb_state
{
    UI_AG_IDLE = 0,
    UI_AG_DESCEND,    /* 下探到低位 */
    UI_AG_CLOSE,      /* 闭合爪子 */
} ui_autograb_state_t;
static ui_autograb_state_t g_autograb_state = UI_AG_IDLE;

/* ------------------------------------------------------------------------- */
/* 底层绘制                                                                   */
/* ------------------------------------------------------------------------- */
static uint16_t * ui_fb(void)
{
    return (uint16_t *) gp_frame_buffer;
}

static void ui_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    uint16_t * fb = ui_fb();

    if (x < 0)          { w += x; x = 0; }
    if (y < 0)          { h += y; y = 0; }
    if (x >= UI_SCREEN_W || y >= UI_SCREEN_H) { return; }
    if ((x + w) > UI_SCREEN_W) { w = UI_SCREEN_W - x; }
    if ((y + h) > UI_SCREEN_H) { h = UI_SCREEN_H - y; }
    if (w <= 0 || h <= 0)      { return; }

    for (int row = 0; row < h; row++)
    {
        for (int col = 0; col < w; col++)
        {
            fb[(uint32_t) (y + row) * g_hstride + (uint32_t) (x + col)] = color;
        }
    }
}

static void ui_draw_char(int px, int py, char ch, uint16_t fg, int scale)
{
    uint16_t * fb = ui_fb();

    if (((uint8_t) ch < 32U) || ((uint8_t) ch > 126U))
    {
        return;
    }
    if (scale < 1)
    {
        scale = 1;
    }

    const uint8_t * glyph = &font_8x8[((uint8_t) ch - 32U) * 8U];
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
                        fb[y * g_hstride + x] = fg;
                    }
                }
            }
            bits <<= 1;
        }
    }
}

static void ui_draw_string(int px, int py, const char * str, uint16_t fg, int scale)
{
    while (*str)
    {
        ui_draw_char(px, py, *str, fg, scale);
        px += 8 * scale;
        str++;
    }
}

/* 16x16 中文点阵：单字绘制（scale 倍放大，直接写 framebuffer） */
static void ui_draw_cn_char(uint16_t * fb, int stride, int x, int y,
                            uint16_t glyph_idx, uint16_t color, int scale)
{
    const uint8_t * g = &g_cn_glyphs[glyph_idx * CN_FONT_BYTES];
    for (int gy = 0; gy < CN_FONT_H; gy++)
    {
        uint8_t hi = g[gy * 2];
        uint8_t lo = g[gy * 2 + 1];
        for (int gx = 0; gx < CN_FONT_W; gx++)
        {
            uint8_t bit = (gx < 8) ? (uint8_t) ((hi >> (7 - gx)) & 1U)
                                   : (uint8_t) ((lo >> (15 - gx)) & 1U);
            if (0U != bit)
            {
                for (int dy = 0; dy < scale; dy++)
                {
                    for (int dx = 0; dx < scale; dx++)
                    {
                        fb[(y + gy * scale + dy) * stride + (x + gx * scale + dx)] = color;
                    }
                }
            }
        }
    }
}

/* 中文（UTF-8）字符串居中绘制；ASCII 字符混排用 8x8 字库 */
static void ui_draw_cn_centered(int cx, int cy, const char * str, uint16_t color, int scale)
{
    int count = 0;
    const char * p = str;
    while (*p)
    {
        if ((uint8_t) *p >= 0x80U) { p += 3; } else { p += 1; }
        count++;
    }
    int total_w = count * CN_FONT_W * scale;
    int x0 = cx - total_w / 2;
    int y0 = cy - CN_FONT_H * scale / 2;
    uint16_t * fb = (uint16_t *) gp_frame_buffer;
    int stride = (int) g_hstride;

    p = str;
    while (*p)
    {
        if ((uint8_t) *p >= 0x80U)
        {
            uint8_t b0 = (uint8_t) p[0];
            uint8_t b1 = (uint8_t) p[1];
            uint8_t b2 = (uint8_t) p[2];
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
                ui_draw_cn_char(fb, stride, x0, y0, idx, color, scale);
            }
            x0 += CN_FONT_W * scale;
            p += 3;
        }
        else
        {
            char tmp[2] = { *p, 0 };
            ui_draw_string(x0, y0 + (CN_FONT_H * scale - 8 * scale) / 2, tmp, color, scale);
            x0 += 8 * scale;
            p += 1;
        }
    }
}

static void ui_draw_string_centered(int cx, int cy, const char * str, uint16_t fg, int scale)
{
    /* 含中文（UTF-8 首字节 >=0x80）→ 16x16 点阵字库；纯 ASCII → 8x8 字库 */
    bool has_cn = false;
    for (const char * p = str; *p; p++)
    {
        if ((uint8_t) *p >= 0x80U) { has_cn = true; break; }
    }
    if (has_cn)
    {
        ui_draw_cn_centered(cx, cy, str, fg, scale);
        return;
    }
    int text_w = (int) strlen(str) * 8 * scale;
    int text_h = 8 * scale;

    ui_draw_string(cx - text_w / 2, cy - text_h / 2, str, fg, scale);
}

/* 圆角矩形填充（四角画 1/4 圆，半径 r） */
static void ui_fill_rect_rounded(int x, int y, int w, int h, int r, uint16_t color)
{
    if (r > w / 2) { r = w / 2; }
    if (r > h / 2) { r = h / 2; }
    if (r < 1) { ui_fill_rect(x, y, w, h, color); return; }

    /* 主体：中间横条 + 左右竖条（留出四角） */
    ui_fill_rect(x + r, y, w - 2 * r, h, color);
    ui_fill_rect(x, y + r, r, h - 2 * r, color);
    ui_fill_rect(x + w - r, y + r, r, h - 2 * r, color);

    /* 四角圆弧 */
    uint16_t * fb = ui_fb();
    for (int dy = 0; dy < r; dy++)
    {
        for (int dx = 0; dx < r; dx++)
        {
            int cx = r - 1;
            int cy = r - 1;
            int d2 = (dx - cx) * (dx - cx) + (dy - cy) * (dy - cy);
            if (d2 <= (r - 1) * (r - 1))
            {
                /* 左上 */
                fb[(uint32_t) (y + dy) * g_hstride + (uint32_t) (x + dx)] = color;
                /* 右上 */
                fb[(uint32_t) (y + dy) * g_hstride + (uint32_t) (x + w - 1 - dx)] = color;
                /* 左下 */
                fb[(uint32_t) (y + h - 1 - dy) * g_hstride + (uint32_t) (x + dx)] = color;
                /* 右下 */
                fb[(uint32_t) (y + h - 1 - dy) * g_hstride + (uint32_t) (x + w - 1 - dx)] = color;
            }
        }
    }
}

/* 面板：圆角实色底 + 圆角 1px 边框 */
static void ui_draw_panel(int x, int y, int w, int h, uint16_t bg, uint16_t border)
{
    ui_fill_rect_rounded(x, y, w, h, UI_RADIUS, border);
    ui_fill_rect_rounded(x + 1, y + 1, w - 2, h - 2,
                         (UI_RADIUS > 1) ? (UI_RADIUS - 1) : 1, bg);
}

/* RGB565 颜色插值（用于渐变）：t=0 返回 a，t=255 返回 b */
static uint16_t ui_color_lerp(uint16_t a, uint16_t b, int t, int tmax)
{
    int r1 = (a >> 11) & 0x1F;
    int g1 = (a >> 5) & 0x3F;
    int b1 = a & 0x1F;
    int r2 = (b >> 11) & 0x1F;
    int g2 = (b >> 5) & 0x3F;
    int b2 = b & 0x1F;
    int num = (tmax > 0) ? t : 0;

    int r = r1 + ((r2 - r1) * num) / tmax;
    int g = g1 + ((g2 - g1) * num) / tmax;
    int bb = b1 + ((b2 - b1) * num) / tmax;
    if (r < 0) { r = 0; } if (r > 31) { r = 31; }
    if (g < 0) { g = 0; } if (g > 63) { g = 63; }
    if (bb < 0) { bb = 0; } if (bb > 31) { bb = 31; }
    return (uint16_t) (((uint16_t) r << 11) | ((uint16_t) g << 5) | (uint16_t) bb);
}


/* 实心圆 */
static void ui_fill_circle(int cx, int cy, int r, uint16_t color)
{
    uint16_t * fb = ui_fb();
    for (int dy = -r; dy <= r; dy++)
    {
        int yy = cy + dy;
        if (yy < 0 || yy >= UI_SCREEN_H) { continue; }
        for (int dx = -r; dx <= r; dx++)
        {
            int xx = cx + dx;
            if (xx < 0 || xx >= UI_SCREEN_W) { continue; }
            if ((dx * dx + dy * dy) <= (r * r))
            {
                fb[(uint32_t) yy * g_hstride + (uint32_t) xx] = color;
            }
        }
    }
}

/* 圆环 */
static void ui_draw_circle_ring(int cx, int cy, int r, int thick, uint16_t color)
{
    uint16_t * fb = ui_fb();
    int r2 = r * r;
    int r1 = (r - thick) * (r - thick);
    if (r1 < 0) { r1 = 0; }
    for (int dy = -r; dy <= r; dy++)
    {
        int yy = cy + dy;
        if (yy < 0 || yy >= UI_SCREEN_H) { continue; }
        for (int dx = -r; dx <= r; dx++)
        {
            int xx = cx + dx;
            if (xx < 0 || xx >= UI_SCREEN_W) { continue; }
            int d2 = dx * dx + dy * dy;
            if (d2 <= r2 && d2 >= r1)
            {
                fb[(uint32_t) yy * g_hstride + (uint32_t) xx] = color;
            }
        }
    }
}

/* 3D 按钮：底部阴影 + 垂直渐变 + 顶部高光 + 圆角边框 */
static void ui_draw_button(int x, int y, int w, int h,
                           const char * label, int scale,
                           bool pressed, uint16_t border,
                           uint16_t bg_top, uint16_t bg_bottom,
                           uint16_t label_color)
{
    if (pressed)
    {
        /* 按下：整体压暗 */
        bg_top    = ui_color_lerp(bg_top, 0x0000U, 40, 100);
        bg_bottom = ui_color_lerp(bg_bottom, 0x0000U, 40, 100);
    }

    /* 底部阴影（2px 偏移的浅灰圆角） */
    ui_fill_rect_rounded(x + 2, y + 3, w, h, UI_RADIUS, UI_COLOR_SHADOW);

    /* 边框（圆角整块） */
    ui_fill_rect_rounded(x, y, w, h, UI_RADIUS, border);

    /* 渐变主体（内缩 1px） */
    int rows = h - 2;
    for (int row = 0; row < rows; row++)
    {
        uint16_t c = ui_color_lerp(bg_top, bg_bottom, row, rows - 1);
        int rr = (0 == row) || (row == rows - 1) ? UI_RADIUS - 1 : 0;
        ui_fill_rect_rounded(x + 1, y + 1 + row, w - 2, 1, rr, c);
    }

    /* 顶部高光（2px 亮条） */
    ui_fill_rect(x + 3, y + 1, w - 6, 1, ui_color_lerp(bg_top, 0xFFFFU, 55, 100));
    ui_fill_rect(x + 4, y + 2, w - 8, 1, ui_color_lerp(bg_top, 0xFFFFU, 30, 100));

    ui_draw_string_centered(x + w / 2, y + h / 2 + (pressed ? 1 : 0), label, label_color, scale);
}

/* ------------------------------------------------------------------------- */
/* 状态文字                                                                   */
/* ------------------------------------------------------------------------- */
/* 状态徽章文案（REMOTE 中间徽章） */
static const char * ui_state_label(ui_power_t power)
{
    switch (power)
    {
        case UI_POWER_ON:
            return "控制运行中";   /* 运行中 */
        case UI_POWER_LOCKED:
            return "控制停止中";   /* 停止中 */
        case UI_POWER_OFF:
        default:
            return "控制待机";   /* 待机 */
    }
}

static uint16_t ui_power_color(ui_power_t power)
{
    switch (power)
    {
        case UI_POWER_ON:
            return UI_COLOR_GREEN;
        case UI_POWER_LOCKED:
            return UI_COLOR_RED;
        case UI_POWER_OFF:
        default:
            return UI_COLOR_GRAY;
    }
}

static const char * ui_power_label(ui_power_t power)
{
    switch (power)
    {
        case UI_POWER_ON:
            return "停止";      /* 运行中：点击停止 */
        case UI_POWER_LOCKED:
            return "继续";      /* 锁定：点击恢复 */
        case UI_POWER_OFF:
        default:
            return "开始";      /* 待机：点击开始 */
    }
}

/* ------------------------------------------------------------------------- */
/* AUTO 界面按钮：MODE（切换时画，顶部不被 blit 覆盖）+ START（每帧画）         */
/* ------------------------------------------------------------------------- */
static void ui_draw_mode_btn(void)
{
    ui_draw_button(AUTO_BTN_MODE_X, AUTO_BTN_MODE_Y, AUTO_BTN_MODE_W, AUTO_BTN_MODE_H,
                   "模式", 2, (UI_HIT_MODE == g_touch_last),
                   (UI_MODE_REMOTE == g_mode) ? UI_COLOR_ORANGE : UI_COLOR_BORDER,
                   0xFFFFU, UI_COLOR_PANEL_L, UI_COLOR_DARK_TEXT);
}

static void ui_draw_start_btn(void)
{
    ui_draw_button(AUTO_BTN_START_X, AUTO_BTN_START_Y, AUTO_BTN_START_W, AUTO_BTN_START_H,
                   ui_power_label(g_power), 2, (UI_HIT_START == g_touch_last),
                   ui_power_color(g_power),
                   ui_color_lerp(ui_power_color(g_power), 0xFFFFU, 60, 100),
                   ui_power_color(g_power), 0xFFFFU);
}

/* ------------------------------------------------------------------------- */
/* REMOTE 界面：品牌条 + 相机小窗（blit 透出）+ 控制面板（摇杆 + 抓取/开爪）    */
/* ------------------------------------------------------------------------- */
/* 摇杆区每轮重绘（60Hz）：只画底座圆 + 刻度 + 摇杆头，区域固定（RMT_JOY_*），
 * 绘制量比整个面板小一个数量级，避免下半区撕裂闪烁 */
static void ui_draw_joy_area(void)
{
    /* 底座（白色圆盘 + 蓝灰圆环） */
    ui_fill_circle(RMT_JOY_CX, RMT_JOY_CY, RMT_JOY_R, UI_COLOR_CARD);
    ui_draw_circle_ring(RMT_JOY_CX, RMT_JOY_CY, RMT_JOY_R, 2, UI_COLOR_BORDER);

    /* 十字刻度线 */
    ui_fill_rect(RMT_JOY_CX - 1, RMT_JOY_CY - RMT_JOY_R + 12, 2, RMT_JOY_R * 2 - 24, UI_COLOR_BG2);
    ui_fill_rect(RMT_JOY_CX - RMT_JOY_R + 12, RMT_JOY_CY - 1, RMT_JOY_R * 2 - 24, 2, UI_COLOR_BG2);

    /* 方向箭头提示 */
    ui_draw_string_centered(RMT_JOY_CX, RMT_JOY_CY - RMT_JOY_R + 20, "^", UI_COLOR_GRAY, 2);
    ui_draw_string_centered(RMT_JOY_CX - RMT_JOY_R + 16, RMT_JOY_CY, "<", UI_COLOR_GRAY, 2);
    ui_draw_string_centered(RMT_JOY_CX + RMT_JOY_R - 16, RMT_JOY_CY, ">", UI_COLOR_GRAY, 2);
    ui_draw_string_centered(RMT_JOY_CX, RMT_JOY_CY + RMT_JOY_R - 20, "v", UI_COLOR_GRAY, 2);

    /* 摇杆头（跟随触摸，松开回中） */
    int kx = (UI_HIT_JOY == g_touch_last) ? g_joy_x : RMT_JOY_CX;
    int ky = (UI_HIT_JOY == g_touch_last) ? g_joy_y : RMT_JOY_CY;
    ui_fill_circle(kx, ky, RMT_JOY_KNOB_R, UI_COLOR_ACCENT);
    ui_fill_circle(kx, ky, RMT_JOY_KNOB_R - 10, UI_COLOR_BLUE);
    ui_draw_circle_ring(kx, ky, RMT_JOY_KNOB_R, 2, UI_COLOR_BLUE_D);
}

/* REMOTE 面板底部：电赛 logo（NUEDC，0.8 倍软件缩放，透明色跳过）。
 * 放 y704 起（GRASP/OPEN 与摇杆下方空白区），居中。 */
/* REMOTE 电赛 logo（NUEDC，纯整数 0.8 缩放，透明色跳过） */
static void ui_draw_remote_logo(void)
{
    /* REMOTE 底部双 logo：瑞萨左 + 电赛右，纯整数 0.8 缩放（*5/4），透明色跳过。 */
    uint16_t * fb = (uint16_t *) gp_frame_buffer;
    const int stride = (int) g_hstride;

    /* 瑞萨：0.8 -> 136x58 @ (16, 738)，底部对齐 796 */
    {
        const int dw = (RENESAS_LOGO_WIDTH * 4) / 5;
        const int dh = (RENESAS_LOGO_HEIGHT * 4) / 5;
        const int dst_x = 16, dst_y = 796 - dh;
        for (int y = 0; y < dh; y++)
        {
            int sy = (y * 5) / 4;
            for (int x = 0; x < dw; x++)
            {
                int sx = (x * 5) / 4;
                uint16_t c = g_renesas_logo_pixels[sy * RENESAS_LOGO_WIDTH + sx];
                if (RENESAS_LOGO_TRANSPARENT_RGB565 != c)
                {
                    fb[(y + dst_y) * stride + (x + dst_x)] = c;
                }
            }
        }
    }

    /* 电赛 NUEDC：0.8 -> 152x92 @ (312, 704)，右移 */
    {
        const int dw = (BOTTOM_LOGO_WIDTH * 4) / 5;
        const int dh = (BOTTOM_LOGO_HEIGHT * 4) / 5;
        const int dst_x = 312, dst_y = 704;
        for (int y = 0; y < dh; y++)
        {
            int sy = (y * 5) / 4;
            for (int x = 0; x < dw; x++)
            {
                int sx = (x * 5) / 4;
                uint16_t c = g_bottom_logo_pixels[sy * BOTTOM_LOGO_WIDTH + sx];
                if (BOTTOM_LOGO_TRANSPARENT_RGB565 != c)
                {
                    fb[(y + dst_y) * stride + (x + dst_x)] = c;
                }
            }
        }
    }
}



static void ui_draw_remote_panel(void)
{
    /* 控制面板背景（从相机小窗下沿 y388 开始，盖掉 y388-392 缝隙残留） */
    ui_fill_rect(0, RMT_CAM_Y + RMT_CAM_H, UI_SCREEN_W,
                 RMT_PANEL_Y - (RMT_CAM_Y + RMT_CAM_H), UI_COLOR_BG);
    ui_fill_rect(0, RMT_PANEL_Y, UI_SCREEN_W, RMT_PANEL_H, UI_COLOR_BG);
    ui_fill_rect(0, RMT_PANEL_Y, UI_SCREEN_W, 2, UI_COLOR_BORDER);

    /* BACK / LOCK */
    ui_draw_button(RMT_BACK_X, RMT_BACK_Y, RMT_BACK_W, RMT_BACK_H,
                   "返回", 2, (UI_HIT_BACK == g_touch_last), UI_COLOR_BORDER,
                   0xFFFFU, UI_COLOR_PANEL_L, UI_COLOR_DARK_TEXT);

    /* 状态徽章（白色圆角底 + 彩色文字） */
    ui_draw_panel(RMT_STATE_X, RMT_STATE_Y - 4, RMT_STATE_W, RMT_STATE_H + 8,
                  UI_COLOR_CARD, UI_COLOR_BORDER);
    ui_draw_string_centered(RMT_STATE_X + RMT_STATE_W / 2, RMT_STATE_Y + 12,
                            ui_state_label(g_power), ui_power_color(g_power), 2);

    ui_draw_button(RMT_LOCK_X, RMT_LOCK_Y, RMT_LOCK_W, RMT_LOCK_H,
                   ui_power_label(g_power), 2, (UI_HIT_LOCK == g_touch_last),
                   UI_COLOR_RED_D,
                   UI_COLOR_RED, UI_COLOR_RED_D, 0xFFFFU);

    /* ===== 虚拟摇杆（左半屏） ===== */
    ui_draw_joy_area();

    /* 爪子按钮 */
    ui_draw_button(RMT_GRASP_X, RMT_GRASP_Y, RMT_GRASP_W, RMT_GRASP_H,
                   g_gripper_closed ? "\xe6\x9d\xbe\xe5\xbc\x80" : "\xe6\x8a\x93\xe5\x8f\x96",
                   2, (UI_HIT_GRASP == g_touch_last),
                   g_gripper_closed ? UI_COLOR_BLUE_D : UI_COLOR_GREEN_D,
                   g_gripper_closed ? UI_COLOR_BLUE : UI_COLOR_GREEN,
                   g_gripper_closed ? UI_COLOR_BLUE_D : UI_COLOR_GREEN_D, 0xFFFFU);
    /* 开始按钮 */
    ui_draw_button(RMT_AUTOGRAB_X, RMT_AUTOGRAB_Y, RMT_AUTOGRAB_W, RMT_AUTOGRAB_H,
                   "\xe4\xb8\x8b\xe6\x8a\x93", 2, (UI_HIT_AUTOGRAB == g_touch_last),
                   UI_COLOR_ORANGE, 0xFFFFU, UI_COLOR_ORANGE, 0xFFFFU);

    /* 补光灯按钮（GRASP/OPEN 上方） */
    ui_draw_light_btn(RMT_LIGHT_X, RMT_LIGHT_Y, RMT_LIGHT_W, RMT_LIGHT_H);

    /* 面板底部：电赛 NUEDC logo（空白区居中） */
    ui_draw_remote_logo();
}

/* 整屏重绘（界面切换时调用） */
static void ui_redraw_screen(void)
{
    if (UI_MODE_REMOTE == g_mode)
    {
        ui_draw_remote_panel();
    }
    else
    {
        /* 原工程 banner 渐变 + 瑞萨/电赛 logo（无按钮） */
        bottom_banner_draw(ui_fb(), g_hstride);
        ui_draw_auto_ctrl_bar();
    }
    __DSB();
}

/* ------------------------------------------------------------------------- */
/* 触摸 hit-test                                                              */
/* ------------------------------------------------------------------------- */
static bool ui_point_in_rect(uint16_t x, uint16_t y, int rx, int ry, int rw, int rh)
{
    return ((int) x >= rx) && ((int) x < (rx + rw)) &&
           ((int) y >= ry) && ((int) y < (ry + rh));
}

static ui_hit_t ui_hit_test(uint16_t x, uint16_t y)
{
    if (UI_MODE_REMOTE == g_mode)
    {
        if (ui_point_in_rect(x, y, RMT_BACK_X, RMT_BACK_Y, RMT_BACK_W, RMT_BACK_H))
        {
            return UI_HIT_BACK;
        }
        if (ui_point_in_rect(x, y, RMT_LOCK_X, RMT_LOCK_Y, RMT_LOCK_W, RMT_LOCK_H))
        {
            return UI_HIT_LOCK;
        }
        if (ui_point_in_rect(x, y, RMT_LIGHT_X, RMT_LIGHT_Y, RMT_LIGHT_W, RMT_LIGHT_H))
        {
            return UI_HIT_LIGHT;
        }
        /* 抓取 / 打开 */
        if (ui_point_in_rect(x, y, RMT_GRASP_X, RMT_GRASP_Y, RMT_GRASP_W, RMT_GRASP_H))
        {
            return UI_HIT_GRASP;
        }
        if (ui_point_in_rect(x, y, RMT_AUTOGRAB_X, RMT_AUTOGRAB_Y, RMT_AUTOGRAB_W, RMT_AUTOGRAB_H))
        {
            return UI_HIT_AUTOGRAB;
        }
        /* 摇杆圆内 */
        int dx = (int) x - RMT_JOY_CX;
        int dy = (int) y - RMT_JOY_CY;
        if ((dx * dx + dy * dy) <= (RMT_JOY_R * RMT_JOY_R))
        {
            return UI_HIT_JOY;
        }
        return UI_HIT_NONE;
    }

    /* AUTO 界面 */
    if (ui_point_in_rect(x, y, AUTO_BTN_MODE_X, AUTO_BTN_MODE_Y, AUTO_BTN_MODE_W, AUTO_BTN_MODE_H))
    {
        return UI_HIT_MODE;
    }
    if (ui_point_in_rect(x, y, AUTO_BTN_START_X, AUTO_BTN_START_Y, AUTO_BTN_START_W, AUTO_BTN_START_H))
    {
        return UI_HIT_START;
    }
    if (ui_point_in_rect(x, y, AUTO_LIGHT_X, AUTO_LIGHT_Y, AUTO_LIGHT_W, AUTO_LIGHT_H))
    {
        return UI_HIT_LIGHT;
    }
    return UI_HIT_NONE;
}

/* ------------------------------------------------------------------------- */
/* 状态机动作                                                                 */
/* ------------------------------------------------------------------------- */
/* 补光灯开关：翻转 + P109 高/低 */
static void ui_light_toggle(void)
{
    g_light_on = !g_light_on;
    (void) R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_09,
                             g_light_on ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
}

/* 爪子按钮（只画自己区域，防全面板闪屏） */


/* 绘制指定按钮（pressed=true 高亮）。只处理 REMOTE 右侧三个按钮。 */
static void ui_draw_hit_btn(ui_hit_t h, bool pressed)
{
    if (UI_HIT_LIGHT == h)
        ui_draw_light_btn(RMT_LIGHT_X, RMT_LIGHT_Y, RMT_LIGHT_W, RMT_LIGHT_H);
    else if (UI_HIT_GRASP == h)
        ui_draw_button(RMT_GRASP_X, RMT_GRASP_Y, RMT_GRASP_W, RMT_GRASP_H,
                       g_gripper_closed ? "\xe6\x9d\xbe\xe5\xbc\x80" : "\xe6\x8a\x93\xe5\x8f\x96",
                       2, pressed,
                       g_gripper_closed ? UI_COLOR_BLUE_D : UI_COLOR_GREEN_D,
                       g_gripper_closed ? UI_COLOR_BLUE : UI_COLOR_GREEN,
                       g_gripper_closed ? UI_COLOR_BLUE_D : UI_COLOR_GREEN_D, 0xFFFFU);
    else if (UI_HIT_AUTOGRAB == h)
        ui_draw_button(RMT_AUTOGRAB_X, RMT_AUTOGRAB_Y, RMT_AUTOGRAB_W, RMT_AUTOGRAB_H,
                       "\xe4\xb8\x8b\xe6\x8a\x93", 2, pressed,
                       UI_COLOR_ORANGE, 0xFFFFU, UI_COLOR_ORANGE, 0xFFFFU);
}

/* 补光灯按钮（亮=橙色，灭=灰色） */
static void ui_draw_light_btn(int x, int y, int w, int h)
{
    if (g_light_on)
    {
        ui_draw_button(x, y, w, h, "灯光", 2, (UI_HIT_LIGHT == g_touch_last),
                       UI_COLOR_ORANGE, 0xFFFFU, UI_COLOR_ORANGE, 0xFFFFU);
    }
    else
    {
        ui_draw_button(x, y, w, h, "灯光", 2, (UI_HIT_LIGHT == g_touch_last),
                       UI_COLOR_BORDER, 0xFFFFU, UI_COLOR_PANEL_L, UI_COLOR_DARK_TEXT);
    }
}

/* AUTO 控制条（y600-640）整条绘制：深色渐变底 + 三按钮（模式/开始/灯光）。
 * blit 整行跳过此区（不显示相机），本函数每 8ms overlay 重绘 -> 按钮零闪。 */
static void ui_draw_auto_ctrl_bar(void)
{
    /* 深色渐变底（4 段近似） */
    for (int seg = 0; seg < 4; seg++)
    {
        int y = AUTO_CTRL_Y + seg * 10;
        ui_fill_rect(0, y, UI_SCREEN_W, 10,
                     ui_color_lerp(UI_COLOR_CTRL_TOP, UI_COLOR_CTRL_BOT, seg * 25, 100));
    }
    /* 顶部分隔线 */
    ui_fill_rect(0, AUTO_CTRL_Y, UI_SCREEN_W, 1, UI_COLOR_BORDER);
    /* 三按钮同排 */
    ui_draw_mode_btn();
    ui_draw_start_btn();
    ui_draw_light_btn(AUTO_LIGHT_X, AUTO_LIGHT_Y, AUTO_LIGHT_W, AUTO_LIGHT_H);
}

static void ui_remote_reset_to_home(void)
{
    g_remote_base    = UI_REMOTE_HOME_BASE;
    g_remote_upper   = UI_REMOTE_HOME_UPPER;
    g_remote_forearm = UI_REMOTE_HOME_FOREARM;
}

static void ui_toggle_power(void)
{
    switch (g_power)
    {
        case UI_POWER_OFF:
            g_power = UI_POWER_ON;
            if (UI_MODE_AUTO == g_mode)
            {
                RobotArm_SetPaused(false);   /* 防遥控 LOCKED 残留导致启动后机械臂停滞 */
                if (!g_harvest_started)
                {
                    HarvestTask_Init();
                    g_harvest_started = true;
                }
            }
            else
            {
                ui_remote_reset_to_home();
                RobotArm_MoveToWristDownTime(g_remote_base, g_remote_upper, g_remote_forearm,
                                             UI_REMOTE_GRIPPER_US, 4000U);
            }
            break;

        case UI_POWER_ON:
            /* 锁定：底盘停车 + 机械臂插值冻结，自动流程暂停（主循环不再 Service） */
            g_power = UI_POWER_LOCKED;
            Stepping_Motor_Stopping_Init();
            RobotArm_SetPaused(true);
            break;

        case UI_POWER_LOCKED:
            /* 解锁：机械臂从当前位置继续 */
            g_power = UI_POWER_ON;
            RobotArm_SetPaused(false);
            break;

        default:
            break;
    }

    if (UI_MODE_REMOTE == g_mode)
    {
        /* REMOTE：只重绘 LOCK 按钮 + 状态徽章（小区域，不闪） */
        ui_draw_button(RMT_LOCK_X, RMT_LOCK_Y, RMT_LOCK_W, RMT_LOCK_H,
                       ui_power_label(g_power), 2, false, UI_COLOR_RED_D,
                       UI_COLOR_RED, UI_COLOR_RED_D, 0xFFFFU);
        ui_draw_panel(RMT_STATE_X, RMT_STATE_Y - 4, RMT_STATE_W, RMT_STATE_H + 8,
                      UI_COLOR_CARD, UI_COLOR_BORDER);
        ui_draw_string_centered(RMT_STATE_X + RMT_STATE_W / 2, RMT_STATE_Y + 12,
                                ui_state_label(g_power), ui_power_color(g_power), 2);
        __DSB();
    }
    else
    {
        ui_redraw_screen();
    }
}

static void ui_toggle_mode(void)
{
    if (g_mode == UI_MODE_AUTO)
    {
        g_mode = UI_MODE_REMOTE;
        /* 从自动切遥控：无论 power 状态都登记回默认位（4s 平滑）。
         * LOCKED（STOP 后）时机械臂 paused，回位 move 冻结，点“继续”后平滑执行，防瞬间跳回默认位。 */
        ui_remote_reset_to_home();
        RobotArm_MoveToWristDownTime(g_remote_base, g_remote_upper, g_remote_forearm,
                                     UI_REMOTE_GRIPPER_US, 4000U);
    }
    else
    {
        g_mode = UI_MODE_AUTO;
        /* 回 AUTO 待机：不自动启动流程，舵机保持遥控姿态（防大幅跳变）；用户按开始才启动。 */
        g_power = UI_POWER_OFF;
    }

    ui_redraw_screen();
}

/* ------------------------------------------------------------------------- */
/* 遥控：摇杆双轴步进 + 抓取/开爪                                              */
/* ------------------------------------------------------------------------- */
/* 摇杆步进节流：距上次步进 >= UI_REMOTE_MOVE_MS 才允许下一步（DWT 周期计数 480MHz，
 * 32 位回绕安全）。主循环 ~60Hz 节奏下，摇杆每 ~80ms 平滑步进一次，不会急转。 */
static bool ui_joy_step_allowed(void)
{
    static uint32_t last_cyc = 0U;
    uint32_t now = DWT->CYCCNT;
    uint32_t elapsed = now - last_cyc;
    if (elapsed < ((uint32_t) UI_REMOTE_MOVE_MS * 480000UL))
    {
        return false;
    }
    last_cyc = now;
    return true;
}

static void ui_joy_apply(void)
{
    if (!ui_joy_step_allowed())
    {
        return;
    }

    int32_t base    = (int32_t) g_remote_base;
    int32_t upper   = (int32_t) g_remote_upper;
    int32_t forearm = (int32_t) g_remote_forearm;

    int dx = g_joy_x - RMT_JOY_CX;
    int dy = g_joy_y - RMT_JOY_CY;

    /* 死区：偏移过小不动作 */
    if ((dx * dx + dy * dy) < (RMT_JOY_DEAD * RMT_JOY_DEAD))
    {
        return;
    }

    /* Y 轴：上=前伸（upper/forearm 减小），下=缩回 */
    if (dy < -RMT_JOY_DEAD)
    {
        upper   -= UI_REMOTE_STEP_UPPER;
        forearm -= UI_REMOTE_STEP_FOREARM;
    }
    else if (dy > RMT_JOY_DEAD)
    {
        upper   += UI_REMOTE_STEP_UPPER;
        forearm += UI_REMOTE_STEP_FOREARM;
    }

    /* X 轴（已反转）：左=底座右转，右=底座左转 */
    if (dx < -RMT_JOY_DEAD)
    {
        base += UI_REMOTE_STEP_BASE;
    }
    else if (dx > RMT_JOY_DEAD)
    {
        base -= UI_REMOTE_STEP_BASE;
    }

    if (base < (int32_t) UI_REMOTE_BASE_MIN)
    {
        base = (int32_t) UI_REMOTE_BASE_MIN;
    }
    if (base > (int32_t) UI_REMOTE_BASE_MAX)
    {
        base = (int32_t) UI_REMOTE_BASE_MAX;
    }
    if (upper < (int32_t) UI_REMOTE_UPPER_MIN)
    {
        upper = (int32_t) UI_REMOTE_UPPER_MIN;
    }
    if (upper > (int32_t) UI_REMOTE_UPPER_MAX)
    {
        upper = (int32_t) UI_REMOTE_UPPER_MAX;
    }
    if (forearm < (int32_t) UI_REMOTE_FOREARM_MIN)
    {
        forearm = (int32_t) UI_REMOTE_FOREARM_MIN;
    }
    if (forearm > (int32_t) UI_REMOTE_FOREARM_MAX)
    {
        forearm = (int32_t) UI_REMOTE_FOREARM_MAX;
    }

    g_remote_base    = (uint16_t) base;
    g_remote_upper   = (uint16_t) upper;
    g_remote_forearm = (uint16_t) forearm;

    RobotArm_MoveToWristDownTime(g_remote_base, g_remote_upper, g_remote_forearm,
                                 UI_REMOTE_GRIPPER_US, UI_REMOTE_MOVE_MS);
}


/* ------------------------------------------------------------------------- */
/* 对外接口                                                                   */
/* ------------------------------------------------------------------------- */
void ui_control_init(void)
{
    g_mode  = UI_MODE_AUTO;
    g_power = UI_POWER_OFF;


    ui_redraw_screen();
}

/* 开机初始化提示：显示在相机区中部（深蓝底金字），主循环第一帧 blit 后自然覆盖。
 * 上电后 OV5640 配置 + NPU 初始化 + 首帧采集需 1-2s，此提示避免"黑屏"观感。 */
void ui_control_draw_boot_text(const char * msg)
{
    /* 中文提示：16x16 字库 scale 2 居中 */
    if ((uint8_t) msg[0] >= 0x80U)
    {
        ui_fill_rect(0, 336, UI_SCREEN_W, 32, UI_COLOR_BRAND_BG);
        ui_fill_rect(0, 336, UI_SCREEN_W, 2, UI_COLOR_GOLD);
        ui_draw_cn_centered(UI_SCREEN_W / 2, 336 + 16, msg, UI_COLOR_GOLD, 2);
        __DSB();
        return;
    }

    int text_w = (int) strlen(msg) * 8 * 2;   /* 8x8 字库 scale 2 */
    int x = (UI_SCREEN_W - text_w) / 2;
    if (x < 0) { x = 0; }
    ui_fill_rect(0, 336, UI_SCREEN_W, 32, UI_COLOR_BRAND_BG);
    ui_fill_rect(0, 336, UI_SCREEN_W, 2, UI_COLOR_GOLD);
    ui_draw_string(x, 340, msg, UI_COLOR_GOLD, 2);
    __DSB();
}

void ui_control_service(void)
{
    bool     touching = g_touch_updated && (g_touch_count > 0U);
    ui_hit_t hit      = UI_HIT_NONE;

    if (touching)
    {
        hit = ui_hit_test(g_touch_x, g_touch_y);
    }

    /* 边沿事件：按下瞬间触发一次 */
    if ((UI_HIT_MODE == hit) && (UI_HIT_MODE != g_touch_last))
    {
        ui_toggle_mode();
    }
    if ((UI_HIT_START == hit) && (UI_HIT_START != g_touch_last))
    {
        ui_toggle_power();
    }
    if ((UI_HIT_BACK == hit) && (UI_HIT_BACK != g_touch_last))
    {
        ui_toggle_mode();
    }
    if ((UI_HIT_LOCK == hit) && (UI_HIT_LOCK != g_touch_last))
    {
        ui_toggle_power();
    }
    if ((UI_HIT_LIGHT == hit) && (UI_HIT_LIGHT != g_touch_last))
    {
        ui_light_toggle();
    }

    /* 摇杆：按住时更新摇杆头位置并持续步进（REMOTE + 运行中） */
    if (UI_HIT_JOY == hit)
    {
        g_joy_x = g_touch_x;
        g_joy_y = g_touch_y;
        /* 限制摇杆头在底座圆内（外沿不超底座，防蓝色残影露在纯白圆外） */
        {
            int dx = g_joy_x - RMT_JOY_CX;
            int dy = g_joy_y - RMT_JOY_CY;
            int max_d = RMT_JOY_R - RMT_JOY_KNOB_R;
            int d2 = dx * dx + dy * dy;
            if (d2 > max_d * max_d)
            {
                float s = (float) max_d / sqrtf((float) d2);
                g_joy_x = RMT_JOY_CX + (int) ((float) dx * s);
                g_joy_y = RMT_JOY_CY + (int) ((float) dy * s);
            }
        }
        if ((UI_MODE_REMOTE == g_mode) && (UI_POWER_ON == g_power))
        {
            ui_joy_apply();
        }
    }

    /* 爪子切换（边沿触发，任意电源状态可用） */
    if ((UI_HIT_GRASP == hit) && (UI_HIT_GRASP != g_touch_last))
    {
        g_gripper_closed = !g_gripper_closed;
        RobotArm_MoveToWristDownTime(g_remote_base, g_remote_upper, g_remote_forearm,
                                     g_gripper_closed ? 1350U : 900U, 300U);
    }
    /* 自动抓取（边沿触发，仅在 IDLE 且非移动中启动） */
    if ((UI_HIT_AUTOGRAB == hit) && (UI_HIT_AUTOGRAB != g_touch_last)
        && (UI_AG_IDLE == g_autograb_state) && !RobotArm_IsMoving())
    {
        g_autograb_state = UI_AG_DESCEND;
        /* 下探：upper 降到 1050，forearm 2100，爪子张开 */
        RobotArm_MoveToWristDownTime(g_remote_base, 1050U, 2100U,
                                     900U, 1200U);
    }

    /* 自动抓取状态机轮询 */
    if (UI_AG_IDLE != g_autograb_state)
    {
        if (!RobotArm_IsMoving())
        {
            switch (g_autograb_state)
            {
                case UI_AG_DESCEND:
                    g_autograb_state = UI_AG_CLOSE;
                    RobotArm_MoveToWristDownTime(g_remote_base, 1050U, 2100U,
                                                 1350U, 500U);
                    g_gripper_closed = true;
                    break;

                case UI_AG_CLOSE:
                    g_autograb_state = UI_AG_IDLE;
                    RobotArm_MoveToWristDownTime(UI_REMOTE_HOME_BASE,
                                                 UI_REMOTE_HOME_UPPER,
                                                 UI_REMOTE_HOME_FOREARM,
                                                 1350U, 1200U);
                    g_remote_base    = UI_REMOTE_HOME_BASE;
                    g_remote_upper   = UI_REMOTE_HOME_UPPER;
                    g_remote_forearm = UI_REMOTE_HOME_FOREARM;
                    break;

                default:
                    g_autograb_state = UI_AG_IDLE;
                    break;
            }
            ui_draw_hit_btn(UI_HIT_GRASP, false);
            __DSB();
        }
    }

    /* 触摸状态变化时只重绘变化的按钮区域（不重绘整个面板，防闪屏） */
    if (hit != g_touch_last)
    {
        ui_hit_t old_hit = g_touch_last;
        g_touch_last = hit;
        if (UI_MODE_REMOTE == g_mode)
        {
            ui_draw_hit_btn(old_hit, false);
            ui_draw_hit_btn(hit, true);
            /* 摇杆松开：归位到中心并重绘 */
            if (UI_HIT_JOY == old_hit)
            {
                g_joy_x = RMT_JOY_CX;
                g_joy_y = RMT_JOY_CY;
                ui_draw_joy_area();
            }
        }
        else
        {
            ui_draw_auto_ctrl_bar();
        }
        __DSB();
    }
}

/* 每轮在相机 blit 之后调用：重绘被覆盖/变化的浮层。
 * REMOTE：每轮只重绘摇杆区（摇杆头 60Hz 跟随；面板其余部分状态变化时才全量画）；
 * AUTO：控制条（y600-640）整行被 blit 跳过，相机帧不覆盖 -> 无需每轮重绘，
 * 只在触摸状态变化时由 ui_control_service 重绘（防 8ms 高频重绘频闪）。 */
void ui_control_draw_overlay(void)
{
    if (UI_MODE_REMOTE == g_mode)
    {
        /* 仅操作中重绘摇杆区（拖动跟随）；未操作时摇杆头在中心且相机小窗
         * （y0-392）不覆盖摇杆区（y620）——无需每轮全画白底座（消除刷新闪烁感） */
        if (UI_HIT_JOY == g_touch_last)
        {
            ui_draw_joy_area();
        }
    }
    /* AUTO：控制条在 blit 区外且不被相机帧覆盖，无需每轮重绘 */
    __DSB();
}

/* 整屏重绘当前界面（开屏海报退场/异常恢复时调用） */
void ui_control_redraw_screen(void)
{
    ui_redraw_screen();
    __DSB();
}

ui_mode_t ui_control_get_mode(void)
{
    return g_mode;
}

ui_power_t ui_control_get_power(void)
{
    return g_power;
}

bool ui_control_is_locked(void)
{
    return (UI_POWER_LOCKED == g_power);
}

bool ui_control_get_camera_rect(int * x, int * y, int * w, int * h)
{
    if (UI_MODE_REMOTE == g_mode)
    {
        *x = 0;
        *y = RMT_CAM_Y;
        *w = UI_SCREEN_W;
        *h = RMT_CAM_H;
        return false;
    }

    /* AUTO：品牌条下方 y 28-640（顶部品牌条区域不参与 blit，避免撕裂闪烁） */
    *x = 0;
    *y = AUTO_CAM_Y;
    *w = UI_SCREEN_W;
    *h = AUTO_CAM_H;
    return true;
}

void ui_control_set_detection_count(uint32_t count)
{
    (void) count;   /* HUD 已移除，接口保留 */
}
