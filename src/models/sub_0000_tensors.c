#include "sub_0000_tensors.h"

const TensorInfo sub_0000_tensors[] = {
  { "_split_1_command_stream", 0, 11104, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 1, 437056, "MODEL", 0xffffffff },
  { "_split_1_scratch", 2, 1228800, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 3, 1228800, "FAST_SCRATCH", 0x0 },
  { "serving_default_images_0", 6, 307200, "INPUT_TENSOR", 0x64000 },
  { "model_tf_compat_v1_transpose_6_transpose_70293", 5, 12000, "OUTPUT_TENSOR", 0x5460 },
  { "model_tf_compat_v1_transpose_2_transpose_70281", 4, 3000, "OUTPUT_TENSOR", 0x0 },
};

const size_t sub_0000_tensors_count = sizeof(sub_0000_tensors) / sizeof(sub_0000_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0000_address_serving_default_images_0 = 0x64000;
const uint32_t sub_0000_address_model_tf_compat_v1_transpose_6_transpose_70293 = 0x5460;
const uint32_t sub_0000_address_model_tf_compat_v1_transpose_2_transpose_70281 = 0x0;

