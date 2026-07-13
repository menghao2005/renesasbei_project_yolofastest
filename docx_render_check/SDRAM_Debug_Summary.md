# SDRAM 调试总结：GCC→LLVM 工具链迁移后 SDRAM 数据回环问题

## 1. 问题发现路径

```
framebuffer 写入数据偏差
        ↓
改用 SRAM-only 路径 → 图像输出正常
        ↓
怀疑 SDRAM 缓冲区 → 回头运行 SDRAM 读写测试
        ↓
测试失败：写入值与读回值不匹配
```

## 2. SDRAM 测试失败现象

测试模式：向 `sdram_cache[0..32767]` 顺序写入 `0,1,2,...,32767`，再读回验证。

### 2.1 关键诊断测试（直接写入已知值）

```
[A] direct write+readback test on sdram_cache[0] and [256]...
  Wrote sdram_cache[0]=0xDEADBEEF,  [1]=0x12345678
  Wrote sdram_cache[256]=0xAABBCCDD, [257]=0x11223344
  Read sdram_cache[0]=0xAABBCCDD (expect 0xDEADBEEF)    ← 被 [256] 覆盖！
  Read sdram_cache[1]=0x11223344 (expect 0x12345678)    ← 被 [257] 覆盖！
  Read sdram_cache[256]=0xAABBCCDD (expect 0xAABBCCDD)  ← 正确
  Read sdram_cache[257]=0x11223344 (expect 0x11223344)  ← 正确
```

**结论：向 `sdram_cache[256+n]` 写入会覆盖 `sdram_cache[n]` 的地址空间。即地址偏移 256-word (1024 字节) 就发生回绕。**

### 2.2 地址分析

- `sdram_cache[0]` 地址 = `0x68000000`
- `sdram_cache[256]` 地址 = `0x68000400`

差值为 `0x400` = 1024 字节 = 256×4 字节。在 32-bit 总线模式下，这对应 **地址位 A[10]** 的不同：
- `0x68000000` → A[10]=0
- `0x68000400` → A[10]=1

A[10] 被硬件忽略，说明列地址宽度与实际芯片不匹配。

## 3. 根本原因分析

### 3.1 芯片信息

SDRAM 芯片型号：**Winbond W9812G2KB-6I**

| 参数 | 值 |
|------|-----|
| 容量 | 2M×32bit×4banks = 256Mbit |
| 行地址宽度 | **12 bits** (4096 行) |
| 列地址宽度 | **8 bits** (256 列) |
| Bank 数 | 4 (2 bank bits) |
| 总线宽度 | 32-bit |

### 3.2 RA8P1 SDRAM 控制器的 MXC 配置

RA8P1 的 SDRAM 控制器通过 `SDADR.MXC[1:0]` 位配置列地址宽度：

| MXC 值 | 列地址位数 | 全寄存器值 | 宏定义值 |
|--------|-----------|-----------|---------|
| 0 | **8-bit** | 0x00 | `ROW_ADDR_OFFSET=8` |
| 1 | 9-bit | 0x01 | `ROW_ADDR_OFFSET=9` |
| 2 | 10-bit | 0x02 | `ROW_ADDR_OFFSET=10` |
| 3 | 11-bit | 0x03 | `ROW_ADDR_OFFSET=11` |

**W9812G2KB-6I 有 8-bit 列地址，需要 MXC=0。**

### 3.3 启动流程：两次 SDRAM 初始化的竞争

RA8P1 FSP 框架的启动流程中存在**两次 SDRAM 初始化调用**：

```
启动流程：
┌─────────────────────────────────────────────┐
│  ① R_BSP_WarmStart(BSP_WARM_START_POST_C)   │  ← 先执行
│     └→ R_BSP_SdramInit(true)                 │
│        使用：BSP_CFG_SDRAM_MULTIPLEX_ADDR_SHIFT │
│        位于：ra_cfg/fsp_cfg/bsp/bsp_mcu_family_cfg.h │
│        原值：1  →  MXC=1 (9-bit column)       │  ← 错误！
│                                             │
│  ② hal_entry()                              │  ← 后执行
│     └→ bsp_sdram_init()                      │
│        使用：BSP_PRV_SDRAM_SDADR_ROW_ADDR_OFFSET  │
│        位于：src/board_sdram.c                │
│        原值：9  →  MXC=1 (9-bit column)       │  ← 也是错误！
└─────────────────────────────────────────────┘
```

