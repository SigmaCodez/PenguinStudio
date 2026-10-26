// SPDX-License-Identifier: GPL-3.0-or-later
//! Allocation-free BGRA8 to planar RGB32F preprocessing for the RVM worker.
use std::{mem, panic::catch_unwind, slice};

fn layout(width: usize, height: usize, stride: usize) -> Option<(usize, usize)> {
    if width == 0 || height == 0 {
        return None;
    }
    let row = width.checked_mul(4)?;
    if stride < row {
        return None;
    }
    let input = (height - 1).checked_mul(stride)?.checked_add(row)?;
    let output = width.checked_mul(height)?.checked_mul(3)?;
    if input > isize::MAX as usize || output > isize::MAX as usize / mem::size_of::<f32>() {
        return None;
    }
    Some((input, output))
}

fn convert(input: &[u8], output: &mut [f32], width: usize, height: usize, stride: usize) {
    let plane = width * height;
    let (red, rest) = output.split_at_mut(plane);
    let (green, blue) = rest.split_at_mut(plane);
    for y in 0..height {
        let row = &input[y * stride..y * stride + width * 4];
        let offset = y * width;
        let pixels = row.chunks_exact(4);
        let targets = red[offset..offset + width]
            .iter_mut()
            .zip(green[offset..offset + width].iter_mut())
            .zip(blue[offset..offset + width].iter_mut());
        for (bgra, ((r, g), b)) in pixels.zip(targets) {
            *r = bgra[2] as f32 * (1.0 / 255.0);
            *g = bgra[1] as f32 * (1.0 / 255.0);
            *b = bgra[0] as f32 * (1.0 / 255.0);
        }
    }
}

/// Convert a strided BGRA frame directly into an existing planar RGB tensor.
/// Returns 1 on success, 0 on invalid dimensions/buffers or a contained panic.
/// Validation failures leave output untouched. No allocation occurs on success.
///
/// # Safety
/// The caller must supply live readable/writable allocations for the respective
/// lengths, with exclusive access to output for this call. Numeric validation
/// cannot establish pointer provenance or detect concurrently freed storage.
#[no_mangle]
pub unsafe extern "C" fn penguin_bgra_to_rgb_chw(
    input: *const u8,
    input_len: usize,
    width: usize,
    height: usize,
    stride: usize,
    output: *mut f32,
    output_len: usize,
) -> i32 {
    let Some((needed_input, needed_output)) = layout(width, height, stride) else {
        return 0;
    };
    if input.is_null()
        || output.is_null()
        || input_len < needed_input
        || output_len != needed_output
        || (output as usize) % mem::align_of::<f32>() != 0
    {
        return 0;
    }
    let Some(input_end) = (input as usize).checked_add(needed_input) else {
        return 0;
    };
    let Some(output_end) = (output as usize).checked_add(needed_output * mem::size_of::<f32>())
    else {
        return 0;
    };
    if (input as usize) < output_end && (output as usize) < input_end {
        return 0;
    }
    catch_unwind(|| {
        // SAFETY: caller guarantees live storage; the checks above establish
        // sizes, alignment, non-overlap and slice representability.
        let source = unsafe { slice::from_raw_parts(input, needed_input) };
        let target = unsafe { slice::from_raw_parts_mut(output, needed_output) };
        convert(source, target, width, height, stride);
    })
    .map_or(0, |_| 1)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn channels_padding_and_alpha() {
        let frame = [
            0, 127, 255, 0, 255, 0, 127, 255, 42, 42, 1, 2, 3, 128, 4, 5, 6, 0,
        ];
        let mut out = [0.0; 12];
        assert_eq!(
            unsafe {
                penguin_bgra_to_rgb_chw(
                    frame.as_ptr(),
                    frame.len(),
                    2,
                    2,
                    10,
                    out.as_mut_ptr(),
                    out.len(),
                )
            },
            1
        );
        let expected = [255, 127, 3, 6, 127, 0, 2, 5, 0, 255, 1, 4];
        for (actual, expected) in out.iter().zip(expected) {
            assert!((actual - expected as f32 / 255.0).abs() < 1e-7);
        }
    }

    #[test]
    fn invalid_sizes_do_not_write() {
        let frame = [0_u8; 16];
        for (w, h, stride, len, out_len) in [
            (0, 2, 8, 16, 12),
            (2, 0, 8, 16, 12),
            (2, 2, 7, 16, 12),
            (2, 2, 8, 15, 12),
            (2, 2, 8, 16, 11),
            (2, 2, 8, 16, 13),
            (usize::MAX, 2, 8, 16, 12),
            (2, usize::MAX, 8, 16, 12),
            (2, 3, usize::MAX, 16, 12),
        ] {
            let mut out = [42.0; 13];
            assert_eq!(
                unsafe {
                    penguin_bgra_to_rgb_chw(
                        frame.as_ptr(),
                        len,
                        w,
                        h,
                        stride,
                        out.as_mut_ptr(),
                        out_len,
                    )
                },
                0
            );
            assert_eq!(out, [42.0; 13]);
        }
    }

    #[test]
    fn null_misaligned_and_overlapping_buffers() {
        let mut data = [0_f32; 4];
        let ptr = data.as_mut_ptr();
        assert_eq!(
            unsafe { penguin_bgra_to_rgb_chw(ptr.cast(), 4, 1, 1, 4, ptr, 3) },
            0
        );
        assert_eq!(
            unsafe { penguin_bgra_to_rgb_chw(std::ptr::null(), 4, 1, 1, 4, ptr, 3) },
            0
        );
        let frame = [0_u8; 4];
        assert_eq!(
            unsafe { penguin_bgra_to_rgb_chw(frame.as_ptr(), 4, 1, 1, 4, std::ptr::null_mut(), 3) },
            0
        );
        assert_eq!(
            unsafe {
                penguin_bgra_to_rgb_chw(
                    frame.as_ptr(),
                    4,
                    1,
                    1,
                    4,
                    ptr.cast::<u8>().add(1).cast(),
                    3,
                )
            },
            0
        );
    }
}
