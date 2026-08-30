# AI 流程与内存说明

> 本文件在 2026-08 做了一轮性能优化后重写。旧的「别的 AI 总结」版本已删除。

## 当前主流程

`hal_entry()` 里的初始化顺序：

1. IOPORT / UART9（调试串口）
2. 机械臂 `RobotArm_Init()` + 舵机定时器
3. Dave2D `graphics_init()` + MIPI 显示 + 底部 banner
4. I2C + `ov5640_init()` + 相机配置（VGA RGB565 15fps）
5. CEU 初始化
6. Ethos-U55 `RM_ETHOSU_Open`
7. 进入双缓冲主循环

主循环里每一帧的顺序（采集与处理重叠）：

1. 先 `prepare_camera_capture_buffer`（DCache 失效）并启动下一帧采集 `ceu_capture_start`
2. 等待 VSYNC（带 1s 超时）
3. Dave2D 硬件缩放：640×480 → 屏幕 480×640 区域
4. 画上一次已提交的检测框（`ai_draw_detections`）
5. 对上一帧完整图做预处理 `ai_preprocess_rgb565`
6. `RunModelProfiled(false)` 跑模型（NPU + CPU 子图）
7. 后处理 `ai_postprocess` + 中心偏移 `ai_center_offset_calc`
8. 机械臂抓取状态机 + 服务函数
9. 等待当前采集完成 `ceu_capture_wait`，交换双缓冲指针

## 当前 AI 处理

- 输入：`640×480 RGB565`
- 预处理：转成 `320×320 RGB int8 NHWC`，量化 `q = pixel - 128`，letterbox（640×480 → 320×240 无缩放裁切，上下各 pad 40 行，scale=0.5）
- 推理：TFLite int8，Ethos-U55（NPU 子图 `sub_0000_invoke`）+ CPU 子图 `compute_sub_0001`
- 输出：P4 / P5
  - P4：`1,20,20,3,10`（12000 字节），stride 16，量化 scale=0.26804847 / zp=67
  - P5：`1,10,10,3,10`（3000 字节），stride 32，量化 scale=0.24737312 / zp=69
- 属性顺序（已上板验证）：`tx, ty, tw, th, objectness/conf, cls0..cls4`
- 类别（5 类）：`hongjiao, li, nangua, pingguo, yangcong`
- 后处理：解码 P4/P5 → NMS → 保留 Top1 → 平滑/锁定 → 画框 + 中心偏移（供机械臂抓取）

## 特殊内存区域

### `.sdram`（可缓存 SDRAM）

- `g_image_vga_sdram[2]`：相机双缓冲（每帧 640×480×2 = 614400 字节）

CPU 读写这块更快，预处理直接从这里的显示帧取数。

### `.sdram_nocache`（非缓存 SDRAM）

- `sub_0000_arena[1228800]`：NPU 子图工作区，含模型输入 `serving_default_images_0`、NPU 中间 tensor、NPU 输出（`model_tf_compat_v1_transpose_*`）

NPU 和 CEU 直接读写这里，避免 cache 一致性问题。

### 显示缓冲

- `gp_frame_buffer` 是 GLCDC 显示链路 framebuffer（放在片上 `.ram_nocache`，避免和模型/相机抢 SDRAM 带宽导致抖动）。

## 本次性能优化记录（2026-08）

### 1. 优化 `Transpose()`（`src/models/kernel_library_int.c`）

CPU 子图 `compute_sub_0001` 里的 4 次 `Transpose` 是纯布局转换（`[1,3,10,grid,grid]` → `[1,w,h,3,10]`）。原实现对每个输出元素做 rank 次 `%` 和 `/`，而 Cortex-M85 没有硬件除法，代价很高。

已改为「混合进制里程表」增量进位：除法只在初始化时算一次，内层只剩比较/加减。用工程里 4 组真实 P4/P5 参数验证过输出逐字节一致。

> ⚠️ **注意**：`kernel_library_int.c` 是 RUHMI 生成的代码，**重新转换模型会覆盖此函数**，需重新应用这个优化。

### 2. 编译选项：`-Os` → `-O2` + 去掉 `-fno-unroll-loops`

针对 CPU 热点目录（`Hardware/` 和 `src/models/`），在 `Debug/*/subdir.mk` 里：

- `-Os` → `-O2`（优化速度，此前是优化体积）
- 删除 `-fno-unroll-loops`（恢复默认循环展开）

> ⚠️ **注意**：`subdir.mk` 是 e² studio 自动生成的。如果在 e² studio 里改了工程配置重新生成，会覆盖这些改动，需在 **Project Properties → C/C++ Build → Settings → Optimization** 里同步设置（或直接命令行 `make`）。

### 3. 彻底删除 Transpose（后处理直接读 raw NPU 输出）

