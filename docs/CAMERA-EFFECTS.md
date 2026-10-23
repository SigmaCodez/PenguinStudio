# Camera appearance

Select a camera or another video source and open **Camera appearance** in the
source Inspector. Use the Inspector to manage background removal together with camera shapes.
The Filters dialog exposes the individual appearance and matting filters.

![Camera appearance controls in the source Inspector](images/current/camera-en.png)

- **Shape:** Rectangle, Circle, Oval, or Rounded corners. A circle keeps equal
  pixel radii; an oval fills the source's original aspect ratio. Rounded corners
  have an adjustable radius, expressed as a percentage of half the shorter edge.
- **Background removal:** Off, Automatic (local AI), Green screen, or Blue screen.
  Shapes and background removal can be combined.
- **Key threshold:** Adjusts the green/blue screen separation. Lower values remove
  weaker color spill; higher values retain more of the original image.

Automatic mode uses **Robust Video Matting** through the bundled
[obs-backgroundremoval](https://github.com/royshil/obs-backgroundremoval) plugin.
It runs locally on the CPU with up to four physical CPU cores. No green screen, model
download, cloud account or particular GPU is required. Camera frames stay on the
device. The recurrent model retains information between frames to improve motion
continuity. Hair, transparent items, fast motion and difficult lighting can still
reduce accuracy; use even lighting and check the live preview before recording.

Penguin Studio applies matting to the full frame, then applies the selected shape.
The mask is calculated from the current frame before compositing, including the
first frame after enabling Automatic. It is not reused from the previous video tick.
A tagged companion filter is created for Automatic and removed when switching to
Off, Green screen or Blue screen. Other user-added filters are retained.
The Inspector reports preparation, readiness or failure; errors remain in the
application log. Switch removal off and on to retry initialization.

Older scenes using the original MediaPipe filter remain readable. Select Automatic
in the Inspector (switch off and back on if necessary) to use the new backend.

Settings belong to the **source**, so they apply to every scene using that source.
They are saved with the scene collection and support Undo/Redo from the Inspector.
Other filters are retained. Selecting a new setting re-enables a camera appearance
filter that was disabled in the Filters dialog.

Official x86_64 Linux packages bundle the model, runtime and their license notices.
The Flatpak builder fetches the pinned runtime and OpenCV sources before entering
the offline build.
AppImage and the Arch binary package inherit the same files from the native package.
See `plugins/penguin-backgroundremoval/PROVENANCE.md` for the RVM plugin,
model licenses and exact upstream revision. The legacy model retains its notices
in `plugins/penguin-camera/data/MODEL-NOTICE.txt`.

Current English interface, captured from Penguin Studio 0.6.0. The sample artwork demonstrates rounded corners; it is not a webcam or an AI-removal quality demonstration.

## Validation

`test/penguin-camera-model.cpp` checks model tensor dimensions, real inference and
invalid input rejection. `test/penguin-camera-gui.cpp` exercises the installed
filter and Inspector with generated images in an isolated profile.
`test/penguin-camera-pixels.py` checks rendered alpha, circle versus oval geometry,
green and blue removal, restored unfiltered output, and serialized settings.
The first-frame regression renders a newly created matting filter before any video
tick and verifies that the raw background is already removed.
The DEB jobs run the GUI check before uploading a package. Native Wayland testing
uses the same helper with `QT_QPA_PLATFORM=wayland`.

## Background blur and custom images (next release)

The Camera appearance inspector adds **Blur background (automatic)** and
**Custom background image (automatic)**. Both use the existing local Robust Video
Matting model. Shape controls, including rounded corners, apply after background
processing so the whole camera composition has the selected outline.

Blur strength ranges from 1 to 20. Higher strengths require more GPU work. Custom
images are centered and cropped to fill the camera area without stretching.
Choose a PNG, JPEG, WebP or BMP image up to 8192 × 8192 pixels. Keep the image at
its selected path: the scene stores a reference, not an embedded copy. Missing
images leave the removed background transparent. Effect settings and the image
path participate in undo/redo and scene persistence.

Background processing stays on the device. As with removal, segmentation can
have soft edges around fast movement, hair or low-contrast subjects; these modes
do not promise perfect separation.
