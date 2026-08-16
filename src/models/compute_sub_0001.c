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
 */

#include <stdint.h>

#include "compute_sub_0001.h"

#include "arm_nn_types.h"
#include "arm_nnfunctions.h"
#include "kernel_library_utils.h"

#include "kernel_library_int.h" 

 

void compute_sub_0001(
  // buffer for intermediate results
  uint8_t* main_storage, // should provide at least 15013 bytes of storage

  // inputs
  
  const int8_t model_tf_compat_v1_transpose_2_transpose_70281[3000], // 1,30,10,10
  
  const int8_t model_tf_compat_v1_transpose_6_transpose_70293[12000], // 1,30,20,20
  

  // outputs
  
  int8_t PartitionedCall_0_70298[12000] , // 1,20,20,3,10
  
  int8_t PartitionedCall_1_70299[3000]  // 1,10,10,3,10
  
) {
  // Buffers allocated on the main storage (note: depends on the execution order)
    
  
  int8_t* model_p4_20x20_transpose_70295 = (int8_t *) &main_storage[3008]; // 1,20,10,20,3 == 12000
  
  int8_t* model_p5_10x10_transpose_70297 = (int8_t *) &main_storage[0]; // 1,10,10,10,3 == 3000
  
  

  // Parameters
  







//
// Identity - bypassing model_tf_reshape_Reshape1_70296 operation
//
// Input model_tf_compat_v1_transpose_2_transpose_70281: int8_t - 1,30,10,10
// Output model_tf_reshape_Reshape1_70296: int8_t - 1,3,10,10,10


const int8_t* model_tf_reshape_Reshape1_70296 = model_tf_compat_v1_transpose_2_transpose_70281;





//
// Transpose
//
// Input model_tf_reshape_Reshape1_70296: int8_t - 1,3,10,10,10
// Output model_p5_10x10_transpose_70297: int8_t - 1,10,10,10,3
// Perm: ( 0,  4,  2,  3,  1, )

int32_t strides_model_p5_10x10_transpose_70297[5] = { 3000, 1, 100, 10, 1000,  };

int32_t next_dim_sizes_model_p5_10x10_transpose_70297[5] = { 3000, 3000, 300, 30, 3,  };

int32_t dim_sizes_model_p5_10x10_transpose_70297[5] = { 3000, 300, 30, 3, 1,  };


Transpose(
      model_tf_reshape_Reshape1_70296
    , model_p5_10x10_transpose_70297
    , 3000
    , 5
    , strides_model_p5_10x10_transpose_70297
    , next_dim_sizes_model_p5_10x10_transpose_70297
    , dim_sizes_model_p5_10x10_transpose_70297
);

//
// Transpose
//
// Input model_p5_10x10_transpose_70297: int8_t - 1,10,10,10,3
// Output PartitionedCall_1_70299: int8_t - 1,10,10,3,10
// Perm: ( 0,  1,  3,  4,  2, )

int32_t strides_PartitionedCall_1_70299[5] = { 3000, 300, 3, 1, 30,  };

int32_t next_dim_sizes_PartitionedCall_1_70299[5] = { 3000, 3000, 300, 30, 10,  };

int32_t dim_sizes_PartitionedCall_1_70299[5] = { 3000, 300, 30, 10, 1,  };


Transpose(
      model_p5_10x10_transpose_70297
    , PartitionedCall_1_70299
    , 3000
    , 5
    , strides_PartitionedCall_1_70299
    , next_dim_sizes_PartitionedCall_1_70299
    , dim_sizes_PartitionedCall_1_70299
);

//
// Identity - bypassing model_tf_reshape_1_Reshape1_70294 operation
//
// Input model_tf_compat_v1_transpose_6_transpose_70293: int8_t - 1,30,20,20
// Output model_tf_reshape_1_Reshape1_70294: int8_t - 1,3,10,20,20


const int8_t* model_tf_reshape_1_Reshape1_70294 = model_tf_compat_v1_transpose_6_transpose_70293;





//
// Transpose
//
// Input model_tf_reshape_1_Reshape1_70294: int8_t - 1,3,10,20,20
// Output model_p4_20x20_transpose_70295: int8_t - 1,20,10,20,3
// Perm: ( 0,  4,  2,  3,  1, )

int32_t strides_model_p4_20x20_transpose_70295[5] = { 12000, 1, 400, 20, 4000,  };

int32_t next_dim_sizes_model_p4_20x20_transpose_70295[5] = { 12000, 12000, 600, 60, 3,  };

int32_t dim_sizes_model_p4_20x20_transpose_70295[5] = { 12000, 600, 60, 3, 1,  };


Transpose(
      model_tf_reshape_1_Reshape1_70294
    , model_p4_20x20_transpose_70295
    , 12000
    , 5
    , strides_model_p4_20x20_transpose_70295
    , next_dim_sizes_model_p4_20x20_transpose_70295
    , dim_sizes_model_p4_20x20_transpose_70295
);

//
// Transpose
//
// Input model_p4_20x20_transpose_70295: int8_t - 1,20,10,20,3
// Output PartitionedCall_0_70298: int8_t - 1,20,20,3,10
// Perm: ( 0,  1,  3,  4,  2, )

int32_t strides_PartitionedCall_0_70298[5] = { 12000, 600, 3, 1, 60,  };

int32_t next_dim_sizes_PartitionedCall_0_70298[5] = { 12000, 12000, 600, 30, 10,  };

int32_t dim_sizes_PartitionedCall_0_70298[5] = { 12000, 600, 30, 10, 1,  };


Transpose(
      model_p4_20x20_transpose_70295
    , PartitionedCall_0_70298
    , 12000
    , 5
    , strides_PartitionedCall_0_70298
    , next_dim_sizes_PartitionedCall_0_70298
    , dim_sizes_PartitionedCall_0_70298
);

}
