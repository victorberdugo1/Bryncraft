# Native Renderer (Raylib + Emscripten)

This directory is the entire rendering layer. React never draws into the
canvas — it only sends JSON over the bridge in `src/lib/wasmBridge.ts`.

## Files

| File | Purpose |
|---|---|
| `main.c` | Window/render-texture setup, JS-exported functions, main loop, effect dispatch |
| `json_mini.h/.c` | Allocation-free JSON parser for decoding `{effect, params}` messages |
| `effects/effect_common.h` | Shared `EffectKind` enum + naming convention every effect module follows |
| `effects/raylib.h`, `effects/libraylib.a` | Local dev copies of raylib, shared by every effect's standalone demo below (`-I.. -L..` from inside each effect folder) |
| `effects/main.c` | Combined demo — all effects in one raylib window, switched with keys 0-3 |
| `video_export_emscripten.js` / `video_export.h` | MediaRecorder + FFmpeg.wasm capture/encode pipeline (reused from the chess_viewer reference project) |
| `shell.html` | Emscripten HTML shell — canvas + loading overlay, no UI widgets |
| `Makefile` | emsdk + raylib(PLATFORM_WEB) + ffmpeg.wasm bootstrap and build |

## Effect folders

Each effect under `effects/` is a self-contained subfolder: the header the
WASM build compiles, a minimal standalone demo, its own `README.md`
(the one the app's "README" code-panel tab actually loads), and whatever
extra file that effect needs (font, build script, etc.) that shows up under
the app's "Extra" tab.

| Folder | Header | Standalone demo | Extra files |
|---|---|---|---|
| `effects/ascii/` | `ascii_effect.h` | `main000.c` | `NotoSansJP-Kana.ttf` (Matrix mode font) |
| `effects/particles/` | `particles_effect.h` | `main001.c` | — |
| `effects/crt/` | `crt_effect.h` | `main002.c` | GLSL shader (extracted from `CRT_FS_SOURCE`, not a separate file) |
| `effects/opencv/` | `opencv_effect.h` | `main003.c` | `opencv_build_and_run.sh`, `opencv_build_and_run.bat` |

## Message contract

React calls (via `Module.ccall`):

```c
void js_set_effect_json(const char *json);   // { "effect": "ascii", "params": { ... } }
const char *js_get_stats_json(void);         // { "fps", "resolutionW", "resolutionH", "frame", "effect", "gpuFrameTimeMs" }
```

Both are declared `EMSCRIPTEN_KEEPALIVE` in `main.c` and listed in the
Makefile's `EXPORTED_FUNCTIONS`.

## Build

First build (installs emsdk + raylib — takes a while, several GB of disk):

```bash
cd native
make raylib   # clones/builds emsdk + raylib for PLATFORM_WEB
make ffmpeg   # fetches precompiled ffmpeg.wasm
make          # compiles index.html/js/wasm via emcc
make run      # serves on http://localhost:8000 for a standalone check
```

## Wire it into the Vite app

```bash
make copy     # copies index.js/.wasm + video_export_emscripten.js (as video_export.js) into ../public/wasm/
```

`wasmBridge.ts` probes `/wasm/index.js` on startup; if present, it loads the
real renderer. If absent, the app runs on the Canvas2D preview in
`src/lib/mockRenderer.ts`, which mirrors the same three effects and the same
parameter contract so the UI is fully interactive either way.

## Adding a new effect

Full walkthrough (verified end-to-end): [`docs/adding-a-new-effect.md`](../docs/adding-a-new-effect.md)
at the repo root. Short version: one new folder under `effects/<name>/`
(header + optional demo + optional README), one new `src/effects/<name>.ts`,
and 3 one-line registrations (`EFFECT_LIST` in `effects/effect_common.h`,
one `#include` in `main.c`, one entry in `EFFECT_MODULES` in
`src/effects/index.ts`). Don't duplicate the steps here — keep this file
as the low-level map of what lives in `native/`, and let the doc above be
the single source of truth for the walkthrough, so the two don't drift
out of sync again.
