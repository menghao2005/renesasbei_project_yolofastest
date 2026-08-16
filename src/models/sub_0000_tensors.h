#ifndef __SUB_0000_TENSORS_H__
#define __SUB_0000_TENSORS_H__

#include <stddef.h>
#include <stdint.h>
#include "ethosu_common.h"

extern const TensorInfo sub_0000_tensors[];
extern const size_t sub_0000_tensors_count;

#define kArenaSize_sub_0000 1228800

// Addresses for each input and output buffer inside of the arena
extern const uint32_t sub_0000_address_serving_default_images_0;
extern const uint32_t sub_0000_address_model_tf_compat_v1_transpose_6_transpose_70293;
extern const uint32_t sub_0000_address_model_tf_compat_v1_transpose_2_transpose_70281;


#endif // __SUB_0000_TENSORS_H__
