**问题现象**
我正在使用 RA8P1 的 GLCDC + MIPI DSI 驱动一块 4.3 寸 480x800 MIPI 屏，屏幕控制 IC 为 ST7102，屏模组资料标注为 BOE 4.3 / YDP430BT009 / GX09C\_ST7102+BOE4.3\_2LANE\_90HZ 相关资料。

当前屏幕可以正常点亮，初始化过程无 FSP error，RTT 中没有 GLCDC open/start failed，也没有观察到 GLCDC Graphics 1 Underflow 打印。屏幕显示纯色时完全正常，但显示图片、色块分割图、测试图案时，会出现明显重影/虚影/密集条纹。现象很像奇偶行起始 X 坐标不同：例如第 1 行从 x=0 开始，第 2 行像从 x=若干像素偏移开始，第 3 行又正常，第 4 行又偏移。条纹位置会跟随图像内容边界移动，不是固定在屏幕某些物理行/列。

**已验证现象**

* ST7102 内部 BIST 测试图案正常。通过发送 0xB5, 0x85 后，屏幕内部色块/彩条/测试图显示完美，无错位、无重影。
* 纯色填充正常，例如全红、全绿、全白等没有可见异常。
* 显示企鹅图片、分区色块、边界明显的合成图案时出现重影/密集条纹。
* 图案边界移动后，异常位置也跟着移动。
* DCache 已关闭，BSP\_CFG\_DCACHE\_ENABLED=0，所以不像缓存未 clean 导致。
* 开启 GLCDC Graphics 1 Underflow interrupt 后，没有观察到 underflow 计数增加。
* framebuffer 地址、stride、RGB565 写入方式已反复检查。当前 GLCDC input layer 是 RGB565，framebuffer 按 2 字节/像素处理，DISPLAY\_BUFFER\_STRIDE\_PIXELS\_INPUT0 为 480，行跨度看起来正确。
* 将写入地址从 fb\_background[0] 改为 g\_display\_cfg.input[0].p\_base / gp\_single\_buffer 后，现象无变化，基本排除写入缓冲区和 GLCDC 扫描缓冲区不一致。

**RA8P1 GLCDC + MIPI DSI 驱动 ST7102 4.3 寸屏幕重影问题说明**

问题交接与技术支持请求 | 整理日期：2026-06-21

|  |  |
| --- | --- |
| **项目** | **说明** |
| **目标** | 请协助确认 RA8P1 GLCDC/MIPI DSI 视频输出配置、DSI video packet 行为，以及与 ST7102/BOE 480x800 2-lane 屏的匹配关系。 |
| **核心现象** | 纯色与 ST7102 内部 BIST 正常；外部 GLCDC/DSI 视频流显示图片、色块边界、棋盘格时出现固定规律的横向错位/密集条纹。 |
| **当前判断** | 更像外部视频流进入屏幕后的 packet/timing/format/初始化匹配问题，而不像单纯 LCD 玻璃损坏、显存越界、DCache 或 GLCDC underflow。 |

# 1. 项目与硬件背景

|  |  |
| --- | --- |
| **项目** | **当前信息** |
| MCU/平台 | Renesas RA8P1，e2 studio/FSP 工程，主要路径 D:\e2s\_workspace\RA8P1\_Demo\_LLVM\src。 |
| 显示链路 | GLCDC 输出视频流，经 RA MIPI DSI 发送到 4.3 寸 MIPI DSI TFT 屏。 |
| 屏幕 | 4.3 寸 480 x RGB(3) x 800，ST7102 + BOE，资料中出现 YDP430BT009-V1 与 GX09C\_ST7102+BOE4.3\_2LANE\_90HZ 两组初始化来源。 |
| 测试范围 | 当前仅测试 LCD 显示，CEU/OV5640 摄像头相关路径可暂时忽略。 |
| 硬件连接 | 自制转接板，恒流背光可正常点亮；MIPI D0/CLK/D1 均按差分走线，组内等长。用户提供长度：D0P/N 约 1167 mil，CLKP/N 约 1143 mil，D1P/N 约 1140 mil。 |

# 2. 当前问题现象

