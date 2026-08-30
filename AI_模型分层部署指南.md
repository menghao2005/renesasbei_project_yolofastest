# AI 模型分层部署指南:每层 CPU/NPU 怎么分、数据放哪、主流方案

> 本文所有数字均来自本工程真实产物,可直接复现核对:
> - 算子清单与切分:`conversion_results/conversion_tool_settings/Conversion_Tool_260714_1954*.log`(MERA 2.6.0 + Vela 4.2.0,模型 `best_heads.onnx`,YOLO-Fastest 变体)
> - 部署代码:`src/models/`(sub_0000 系列)、`Debug/memory_regions.lld`(内存实配)
> - 板上实测:UART9 的 `[MODEL]`/`[PIPE]` 打印

---

## 1. 一句话原理

**Ethos-U55 是"卷积专用"固定功能加速器,不是通用处理器。** 转换工具(MERA/Vela)逐个检查模型算子:

- 卷积家族(Conv2D / DepthwiseConv2D / FullyConnected)、逐元素加减乘、池化、上/下采样、常见激活(Sigmoid/Tanh/ReLU/ReLU6)→ **编译成 NPU 命令流**,生成 `sub_XXXX`(command stream + arena);
- NPU 不支持或受限的算子(Transpose、部分 Reshape/Concat、Softmax、ArgMax、StridedSlice、Gather、TopK、NMS 等)→ **回退 CPU**,生成 `sub_XXXX__CPU_C_CODEGEN`(本工程旧称 `compute_sub_0001`)。

于是一张完整的图被 **`DivideByTarget` 切成若干 region**,按拓扑顺序交替执行:NPU 子图 ↔ CPU 子图。**CPU 子图越多,数据在 SDRAM 里来回倒腾的次数越多,延迟越差**——好的部署 = 尽量少的 region 切换 + 尽量小的 CPU 岛。

## 2. 本模型的算子清单(量化后实测)

量化阶段输出(100 张标定图,MinMax 校准):

| 节点类型 | 数量 | 权重体积 | 归属 |
|---|---:|---:|---|
| TFLiteQConv2dBias | **86** | 308.4 KB | **NPU** |
| TFLiteRelu6 | 56 | — | 被融合进 Conv,不占独立算子 |
| TFLiteQAdd | 18 | — | **NPU**(neck 捷径相加) |
| TFLiteTranspose | 5 | 88 B | **CPU**(布局转换) |
| TFLiteResizeNearestNeighbor | 1 | 8 B | **NPU**(FPA 上采样) |
| TFLiteConcatenate | 1 | — | 图边界(CPU/NPU 交界) |
| CanReshape | 2 | 40 B | CPU(元数据/布局) |
| Var(输入) | 1 | 0 | — |

合计 **169 个节点 / 287.8K 参数;fp32 1.2 MB → int8 308.5 KB(3.73× 压缩)**。
结构上对应 YOLO-Fastest 的三段:主干(Conv+ReLU6)→ FPA 特征金字塔(ResizeNN + Concat)→ 两个检测头。

## 3. 实际切分:NPU/CPU 区域划分(逐层分配结果)

### 3.1 转换日志的切分

`DivideByTarget` 把整图切成 **5 个 region:3 个 CPU + 2 个 NPU**,形态为 `CPU | NPU | CPU | NPU | CPU`。两个 NPU 区域的 Vela 独立报告:

| | NPU 大区域 | NPU 小区域 |
|---|---:|---:|
| NPU 算子数 | 99(占该区域 **100%**) | 9(占该区域 **100%**) |
| 计算量 | 136.3 M MACs | 14.9 M MACs |
| SRAM 需求(arena) | **1200.00 KiB** | 96.88 KiB |
| Flash 需求(权重+命令流) | 389.03 KiB | 47.14 KiB |
| NPU 输入/输出 SRAM 流量 | 11.77 / 8.70 MB/批 | 0.54 / 0.25 MB/批 |
| 权重 Flash 流量 | 1.47 MB/批 | 0.17 MB/批 |

