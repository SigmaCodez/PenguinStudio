// SPDX-License-Identifier: GPL-3.0-or-later
//! Bounded, allocation-free conversion of six-class hand-model logits to alpha.
use std::{mem, mem::MaybeUninit, panic::catch_unwind, slice};

fn lengths(
    width: usize,
    height: usize,
    input_stride: usize,
    output_stride: usize,
) -> Option<(usize, usize)> {
    if width == 0 || height == 0 || output_stride < width {
        return None;
    }
    let row = width.checked_mul(6)?;
    if input_stride < row {
        return None;
    }
    let input = (height - 1).checked_mul(input_stride)?.checked_add(row)?;
    let output = (height - 1)
        .checked_mul(output_stride)?
        .checked_add(width)?;
    if input > isize::MAX as usize / mem::size_of::<f32>()
        || output > isize::MAX as usize / mem::size_of::<f32>()
    {
        return None;
    }
    Some((input, output))
}

fn convert(
    input: &[f32],
    output: &mut [MaybeUninit<f32>],
    width: usize,
    height: usize,
    input_stride: usize,
    output_stride: usize,
) {
    for y in 0..height {
        let source = &input[y * input_stride..y * input_stride + width * 6];
        let target = &mut output[y * output_stride..y * output_stride + width];
        for (values, result) in source.chunks_exact(6).zip(target) {
            // Match std::max_element: an initial NaN remains the maximum and
            // later NaNs do not replace it. f32::max would behave differently.
            let mut maximum = values[0];
            for &value in &values[1..] {
                if maximum < value {
                    maximum = value;
                }
            }
            let mut sum = 0.0;
            let mut skin = 0.0;
            for (channel, &value) in values.iter().enumerate() {
                let probability = (value - maximum).exp();
                sum += probability;
                if channel == 2 {
                    skin = probability;
                }
            }
            let alpha = ((skin / sum - 0.2) / 0.45).clamp(0.0, 1.0);
            result.write(alpha * alpha * (3.0 - 2.0 * alpha));
        }
    }
}

/// Convert strided NHWC six-class logits to a strided skin-alpha mask.
/// Lengths and strides are in floats. Padding is not read or written.
/// Returns 1 on success, 0 on invalid layout/buffers or a contained panic.
/// Validation failures leave output untouched. Nonfinite logits retain the
/// existing C++ algorithm's NaN behavior; they are not silently reinterpreted.
///
/// # Safety
/// The caller guarantees live initialized input storage throughout its accessed
/// span, including inter-row padding, and live writable output storage with
/// exclusive access. Output may be uninitialized; padding remains untouched.
/// Numeric checks cannot prove pointer provenance or input initialization.
#[no_mangle]
pub unsafe extern "C" fn penguin_logits6_to_skin_mask(
    input: *const f32,
    input_len: usize,
    width: usize,
    height: usize,
    input_stride: usize,
    output: *mut f32,
    output_len: usize,
    output_stride: usize,
) -> i32 {
    let Some((needed_input, needed_output)) = lengths(width, height, input_stride, output_stride)
    else {
        return 0;
    };
    if input.is_null()
        || output.is_null()
        || input_len < needed_input
        || output_len < needed_output
        || (input as usize) % mem::align_of::<f32>() != 0
        || (output as usize) % mem::align_of::<f32>() != 0
    {
        return 0;
    }
    let Some(input_end) = (input as usize).checked_add(needed_input * mem::size_of::<f32>()) else {
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
        // SAFETY: the caller supplies live storage; validation establishes
        // representable lengths, aligned floats and disjoint accessed ranges.
        let source = unsafe { slice::from_raw_parts(input, needed_input) };
        // OpenCV allocates the destination without initializing its floats.
        // MaybeUninit permits that storage (and untouched row padding); each
        // logical output pixel is initialized by write(), never read first.
        let target =
            unsafe { slice::from_raw_parts_mut(output.cast::<MaybeUninit<f32>>(), needed_output) };
        convert(source, target, width, height, input_stride, output_stride);
    })
    .map_or(0, |_| 1)
}