`compute_sub_0001` 的 4 次 `Transpose` 本质是纯布局转换 `[1,3,10,grid,grid]` → `[1,w,h,3,10]`。已改为**后处理直接读 raw NPU 输出**，`model.c` 不再调用 `compute_sub_0001`：

- `GetModelOutputPtr_*` 直接返回 raw 指针（`sub_0000_arena + sub_0000_address_model_tf_compat_v1_transpose_*`）
- `ai_decode_branch` 用映射 `final[w][h][anc][attr] == raw[anc][attr][h][w]` 直接索引（已用 Python 对拍验证）
- `cpu_us` 归零，`[MODEL]` 的 `cpu=` 应为 0

> ⚠️ `model.c` 里 `compute_arena_sub_0001`、`buf_PartitionedCall_*` 定义已闲置（`--gc-sections` 会回收或浪费少量 SDRAM），后续可清理。——**已于 2026-08-31 彻底删除，见下方清理记录。**

### 4. 后处理 logit 阈值（免 sigmoid/expf）

objectness 每帧 1500 次 `expf`，改为**反量化到 logit 空间直接比较阈值**（sigmoid 单调，等价），只有通过 obj+cls 阈值的少数 cell 才真正算 sigmoid。logit 阈值：`logit(0.55)=0.2006707`、`logit(0.60)=0.4054651`。

> 注意：若以后改 `AI_POST_OBJ_THRESHOLD` / `AI_POST_CLS_THRESHOLD`，需同步改 `ai_decode_branch` 里的这两个 logit 常量。

## 性能状态

> 以下为**优化前**的实测数据（旧文档遗留），优化后待上板重新测量。

| 阶段 | 优化前耗时 |
|---|---|
| 预处理 pre | 6~8 ms |
| 模型 model | 138~140 ms |
| 总耗时 total | 184~188 ms |

优化后请重点看 UART 打印的 `[MODEL]` 行的 `cpu=`（Transpose 时间）和 `[PIPE]` 行的 `model=`、`total=`，对比优化前的数据。

### 已知的小问题

- `model.c` 里 `model_profile_t` 的 `copy_us` 字段从未被赋值，UART 打印的 `copy=` 恒为 0，无参考意义。
- `RunModelProfiled(false)` 传的 `clean_outputs=false`，NPU 输出不清零（正常，性能考虑）。

## 2026-08-31 代码清理与修正记录

### 1. 删除死代码（已验证编译 + 链接通过）

- `src/models/compute_sub_0001.{c,h}`、`kernel_library_int.{c,h}`、`kernel_library_utils.{c,h}`：CPU 子图 Transpose 废弃后整链已无人调用，连同 `Debug/*/subdir.mk` 里的编译项一并移除（省 ~30KB SDRAM + 编译时间）。
- `Hardware/qspi_flash_test.{c,h}`：QSPI 读写测试工具，全工程无调用。
- `model.c/h` 的 `RunModel()`：只有 `RunModelProfiled()` 在被使用。
- `ai_center_offset_print()`：早先的调试打印，已无人调用；`hal_entry.c` 对应的多余 `#include "ai_center_offset.h"` 一并移除。

### 2. D-Cache 维护方式修正

- `prepare_camera_capture_buffer`：`SCB_CleanInvalidateDCache_by_Addr` → `SCB_InvalidateDCache_by_Addr`。采集缓冲由 CEU（DMA）写入、CPU 只读，启动采集前只需失效（丢弃缓存里的旧行），Clean 反而会把缓存中的陈旧数据写回 SDRAM。
- `ceu.c` 的 `yuv422_to_rgb888 / yuv422_to_rgb565`：删除了函数内的 `SCB_EnableDCache / SCB_DisableDCache` 开关（在函数里全局开关 DCache 会影响中断/其他模块的缓存一致性，改由调用侧统一管理）。

### 3. 首帧等待改为非阻塞 spinner

`hal_entry` 首帧不再用 `ceu_capture_wait(5000)` 阻塞（spinner 会冻住 5 秒），改为轮询 `g_ceu_completed_buf`、每 ~33ms 重绘一次 spinner，5s 超时后重试一次 kick，仍失败则继续进主循环（断流自愈兜底）。

### 4. 相机显示区域调整（AUTO 界面）

AUTO 模式相机 blit 目标从全屏 480x640 改为 480x600（y600~640 为模式/开始/灯光三按钮控制条，整行不显示相机，按钮零遮挡零闪烁）；REMOTE 模式顶部小窗 480x392。AI 推理仍对整帧 640x480 做，与显示裁剪无关。

## 后续可继续优化的方向

1. **NPU 架构层面**：模型从 SDRAM 跑，若下一版模型能塞进片上 SRAM、或提高 NPU 时钟、或降输入分辨率/换更小 backbone，才能把 ~138ms 这个硬成本（NPU 主时间）拦腰砍。软件层面已基本榨干。
2. **性能数据待重测**：上板看 UART 的 `[PIPE]`/`[MODEL]` 行，回填本文「性能状态」表。
