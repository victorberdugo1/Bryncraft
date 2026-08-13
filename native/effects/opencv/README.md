# TouchDesigner Hand Tracker

Live hand detection (OpenCV, all in C/C++ — no JS/external models except
the initial `.onnx` load): the camera shows fully, no black bars, and
each detected hand gets covered by a liquid-glass blob; bring both hands
close together and a slime bridge connects them.

There are two independent styles, each selectable from a dropdown:

| Param | Where it applies |
|---|---|
| `handStyle` | Inside the glass/slime of the hands |
| `bgStyle` | The camera background, outside the glass/slime |

Options for both: `none` (passthrough, no filter) / `ascii` / `matrix` /
`crt` / `edges`.

These 4 styles **are not reimplemented here**: `touchdesigner_effect.h`
reuses them as-is by calling `AsciiEffect_Draw`/`AsciiEffect_SetMode`
(`ascii_effect.h`), `CrtEffect_Draw` (`crt_effect.h`) and
`OpencvEffect_Draw`/`OpencvEffect_SetEdgesMode` (`opencv_effect.h`) — it
only declares those symbols as `extern`, it doesn't include those headers.

---

## ✅ What you need to compile this (read this first)

The most common failure isn't a code bug: it's that one of the files
below is missing next to the `.c`/`.h`, or the `.onnx` model ended up in
the wrong folder. Use the **"Download all (.zip)"** button in the app's
**Extra** tab — it grabs everything below in one step, so you don't have
to check items off one by one:

| # | File | What it is | Always required? |
|---|---|---|---|
| 1 | `touchdesigner_effect.h` | The effect itself (hand detection + liquid glass + slime) | Yes |
| 2 | `main.c` | Entry point: opens the window, opens the camera, calls the effect every frame | Yes |
| 3 | `touchdesigner_build_and_run.sh` / `.bat` | Builds and runs everything with one command | Yes (or compile by hand with the same flags) |
| 4 | `raylib.h`, `rlgl.h` | raylib headers (window, render texture, drawing) | Yes |
| 5 | `libraylib.a` (Windows **or** Linux build, whichever matches) | Prebuilt raylib library | Yes |
| 6 | `palm_detection_mediapipe_2023feb.onnx` | Palm-detection model (MediaPipe, converted to ONNX) | Yes — **without this there is no hand detection at all**, even if everything else compiles fine |
| 7 | `ascii_effect.h`, `crt_effect.h`, `opencv_effect.h` | The 4 styles (`ascii`/`matrix` share a header, `crt`, `edges`) | Only if you want `handStyle`/`bgStyle` to be something other than `none` — see note below |

### 🎯 About the `.onnx` model — the part that confuses people most

`palm_detection_mediapipe_2023feb.onnx` **isn't compiled or linked**: it's
a neural network that OpenCV (`cv::dnn::readNet`) loads *at runtime*, the
first time the program needs to detect a hand. Because of that:

- It doesn't need to be declared in any `#include` or in the build script.
- But if the executable can't find it on startup, the app still runs
  fine — window open, camera working — **it just never detects any
  hand**, with no visible on-screen error (only a console message:
  `[touchdesigner] Could not load ...`). It's the easiest failure to
  mistake for "the code is broken."
