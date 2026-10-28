# Local9 CPU timing and simplified workspace review

Local candidate: `0.7.1-1+ubuntu26.04.local9`. Public version remains 0.7.1.
Nothing in this review authorizes publication or a release tag.

## CPU experiment — excluded from the review package

The scheduling and image-thread changes below were tested, then removed from the
repository candidate and local9 package after GPU regression testing. Local9 keeps
the installed local8 camera binary unchanged. The experimental patch, binaries and
tests remain under `/tmp/penguin-local9` for investigation; no performance fix is
claimed for the shipped local review package.

## Experimental CPU processing

Local8's isolated CPU run reported two rendering stalls. A fresh installed-local8
comparison reproduced one stall during a 30-second recording. A first candidate
with dynamic work blocks alone had no stalls in a short recording, but three in a
90-second recording; that candidate was not accepted as a complete correction.

ONNX Runtime documents dynamic work blocks as a mitigation for latency variance:
https://onnxruntime.ai/docs/performance/tune-performance/troubleshooting.html#i-am-seeing-high-latency-variance

The experimental CPU sessions used dynamic_block_base=4 while retaining disabled idle spinning.
When hand recovery runs concurrently, primary RVM uses at most two CPU threads;
the heavier hand companion keeps the configured budget. Sessions without the
companion keep their previous budget. GPU inference routing and failure fallback
remain unchanged. The module’s private OpenCV image operations now use one thread, avoiding a
second 16-thread pool competing with ONNX on the test machine. This does not alter
other plugins’ OpenCV libraries. Input size, models, thresholds and same-frame mask pairing do
not change; no frame-skipping or lower-quality mask path was introduced.

An isolated paired-model experiment on this machine compared thread allocations.
4+4 had a 23.57 ms median; 2+4 had a 21.33 ms median. These are diagnostic samples,
not application-wide CPU-utilization claims. The real-model regression compares
16 changing frames for each model with one and four configured threads. Hand
logits match exactly; primary visible alpha differs by at most 0.00000537 and
recurrent tensors by at most 0.0000218 due to floating-point reduction order.
The test bounds visible alpha at 0.00001 and recurrent activations at 0.0001.

## A simpler workspace

Scenes are the starting point in the left rail, with a short explanation and a
Manage entry for the full scene editor. Sources and Audio mixer stay below the
preview instead of also having duplicate navigation buttons. Source settings,
Streaming channels, Title & category and Live chat share one right-side context
area. Native actions opening these areas also synchronize the selected tool.

The top bar keeps Go live, Record, Settings and Advanced. Duplicate destination,
stream-details and inspector shortcuts are removed. Go live no longer has a second
menu duplicating navigation. Replay clip saving, Studio mode and the separate
primary-stream control remain reachable through Advanced, using the same native
actions and validations. Settings is labeled instead of being an unexplained icon.

The presentation adapter preserves original controls and their state. Closing the
Workspace dock restores shortcuts; reopening it reapplies the compact presentation.
The previous layout is saved as BeforeSimpleLayout before the one-time migration;
older PreviousLayout evidence remains intact. Custom layouts are not reset on every
start. Public packages do not enable this optional local presentation module.

## Validation in progress

Evidence uses isolated English profiles and synthetic media under
`/var/tmp/penguin-obs33/local9-*`. Diagnostic QWidget grabs cannot correctly capture
the native OBS video surface, so they are not public product previews. Actual
recordings are decoded separately.

- `simple-ui`: initial presentation test exposed six obsolete layout assertions.
  They were updated to check the new one-panel design and hidden shortcuts; source
  editing, output routing and recording checks remain active.
- `simple-ui-fixed`: 232 checks passed at FontScale 14. The compact-layout capture
  was inspected. No warning/error entries and clean shutdown.
- Real-model scheduling comparisons passed within the explicit bounds above.
- `balanced-long-cpu`: dynamic blocks plus the smaller primary pool still recorded
  five stalls in 90 seconds; it was not accepted as the final fix.
- `image-threading`: OpenCV reported 16 threads by default; 24 parallel/serial
  hand-mask comparisons across landscape, portrait and full-HD-like sizes were exact.
- `simple-navigation`: hidden duplicate shortcuts, labeled Advanced tools, scene
  search, panel routing, previous-layout restoration and native-control fallback
  passed. Automatic Wayland focus was not granted, so the synthetic keyboard
  activation check remains unverified and is not presented as a manual test.

- `serial-image-long-cpu`: the final scheduling candidate recorded 90 seconds,
  with one reported rendering stall (rounded to 0.0% by OBS). Full recording decode
  succeeded and shutdown reported zero memory leaks. This is not a zero-stall result.
  Median render_main_texture time was 21.79 ms, versus 24.48 ms in the short local8
  comparison; durations differ, so this is indicative rather than a controlled
  benchmark or a guarantee about CPU load.

- `final-gpu`: a package build overlapped the recording; 205 stalls were reported.
  This contaminated run was rejected rather than counted as a pass.
- `final-gpu-isolated`: the experimental camera binary still reported 13 stalls
  without package building. Its recording decoded and shutdown was clean.
- `comparison-installed-gpu`: unchanged local8 reported no losses.
- `ui-only-gpu`: new simplified UI with the unchanged local8 camera binary reported
  no rendering or encoding losses and zero memory leaks on shutdown.
- `final-fallback`: the experimental CPU fallback recorded and decoded successfully,
  with no losses in this short run; it does not override the longer CPU/GPU failures.

## Final local review package

- Version: `0.7.1-1+ubuntu26.04.local9`; public version unchanged.
- SHA-256: `2452f78c5a2ba944eefeade385c7af7e4986dac50aa62b36efe45ac21c73c183`.
- Package camera binary is byte-identical to the installed local8 binary.
- `final-dialogs`: all 22 command/search/settings checks passed under Wayland;
  no warning/error entries and zero memory leaks on shutdown.
- Dependency simulation: only Penguin Studio upgraded; no packages added or removed.
- Normal Ubuntu administrator confirmation completed. Installed version verified as
  `0.7.1-1+ubuntu26.04.local9`; `dpkg -V penguin-studio` reported no deviations.
  No passwordless authorization was configured.
- `installed-keyboard`: the user confirmed visible focus, Tab/Enter panel changes
  and Escape closing Advanced. The event monitor recorded six Enter events, two
  Escape events and visits to all three requested context panels. It did not
  capture Tab events; Tab/focus acceptance is user-reported rather than inferred.
  Native platform was Wayland. The installed app exited normally with code 0,
  no warning/error entries and zero reported memory leaks.

The remaining
isolated CPU timing outlier is explicitly unresolved; the local review build must
not be described as eliminating all rendering delays.
