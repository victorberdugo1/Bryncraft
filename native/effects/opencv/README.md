# OpenCV Vision

Runs entirely on the CPU via OpenCV — no fragment shader. Five modes:
`edges` (Canny), `contours`, `optical_flow`, `bg_subtract` (background
subtraction), and `face_detect` (Haar cascade). On a real desktop, it can
also grab the system camera directly (`OcvCamera_Open`/
`OcvCamera_CaptureInto`) instead of the rendered scene.

> **Unlike `ascii`/`particles`/`crt`, this is the only effect in the
> family that isn't compiled with plain `gcc`.** OpenCV has no C API, so
> `opencv_effect.h` needs a C++ compiler pointed straight at it (`-x
> c++`), needs to link against `libopencv`, and on Windows also needs to
> obtain that library (it doesn't ship with MinGW). That's why this
> folder, unlike the other three, ships build scripts
> (`opencv_build_and_run.sh` / `.bat`) instead of just a one-line `gcc`
> command in the README.

## Files in this folder

| File | What it is |
|---|---|
| `opencv_effect.h` | Single-header in stb_image.h style: flat C interface at the top (what `main.c` parses), real OpenCV pipeline in C++ below `OPENCV_EFFECT_IMPLEMENTATION` |
| `main003.c` | Minimal standalone demo — just this effect, without the rest of Bryncraft |
| `opencv_build_and_run.sh` | Builds and runs the demo on Linux/macOS (requires `libopencv-dev` via pkg-config) |
| `opencv_build_and_run.bat` | Downloads a prebuilt OpenCV-MinGW if needed, builds and runs the demo on Windows |

Unlike `ascii`/`particles`/`crt`, `opencv_effect.h` has no companion
`.cpp`: it's compiled by pointing a C++ compiler directly at this header
(`-x c++`), because OpenCV has no pure-C API.

## `face_detect` needs `haarcascade_frontalface_default.xml`

The `face_detect` mode uses `cv::CascadeClassifier`, which doesn't ship
with the model embedded: you need to supply the Haar cascade XML at
runtime. Without that file, `face_detect` doesn't throw an error — it
simply detects nothing (no box gets drawn).

- `opencv_effect.h` does **not** read the XML directly from disk: it
  expects the bytes to be passed in via
  `js_set_cascade_data(size_t bufSize, uint8_t *buf)` (meant to be set
  from JS in the WASM build). Internally it dumps them to
  `/tmp/cascade.xml` and loads from there with
  `CascadeClassifier::load()`.
- For the standalone demo (`main003.c`), this is handled by reading the
  XML from disk by hand and passing it to `js_set_cascade_data()` from
  `main()` — no need to touch `opencv_effect.h` for that.
- On Windows, `/tmp/cascade.xml` resolves to `<actual_drive>:\tmp\...`;
  if that folder doesn't exist, the internal dump fails silently.
  `main003.c` creates `/tmp` at startup just in case.
- The XML comes from the official OpenCV repo
  (`data/haarcascades/haarcascade_frontalface_default.xml`) — if you use
  `opencv_build_and_run.bat`, it's already included inside
  `opencv-mingw\etc\haarcascades\`, you just need to copy it next to the
  compiled `.exe`.

## JSON contract (`OpencvEffect_SetParams`)

```json
{ "effect": "opencv", "params": {
  "mode": "edges | contours | optical_flow | bg_subtract | face_detect",
  "processScale": 0.5, "mirror": false,

  "cannyLow": 50.0, "cannyHigh": 150.0, "blur": 3,
  "edgeOnSource": true, "edgeColor": "#RRGGBB",

  "contourMinArea": 100.0, "contourThickness": 2, "contourFill": false,
  "contourColor": "#RRGGBB",

  "flowStrength": 1.0, "flowArrows": true, "flowArrowStep": 16,

  "bgHistory": 500, "bgVarThreshold": 16.0,
  "bgShadows": true, "bgMaskOnly": false,

  "faceScaleFactor": 1.1, "faceMinNeighbors": 5,
  "faceMinSizeFraction": 0.1, "faceBoxColor": "#RRGGBB",
  "faceShowCount": true
} }
```

## Building the standalone demo

`raylib.h` / `libraylib.a` are one level up, in `native/effects/` (shared
by the 4 demos). The scripts assume they are run **from this folder**
(they use `-I..`/`-L..` to find `raylib.h`/`libraylib.a`, and compile
`main003.c` / `opencv_effect.h`, which sit next to them).

```bash
# Linux/macOS — requires: sudo apt install libopencv-dev (or equivalent)
./opencv_build_and_run.sh
```

```bat
:: Windows — downloads OpenCV-MinGW the first time, then builds with MinGW
opencv_build_and_run.bat
```

### What `opencv_build_and_run.sh` does

Four steps, in order:

1. `pkg-config --exists opencv4` — bails out early with a clear message if
   `libopencv-dev` (or the distro's equivalent) isn't installed, instead
   of failing later with cryptic linker errors.
2. Compiles `main003.c` (plain C) with `gcc`.
3. Compiles `opencv_effect.h` (C++) with `g++ -x c++
   -DOPENCV_EFFECT_IMPLEMENTATION`, using `pkg-config --cflags opencv4`
   for whatever OpenCV includes the system has installed.
4. Links both `.o` files with `g++` — `-lraylib` + typical Linux libs
   (`-lm -lpthread -ldl -lrt -lX11`) plus `pkg-config --libs opencv4` for
   OpenCV's `.so` files — and removes the intermediate `.o` files.

At the end it runs `./opencv_demo` directly. It's literally the same
three-line `gcc`/`g++`/`g++` sequence shown further below under "Manual",
just with the `pkg-config` check up front and without having to type the
flags by hand every time.

### What `opencv_build_and_run.bat` does

On Windows there's no `apt install libopencv-dev` equivalent, and OpenCV
usually doesn't come prebuilt for MinGW, so the script first makes sure a
usable copy exists before compiling anything:

1. If `opencv-mingw\include\opencv2\core.hpp` already exists (from a
   previous run), it jumps straight to compiling — it doesn't download
   anything again.
2. If it doesn't exist, it clones (sparse checkout, only the needed
   folder) the `puccj/opencv-mingwx64` repo with the prebuilt OpenCV
   4.8.0 binaries for MinGW-w64, and moves that folder to
   `opencv-mingw\` next to the script.
3. Builds `INC`/`LIBS` pointing both at `raylib` (`-I.. -L..`, same as on
   Linux) and at `opencv-mingw\include` / `opencv-mingw\x64\mingw\lib`,
   and adds `opencv-mingw\x64\mingw\bin` to the session `PATH` so
   OpenCV's `.dll` files can be found at runtime.
4. Compiles and links with `gcc`/`g++` the same way as on Linux, but
   linking `-lgdi32 -lwinmm` (instead of `-lpthread -ldl -lrt -lX11`) and
   the specific OpenCV libs this effect uses (`opencv_objdetect480`,
   `opencv_video480`, `opencv_videoio480`, `opencv_imgproc480`,
   `opencv_core480`).
5. Removes the intermediate `.o` files, runs `opencv_demo.exe`, and
   leaves the console open with `pause` at the end (so errors can be read
   if something fails).

`opencv-mingw\` stays cached next to the script after the first run —
deleting it forces the script to download it again next time.

Manual, equivalent to the Linux/macOS script:

```bash
gcc  -c main003.c -o main003.o -I..
g++  -DOPENCV_EFFECT_IMPLEMENTATION -x c++ -c opencv_effect.h -o opencv_effect.o -I.. -std=c++20 $(pkg-config --cflags opencv4)
g++  main003.o opencv_effect.o -o opencv_demo -L.. -lraylib -lm -lpthread -ldl -lrt -lX11 $(pkg-config --libs opencv4)
```

## Full build (WASM, inside Bryncraft)

This is built as part of the `native/` build (`make` from `native/`, see
`native/README.md`). The WASM build uses a trimmed-down OpenCV (only
core/imgproc/video/objdetect — no `videoio`, there's no camera on the
Emscripten side); real camera capture (`OcvCamera_*`) only exists outside
`__EMSCRIPTEN__`, i.e. only in this standalone demo.
