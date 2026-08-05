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

## Adding a fourth effect

1. Add its `EffectId` + `ParamSchema[]` in `src/types/effects.ts` (drives the
   Inspector, Code panel, and mock renderer automatically).
2. Add the enum value in `effects/effect_common.h`.
3. Create `effects/<name>/<name>_effect.h` implementing `SetParams` / `Update` / `Draw`,
   plus a minimal `effects/<name>/main<NNN>.c` standalone demo and an
   `effects/<name>/README.md` — same layout as the existing folders.
4. Dispatch it in `main.c`'s `js_set_effect_json` switch and `UpdateDrawFrame`,
   including it as `effects/<name>/<name>_effect.h`.
5. `EFFECT_HEADERS` in the `Makefile` already globs `effects/*/*.h`, so no
   Makefile change is needed unless the effect needs its own build step
   (like `opencv/` does).