**关键时序问题**：第①步 warmstart 先执行，完成 SDRAM 初始化序列后 SDADR 寄存器可能在硬件层面被锁定。等第②步 `hal_entry` 调用 `bsp_sdram_init()` 时，即使修改了 MXC 值也无法生效。

### 3.4 为什么 GCC 工具链下能正常工作？

GCC 工具链的 `configuration.xml` 中 FSP GUI 配置了 `addr_shift=9`，推测原始 GCC 工程连接的 SDRAM 芯片是 **9-bit 列地址**（如 512 列 × 32-bit × 4bank = 64MB 芯片）的型号。MXC=1 对那种芯片是正确的。

LLVM 工具链迁移后，硬件换了 **W9812G2KB-6I（8-bit 列地址）**，但配置没有同步更新：

| | 原始 GCC 工程 | LLVM 迁移后 |
|---|---|---|
| SDRAM 芯片 | 未知（可能 9-bit col） | **W9812G2KB-6I** |
| 列地址宽度 | **9-bit** | **8-bit** |
| 需要的 MXC | 1 | **0** |
| configuration.xml | `addr_shift=9` ✅ | `addr_shift=9` ❌ |
| board_sdram.c | 未使用（或 ROW_ADDR_OFFSET=9） | ROW_ADDR_OFFSET=9 ❌ |

## 4. 解决思路与步骤

### 步骤 1：诊断测试，精确定位故障模式

设计了已知值写入+回读的针对性测试，发现 `sdram_cache[256]` 覆盖 `sdram_cache[0]`，定位到**精确的地址回绕步长为 256-word**（=1024 字节 = 地址位 A[10]）。

### 步骤 2：排除链接脚本 Section 名问题

发现代码中使用了 `.nocache_sdram` section，但链接脚本中定义的 section 名是 `.sdram_nocache`。修复后确认变量确实放在了 SDRAM（`sdram_cache = 0x68000000`），排除了变量放置问题。

### 步骤 3：尝试修改 ROW_ADDR_OFFSET

在 `board_sdram.c` 中尝试将 `ROW_ADDR_OFFSET` 从 9 改为 8，但测试结果**完全不变**。这说明修改没有生效——寄存器写入可能被覆盖或者根本没有执行到。

### 步骤 4：发现 warmstart 的隐秘 SDRAM 初始化

通过搜索 `bsp_sdram_init` / `R_BSP_SdramInit` 的调用关系，发现 warmstart 在 `hal_entry()` 之前就已经调用了 `R_BSP_SdramInit()`，使用的是另一个独立的宏定义 `BSP_CFG_SDRAM_MULTIPLEX_ADDR_SHIFT`：

```c
// hal_warmstart.c (比 hal_entry 更早执行)
if (BSP_WARM_START_POST_C == event) {
    R_BSP_SdramInit(true);  // ← 使用 BSP_CFG_SDRAM_MULTIPLEX_ADDR_SHIFT
}
```

### 步骤 5：最终修复

确认芯片型号后，同时修正两个配置点：

| 文件 | 修改 | 说明 |
|------|------|------|
| `ra_cfg/fsp_cfg/bsp/bsp_mcu_family_cfg.h` | `BSP_CFG_SDRAM_MULTIPLEX_ADDR_SHIFT` **1→0** | 修正 warmstart 使用的 FSP 框架配置 |
| `src/board_sdram.c` | `ROW_ADDR_OFFSET` **9→8** | 修正 hal_entry 中使用的本地配置 |
| `src/board_sdram.c` | MXC 写入移到初始化序列**之前** | 避免寄存器锁定问题 |

## 5. 经验总结

1. **排查硬件地址问题时，先做直接已知值写入测试**——它比顺序递增模式能更精确地定位地址映射错误（精确到哪个地址位出错）。

2. **同一硬件可能存在多个初始化入口**——FSP 框架的 warmstart 机制在 `hal_entry`/`main` 之前运行，如果 BSP 配置宏和本地代码不一致，会形成隐蔽的配置冲突。

3. **更换工具链时务必核对硬件配置**——GCC→LLVM 迁移如果伴随了硬件变更（SDRAM 芯片型号不同），所有相关的寄存器配置宏都需要重新审查。

4. **打印寄存器回读值是验证寄存器写入是否生效的最直接手段**——本案例中如果在测试中打印 `SDADR` 寄存器值，可以更快发现 MXC 始终为 1。