> 报告里的 `CPU operators = 0 (0.0%)` 指**每个 NPU 区域内部**没有 CPU 算子;全部布局类算子都落在 3 个 CPU region 里。配置:Ethos-U55-256 @ 500 MHz,memory mode `Sram_Only`,权重常量被自动改放 **OnChipFlash**(`Changing const_mem_area from Sram to OnChipFlash`)。

### 3.2 最终部署版的切分(实际跑在板子上的)

`src/models/` 只有两个子图:

- **`sub_0000`(NPU)**:arena `kArenaSize_sub_0000 = 1228800 B = 1200 KiB`,与上表大区域完全一致——主干 + FPA + 双头的全部卷积类算子一次跑完,输出直接是两个检测头的 raw tensor(`model_tf_compat_v1_transpose_6/2_*`,12000 + 3000 B)。
- **`compute_sub_0001`(CPU)**:4 个 Transpose,做 `[1,3,10,grid,grid] → [1,grid,grid,3,10]` 的属性布局转换(已于 2026-08-31 删除——后处理直接按 `raw[anc][attr][h][w]` 索引 raw 输出,`cpu_us` 归零,还省下 30KB SDRAM)。

也就是说,最终导出的 ONNX 已经把中间布局算子清理干净,只剩输出端 Transpose 这一个 CPU 岛;而我们进一步把这个岛也消掉了。**当前板上的真实分工:100% 的神经网络算子在 NPU,CPU 只做预处理(letterbox/量化)和后处理(解码/NMS/画框),这两块本来就是约定俗成不放进模型图的。**

### 3.3 逐层分配总表(按算子类)

| 模型里的层 | 数量 | 执行单元 | 备注 |
|---|---:|---|---|
| Conv2D(+融合 ReLU6) | 86 | **NPU** | 命令流主体,136+15 M MACs |
| Add(残差) | 18 | **NPU** | |
| ResizeNearestNeighbor | 1 | **NPU** | FPA 上采样 |
| Concat | 1 | NPU/CPU 交界 | 转换日志显示被融合处理 |
| Transpose(输出布局) | 2~5 | **CPU → 已删除** | `[1,3,10,g,g] → [1,g,g,3,10]`,后处理直读 raw |
| Reshape | 2 | CPU(近零成本,纯元数据) | |
| 解码/sigmoid/expf/NMS/画框 | — | **CPU(M85 手写)** | 从不放进模型图;logit 阈值预筛后每帧只有少数 cell 真正算 sigmoid |

## 4. 为什么这么切:判断规则

1. **查支持度**:Vela 给每个 TFLite 算子标 support level(Full / Extended = NPU;CPU only = 回退)。U55 对卷积家族支持最全;Transpose 仅支持部分置换,Reshape 零成本(纯重解释),Softmax/池化/Reduction 受 shape 限制。
2. **看 CPU 岛的位置**:落在主干中间的 CPU 岛最伤(两侧数据都要搬);落在输入/输出端的代价小。本工程的 Transpose 岛恰好在输出端,所以删掉后没有任何"搬运回 NPU"的问题。
3. **看 arena 大小**:NPU region 的 arena 决定它能塞进哪块内存(见 §6)。
4. **板上验证**:`[MODEL] cpu=` 看 CPU 子图耗时;若不为 0 且很大,优先消灭 CPU 岛,而不是优化 CPU 代码。

## 5. 换别的模型怎么办(通用方法论 + 常见模型类型的典型分配)

### 5.1 四步评估法

1. **导出 ONNX 时就有"算子白名单"意识**:NHWC 布局、避免 Gather/StridedSlice/TopK 留在图里、Upsample 映射成 ResizeNearest、能在外部做的变换(如输出的 Transpose/Softmax/decode)直接砍出模型。
2. **转换后先看两处**:算子 census(哪些回 CPU)和 `DivideByTarget` 的 region 数与 CPU 岛大小;目标:**region 少、CPU 岛薄、 arena 放得下**。
3. **看内存**:arena ≤ 片上 SRAM 就用 SRAM;放不下才去 SDRAM(代价是 NPU 访存带宽)。
4. **上板量测**:`[MODEL] cpu=/npu=`、`[PIPE] model=/total=`,对齐 Vela 报告的 MACs 数量级做 sanity check。

