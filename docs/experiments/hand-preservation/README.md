# Experimental hand preservation

This prototype is **not enabled in release packages**. The user has accepted live hand visibility; reproducible zero-loss performance during motion remains unproven. Do not replace the installed filter with it as a released fix.

## Approach

Combine the existing [Robust Video Matting](https://github.com/PeterL1n/RobustVideoMatting) alpha with the body-skin class from the [MediaPipe Selfie Multiclass model distributed by obs-backgroundremoval](https://github.com/royshil/obs-backgroundremoval/tree/1.4.1). The latter identifies exposed skin, not hand landmarks. It can therefore also retain unwanted skin-like regions and does not guarantee preservation of every moving finger.

Both models process the same frame. RVM runs concurrently with the skin model, and compositing waits for both results. There is no queue of older masks. Skin logits use stable softmax; the skin probability is smoothly mapped from 0.20 to 0.65 and combined using maximum alpha. Input is letterboxed before skin inference. RVM remains responsible for clothing and hair.

The experimental configuration uses three intra-op threads per model, one inter-op thread, and disables ONNX Runtime intra/inter-op spinning in both sessions. Other thread configurations and sequential inference were also measured and must not be confused with this candidate.

## Local test integration

These headers are an overlay for `plugins/penguin-backgroundremoval/vendor/src/models/ModelRVM.hpp`; `HandRecovery.hpp` must be available beside that header or on its include path. They are deliberately outside the production build. The existing plugin needs the extra model in its module data directory, with its adjacent license notice. Configure the RVM session in `ort-session-utils.cpp` with the thread settings above. The zero-context `session-options.patch` records those experimental changes (`git apply --unidiff-zero` in an isolated checkout). Build an isolated plugin, then load it through a temporary filesystem overlay with a separate test profile. Do not modify the system installation for this experiment.

- Model: `selfie_multiclass_256x256.with_runtime_opt.ort`
- Source: `royshil/obs-backgroundremoval` 1.4.1
- SHA-256: `317054e916e36f47457a85afa3ef6a6cc8f5d35f3cb22e5358fd1a3e6cfb8eed`
- Model license: Apache-2.0; retain Google, Katsuya Hyodo, Roy Shilkrot and Kaito Udagawa notices.
- Existing RVM model and inherited adapter notices remain unchanged.

## Acceptance criteria

Use the same private Facecam motion reference for comparison, then test the real Facecam under Wayland. Verify moving hands visually, decode the whole recording, and inspect **both** rendering-lag and encoder-skip counters. An output counter of zero dropped frames alone is insufficient. Confirm normal shutdown. Do not publish webcam recordings or private frames.

Promotion requires repeatable live results without additional frame loss, user confirmation of moving-hand quality, failure-path tests, appropriate configuration/UI, packaging checks and publication maintenance. None of those outstanding requirements is implied by this experimental source being present.

## Results so far

See `results.json` for the redacted counters. The final three-thread configuration completed a 40-second motion-reference replay and a 70-second Facecam capture without logged render-lag or encoder-skip events; both files decoded fully and both sessions reported zero memory leaks. Sampled processed Facecam frames showed only the replacement background, so that earlier run did not complete live motion validation. See the subsequent test below.

Rejected candidates include MODNet (webcam/photographic), PP-HumanSeg, standalone Selfie Multiclass, RVM ResNet50, and Live Background Removal Lite 4.0.1. The inspected examples either lost hand detail, introduced unwanted foreground regions, or added cost without a clear quality benefit. These are observations for this local reference, not universal rankings. The Lite test also logged a frontend callback-removal warning during shutdown and is not proposed for integration.

The first dual-model implementation and several thread configurations caused additional frame losses. They are recorded as failures rather than being hidden by the output's separate zero-dropped-frame counter. The diagnostic run includes frame-dump overhead and is not a clean performance baseline.

## Repeated live motion test — 2026-10-25

The user returned to the Facecam and repeated rapid head and hand movements. They confirmed that hands remained visible and the image looked fluid. Inspected frames show both hands retained; fast-moving boundaries remain soft rather than perfectly defined. The approximately 70-second recording fully decoded, and normal shutdown reported zero memory leaks with no logged application warnings/errors.

The motion run nevertheless logged **one rendering-lag frame and one encoder-skipped frame**. The displayed percentage rounded to 0.0%; the events must not be described as zero. A subsequent 70-second control capture reported neither event and decoded successfully, but sampled frames showed no person. That control is not evidence of a second successful moving-person test. The installed 0.6.1 package remains unchanged, and the prototype is still outside the production build.
