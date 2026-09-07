# Reframe — 16:9 to 9:16 auto vertical crop

Continuously picks the crop window that keeps "what matters" in frame at a
narrower target aspect ratio (9:16 by default) — the same idea as
freecropper's landscape-to-vertical converter, but live and automatic:

1. **Face detection** — the same Haar cascade used by the `opencv` effect
   (`haarcascade_frontalface_default.xml`) finds every face in the frame.
2. **Tracking** — a lightweight per-face tracker matches detections across
   frames (nearest-center matching), so each face keeps a stable identity
   instead of flickering in and out as a fresh, unrelated box every time
   the cascade re-runs.
3. **Active-speaker heuristic** — since there's no audio available inside
   the render pipeline, each tracked face gets a "how much is the mouth
   area moving right now" score (frame-differencing over the lower third
   of the face box). The face with the highest score is treated as the
   person currently talking.
4. **Smooth panning** — the crop's focus point eases toward whichever face
   is selected (exponential smoothing + a max pan speed, so it never
   whip-pans), and switching from one face to another only happens after a
   cooldown and a clear activity margin — otherwise two people trading
   quick lines would make the camera flicker back and forth.
5. **Crop** — a `targetAspectW : targetAspectH` window (9:16 by default) is
   cut out of the *original*, full-resolution frame around that focus
   point and scaled to fill the output canvas. No re-encoding of the image
   itself happens (unlike `opencv`'s other modes) — this effect only
   decides *where* to crop.

If no face is detected, `fallbackMode` decides what happens: `center`
recenters the crop, `lastKnown` freezes it at the last known focus point.

---

## ✅ What you need to compile this

| # | File | What it is | Always required? |
|---|---|---|---|
| 1 | `reframe_effect.h` | The effect itself (detection, tracking, panning, crop) | Yes |
| 2 | `main005.c` | Entry point: opens a 9:16 window, opens the camera, calls the effect every frame | Yes |
| 3 | `reframe_build_and_run.sh` / `.bat` | Builds and runs everything with one command | Yes (or compile by hand with the same flags) |
| 4 | `raylib.h` | raylib header (window, render texture, drawing) | Yes |
| 5 | `libraylib.a` (Windows **or** Linux build, whichever matches) | Prebuilt raylib library | Yes |
| 6 | `haarcascade_frontalface_default.xml` | Haar cascade face-detection model | Yes — without this the crop stays on `fallbackMode` forever, no face tracking at all |

> **This isn't a one-line `gcc` build.** Like `opencv_effect.h`, the
> `REFRAME_EFFECT_IMPLEMENTATION` section (at the end of
> `reframe_effect.h`) needs a C++ compiler pointed at the header (`-x
> c++`) and linking against OpenCV (`core`, `imgproc`, `objdetect`,
> `videoio`). That's why there's a build script instead of a plain
> one-liner — same pattern as `native/Makefile`'s `OPENCV_OBJ` /
> `REFRAME_OBJ`.

The cascade XML isn't compiled or linked: it's loaded at runtime through
`js_set_cascade_data()` (web build) or read straight off disk by
`main005.c` (native demo, via `LoadFaceCascadeFromDisk`). Keep it in the
same folder you run the built binary from.

## Params (`ReframeEffect_SetParams`)

| Param | What it does |
|---|---|
| `targetAspectW`, `targetAspectH` | Output crop aspect ratio (9:16 by default) |
| `processScale` | Resolution the detector runs at, relative to the source frame — lower is faster, less accurate |
| `mirror` | Horizontal mirror before detecting (front camera = selfie) |
| `faceScaleFactor`, `faceMinNeighbors`, `faceMinSizeFraction` | Standard Haar cascade tuning knobs |
| `zoom` | How tightly framed the crop is (1 = the widest crop that still fills the target aspect, higher = zoomed in) |
| `headroom` | Vertical offset applied above the detected face center, so the crop doesn't center on the forehead |
| `panSmoothing` | How quickly the crop eases toward the focus point (0 = frozen, 1 = snappy) |
| `maxPanSpeed` | Hard cap on pan speed (fraction of frame width per second), independent of `panSmoothing` |
| `switchCooldown` | Minimum seconds between switching the focus from one face to another |
| `deadZone` | Ignore focus deltas smaller than this (fraction of frame size) — kills micro-jitter |
| `activeSpeakerDetection` | `true`: follow the face with the most mouth-region motion. `false`: always follow the largest face |
| `fallbackMode` | `"center"` or `"lastKnown"` — what to do when no face is detected |
| `showDebugOverlay`, `debugBoxColor`, `debugTargetColor` | Draw tracked face boxes + a label over the crop, for tuning params live (turn off before the final export) |

Part of [Bryncraft](https://bryncraft.online/) — created by Victor Berdugo.
