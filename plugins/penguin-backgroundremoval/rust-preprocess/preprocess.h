// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Input is BGRA8 (stride in bytes); output is three RGB planes in [0, 1].
// Caller owns both live buffers and guarantees exclusive access to output.
// Output must contain exactly width * height * 3 floats. Returns 1 on success.
// Invalid sizes, alignment or overlap return 0 without writing output.
int32_t penguin_bgra_to_rgb_chw(const uint8_t *input, size_t input_len, size_t width,
                              size_t height, size_t stride, float *output, size_t output_len);
#ifdef __cplusplus
}
#endif
