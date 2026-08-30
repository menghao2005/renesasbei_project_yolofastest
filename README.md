# renesasbei_project_yolofastest

基于 **Renesas RA8P1**(Cortex-M85 @ 1GHz + Ethos-U55 NPU @ 500MHz)的水果识别与机械臂自动抓取演示工程:
OV5640 相机采集 → **YOLO-Fastest int8 全 NPU 推理** → LCD 手绘 UI 实时画框 → 语音 / 触摸 / 遥控交互 → 舵机机械臂自动抓取。

> 工程内部构建名 `RA8P1_Demo_LLVM_V4`(e2 studio / LLVM Embedded Toolchain for ARM)。

---

## 功能特性

- **AI 检测(AUTO 模式)**:YOLO-Fastest 变体 `best_heads.onnx` 经 Renesas RUHMI/MERA 工具链量化(int8)并编译为 Ethos-U55 命令流,**105 个卷积类算子 100% 跑在 NPU**,CPU 侧零算子(`cpu_us = 0`);后处理(解码/NMS/平滑)在 M85 上完成,检测框 + 中文类别标签直绘 framebuffer。
- **流水线**:CEU DMA 双缓冲采集与 NPU 推理重叠执行,中断级 kick,实测 **~7.2 fps**,断流 2s 自愈恢复。
- **手绘双界面 UI**:480×800 屏,AUTO(检测 + 抓取)与 REMOTE(摇杆遥控)两个界面,纯 framebuffer 绘制(品牌条 / 底部 banner / 按钮 / spinner / 检测计数),触摸 + 语音双输入,按钮零遮挡零闪烁。
- **机械臂自动抓取**:由检测框中心偏差 `g_ai_center_offset` 驱动的非阻塞状态机(对准 → 下探 → 闭合 → 缩回),含树上抓取模式与面积判据。
- **交互**:GT911/st7123 触摸、ASRPRO 语音模块(UART 帧协议)、差速底盘步进电机(按距离行驶)。
- **诊断**:HardFault/BusFault 等异常现场经 SEGGER RTT dump(PC/LR/栈帧);UART9 `[PIPE]`/`[MODEL]` 行输出逐帧性能剖析。

## 硬件组成

| 模块 | 型号 / 接口 |
|---|---|
| MCU | Renesas RA8P1(Cortex-M85 1GHz + Ethos-U55-256 NPU,板载 SDRAM) |
| 相机 | OV5640,DVP 接口,640×480 RGB565 @ 15fps,经 CEU DMA 写 SDRAM 双缓冲 |
| 显示 | 480×800 MIPI-DSI 屏(GLCDC + Dave2D 硬件缩放),ST7102/触摸中断 |
| 触摸 | GT911 / st7123 |
| 语音 | ASRPRO 模块(UART9 之外的串口,帧协议) |
| 执行机构 | 舵机机械臂(GPT 定时器 PWM)+ 步进电机差速底盘(GPT 比较翻转波形) |
| 调试 | UART9 调试串口 + SEGGER RTT + J-Link 烧录 |

## 目录结构

