#ifndef UI_CONTROL_H_
#define UI_CONTROL_H_

#include <stdbool.h>
#include <stdint.h>

/* 工作模式：AUTO = 自动抓取（当前工程逻辑），REMOTE = 屏幕遥控机械臂 */
typedef enum e_ui_mode
{
    UI_MODE_AUTO = 0,
    UI_MODE_REMOTE
} ui_mode_t;

/* 电源/开关三态：
 *   OFF    = 上电待机，机械臂在车库位不动（不抓取）
 *   ON     = 运行中（AUTO 跑抓取流程 / REMOTE 响应遥控按键）
 *   LOCKED = 用户再按一次开关，锁住当前状态：机械臂冻结、底盘停车、
 *            自动流程暂停（再按一次恢复继续）
 */
typedef enum e_ui_power
{
    UI_POWER_OFF = 0,
    UI_POWER_ON,
    UI_POWER_LOCKED
} ui_power_t;

void ui_control_init(void);      /* 画 AUTO 界面（banner + 品牌条 + 按钮），上电调用一次 */
void ui_control_service(void);   /* 主循环每帧调用：触摸检测 + 状态机 + 遥控步进 + 浮层重绘 */

ui_mode_t  ui_control_get_mode(void);
ui_power_t ui_control_get_power(void);
bool       ui_control_is_locked(void);
void       ui_control_set_detection_count(uint32_t count);  /* REMOTE 界面 HUD 目标数 */
void       ui_control_draw_overlay(void);                   /* 每帧相机 blit 后调用：重绘浮层 */

/* AUTO 控制条按钮矩形（与 ui_control.c 同值）：三按钮同排 y600-640，
 * hal_entry blit 整行跳过该控制条（不显示相机，按钮零闪）。 */
#define UI_AUTO_BTN_MODE_Y   (600)
#define UI_AUTO_BTN_MODE_H   (40)
#define UI_AUTO_BTN_MODE_X   (12)
#define UI_AUTO_BTN_MODE_W   (144)
#define UI_AUTO_BTN_START_Y  (600)
#define UI_AUTO_BTN_START_H  (40)
#define UI_AUTO_BTN_START_X  (168)
#define UI_AUTO_BTN_START_W  (144)
void       ui_control_redraw_screen(void);                   /* 整屏重绘当前界面（海报退场/异常恢复） */
void       ui_control_draw_boot_text(const char * msg);      /* 上电初始化提示（相机区深蓝底金字，首帧 blit 后覆盖） */

/* 当前界面下相机画面应 blit 到的目标区域（hal_entry 每帧调用）：
 *   AUTO   -> (0, 0, 480, 640) 全屏
 *   REMOTE -> (0, 32, 480, 360) 顶部小窗
 * 返回 true 表示当前是 AUTO 界面（hal_entry 据此决定是否画检测框）。 */
bool ui_control_get_camera_rect(int * x, int * y, int * w, int * h);

/* 语音模块接口 */
bool ui_control_get_voice_enabled(void);
bool ui_control_get_light_on(void);
void ui_autograb_start(void);
void ui_toggle_power(void);
void ui_toggle_mode(void);
void ui_light_toggle(void);
void ui_gripper_grasp_voice(void);
void ui_gripper_open_voice(void);

#endif /* UI_CONTROL_H_ */
