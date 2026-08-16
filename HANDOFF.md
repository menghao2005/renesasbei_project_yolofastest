# RA8P1 采摘机器人工程 · 交接文档（V2 主工程）

> 供新会话快速上手。读完本文件即可继续开发，无需向用户重复提问基础问题。
> 最后更新：2026-08-15（V2 工程建立：队友原始工程为基线 + 手绘 UI 移植成功）

---

## 0. ⚠️ 工程位置（2026-08-15 变更）

**当前主工程（本文件所在目录）：**
```
C:\Users\menghao2005\Downloads\RA8P1_Peoject\RA8P1_Demo_LLVM_V2
```
- 工程名：RA8P1_Demo_LLVM_V2（.project 已改名；elf 名 RA8P1_Demo_LLVM_V2.elf）
- **旧工程 `RA8P1_Demo_LLVM_current (1)` 已废弃**（原因已查明：**品牌条 logo 位图绘制 `ui_draw_bitmap_scaled` 导致 HardFault**——与 g_hstride 配合写坏内存，Dave2D 调用跳飞 PC=0xFBE0（恰好是 logo 金色像素值），IBUSERR。旧工程串口停在 `UI v2 init` 后实为同一 bug，当时误判为 I2C 卡死。V2 去掉品牌条 logo 后正常）
- V2 = 队友原始工程（AI 采摘链路完整、验证能跑）+ 移植的手绘双界面 UI + fault_diag + RobotArm_SetPaused

## 1. 工程概况

**作品**：2026 全国大学生电子设计竞赛（瑞萨杯）——「轻量化端侧AI赋能的眼手协同果蔬采摘机器人」
**团队**：苏州工学院（孟浩/陆豪/孙億浩）
**主控**：Renesas RA8P1（Cortex-M85 480MHz，Ethos-U55 NPU）
**功能**：OV5640 相机 → CEU 采集 → NPU(YOLOFastest int8) 水果检测 → 5 舵机机械臂抓取 + 步进电机底盘

## 2. 硬件资源

- QSPI NOR Flash 32MB；32 位 SDRAM 16MB（链接脚本 SDRAM_LENGTH=0x08000000 是 128MB 配置，实际 16MB，当前用量 ~3MB）
- 屏幕：480x800 竖屏 RGB565（GLCDC + MIPI-DSI + Dave2D）
- 触摸：GT911（gt911.c，全局 g_touch_x/y/count/updated）
- 机械臂：5 舵机（GPT 非阻塞插值）；底盘：步进电机

## 3. 构建环境（重要，含新踩的坑）

**命令行构建（Git Bash）：**
```bash
export PATH="/c/Renesas/e2_studio/toolchains/llvm_arm/ATfE-21.1.1-Windows-x86_64/bin:/c/Renesas/e2_studio/eclipse/plugins/com.renesas.ide.exttools.gnumake.win32.x86_64_4.3.1.v20240909-0854/mk:$PATH"
cd "工程/Debug" && make all
```
- 产物：`Debug/RA8P1_Demo_LLVM_V2.elf`（text ~622KB / bss ~3.2MB）
- 严格警告 `-Wconversion` 等，必须零警告（CMSIS 头文件既有警告除外）

**⚠️ 从队友/他人机器复制工程后必查（本次踩过）：**
1. `Debug/makefile.init` 里有 `export PATH=...` **覆盖环境 PATH**，是生成机器的路径快照——换机器必须改成自己机器的（本次队友 `D:\Renesas` → 本机 `C:\Renesas`），否则 echo/clang 全找不到、构建报 `process_begin: CreateProcess(NULL, echo ...) failed`
2. 所有 `subdir.mk` 里的 `-I"E:\\Download\\..."` 绝对路径是队友机器路径，需批量替换成本机工程路径（Python 脚本替换）
3. `Debug/` 下必须有 `.sbd`（smart bundle，烧录必需）：`RA8P1_Demo_LLVM_V2.sbd` 已从队友工程复制改名（.sbd 是 ZIP，内部只有芯片 secure 配置 R7KA8P1KFLCAC.rzone 等，**无工程名/elf 名引用，可直接复制改名**）。若 IDE Clean 后 .sbd 消失且烧录报 "Cannot find smart bundle"：从任意同芯片工程 Debug/ 复制 .sbd 改名即可
4. secure 三件套（.secure_azone/.secure_rzone/.secure_xml）在 Debug/ 下（IDE 构建自动生成）