#[cfg(test)]
mod tests {
    use super::*;
    fn reference(v: &[f32]) -> f32 {
        let mut max = v[0];
        for &x in &v[1..] {
            if max < x {
                max = x;
            }
        }
        let mut sum = 0.0;
        for &x in v {
            sum += (x - max).exp();
        }
        let a = (((v[2] - max).exp() / sum - 0.2) / 0.45).clamp(0.0, 1.0);
        a * a * (3.0 - 2.0 * a)
    }
    #[test]
    fn finite_and_nonfinite_logits_preserve_results() {
        let mut cases = vec![
            [0.0; 6],
            [-1000.0; 6],
            [1000.0, -1000.0, 500.0, 0.0, 1.0, -1.0],
            [
                -0.0,
                0.0,
                f32::MIN_POSITIVE,
                f32::from_bits(1),
                -f32::MIN_POSITIVE,
                -f32::from_bits(1),
            ],
            [f32::NEG_INFINITY; 6],
        ];
        for i in 0..6 {
            for value in [
                f32::NAN,
                f32::INFINITY,
                f32::NEG_INFINITY,
                f32::MAX,
                f32::MIN,
            ] {
                let mut v = [0.1, -0.5, 0.7, 1.0, 0.0, -1.0];
                v[i] = value;
                cases.push(v);
            }
        }
        let mut state = 103_u32;
        for _ in 0..4096 {
            let mut v = [0.0; 6];
            for x in &mut v {
                state = state.wrapping_mul(1664525).wrapping_add(1013904223);
                *x = (state % 1601) as f32 / 100.0 - 8.0;
            }
            cases.push(v);
        }
        for v in cases {
            let mut out = [MaybeUninit::uninit()];
            convert(&v, &mut out, 1, 1, 6, 1);
            let expected = reference(&v);
            // SAFETY: convert initializes every logical pixel, including this
            // sole element. The test deliberately starts with fresh storage.
            let actual = unsafe { out[0].assume_init() };
            assert!(actual.to_bits() == expected.to_bits() || actual.is_nan() && expected.is_nan());
        }
    }
    #[test]
    fn cropped_rows_and_output_padding() {
        for width in 1..=33 {
            for padding in [0, 1, 7] {
                let height = 3;
                let input_stride = width * 6 + padding;
                let output_stride = width + padding;
                let (ni, no) = lengths(width, height, input_stride, output_stride).unwrap();
                let mut input = vec![f32::NAN; ni + 1];
                let mut output = vec![-42.0; no + 2];
                for y in 0..height {
                    for x in 0..width * 6 {
                        input[1 + y * input_stride + x] = (x % 17) as f32 / 3.0;
                    }
                }
                assert_eq!(
                    unsafe {
                        penguin_logits6_to_skin_mask(
                            input.as_ptr().add(1),
                            ni,
                            width,
                            height,
                            input_stride,
                            output.as_mut_ptr().add(1),
                            no,
                            output_stride,
                        )
                    },
                    1
                );
                for y in 0..height {
                    for x in 0..width {
                        assert_eq!(
                            output[1 + y * output_stride + x],
                            reference(
                                &input[1 + y * input_stride + x * 6
                                    ..1 + y * input_stride + x * 6 + 6]
                            )
                        );
                    }
                    if y + 1 < height {
                        for x in width..output_stride {
                            assert_eq!(output[1 + y * output_stride + x], -42.0);
                        }
                    }
                }
                assert_eq!(output[0], -42.0);
                assert_eq!(output[no + 1], -42.0);
            }
        }
    }
    #[test]
    fn invalid_layouts_and_pointers_do_not_write() {
        let input = [0.0; 24];
        let mut out = [-42.0; 4];
        for (w, h, si, so, ni, no) in [
            (0, 2, 12, 2, 24, 4),
            (2, 0, 12, 2, 24, 4),
            (2, 2, 11, 2, 24, 4),
            (2, 2, 12, 1, 24, 4),
            (2, 2, 12, 2, 23, 4),
            (2, 2, 12, 2, 24, 3),
            (usize::MAX, 2, 12, 2, 24, 4),
            (2, usize::MAX, 12, 2, 24, 4),
            (2, 3, usize::MAX, 2, 24, 4),
            (2, 3, 12, usize::MAX, 24, 4),
        ] {
            assert_eq!(
                unsafe {
                    penguin_logits6_to_skin_mask(
                        input.as_ptr(),
                        ni,
                        w,
                        h,
                        si,
                        out.as_mut_ptr(),
                        no,
                        so,
                    )
                },
                0
            );
            assert_eq!(out, [-42.0; 4]);
        }
        assert_eq!(
            unsafe {
                penguin_logits6_to_skin_mask(std::ptr::null(), 6, 1, 1, 6, out.as_mut_ptr(), 1, 1)
            },
            0
        );
        assert_eq!(
            unsafe {
                penguin_logits6_to_skin_mask(input.as_ptr(), 6, 1, 1, 6, std::ptr::null_mut(), 1, 1)
            },
            0
        );
        assert_eq!(
            unsafe {
                penguin_logits6_to_skin_mask(
                    input.as_ptr().cast::<u8>().add(1).cast(),
                    6,
                    1,
                    1,
                    6,
                    out.as_mut_ptr(),
                    1,
                    1,
                )
            },
            0
        );
        assert_eq!(
            unsafe {
                penguin_logits6_to_skin_mask(
                    input.as_ptr(),
                    6,
                    1,
                    1,
                    6,
                    out.as_mut_ptr().cast::<u8>().add(1).cast(),
                    1,
                    1,
                )
            },
            0
        );
        let mut overlapping = [-42.0; 6];
        assert_eq!(
            unsafe {
                penguin_logits6_to_skin_mask(
                    overlapping.as_ptr(),
                    6,
                    1,
                    1,
                    6,
                    overlapping.as_mut_ptr(),
                    1,
                    1,
                )
            },
            0
        );
        assert_eq!(overlapping, [-42.0; 6]);

        // Both directions of partial overlap must fail before creating slices.
        let mut partial = [-42.0; 14];
        for (input_offset, output_offset) in [(0, 11), (1, 0)] {
            assert_eq!(
                unsafe {
                    penguin_logits6_to_skin_mask(
                        partial.as_ptr().add(input_offset),
                        12,
                        2,
                        1,
                        12,
                        partial.as_mut_ptr().add(output_offset),
                        2,
                        2,
                    )
                },
                0
            );
            assert_eq!(partial, [-42.0; 14]);
        }

        // Aligned addresses whose accessed spans would wrap must be rejected
        // numerically, without dereferencing these deliberately invalid pointers.
        let address = usize::MAX & !(mem::align_of::<f32>() - 1);
        assert_eq!(
            unsafe {
                penguin_logits6_to_skin_mask(
                    address as *const f32,
                    6,
                    1,
                    1,
                    6,
                    out.as_mut_ptr(),
                    1,
                    1,
                )
            },
            0
        );
        assert_eq!(
            unsafe {
                penguin_logits6_to_skin_mask(input.as_ptr(), 6, 1, 1, 6, address as *mut f32, 1, 1)
            },
            0
        );
        assert_eq!(out, [-42.0; 4]);
    }
}