- **Where it has to live:** in the same folder you *run* the already-built
  binary from (the `cwd` when you run `./touchdesigner_demo` or
  `touchdesigner_demo.exe`) — not in a subfolder, and not next to the
  source files if you're going to run the program from somewhere else.
  Both build scripts (`.sh`/`.bat`) already run the binary from that same
  folder, so if you dropped the model next to the `.c`/`.h` and used those
  scripts, it's already in the right place. The `.bat` even warns you if
  it's missing (`WARNING: palm_detection_mediapipe_2023feb.onnx not found
  next to this script`); the Linux/macOS script has no such warning, so
  keep an eye on it yourself there.
- Internally the code first looks for
  `./palm_detection_mediapipe_2023feb.onnx`, and if that's not found it
  falls back to a relative path (`../../assets/cv/...`) that only exists
  if you're running from **inside the full Bryncraft repo** — it won't
  exist if you're working from just the flat zip the Extra tab downloads.
  For that case (99% of the time you compile outside the repo) the model
  needs to sit right next to the executable.

### 🎨 About `ascii_effect.h` / `crt_effect.h` / `opencv_effect.h`

Unlike the web build (where all three styles are always enabled), in the
native build these three headers sit behind compile flags
(`TD_ENABLE_ASCII`, `TD_ENABLE_CRT`, `TD_ENABLE_OPENCV_EDGES`) — if those
aren't defined, setting `handStyle`/`bgStyle` to `ascii`, `matrix`, `crt`
or `edges` simply does nothing at runtime (it behaves like `none`), even
if you had it selected in the app before exporting.

- **`touchdesigner_build_and_run.bat` (Windows)** already auto-detects
  whether these `.h` files are present next to the script (or in
  `..\ascii`, `..\crt`, `..\opencv`) and defines the matching flag on its
  own. If you didn't download them, it still builds fine, just without
  those styles.
- **`touchdesigner_build_and_run.sh` (Linux/macOS)**, as it stands today,
  **never defines any of the three flags**, so even if you download all
  three `.h` files and place them next to the script, `ascii`/`matrix`/
  `crt`/`edges` won't be enabled in that build until `-DTD_ENABLE_ASCII
  -DTD_ENABLE_CRT -DTD_ENABLE_OPENCV_EDGES` are added to the script (or
  passed by hand when compiling `main.c` and `touchdesigner_effect.h`).
  If you're only using `handStyle`/`bgStyle = none`, this doesn't affect
  you.

> **This isn't a one-line `gcc` build.** The
> `TOUCHDESIGNER_EFFECT_IMPLEMENTATION` section (at the end of
> `touchdesigner_effect.h`) needs a C++ compiler pointed at the header
> (`-x c++`) and linking against OpenCV (`core`, `imgproc`, `videoio`,
> `dnn`, `video` — version **4.7 or newer**, for
> `cv::dnn::blobFromImageWithParams`). That's why there are build scripts
> instead of a plain one-liner. `opencv_effect.h` (needed if you use the
> `edges` style) is also compiled separately, same as in
> `native/Makefile` (`OPENCV_OBJ`).

---

## Where the hand detection lives

**All in one file: `touchdesigner_effect.h`.** OpenCV (`cv::dnn`), no JS
involved except for the initial ONNX model load
(`src/utils/palmModelLoader.ts`). The flow:

1. The camera/video reaches `scene` — in the web build via
   `js_set_video_frame` (`native/main.c`, fed by
   `wasmBridge.pushCameraFrame` every rAF); in the native build, via
   `TD_HandCamera_Open`/`TD_HandCamera_CaptureInto` (OpenCV
   `cv::VideoCapture`, only compiled outside Emscripten).
2. `TouchdesignerEffect_Draw()` calls `TD_DetectHands(scene, ...)` every
   `handDetectSkip` frames: it downscales `scene` (`handProcessScale`),
   reads it back to the CPU with `LoadImageFromTexture` (same pattern as
   `RunFaceDetect()` in `opencv_effect.h`) and passes it to
   `TD_HandTracking_ProcessFrame()` — an SSD palm detector (MP-PalmDet),
   implemented in the `TOUCHDESIGNER_EFFECT_IMPLEMENTATION` section at the
   end of this same `touchdesigner_effect.h` (C++, compiled separately
   with em++/g++, see `TOUCHDESIGNER_OBJ` in `native/Makefile`): up to 2
   hands with position, palm size, and approximate rotation.
3. `TD_DetectHands` reads `TD_HandTracking_GetHandCount/GetHand` and fills
   `TD_g_hands[0..1]` directly — with no external channel in between.
   This is the **only** path detection should ever run through: the
   native camera capture should just fill the scene texture, and should
   never call the detector on its own with the full-resolution frame
   (that misaligns the scale of `palmSize` against the
   `handProcessScale` that `TD_DetectHands` uses to normalize it, and
   ends up inflating the blob size).
4. `TouchdesignerEffect_Draw` computes `bgScene = TD_ApplyStyle(bgStyle,
   effScene)` and draws it as the background; it computes `handScene =
   TD_ApplyStyle(handStyle, effScene)`, and that's the texture that
   `TD_CaptureLensRegion`/`TD_CaptureBridgeLensRegion` crop to show inside
   each hand's liquid glass / slime bridge. `TD_ApplyStyle` caches the
   result per frame (if `handStyle == bgStyle` it won't render the same
   style twice).

`TouchdesignerEffect_SetHandData()` still exists as a manual override for
when `autoDetectHands` is `false`. The real app never calls it:
`autoDetectHands` is `true` by default, in both the web build and the
one exported for compiling.

## Params (`TouchdesignerEffect_SetParams`)

| Param | What it does |
|---|---|
| `handStyle` | `"none"` / `"ascii"` / `"matrix"` / `"crt"` / `"edges"` — style inside the hands' glass/slime |
| `bgStyle` | Same, but for the camera background |
| `mirror` | Horizontal mirror before detecting hands (front camera = selfie) |
| `handReanchorInterval`, `handProcessScale`, `handDetectSkip` | Tune detection cost/latency |
| `glassColor`, `glassEnabled`, `glassSize` | Liquid glass over each hand |
| `slimeEnabled`, `slimeDistance` | Slime bridge between the two hands |
| `showHandCount` | Overlay showing the number of detected hands |
| `showCameraBg`, `bgFallbackColor` | Background: camera (with the chosen `bgStyle`) or a flat color |

Part of [Bryncraft](https://bryncraft.online/) — created by Victor Berdugo.
