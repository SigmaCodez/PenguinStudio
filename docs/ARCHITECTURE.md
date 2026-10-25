# Penguin Studio architecture

## Upstream baseline

OBS Studio 33.0.0 (`72e86655715d9a25d69ce446aa4dd44ba58e10d2`) is the baseline. This major update changes plugin loading, resource paths and some APIs; external plugin compatibility requires validation. Product identity, configuration root, desktop identity and theme are separate. The `main` branch starts from an independent source snapshot without OBS Git ancestry; [source provenance and history](HISTORY.md) records the distinction. Review upstream changes against the baseline and import selected patches with their attribution, followed by validation.

## Native workspace

`frontend/widgets/PenguinWorkspace` owns the top production bar, canvas header,
panel headers, inspector dock and versioned dock-layout migration. It reuses
the existing OBS action and control objects so their availability and output
state remain authoritative. The application menu retains the inherited menu
tree and window shortcuts. Resetting the workspace preserves scene and source
data; subsequent launches restore the saved layout.

`frontend/widgets/PenguinInspector` reads the selected scene item and updates
real libobs transforms, crop and flags. Changes enter the existing undo stack.
Selection refresh does not retain source references between events, and both
workspace timers stop before the OBS shutdown sequence. The native mixer,
canvas renderer, source tools and dialogs continue to use the OBS engine.

## Output module

`plugins/penguin-outputs` is built into the standard plugin set when the frontend is enabled. It creates a native Qt dock and first-run connection wizard using the public frontend API. Destinations use `rtmp_custom` services and `rtmp_output` outputs, with their own H.264 video and AAC audio encoders.

Landscape reads the main program video. Portrait renders the program into a private 1080 × 1920 canvas using center crop or fit; it does not change the main canvas. OBS output signals drive status updates. Output resources must be stopped and released before their encoders and private canvases. Profile and scene-collection changes stop additional outputs. All output paths require SDR and an available H.264/AAC encoder.

There is no promise of eight simultaneous hardware sessions: that depends on the GPU. Additional destinations consume additional upload bandwidth even when their contents match. Encoder sharing, independent portrait composition, per-destination recording and synchronized transitions across editable canvases are not implemented.

## Platform metadata module

`plugins/penguin-platforms` is a separate native Qt Widgets/Network dock. It has no embedded browser dependency. Twitch uses the public-client device grant and the Helix channel update API. YouTube uses a system-browser desktop authorization flow with PKCE, random state, a loopback-only ephemeral callback listener and the Data API.

Network calls use TLS verification, finite timeouts, bounded response buffers, explicit redirect rejection and generation identifiers to reject callbacks from disconnected sessions. Tokens remain in memory. YouTube title changes read the existing video snippet and preserve other mutable fields when updating it.

Metadata authorization does not configure an RTMP encoder or fetch a stream key. The output wizard remains a separate explicit server/key connection. This avoids claiming native account integration for providers whose public APIs or account eligibility do not permit it.

## Packaging and identity

Application ID: `io.github.SigmaCodez.PenguinStudio`.

User command: `penguin-studio`.

Internal OBS ABI and engine binary names are preserved. Native packages therefore declare conflicts with their OBS counterparts. Flatpak and Snap provide isolated distribution targets. Package generation is distinct from store publication, signing, installation validation and end-to-end live-stream validation.

Penguin package builds run through `.github/workflows/penguin-linux.yml`.
The inherited OBS nightly maintenance workflow is manual-only so this standalone
repository does not automatically run OBS's unrelated cross-platform nightly matrix.