* 屏幕可以正常上电发光，MIPI 初始化流程未打印 Failed，GLCDC/MIPI 初始化整体能跑通。
* 显示纯色时肉眼观察基本正常，没有明显横向错位或异常花屏。
* 通过 ST7102 内部 BIST/测试图案指令（曾加入 0xB5, 0x85）后，屏幕内部显示的多种测试图案非常正常，没有错位。
* 显示 MCU/GLCDC 外部帧缓冲生成的图片、色块边界、棋盘格、灰阶细线时，出现固定规律的密集条纹/重影。
* 现象类似奇数行与偶数行的水平起始位置不同：例如第一行从 x=0 开始，第二行像从 x=若干像素偏移开始，第三行又回到 x=0。
* 条纹位置会随绘图内容边界移动，不是固定在屏幕某些物理列/行上。

![](data:image/jpeg;base64...)

图 1：色块分割边界出现密集横向条纹，纯色区域相对稳定。

![](data:image/jpeg;base64...)

图 2：彩条、棋盘格和灰阶细线区域可见规律性错位/重影。

![IMG_256](data:image/jpeg;base64...)

图 3：工程测试图像可见规律性错位/重影。

# 3. 当前主要配置

## 3.1 GLCDC 输入/帧缓冲

|  |  |
| --- | --- |
| **配置项** | **当前值/说明** |
| 输入层格式 | DISPLAY\_IN\_FORMAT\_16BITS\_RGB565 |
| 帧缓冲 | fb\_background[2][DISPLAY\_BUFFER\_STRIDE\_BYTES\_INPUT0 \* DISPLAY\_VSIZE\_INPUT0]，位于 .sdram\_noinit。 |
| 像素字节数 | BYTES\_PER\_PIXEL = 2，LCD\_BITS\_PER\_PIXEL = 16。 |
| 行跨度 | DISPLAY\_BUFFER\_STRIDE\_PIXELS\_INPUT0；当前 480 像素宽，RGB565 每行约 960 bytes，满足 4 字节对齐。 |
| D/AVE 2D | d2\_framebuffer(..., LCD\_XSTRIDE\_PHYS, LCD\_XSIZE\_PHYS, LCD\_YSIZE\_PHYS, d2\_mode\_rgb565)。 |
| DCache | BSP\_CFG\_DCACHE\_ENABLED=0，当前不按 DCache 脏数据方向排查。 |

## 3.2 GLCDC 输出 Timing

|  |  |  |  |
| --- | --- | --- | --- |
| **方向** | **FSP 字段** | **当前值** | **换算/备注** |
| H | total\_cyc | 564 | Linux/商家资料对应：480 + 2 + 40 + 40 = 562；FSP 字段 back\_porch 包含 sync\_width 时，当前有效 HBP = 42 - 2 = 40。 |
| H | display\_cyc | 480 | 有效显示宽度。 |
| H | back\_porch | 42 | FSP 生成 DSI horizontal\_back\_porch = 42 - 2 = 40。 |
| H | sync\_width | 2 | HSYNC 宽度。 |
| V | total\_cyc | 1124 | 800 + 2 + 10 + 310 = 1122；FSP 字段 back\_porch 包含 sync\_width 时，当前有效 VBP = 12 - 2 = 10。 |
| V | display\_cyc | 800 | 有效显示高度。 |
| V | back\_porch | 12 | FSP 生成 DSI vertical\_back\_porch = 12 - 2 = 10。 |
| V | sync\_width | 2 | VSYNC 宽度。 |

|  |  |
| --- | --- |
| **信号项** | **当前/已试情况** |
| HSYNC/VSYNC polarity | 当前 Low active；High/Low 及相关边沿曾反复测试，未解决。 |
| DE polarity | High active 可显示；DE Low 曾导致黑屏/不亮。 |
| sync edge | 当前 Falling；边沿相关配置曾测试，未解决。 |
| output format | 当前 DISPLAY\_OUT\_FORMAT\_24BITS\_RGB888；也试过 16-bit RGB565，现象基本无变化。 |

## 3.3 MIPI DSI / MIPI PHY

