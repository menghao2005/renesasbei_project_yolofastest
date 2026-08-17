#include "Uart9_Debug.h"
/*
 * This file is developed by EdgeCortix Inc. to be used with certain Renesas Electronics Hardware only.
 *
 * Copyright © 2025 EdgeCortix Inc. Licensed to Renesas Electronics Corporation with the
 * right to sublicense under the Apache License, Version 2.0.
 *
 * This file also includes source code originally developed by the Renesas Electronics Corporation.
 * The Renesas disclaimer below applies to any Renesas-originated portions for usage of the code.
 *
 * The Renesas Electronics Corporation
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED 'AS IS' AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Changed from original python code to C source code.
 * Copyright (C) 2017 Renesas Electronics Corporation. All rights reserved.
 *
 * This file also includes source codes originally developed by the TensorFlow Authors which were distributed under the following conditions.
 *
 * The TensorFlow Authors
 * Copyright 2023 The Apache Software Foundation
 *
 * This product includes software developed at
 * The Apache Software Foundation (http://www.apache.org/).
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "model.h"

// CPU compute declarations
#include "sub_0000_invoke.h"
#include "compute_sub_0001.h"
#include "bsp_api.h"

#define MODEL_PROFILE_PRINT_INTERVAL (30U)

extern void DWT_init(void);
extern uint32_t DWT_get_count(void);
extern uint32_t DWT_count_to_us(uint32_t delta_count);

static model_profile_t g_last_model_profile;

static uint32_t model_profile_elapsed_us(uint32_t start_count)
{
  return DWT_count_to_us((uint32_t)(DWT_get_count() - start_count));
}

static void model_profile_add(uint32_t *p_total_us, uint32_t start_count)
{
  *p_total_us += model_profile_elapsed_us(start_count);
}

static void model_profile_print_every(const model_profile_t *p_profile)
{
  static uint32_t s_frame_count = 0U;

  s_frame_count++;
  if ((1U == s_frame_count) || (0U == (s_frame_count % MODEL_PROFILE_PRINT_INTERVAL)))
  {
    DBG_LOG("[MODEL] frame=%lu backend=tflite-int8(cpu+npu) total=%lu us cpu=%lu us npu=%lu us copy=%lu us\r\n",
           (unsigned long) s_frame_count,
           (unsigned long) p_profile->total_us,
           (unsigned long) p_profile->cpu_us,
           (unsigned long) p_profile->npu_us,
           (unsigned long) p_profile->copy_us);
  }
}

/*
 * 这里放的是 CPU-only 的后处理输出缓存，已移到 cacheable SDRAM。
 * 这样后处理和画框读写都会更快。
 * 5类: P4=20x20x3x10=12000, P5=10x10x3x10=3000
 */
int8_t buf_PartitionedCall_0_70298[12000] BSP_PLACE_IN_SECTION(".sdram");
int8_t buf_PartitionedCall_1_70299[3000] BSP_PLACE_IN_SECTION(".sdram");

/*
 * 这里是 CPU-only 的子图工作区，也保留在 cacheable SDRAM。
 * NPU 子图的输入/中间区仍由生成代码里的 sub_0000_arena 承担。
 */
uint8_t compute_arena_sub_0001[kBufferSize_sub_0001] BSP_PLACE_IN_SECTION(".sdram");

  // 模型输入：指向生成代码的 sub_0000_arena，当前仍在 .sdram_nocache。
int8_t* GetModelInputPtr_serving_default_images_0() {
  return (int8_t*) (sub_0000_arena + sub_0000_address_serving_default_images_0);
}


  // 模型输出：P4/P5 直接返回 raw NPU 输出指针（已省略 Transpose，后处理直接读 raw）。
  // P4 raw = model_tf_compat_v1_transpose_6_transpose_70293（[1,3,10,20,20]，12000 字节）
  // P5 raw = model_tf_compat_v1_transpose_2_transpose_70281（[1,3,10,10,10]，3000 字节）
int8_t* GetModelOutputPtr_PartitionedCall_0_70298() {
  return (int8_t*) (sub_0000_arena + sub_0000_address_model_tf_compat_v1_transpose_6_transpose_70293);
}

int8_t* GetModelOutputPtr_PartitionedCall_1_70299() {
  return (int8_t*) (sub_0000_arena + sub_0000_address_model_tf_compat_v1_transpose_2_transpose_70281);
}


static void run_model_internal(bool clean_outputs, model_profile_t *p_profile) {
  uint32_t start_count;
  uint32_t total_start_count = 0U;

  if (NULL != p_profile)
  {
    memset(p_profile, 0, sizeof(*p_profile));
    total_start_count = DWT_get_count();
  }

  // NPU Unit
  start_count = DWT_get_count();
  sub_0000_invoke(clean_outputs);
  if (NULL != p_profile) { model_profile_add(&p_profile->npu_us, start_count); }  //当前时间-start_count

  // CPU Unit：compute_sub_0001 的 4 次 Transpose 已删除（后处理直接读 raw NPU 输出），
  // 不再有单独的 CPU 子图执行时间，cpu_us 恒为 0。
  if (NULL != p_profile) { p_profile->cpu_us = 0U; }

  if (NULL != p_profile)
  {
    p_profile->total_us = model_profile_elapsed_us(total_start_count);
  }
}

void RunModel(bool clean_outputs) {
  run_model_internal(clean_outputs, NULL);
}

const model_profile_t * RunModelProfiled(bool clean_outputs) {
  run_model_internal(clean_outputs, &g_last_model_profile);
  model_profile_print_every(&g_last_model_profile);
  return &g_last_model_profile;
}