**IDE 构建**：用户用 e² studio Debug 配置 Run。IDE managed build 每次重新生成 subdir.mk/makefile——手动补丁会被覆盖。新增源文件/新 include 路径必须注册进 `.cproject`（sourceEntries 目录级 `name="Hardware"` 已含 Hardware/ 全部 .c；compiler.includesFiles 已有 Hardware 路径）。

**patch 工具 CRLF 坑**：hal_entry.c 等 CRLF 文件里字符串含 `\r\n` 时，patch 会把转义拆坏（`\r` 变真实换行）——必须用 Python 字节级替换（write_file 写 .py 再执行）。

## 4. UI 实现（手绘帧缓冲，无 LVGL）

同旧版：Hardware/ui_control.c（~950 行）直接写 framebuffer，8x8 点阵字库（font_8x8.h，仅 ASCII）。

### AUTO 界面（上电默认）
```
0-28    品牌条：深蓝底(0x0907)+金色底线，金字 RENESAS CUP 2026 + NUEDC（**无 logo 位图**——用户要求，且 logo 绘制会 HardFault）
28-640  相机画面 480x612 铺满（blit 从 y=28 开始）+ MODE(360,484,104x40) + START(272,540,192x56)
640-800 原工程 banner 渐变（bottom_banner.c，深绿渐变）+ 瑞萨/电赛 logo
```
- START 两态：RUN（绿）↔ STOP（红），锁定显示 RESUME
- MODE/START 每帧重画（ui_control_draw_overlay，在 blit 后）