|  |  |
| --- | --- |
| **配置项** | **当前/已试情况** |
| video data type | 当前 MIPI\_DSI\_VIDEO\_DATA\_24RGB\_PIXEL\_STREAM；也试过 16RGB。 |
| data lanes | 当前 2 lane；1 lane 曾测试但无显示。 |
| sync\_pulse | 当前 sync\_pulse = 0；Sync Event / Sync Pulse、HSE/VSE transmitted/not transmitted 均试过。 |
| continuous clock | Enable；相关选项反复测试未解决。 |
| No LP in HSA/HBP/HFP | 三项开/关均试过，未解决。 |
| EoTP | Enable/Disable 均试过，未解决。 |
| DSI PLL | 820 MHz 在当前配置下屏幕不亮；1000 MHz 可以显示但仍重影。 |
| video\_mode\_delay | 尝试调整过，无明显变化。 |

|  |
| --- |
| // ra\_gen/common\_data.c 当前关键片段 .sync\_pulse = (0), .data\_type = MIPI\_DSI\_VIDEO\_DATA\_24RGB\_PIXEL\_STREAM, .vertical\_active\_lines = 800, .vertical\_sync\_lines = 2, .vertical\_back\_porch = (12 - 2), .vertical\_front\_porch = (1124 - 800 - 12 - 2), .horizontal\_active\_lines = 480, .horizontal\_sync\_lines = 2, .horizontal\_back\_porch = (42 - 2), .horizontal\_front\_porch = (564 - 480 - 42 - 2), .video\_mode\_delay = 100, .num\_lanes = 2, .continuous\_clock = (1), |

# 4. 屏幕资料与初始化来源

|  |  |
| --- | --- |
| **资料/代码** | **用途与观察** |
| YDP\_430\_BT\_009\_V1\_a418b8470b.pdf | 产品网站下载的 4.3 寸屏资料，包含分辨率/面板信息。 |
| ST\_7102\_Datasheet\_V0\_22\_01eb7cedda.pdf | ST7102 芯片资料，用于查询私有寄存器、BIST、GIP/source scan 相关配置。 |
| ST7102+BOE4.3\_YDP430BT009-V1.c | 最早从产品网站下载的初始化代码；移植后仍有显示重影。 |
| GX09C\_ST7102+BOE4.3\_2LANE\_90HZ.c | 后续在 gitee/资料包中发现，docs 中也引用该 2-lane 90Hz 初始化。当前更倾向以此作为屏厂参考初始化。 |
| ESP-IDF ST7102 示例 | 包含 esp\_lcd\_st7102 / st7102\_init\_cmds.h 等，函数封装与 RA FSP 不同，但可参考初始化表和 timing。 |
| Linux dtsi 示例 | 明确给出 MIPI\_DSI\_MODE\_VIDEO | MIPI\_DSI\_MODE\_VIDEO\_BURST | MIPI\_DSI\_MODE\_LPM，format RGB888，lanes=2，clock-frequency=56750760。 |
| Renesas RA8D1 CEU+GLCDC+MIPI-DSI 示例 | 同类 FSP 工程参考，但屏幕不同，且示例中存在 BYTES\_PER\_PIXEL=4 等不适合当前 RGB565 输入路径的代码，不宜照搬。 |

|  |
| --- |
| **关于 ST7102 内部 BIST 的重要观察**  加入内部测试图案指令后，屏幕自身输出的测试图案无错位。这强烈说明屏玻璃、source/gate 扫描基础能力、背光与大部分 MIPI 初始化是可工作的；问题更集中在外部视频流进入 ST7102 后的 packet/timing/format/桥接匹配。 |

# 5. 已完成排查与结果

