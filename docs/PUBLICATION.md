# Current GitHub text and GUI previews

Public product text, captions and current screenshots use English. Refresh them
for every release and whenever the visible interface changes.

1. Build or install the candidate version and use a separate `en-US` demo profile.
   Capture the actual window, including its real preview surface. Use synthetic
   media; never use personal webcam footage, desktop contents or signed-in accounts.
2. Replace `docs/images/current/workspace-en.png`, `camera-en.png` and
   `stream-details-en.png`. Visually inspect every image for English labels,
   current controls, readable text and private information. Do not edit labels
   into an old screenshot or represent an illustration as the application UI.
3. Update `manifest.json` with the application version, package/source identity,
   capture date and method, and each PNG's SHA-256. These are provenance records,
   not a substitute for visual inspection. XComposite captures under XWayland
   include native preview surfaces; a plain Qt widget grab may omit them.
4. Refresh the README release, package filenames/checksums, workspace/camera/platform
   guides and version-specific release notes. Keep older verification records
   clearly labeled as historical; preserve author and license notices.
5. Run `python3 scripts/check-publication.py`. CI runs this for documentation
   changes and before Linux package builds. A version bump needs reviewed previews
   from that candidate version; do not merely change the old manifest's version.
6. After package verification, update the current GitHub release description with
   final test results and link the matching English previews using an immutable
   documentation commit URL. Preserve existing download links and hashes.

The release publisher embeds all three reviewed previews automatically, pinned
to the built source commit. The maintainer still supplies and visually reviews
fresh captures; CI does not translate screenshots or invent new UI images.

The current previews were captured from the installed application using native
window capture on Ubuntu. The program canvas contains original local demo artwork;
the rounded-corner example demonstrates shape controls, not portrait segmentation.
Provider views are intentionally signed out.