```
├── src/
│   ├── hal_entry.c          # 主入口:初始化 + 双缓冲主循环 + AI 流水线调度
│   ├── models/              # RUHMI 生成 + 手改的推理封装(NPU 子图 sub_0000)
│   │   ├── model.c/h        #   GetModelInputPtr / GetModelOutputPtr / RunModelProfiled
│   │   ├── sub_0000_invoke.c / sub_0000_command_stream.c   # NPU 命令流调用
│   │   ├── sub_0000_model_data.c                           # int8 权重(308.5KB,放 Flash)
│   │   └── sub_0000_tensors.h                              # arena 1228800B 与张量偏移
│   ├── CAMERA/              # OV5640 驱动、CEU 采集、YUV→RGB 转换、I2C 封装
│   ├── Graphics/            # Dave2D 封装(blit 硬件缩放)
│   ├── gt911.c              # 触摸 IC 驱动
│   ├── mipi_dsi_ep.c        # MIPI-DSI 屏初始化 + ST7123 触摸中断接入
│   └── board_sdram.c        # SDRAM 初始化
├── Hardware/
│   ├── ai_preprocess.c      # RGB565 → letterbox 320×320 int8(查表量化)
│   ├── ai_postprocess.c     # P4/P5 raw 解码 + NMS + 平滑 + framebuffer 画框
│   ├── ai_center_offset.c   # 检测框中心 ↔ 显示区中心偏差(给抓取状态机)
│   ├── harvest_task.c       # 自动抓取状态机
│   ├── robot_arm.c          # 舵机机械臂(角度映射 / 缓动 / 抓取服务)
│   ├── Stepping_Motor.c     # 差速底盘步进电机驱动
│   ├── ui_control.c         # 手绘双界面 UI 状态机(按钮 / 摇杆 / 模式)
│   ├── Asrpro.c             # ASRPRO 语音模块协议
│   ├── bottom_banner.c      # 底部 banner 绘制
│   └── fault_diag.c         # HardFault 等异常现场诊断
├── ra_gen/ ra_cfg/ ra/      # FSP 生成代码 + FSP 源码(含 Ethos-U55 / TFLite-Micro)
├── conversion_results/      # 模型转换产物与日志(量化统计 / 分区 / Vela 报告)
├── script/                  # 链接脚本 fsp.lld
├── AI_流程与内存说明.md      # AI 链路 / 工作区 / 性能优化记录
├── AI_模型分层部署指南.md    # ★ 每层 CPU/NPU 如何分配 + SRAM/SDRAM/Flash 放置通则
└── HANDOFF.md               # 历史交接文档
```

## 构建与烧录

**e2 studio(推荐)**:安装 FSP + RUHMI Model Transfer 插件,导入本工程,选 `RA8P1_Demo_LLVM_V4` Debug/Release 配置直接 Build,用 J-Link 烧录(工程根目录附 `.jlink` 脚本)。

**命令行**:

```bash
# 工具链:LLVM Embedded Toolchain for ARM(e2 studio 自带,加入 PATH)
set PATH=C:\Renesas\e2_studio\toolchains\llvm_arm\ATfE-<版本>\bin;%PATH%
cd Debug
make -j          # 需把 script/fsp.lld 放到 Debug/ 或改用 e2 studio 构建
```

> ⚠️ `Debug/*/subdir.mk` 是 e2 studio 自动生成的;在 IDE 里改过工程配置重新生成后,记得同步 `Project Properties → C/C++ Build → Settings → Optimization`(`Hardware/` 与 `src/models/` 目录用 `-O2`)。

## 性能数据(实测)

| 指标 | 数值 |
|---|---|
| 相机出帧(中断级 kick 流水线) | ~7.2 fps |
| NPU 推理(model=) | ~138 ms(136.3M MACs,u55-256 @ 500MHz,硬成本) |
| CPU 子图耗时(cpu=) | **0**(Transpose 已废弃,后处理直读 raw NPU 输出) |
| 预处理(pre=) | 数 ms 级(查表量化,Q16 定标最近邻) |
| 模型体积 | int8 权重 308.5 KB(原 fp32 1.2 MB,287.8K 参数) |

UART9 每 30 帧打印一次:`[PIPE] frame= fps= total= ceu_wait= blit= pre= model= post= ...` 与 `[MODEL] frame= total= cpu= npu= copy=`。

## 相关文档

- [AI_流程与内存说明.md](AI_流程与内存说明.md) —— 当前主流程、特殊内存区域(`.sdram` / `.sdram_nocache`)、性能优化与清理记录
- [AI_模型分层部署指南.md](AI_模型分层部署指南.md) —— **本模型每一层在 CPU/NPU 上的分配实测**、换其他模型时的评估方法、SRAM/SDRAM/Flash 放置通则与主流部署方案对比
- [HANDOFF.md](HANDOFF.md) —— 历史交接文档

## 致谢与许可

- 推理运行时与生成代码含 EdgeCortix / Renesas Electronics / TensorFlow(Apache-2.0)版权代码,各源文件头部保留其原始声明。
- 模型基于开源 [YOLO-Fastest(dog-qiuqiu/ModelZoo-YOLOFastest)](https://github.com/dog-qiuqiu/YOLOFastest) 体系自训练(数据集 fruit_6lei:红椒 / 梨 / 南瓜 / 苹果 / 洋葱 5 类)。
