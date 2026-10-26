// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Lengths/strides are measured in floats, not bytes. Buffers must be live,
// float-aligned and nonoverlapping. Input must be initialized throughout its
// accessed span (including row padding); output requires exclusive access but
// may be uninitialized. Only logical output pixels are initialized by this call.
// Converts width × height pixels from strided six-class logits, leaving padding
// untouched. Returns 1 on success, 0 on invalid input or contained panic.
int32_t penguin_logits6_to_skin_mask(const float *input, size_t input_len, size_t width,
                                   size_t height, size_t input_stride, float *output,
                                   size_t output_len, size_t output_stride);
#ifdef __cplusplus
}
#endif