### REMOTE 界面（MODE 键切换）
```
0-28    品牌条；28-388 相机小窗 480x360（无 HUD 角标——TARGET 已删，用户要求）
392-800 控制面板：BACK(8,400) / 状态徽章(白卡片) / LOCK(368,400 红)
        左：虚拟摇杆（圆心140,620 半径88 + 摇杆头34 跟随手指）
        右：GRASP(292,540,164x64 绿) + OPEN(292,636,164x64 蓝)
```
- 摇杆步进分辨率（2026-08-15 减半防急转）：BASE=20 / UPPER=12 / FOREARM=15 us，DWT 节流 ≥80ms/步（ui_joy_step_allowed）；主循环 vsync 60Hz 节奏保证摇杆流畅跟手（不受相机 8fps 限制）
- **2026-08-16 修复（卡住+黑屏+摇杆不跟手）**：① UI 浮层重绘移出相机帧门控（每轮 60Hz 无条件重绘，REMOTE 下每轮画整个面板→摇杆头拖动实时跟随，之前只在按下/松开瞬间重绘）② 相机断流自愈：主循环检测 ~2s 无帧 → `ceu_recover()`（ceu.c 新增，换同步配置+重开 CEU）→ 重新 kick，之前断流后 UI/画面永久冻结
- **2026-08-16 修复 v2（屏闪/残留）**：① AUTO 品牌条+按钮每轮重绘 + blit 后立即重绘（防相机覆盖残留/按钮闪烁）② REMOTE 面板背景从相机窗下沿 y388 起画（盖掉 388-392 缝隙残留）③ REMOTE 每轮只重绘摇杆区 `ui_draw_joy_area()`（绘制量 -85%，下半区撕裂消失）；全面板只在按下/松开/状态变化时画
- **2026-08-16 修复 v3（触摸失灵 + 帧率感知 + 画面稳定）**：① 触摸：删掉每次触摸的 printf（串口阻塞拖垮 I2C 时序）+ 读失败重试 1 次 + 连续 5 次失败自动 IIC Close/Open 复位（`st7123_i2c_recover`）+ 失败打印降频（25:1）② fps 打印改真实画面帧率（帧周期含相机等待，≈8fps；之前 18.26 是处理吞吐虚高）③ `ceu_capture_start` 不再每帧轮换同步极性（16 种极性每帧换→CEU 同步抖动/错误帧），仅 init/断流自愈时轮换
- **2026-08-16 修复 v4（品牌条闪烁 + 上电黑屏）**：① 品牌条重绘从"60Hz overlay 每轮"改为"相机帧 blit 后（8fps）"（`ui_control_draw_brand_bar`，AUTO 仅）——60Hz 全量 fill 480x28 撕裂闪烁，8fps 不可见且仍防 blit 边缘残留；banner logo 从不重绘所以一直不闪 ② 上电初始化提示 `ui_control_draw_boot_text()`：相机区深蓝底金字按阶段显示 CAMERA INIT → NPU INIT → READY（OV5640 898 次 I2C 写入 + NPU 初始化 ≈1-2s 相机区黑屏，提示消除"黑屏"观感）
- **2026-08-16 开屏海报（v5）**：tjpgd（ChaN，纯 C）解码内置 JPEG 480x800 全屏海报（真机照片+UI 风格框架，62.6KB flash）→ 上电 `jpeg_show_splash()` 覆盖初始界面 → 相机/NPU 初始化期间显示 → 主循环第一帧 blit 前 `ui_control_redraw_screen()` 一次性重绘界面退场。tjpgd 改造：去 RT-Thread 依赖（rtthread.h/rt_inline→static inline），JD_FORMAT=1 RGB565，JD_SZBUF=4096。新增：Hardware/tjpgd.c/h、jpeg_splash.c/h、splash_jpeg_data.c（自动生成）；品牌条改为**只绘制一次**（ui_redraw_screen 时）+ 一次性 [BRAND] OK/CORRUPTED 验证（blit 是否越界污染金字区）
- **⚠️ tjpgd 工作区坑（HardFault 根因）**：tjpgd 的 pool（jd_prepare 传入）要同时容纳流输入缓冲（JD_SZBUF）**+ Huffman/量化表/IDCT/MCU 缓冲**——只给 4096 会**溢出写坏 pool 相邻 SRAM**（s_work 与 g_hstride 相邻，LR=0x221351A8 实锤）→ 函数返回地址被 JPEG 数据污染 → 跳飞到海报数据区 IBUSERR（PC=0xFBE0 金色常量值，BFAR=0x020A7660=g_splash_jpg 内）。**修复：pool 给 32768**；jpg_out_func 加 rect 越界防御（OOB 中断解码）
- **2026-08-16 海报已回滚（用户决定）**：pool 32KB 后仍崩（BFAR 0x020A7660→0x020A7710 几乎同位 = 确定性跳飞非溢出）→ 反汇编发现 PC=0xFBE0 是**空 ITCM**（非金色常量，程序早已跳飞后 garbage 执行路径）→ 真正跳飞是某间接跳转目标被写成 flash 数据地址（g_splash_jpg 内，XN 区取指 IBUSERR）。**海报方案挂起**（hal_entry 已移除 jpeg_show_splash/退场逻辑，subdir.mk 已移除 tjpgd/jpeg_splash/splash_jpeg_data；文件保留在 Hardware/ 未删，以后可查）。恢复为 boot text 提示版（CAMERA INIT→NPU INIT→READY）
- **Dave2D blit 耗时**：AUTO 全屏缩放 ~10ms（硬件时钟相关，属处理链固有）；REMOTE 小窗更快；AUTO 每帧处理链 = kick(DCache 清洗) + blit 10ms + pre 3ms + NPU 20.6ms ≈ 54ms，期间触摸/UI 阻塞（AUTO 固有代价，REMOTE 无 AI 快很多）
- 面板按钮/徽章：状态变化时由 ui_control_service 全量重绘；摇杆区每轮由 overlay 重绘
- 状态机：POWER OFF --START--> ON --START/LOCK--> LOCKED --START/LOCK--> ON
- LOCK = 电源三态（RobotArm_SetPaused 冻结机械臂 + 底盘停车）
- 面板在 blit 区外；摇杆区每轮重绘（ui_control_draw_overlay → ui_draw_joy_area，摇杆头实时跟随），按钮/徽章状态变化时全量重绘
- **Dave2D**：已用于相机缩放 blit（graphics_blit_scale 硬件加速）；UI 图元（圆/渐变/文字）为 CPU 直写 framebuffer（Dave2D 不支持图元绘制）
- **CPU1（双核）**：当前工程 `BSP_MULTICORE_PROJECT=0`（单核模式），启用需 FSP 多核工程改造（CPU0/1 双固件分区+IPC+linker），比赛前不建议动