|  |  |  |  |
| --- | --- | --- | --- |
| **排查项** | **已做操作** | **结果** | **倾向结论** |
| 纯色显示 | 填充 framebuffer 为纯色。 | 纯色正常。 | 不是全局链路完全失败，也不像简单 lane 不通。 |
| 屏内 BIST | 发送 0xB5, 0x85 进入内部测试图案。 | 多种测试图案完全正常。 | 屏内部扫描/玻璃基础功能正常；外部视频流路径更可疑。 |
| GLCDC stride | 检查 DISPLAY\_BUFFER\_STRIDE\_PIXELS\_INPUT0、BYTES\_PER\_PIXEL=2、RGB565 写入。 | 行跨度逻辑与 RGB565 匹配。 | 不是典型每行跨度字节数错误。 |
| DCache | 确认 BSP\_CFG\_DCACHE\_ENABLED=0。 | 缓存方向基本排除。 | 无需依赖 Clean/Invalidate 修复。 |
| GLCDC underflow | 开启 Underflow 1 中断和 callback 计数/打印。 | 直到显示企鹅图也未观察到 underflow 打印。 | 不像 GLCDC 带宽不足或取数断流。 |
| 色彩格式 | GLCDC output RGB565/RGB888、DSI 16RGB/24RGB、ST7102 0x3A=0x55/0x77 均曾尝试。 | 现象变化不明显。 | 需要确认 RA8P1 GLCDC->DSI 转换与 DSI data type 的真实生效路径。 |
| Timing/polarity | HSYNC/VSYNC polarity、sync edge、DE polarity、PCLK 37.8/40/57/60 MHz 附近尝试。 | DE Low 会黑屏，其余未解决重影。 | 不是简单极性/像素时钟一个参数能解释。 |
| DSI packet 相关 | Sync Event/Sync Pulse、HSE/VSE、EoTP、No LP、continuous clock 反复测试。 | 未解决。 | 仍需官方确认 RA FSP 是否支持/如何配置 burst video mode。 |
| Lane 数 | 2 lane 可显示；1 lane 测试无显示。 | 1 lane 无显示。 | 当前屏/初始化/带宽更可能要求 2 lane；单 lane 不宜作为主方向。 |
| PHY 频率 | 820 MHz 和 1000 MHz 测试。 | 820 MHz 黑屏，1000 MHz 显示但重影。 | 速率裕量会影响是否点亮，但不是当前重影唯一因素。 |
| 论坛检索 | 检索 Renesas community、野火论坛、21ic 等中文/英文关键词。 | 未找到高度匹配公开案例。 | 建议提交给瑞萨/屏厂做寄存器级确认。 |

# 6. 初步判断

|  |
| --- |
| **当前最有价值的缩小范围**  如果屏内 BIST 完全正常，而外部 GLCDC/DSI 视频流在边界和奇偶行上出现规律性错位，则问题优先级应从 framebuffer/DCache/纯硬件损坏，转向 RA8P1 DSI video packet 模式、GLCDC 输出格式到 DSI data type 的桥接、ST7102 对 video burst/non-burst 的要求，以及屏厂私有初始化表与当前视频模式是否严格匹配。 |

* 不优先怀疑：LCD 玻璃坏、背光问题、单纯 D0/D1/CLK 某一对完全不通、DCache 未刷、GLCDC underflow、RGB565 framebuffer 基本行跨度错误。
* 仍需重点确认：RA8P1 FSP 的 MIPI DSI 是否能配置 Linux dtsi 中的 MIPI\_DSI\_MODE\_VIDEO\_BURST 等价模式，或者默认 packet 行为是否与该屏要求不一致。
* 仍需重点确认：GLCDC input RGB565 -> output RGB888 -> DSI 24RGB 是否为 RA8P1 官方支持且推荐的转换链路；若屏厂文档写 480 x RGB(3) x 800，最终 DSI video data type 是否应固定为 RGB888。
* 仍需重点确认：ST7102 私有寄存器中是否存在 odd/even source scan、GIP、source mapping、line buffer/pixel packing 相关参数需要与 video mode/burst mode 联动。
* 硬件方向不能 100% 排除，但用户提供的 lane 间长度差约几十 mil，通常更容易表现为随机误码/闪点/黑屏，而不是稳定的奇偶行水平偏移。建议官方给出 D-PHY 眼图/时序测量关注点。

# 7. 希望瑞萨协助确认的问题

