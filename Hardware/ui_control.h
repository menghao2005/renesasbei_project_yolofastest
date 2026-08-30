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

/* 读取 UI 状态机当前快照（只读，供 hal_entry/语音模块等查询） */
/* ui_control_get_mode()  -> 当前界面：AUTO 自动抓取 / REMOTE 遥控
 * ui_control_get_power() -> 当前电源三态：OFF 待机 / ON 运行 / LOCKED 锁定
 * ui_control_is_locked() -> 是否处于 LOCKED（机械臂冻结、流程暂停） */
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
 *   AUTO   -> (0, 0, 480, 600) 顶部相机区（y600-640 为三按钮控制条，不显示相机）
 *   REMOTE -> (0, 0, 480, 392) 顶部小窗
 * AI 检测仍对整帧 640x480 全图推理，与显示裁剪无关。
 * 返回 true 表示当前是 AUTO 界面（hal_entry 据此决定是否画检测框）。 */
bool ui_control_get_camera_rect(int * x, int * y, int * w, int * h);

/* 语音模块接口 */
/* （Asrpro.c 解析语音命令后调用，行为详见 ui_control.c 实现）
 * ui_control_get_voice_enabled() -> 语音功能是否开启（屏幕"语音"按钮切换）
 * ui_control_get_light_on()      -> 补光灯是否点亮
 * ui_autograb_start()            -> 语音"下抓"：启动自动抓取子状态机
 * ui_toggle_power()              -> 电源三态切换：开始/停止/继续
 * ui_toggle_mode()               -> AUTO <-> REMOTE 界面切换
 * ui_light_toggle()              -> 补光灯开关翻转（P109 高电平亮）
 * ui_gripper_grasp_voice()       -> 语音"抓取"：爪子闭合
 * ui_gripper_open_voice()        -> 语音"松开"：爪子张开 */
bool ui_control_get_voice_enabled(void);
bool ui_control_get_light_on(void);
void ui_autograb_start(void);
void ui_toggle_power(void);
void ui_toggle_mode(void);
void ui_light_toggle(void);
void ui_gripper_grasp_voice(void);
void ui_gripper_open_voice(void);

#endif /* UI_CONTROL_H_ */
