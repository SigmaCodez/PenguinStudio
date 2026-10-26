# Local7 unified workspace design

Local review candidate: `0.7.1-1+ubuntu26.04.local7` (Ubuntu 26.04, amd64).
Public product version remains 0.7.1. No release or publication is implied.

## Changes

The Penguin theme now follows the QML navigation's ink surfaces, violet selection
and mint status colors. Rounded content cards, segmented tabs, input borders,
focus indicators, buttons, menus, mixer channels and chat surfaces share that
language across the existing Qt Widgets UI. Text heights remain based on the
configured font size; split-button menu hit areas remain separate from labels.

Navigation uses original SVG icons and a reusable `WorkspaceButton` component.
Sources and Audio mixer join Inspector, Destinations, Stream details and Live chat.
Scene search is case-insensitive and never changes the scene collection. **Edit**
beside Scenes opens the native scene manager, preserving its add, remove, rename,
reorder and context-menu operations.

On the first local7 start, navigation moves left and sources/mixer move below the
preview. The inspector remains right. The previous dock state is saved before
this arrangement. **Workspace layout → Restore previous layout** restores it;
**Arrange workspace** reapplies the new layout. Subsequent starts retain the
user's changes. Closing Workspace preview remains supported.

This is a coordinated QML/Qt Widgets design update. It does not replace every
native dialog with QML or change OBS rendering, camera models, encoders, platform
authentication, moderation or updater behavior.

## Wayland renderer correction

The initial rearrangement test crashed in repeated calls between Qt Wayland's
`contentFBO()` and `QOpenGLFramebufferObject` construction. Selecting Qt Quick's
software backend before constructing the navigation removed the reproduced
failure. This selection affects Qt Quick in the process, not OBS video rendering,
hardware encoding or camera inference. Third-party QML plugins require their own
compatibility checks. No CPU/GPU utilization improvement is claimed.

Qt documents the supported software backend at:
https://doc.qt.io/qt-6/qquickwidget.html#graphics-api-support

## Validation evidence

All test profiles are isolated and English. Media fixtures are synthetic.
Evidence remains local under `/var/tmp/penguin-obs33/design-local7-*`.

- `layout-fixed`: 242 workspace assertions passed, with a decodable recording and
  clean shutdown. Layout assertions now verify the new left-navigation/bottom-source
  arrangement when enabled. The native scene manager is opened for rename tests;
  rename, cancellation, source synchronization and restoration remain asserted.
- `dialogs`: 18 captured views with 172 input-height checks at FontScale 14; no
  reported horizontal overflow. Settings and effect-dialog samples were inspected.
- `wizard`: both video fields fit their text at FontScale 14; capture inspected.
- `panels` and `navigation-final`: source/audio routing, case-insensitive search,
  empty search results, scene selection and layout restoration exercised. Earlier
  automated focus failures and the synthetic-popup warning remain in those logs.
- `final-controls`: search, scene manager, source/audio routing, old-layout
  restoration and new-layout reapplication passed at FontScale 14 without log
  warnings. Keyboard activation remains subject to the limitation below.
- `gpu`: GPU inference and recording succeeded; the 30.1-second recording decoded
  with 901 video frames. OBS reported 2 rendering stalls (0.2%). A subsequent
  unchanged local6 baseline reported 51 stalls (4.9%) over 33.5 seconds. These are
  individual runs, not evidence of a general performance gain or a zero-loss guarantee.
- `gpu-repeat`: the final package recorded 30.1 seconds with 900 decoded video
  frames and GPU inference active. OBS reported 3 rendering stalls (0.3%). Shutdown
  was clean, with no warning/error log entries and zero OBS-reported memory leaks.
  The nonzero stall counts remain an explicit limitation of the test results.
- `chat`: both disconnected platform panels captured and visually reviewed; no
  new live account authorization, chat send or moderation test is claimed.

Qt widget grabs cannot reliably capture the native OBS video surface. Garbled
preview pixels in these diagnostic grabs are not approved product screenshots;
recording decoding checks the output separately. No historical or public release
preview has been replaced by these experimental captures.

The local6 physical keyboard check was confirmed by the user. Automated Wayland
window activation remains unreliable; local7 automatic focus results must not be
reported as a fresh physical-keyboard acceptance test.

Package SHA-256:
`b44fc9f4e2b596f23a81b93d16c89dc9fc9bf7fbeecd83295eb03265f0e52cf8`.