### 配色（浅蓝灰精致版，2026-08-15 更新）
- 全部 RGB565 宏已修正为 16 位正确值（旧版 24 位值截断成怪色：如"绿"0x1FD6 实际青绿）
- BG=0xDF3E 浅蓝灰、CARD 白、BORDER=0xADFA 蓝灰描边、DARK_TEXT=0x1189 深蓝灰
- 品牌条深藏青 0x0907 + 金字 0xFBE0；GREEN=0x1EAA、RED=0xF208、BLUE=0x3D3F、ACCENT=0x2C5D
- 按钮：投影(0xAE3B) + 垂直渐变 + 顶部高光 + 圆角；浅色按钮深字、彩色按钮白字
- 状态徽章：白色圆角卡片 + 蓝灰描边 + 彩色文字

## 5. 关键文件

| 文件 | 作用 |
|---|---|
| `src/hal_entry.c` | 主循环：**触摸/UI 服务每轮执行 → 相机帧非阻塞轮询（`g_capture_ready`）→ kick 下一帧（流水线）→ blit(动态窗口) → 检测框(仅 AUTO) → AI(仅 AUTO) → UI 浮层每轮无条件重绘（60Hz 跟手，与相机帧率解耦）→ vsync 60Hz 节奏**；**无帧 ~2s 超时自动 `ceu_recover()`（断流自愈，UI 不冻结）** |
| `Hardware/ui_control.c/.h` | 全部 UI（双界面/摇杆/状态机） |
| `Hardware/robot_arm.c/.h` | 机械臂（含 RobotArm_SetPaused 冻结） |
| `Hardware/bottom_banner.c` | banner 渐变 + 瑞萨/电赛 logo |
| `Hardware/ai_postprocess.h` | 检测框映射常量 FB_CAM_W=480, FB_CAM_H=612, FB_BOX_OFFSET_Y=28 |
| `Hardware/fault_diag.c` | HardFault 诊断（RTT 打印） |
| `src/CAMERA/ov5640.c` | 相机驱动（⚠️ ov5640_init 内 get_chip_id 失败会 while(1) 死循环——相机排线/供电问题会导致整机卡死，串口停在 `[DBG] ov5640_init...` 后无输出） |

## 6. 已解决的问题（避免重复踩坑）

1. **旧工程 (1) 烧录后相机 I2C 卡死**：驱动文件与队友版一致 → 换队友工程为基线重建 V2 解决（未定位根因，疑似旧工程 IDE 构建状态污染）
2. **队友 makefile 路径硬编码**：`E:\Download\...`（subdir.mk -I 路径）+ `makefile.init` PATH（D:\Renesas）→ 已批量替换/复制本机版
3. **smart bundle 缺失**：复制同芯片工程 .sbd 改名（内部无工程名引用）
4. **patch 工具 CRLF 字符串坑**：hal_entry.c 用 Python 字节级
5. **e² studio 覆盖手动补丁**：构建文件改动走 .cproject
6. **clang @file 转义**：makefile 参数文件路径用普通引号 + 双反斜杠
7. **相机帧率**：OV5640 DVP 卡 8fps，软件提升有限，勿再折腾寄存器
8. **LVGL 已移除**：勿再引入（用户拍板：代码多/烧录慢/HardFault 史）

## 7. 待办

1. **用户验证 V2 完整功能**：摇杆遥控/GRASP/OPEN/锁定/自动抓取（V2 首次移植，功能需全流程实测）
2. UI 进一步美化（用户嫌"简陋枯燥"）——**改 UI 前必须先出 PIL 效果图预览确认**
3. 相机偶发初始化失败排查（如果复现）：先查排线/供电，再考虑把 ov5640_init 的 while(1) 改成容错返回

## 8. 用户偏好

- 中文回复；专业术语中英对照
- UI：保留 RENESAS CUP 2026 + NUEDC 品牌；无帧率显示；界面文字仅英文（8x8 字库无中文）
- 先出效果图预览确认再写 UI 代码
- 用户会自己在工程外手动改文件（PPT 等），继续编辑前重读最新版
- "烧录慢工程就有问题"：保持代码轻量，勿再引入大库

## 9. 常用操作速查

- 命令行构建：见第 3 节 export + make
- 烧录：e² studio 导入 RA8P1_Demo_LLVM_V2 → Debug 配置 Run（.sbd 已在 Debug/）
- 看故障：JLinkRTTViewer.exe 连 R7KA8P1KF；HardFault 会打 RTT（fault_diag）
- 触摸方向不对：改 ui_control.c hit-test 坐标映射
- 机械臂参数：ui_control.c 的 UI_REMOTE_* 宏