### 5.2 常见模型类型的典型 CPU/NPU 分配

| 模型类型 | 放 NPU | 留 CPU(或图外) | 注意点 |
|---|---|---|---|
| CNN 分类 | 全部卷积主干 | 末端 Softmax(很轻,无所谓) | 最理想情形,通常单 NPU region |
| YOLO 系检测 | backbone + neck + head 卷积 | decode / Transpose / NMS | **本工程路线:输出砍 Transpose 直读 raw,NMS 手写** |
| 语义分割 | 卷积主干 | argmax、上采样对齐、CRF | 输出是 per-pixel 大 tensor,注意输出搬运量 |
| Transformer / 小 LM | 能映射成 FC/Conv 的矩阵乘 | Softmax、LayerNorm、GELU、attention 缩放 | U55 对 attention 支持有限,token embedding/采样建议 CPU;attention 重 modelo 建议 U65 或 CPU 为主 |
| 音频(KWS/命令词) | 卷积声学模型 | MFCC/前端特征、CTC/关键词比对 | 前端特征提取是 DSP 活,CPU 做 |
| 异常检测/AE | 卷积主体 | 误差阈值判断 | |

### 5.3 消灭 CPU 岛的常用手段

- **结构改写**:用 NPU 友好算子替代(如把 SiLU 写成 Conv+Mul 融合形式、把通道打乱换成 1×1 Conv);
- **图外化**:Transpose/decode/NMS 挪出模型,像本工程一样"后处理直读 raw";
- **量化**:PTQ(MinMax / 百分位 / 熵校准,本工程用 MinMax 100 样本)够用就别 QAT;权重 int8、激活 int8,Flash 体积与带宽同降;
- **Vela/MERA 选项**:`--optimise Performance|Size`、`memory_mode`、`accelerator-config`(u55-128/256,MAC 阵列越大越快)。

## 6. 数据放哪:SRAM / SDRAM / Flash 放置通则

### 6.1 本芯片(RA8P1)的内存实配(链接脚本 `Debug/memory_regions.lld`)

| 区域 | 基址 | 容量 | 特点 |
|---|---|---:|---|
| 片上 SRAM(RAM) | 0x2200_0000 | **1824 KiB** | 最快(峰值 1.86 GB/s),NPU 高速口 |
| ITCM / DTCM | 0x0 / 0x2000_0000 | 128 + 128 KiB | 零抖动,放关键代码/栈 |
| Code Flash(XIP) | 0x0200_0000 | 1 MiB | 只读,带 cache |
| SDRAM | 0x6800_0000 | **128 MiB** | 大而慢,分可缓存/非缓存段 |
| OSPI0/1 | 0x8000_0000 等 | 128~256 MiB | QSPI 大容量资源/字库/OTA |

### 6.2 放置决策表

| 数据类型 | 首选 | 理由 | 本工程实例 |
|---|---|---|---|
| 模型权重(const)| **片上 Flash(XIP)** | 只读、掉电保持、cache 命中后带宽够用 | `sub_0000_model_data.c` 389 KiB → Flash;Vela 实测权重流量 1.47 MB/批 @ 0.11 GB/s |
| NPU 命令流 | Flash | 小且只读 | `sub_0000_command_stream.c` |
| **NPU arena** | **优先片上 SRAM;放不下 → SDRAM 非缓存段** | arena 是 NPU 每帧高频读写的工作区,带宽决定推理时间 | 本工程 arena 1200 KiB + 显示 fb 750 KiB 超过 SRAM 1824 KiB → arena 放 `.sdram_nocache` |
| DMA 采集缓冲(相机帧) | 大 → SDRAM;小 → SRAM 非缓存 | 外设直写;**必须 32B 对齐 + cache 维护** | `g_image_vga_sdram[2]` 各 600 KiB,可缓存段,采集前 `SCB_InvalidateDCache_by_Addr` |
| 显示 framebuffer | SRAM 非缓存(防与 AI 抢 SDRAM 带宽)或 SDRAM | GLCDC 实时扫描不能被卡 | `gp_frame_buffer` 750 KiB → `.ram_nocache` |
| 栈 / 热变量 / 小字模表 | SRAM / TCM | 延迟最低 | — |
| 大资源(全量字库、语音、OTA 备份) | OSPI/QSPI Flash | 容量换速度 | `assets/` |

