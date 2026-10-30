# 0.7.4 release candidate validation

The native preview correction is validated locally, including the user’s normal profile. Final CI package validation remains separate.

- Candidate frontend was rebuilt from source with version 0.7.4 and the silent startup update policy.
- Simulated updater responses verify a startup check despite a legacy disabled preference, no dialog for a failed/up-to-date background check, and notification for a newer release. Checksum/size/output-active guards still pass. The explicit test-only disable-updater switch remains honored.
- All 237 native Wayland workspace checks passed on the candidate.
- All 35 recording/loopback stream checks passed; completed video/audio were fully decoded and privacy shielding verified. Applications exited normally.
- Three fresh real English XWayland previews were visually inspected; synthetic media only. Publication, branding, locale, release-policy and portable-package checks passed locally.
- User's existing scene shows a black capture. Saved metadata showed visible screen/camera PipeWire sources with normal transforms, no filters, and standard SDR video settings. Portal services are active and the original capture negotiated a stream without an import error. No replaced system libraries were mapped.
- A separate native Wayland scene with a synthetic red source was visibly confirmed by the user, ruling out a universal native preview failure.
- The first isolated PipeWire test expired before selection. In the repeated native Wayland test, the portal selected a 2560 × 1440 source and the user confirmed the shared image was visible. No private screen recording was made. Renewing selection and excluding the overlapping camera did not resolve the existing scene; the independent WebEngine regression below identified the cause.
- The corrected 0.7.4-0+ubuntu26.04.candidate2 was installed with normal administrator confirmation; package integrity and the installed binary were verified. The test-hidden camera was restored. The user confirmed that the normal scene is visible and remains visible after opening chat.

## Reproduced native preview defect

A local synthetic WebEngine dock reproduced the failure independently of PipeWire, cameras, account sign-in or streaming platforms. Before the first browser view, the native display was enabled and drew frames. After the browser view appeared, the widget remained visible/exposed but its OBS display was null and the draw counter stopped at 212 frames. The native surface destruction handler permanently set `destroying=true` and never reset it for the replacement surface.

The correction handles `SurfaceCreated` by resetting that guard and recreating the OBS display, with an Expose-event retry when the new surface is not exposed yet. The existing destruction cleanup remains intact. A dedicated synthetic regression probe checks render-frame continuity, browser replacement and recording during these transitions.

The first fix (listening only for SurfaceCreated on the old QWindow) was insufficient and was not released. Event tracing showed that Qt replaced the QWindow object itself. The completed approach also observes widget WinIdChange, rebinds visibility/screen/surface handlers to the new QWindow and schedules that binding after the native transition. Old signal connections and event filters are removed. The corrected run passed all eight native Wayland assertions with 1,143 preview draw callbacks. It recreated/replaced the browser dock during a recording; the completed recording fully decoded, and sampled frames at 1, 5, 15 and 22 seconds remained red. Shutdown reported zero OBS memory leaks. The known host RADV diagnostic appeared during WebEngine initialization; no claim is made that every third-party diagnostic is absent.
