# Creator tools

Creator tools is included in Penguin Studio, with harmonized controls and integrated recording helpers.
Open **Penguin Studio → Tools → Creator tools**. The existing classic workspace and camera processing remain in place.

## Stream check

**Run stream check** examines the current program scene, visible source dimensions, the primary stream configuration, recording storage, and active audio input mute/routing states. Nested scenes and groups are included. Audio-only media is not incorrectly reported as a missing video picture.

These checks are advisory. They do not start a stream, contact streaming platforms, test upload bandwidth, validate account permission, or automatically change your settings. A configured stream key is not proof that a provider will accept a broadcast. Multi-destination configuration still needs to be reviewed in **Streaming channels**. Check the actual picture and listen to a short recording before a show.

## Performance help

The performance page samples the engine's render, encoding and stream-network loss counters every two seconds while the window is visible. Each type of loss has a separate explanation and suggested next steps. Primary streaming and recording failures also receive a short classification for authorization, storage or encoder problems when the output provides an error. Raw error text, URLs, keys and paths are not copied into this help page. A quiet sample does not establish that the network or account is ready. The assistant does not change encoders or output settings during a live session.

### Recording overview and safe diagnostics

The performance page shows whether the primary recording is running, paused or stopped, its approximate video duration and the output's written-data counter. Duration is calculated from encoded video frames and the configured frame rate, not wall-clock time. Container overhead, split files and output resets can affect the counters; this is not a file-integrity check or a free-space forecast. The primary stream and privacy states are shown alongside it. Values refresh every two seconds while Creator tools is open.

**Copy diagnostics** copies the displayed performance snapshot to your clipboard for manual sharing. It contains only output states, numeric engine counters and fixed troubleshooting advice. It does not copy account identities, source names, recording paths, stream keys or raw error logs. No report is sent automatically. Counters may reset between outputs, and the primary stream counter does not cover every multistream destination.

Stream check also includes **Recording resilience**. Standard MP4/MOV recordings need a clean stop; MKV and hybrid/fragmented containers offer recovery options after interruptions. Custom output settings receive a separate review hint. No format is changed automatically, and no container protects against all storage failures. Test your editor's compatibility before a long session.

## Audio assistant

1. Choose your microphone and select **Measure voice (10 s)**.
2. Speak at your normal streaming volume. The assistant retains only bounded level samples; it does not record, upload or monitor audio through your speakers.
3. Review the suggested fader level. Very quiet input needs attention at the microphone or audio interface; the assistant does not boost above unity.
4. Optionally enable noise suppression. This uses the existing local audio filter and consumes additional CPU.
5. Apply the suggestion. Gentle compression and a limiter are added only when the corresponding filter type is absent. Existing filters and mute states remain unchanged.
6. Listen to a short local recording. **Undo assistant changes** removes only filters created by this transaction and restores the previous volume if you have not changed that volume yourself afterward.

One undo transaction is retained in the current app session. Applied settings persist with the scene collection. Closing the assistant during measurement cancels that measurement. Measurement is unavailable while privacy is active.

## Privacy

Use **Privacy** in the top bar, or **Ctrl+Shift+P** while the main application has keyboard focus. The tools window offers the same action and explicitly labels the resume operation.

The shield replaces the program picture with an opaque dark surface and silences source audio without changing your mute buttons, faders, or scene selection. Recording and streaming continue. New regular audio sources and program-transition changes are protected while it is active. Penguin's portrait output follows the protected program picture. Resuming restores the selected scene and sound without needing to reconstruct the mixer state.

This is a program-output shield, not a camera hardware switch. The local editing preview may still show your camera, and already buffered or transmitted frames cannot be recalled. Independently implemented third-party outputs that bypass Penguin's program and normal source-audio pipeline are outside its scope. Do not disable or remove the temporary privacy filters while the shield is active.

Privacy is a session control: after restarting the app, check your scene before starting an output. Transient filters saved during shutdown are cleaned on scene-collection load. The assistant never starts a stream or recording automatically.

## Recording helpers in 0.7.4

The recording storage check now follows the actual custom FFmpeg file-output directory and verifies that it exists and is writable. A custom network-only output is not presented as a ready local recording target.

Stream check also reports whether replay buffering is running or merely configured, and explicitly reports an active privacy shield. These checks do not start outputs or change privacy settings.

### Recording chapters

In **Creator tools → Performance help**, enter an optional chapter name and select **Add chapter** while recording. Enter in the name field performs the same action. Chapter names are limited to 120 characters. The existing chapter hotkey remains available; no extra toolbar shortcut is added.

A chapter-capable output, such as Hybrid MP4, is required. Unsupported outputs show an explanation and retain your name so it is not silently discarded. The action is unavailable while recording is stopped or paused and rechecks the state when clicked. Finish the recording normally to save its chapter index, then check chapter support in your editor. Chapters do not create separate clip files or YouTube stream markers.

### Highlight notes

**Creator tools → Highlight notes** keeps editing notes for any recording format. Enter an optional note and select **Mark highlight**, or press Enter in the input. Blank input creates a “Highlight” note. The time is derived from encoded video frames, so time spent paused is not added. Notes may also be added while paused. Times are approximate; split outputs can reset counters and are not presented as one continuous file timeline.

Each new recording gets a separate number within the app session, even if Creator tools was hidden. Earlier notes remain available when another recording starts. Up to 1,000 notes of 240 characters are retained; no audio or images are captured by this feature.

Use **Export notes…** to save a UTF-8 text file for editing. Export keeps your notes in the list, and a failed write leaves them available to retry. Notes are held in memory only: export before quitting Penguin Studio. They are not embedded in the video, sent to a platform, or included in copied performance diagnostics. Embedded chapter marks remain a separate feature for compatible containers.