### 6.3 Cache 一致性规则(本工程踩过的坑)

- **CPU 写 → 外设读**:启动 DMA 前 `Clean`(把缓存里的新数据刷下去);
- **外设写 → CPU 读**:读之前 `Invalidate`(丢弃缓存里的旧行)——本工程曾把"采集前 Clean+Invalidate"改成纯 Invalidate,避免把陈旧缓存行写回 SDRAM 污染新帧;
- **不要在转换函数里全局开关 DCache**(会波及中断和其他模块的一致性),统一在缓冲的"生产/消费边界"做维护;
- DMA 缓冲 32 字节对齐(`BSP_ALIGN_VARIABLE(32)`),按整帧长度做 cache 操作。

### 6.4 本工程的容量账本(为什么不放 SRAM)

- NPU arena 1200 KiB + 相机双缓冲 1200 KiB = 2400 KiB > 片上 SRAM 1824 KiB → **大缓冲只能去 SDRAM**;
- 显示 fb 750 KiB 占据 SRAM 非缓存段,剩余留给栈/数据;
- **可做的实验**:若把 fb 挪到 SDRAM,arena(1200 KiB)刚好能塞进 SRAM(1824 KiB),NPU 访存从 SDRAM 总线升级到 1.86 GB/s 片上 SRAM,对 138 ms 推理里"权重 1.47 MB + 输入输出 20.5 MB"的流量是实打实的提速——这是下一个最有价值的优化方向。

## 7. 主流部署方案盘点(MCU/嵌入式侧)

| 方案 | 平台 | 工具链 | 特点 |
|---|---|---|---|
| **ARM Ethos-U + TFLM/LiteRT + Vela**(本工程) | RA8P1、Corstone、Alif/Dolphin 等带 U55/U65 的 SoC | MERA/RUHMI、ethos-u-vela、TFLM | 量化 int8 + NPU 命令流,本仓库即完整实例 |
| CMSIS-NN 纯 CPU | 全系 Cortex-M | CMSIS-NN 库 | 无 NPU 时的最优软件卷积,可与 NPU 混用 |
| Renesas RZ/V Drp-AI | RZ/V2L、V2H | Renesas AI SDK | MP 级 Linux 方案,DRAM 带宽富裕 |
| ST STM32N6 | Neural-ART NPU | STM32Cube.AI / CubeMX | ST 生态一体化,与 RA8P1 同级竞品 |
| NXP eIQ | i.MX RT(部分带 Neutron NPU) | eIQ Toolkit | RT 侧全栈,支持 TF/ONNX/DLR 多后端 |
| microTVM / ONNX Runtime | 通用 | TVM | 灵活但要自己管内存,嵌入式成熟度低于 TFLM |
| 轻量自研(K210/ESP-DL 等) | K210、ESP32-P4 | 厂商 SDK | 便宜、够用,算子支持面窄 |

**选型口诀**:有 NPU 先看"算子覆盖率 + arena 大小"(§5),没 NPU 就 CMSIS-NN/手写定点;模型先行——在 PC 上用 Vela 干跑一次,拿到 region 数、arena、MACs 再定硬件配置,比先买板子再碰壁划算得多。

## 8. 快速检查清单(本工程复用)

- [ ] 切分:`Conversion_Tool_*.log` 搜 `DivideByTarget` / `Network summary`(region 数、CPU operators、SRAM/Flash 用量)
- [ ] arena:`src/models/sub_0000_tensors.h` 的 `kArenaSize_sub_0000` vs 链接脚本 SRAM 容量
- [ ] CPU 占比:UART `[MODEL] cpu=` 应为 0(或极小);不为 0 → 回到 §5.3 消 CPU 岛
- [ ] 带宽:Vela 报告 `Total SRAM bandwidth`/`On-chip Flash bandwidth` 对照实测 fps
- [ ] Cache:DMA 缓冲对齐 32B、生产/消费边界的 Clean/Invalidate(§6.3)