1. RA8P1 FSP MIPI DSI 是否支持与 Linux MIPI\_DSI\_MODE\_VIDEO\_BURST 等价的视频 burst mode？若支持，应在 e2 studio/FSP 哪个字段配置，或是否需要直接写 MIPI DSI 寄存器？
2. 当前 mipi\_dsi\_cfg\_t 里只看到 sync\_pulse、data\_type、timing、video\_mode\_delay、no\_lp 等字段。FSP 生成的 r\_mipi\_dsi.c 是否默认使用 non-burst video mode？这类屏若要求 burst，会不会造成类似奇偶行/边界错位？
3. GLCDC 输入 RGB565、输出 24-bit RGB888、DSI data type 24RGB 这一路径在 RA8P1 上是否推荐？GLCDC output format 改变后画面几乎无变化，是否说明 DSI data type 或 panel 0x3A 未真正按预期生效？
4. 对于 ST7102 + BOE 480x800 2-lane 屏，瑞萨是否有推荐 timing、MIPI PHY PLL、lane byte clock、video packet 参数或参考工程？
5. 若屏幕内部 BIST 正常、GLCDC underflow 未触发、DCache 关闭，瑞萨更建议从 GLCDC、MIPI DSI、MIPI PHY、还是 panel init 哪一层抓寄存器和波形？
6. RA8P1 是否有官方工具/寄存器 dump 方法，可以把 GLCDC 或 MIPI DSI 实际输出的视频流还原/检查为图像，或读取 packet data type、line packet length、blanking packet 等关键状态？
7. 当前 2-lane 布线 lane 间长度差约 27 mil 量级，是否可能导致稳定奇偶行偏移？如果需要示波器验证，请给出 D-PHY HS clock/data、lane skew、LP/HS 切换的建议测量点和判据。
8. ST7102 初始化中类似 0xA6、0xA7、0xAC、0xAD、0xE7 等私有寄存器，是否有与 odd/even scan、source line mapping、GIP 或 RGB/DSI pixel packing 相关的解释？是否需要屏厂提供更完整说明？

# 8. 附：关键参考 timing

|  |  |
| --- | --- |
| **来源** | **关键参数** |
| Linux dtsi | dsi,flags = MIPI\_DSI\_MODE\_VIDEO | MIPI\_DSI\_MODE\_VIDEO\_BURST | MIPI\_DSI\_MODE\_LPM；format RGB888；lanes=2。 |
| Linux dtsi timing | clock-frequency=56,750,760；H: active 480, sync 2, back porch 40, front porch 40；V: active 800, sync 2, back porch 10, front porch 310。 |
| 当前 FSP 换算 | HBP=42-2=40，HFP=564-480-42-2=40；VBP=12-2=10，VFP=1124-800-12-2=310。 |
| 差异点 | FSP UI 的 total/back\_porch 字段表达方式与 Linux dtsi 不完全同名，需要确认 FSP 生成 DSI 寄存器后是否与屏厂 timing 完全一致。 |

# 9. 附：建议一并提交的工程/资料

* 当前工程：D:\e2s\_workspace\RA8P1\_Demo\_LLVM，重点文件 src\mipi\_dsi\_ep.c、src\Graphics\graphics.c、src\Graphics\graphics.h、ra\_gen\common\_data.c、configuration.xml。
* 屏幕资料：ziliao\ST\_7102\_Datasheet\_V0\_22\_01eb7cedda.pdf、ziliao\YDP\_430\_BT\_009\_V1\_a418b8470b.pdf。
* 屏厂/社区初始化：ziliao\GX09C\_ST7102+BOE4.3\_2LANE\_90HZ.c、ziliao\ST7102+BOE4.3\_YDP430BT009-V1.c。
* 参考示例：ziliao\4.3-tft-480x800-mipi-st7102-main\examples\linux\rk3566-lubancat-dsi0-vp0-4.3inch.dtsi、ESP-IDF ST7102 示例、Renesas RA8D1 CEU+GLCDC+MIPI-DSI 示例。
* 建议同时附上本文中的两张现象照片，以及若可获取的示波器 D-PHY 波形或 RA MIPI DSI/GLCDC 关键寄存器 dump。

# 10. 当前结论摘要

|  |
| --- |
| **给技术支持的简短版结论**  本问题不是“屏幕完全不亮”或“颜色格式简单错误”，而是外部视频流显示复杂图案时出现规律性奇偶行水平错位。屏幕内部 BIST 正常、纯色正常、DCache 关闭、GLCDC underflow 未触发，因此请优先协助确认 RA8P1 MIPI DSI video burst/non-burst packet 行为、GLCDC 输出格式到 DSI data type 的转换链路，以及 ST7102 私有初始化是否需要与该视频模式匹配。 |