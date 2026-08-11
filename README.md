# Bryncraft

A browser-based creative atelier — a professional docking editor with
panels for the effect library, inspector, timeline and code — for
building real-time visual effects. Craft particles,
shaders and CPU-vision effects, tweak them live, then export them as
Raylib (C) source code or as transparent video, ready to drop into any
engine.

Raylib compiled to WebAssembly does all the rendering, inside a single
`<canvas>`. React never touches a pixel; it only sends parameters.

**Live:** [bryncraft.online](https://bryncraft.online/)

## Architecture

Two completely separated layers, talking over one flat JSON message:

1. **React (`src/`)** owns every widget: top bar, effect library,
   inspector, timeline, code panel, export dialog. It never draws pixels.
2. **Raylib/WASM (`native/`)** owns rendering, camera, simulation and GPU
   drawing inside one `<canvas>`. It never draws a button.

```json
{ "effect": "ascii", "params": { "contrast": 1.2, "brightness": 0.8, "gamma": 1.1 } }
```

sent by `src/lib/wasmBridge.ts` → `Module.ccall('js_set_effect_json', ...)`
→ decoded in `native/main.c` by the allocation-free parser in
`native/json_mini.c`. The renderer reports back over
`js_get_stats_json()` (`fps`, `resolutionW/H`, `frame`, `effect`,
`gpuFrameTimeMs`), polled from the same bridge.

Every effect (frontend definition **and** native implementation) is
registered from a single list in each layer — see
[Adding a new effect](#adding-a-new-effect) below.

## Running it

There are two ways to run Bryncraft, depending on whether you need the
real renderer or just want to work on the UI.

### Full stack, real renderer (recommended) — Docker

This is the flow that actually compiles the Raylib/WASM renderer and
serves the app the way it's meant to run, over HTTPS.

**Prerequisites:** Docker and Docker Compose.

```bash
# Start (builds images the first time, otherwise just starts them)
make up
```

The app is served by `nginx` — check `docker-compose.yml` for the exact
ports, currently:

- **`https://localhost:4443/`** — TLS (self-signed cert baked into the
  `nginx` image), this is the one to use.
- `http://localhost:8081/` — plain HTTP, redirects straight to the HTTPS
  port above.

`nginx` terminates TLS and proxies everything to the `frontend` service,
which is not exposed to the host directly.

Other useful targets (`make help` lists them all):

```bash
make wasm         # rebuild the frontend image (Vite + WASM) and restart — use this after native/ or src/ changes
make wasm-full     # same, but with --no-cache — use if a stale layer is misbehaving
make logs          # follow logs for all services
make logs-frontend # follow logs for just one service
make down          # stop and remove containers (keeps images/volumes)
make reset         # wipe public/wasm + public/ffmpeg, rebuild with --no-cache, start
```

`make wasm`/`make up` rebuild by re-running `native/Makefile` **inside**
a `wasm-builder` Docker stage (see `Dockerfile`), so you don't need
Emscripten, raylib or OpenCV installed on your host at all for this flow.

### Frontend only — local Vite dev server

```bash
npm install
npm run dev
```

`wasmBridge.ts` probes for `/wasm/index.js` on startup. If it's not
there, it falls back to `src/lib/mockRenderer.ts`, a Canvas2D
implementation of ASCII/Particles/CRT driven by the exact same parameter
schema. This is a fast UI-only preview loop — it is **not** the final
visual output (no shaders, no OpenCV), and it doesn't render the
`opencv` effect at all.

If you want the real WASM renderer without the full Docker stack:

```bash
make wasm-assets   # extracts index.js/.wasm/.data + ffmpeg-core.* into public/ (needs Docker)
npm run dev
```

### Native renderer only — no browser at all

For iterating on a single effect's C code without paying for a full
Vite/Docker cycle, each effect under `native/effects/<name>/` has its own
standalone `main.c` you can compile directly with `gcc`/raylib (see
`native/effects/*/README.md` for the exact command per effect — OpenCV
needs a C++ compiler and its own build script, the rest are a one-liner).
`native/README.md` also documents `cd native && make` to build the full
WASM module by hand (installs its own `emsdk`/raylib/ffmpeg toolchain,
several GB — this is what the Docker `wasm-builder` stage does for you).

## Project layout

```
src/
  types/effects.ts        Shared param-schema primitives
  effects/                One file per effect: params + codegen + thumbnail
    shared.ts              Types + C-formatting helpers (fmtFloat, fmtInt, escapeCString, ...)
    ascii.ts / particles.ts / crt.ts / opencv.ts
    index.ts                EFFECT_MODULES — the single registration point (Record<EffectId, ...> keeps it exhaustive)
  store/useAppStore.ts    Zustand store — active effect, params, viewport, timeline, export
  lib/wasmBridge.ts       JSON bridge to the compiled Module (or mock fallback)
  lib/mockRenderer.ts     Canvas2D preview renderer (dev-time stand-in for native/)
  codegen/generateRaylibCode.ts   Generates the Code/Shader/JSON/README panel content
  components/
    layout/               TopBar, LeftSidebar, CenterViewport, RightInspector,
                           BottomTimeline, CodePanel, ExportPanel, AppShell
    canvas/ViewportCanvas.tsx
    effects/              EffectThumbnail, ParamField
    ui/                   shadcn-style primitives (button, slider, tabs, ...)
native/
  main.c                  Window setup, JS-exported functions, effect dispatch (macro-generated from effect_common.h)
  json_mini.c/.h          Allocation-free JSON parser
  effects/
    effect_common.h        EFFECT_LIST — the single registration point for every effect (enum + string<->enum + dispatch)
    ascii/  particles/  crt/  opencv/     One self-contained folder per effect (header, standalone demo, README, extras)
    main.c                 Combined demo — all effects in one raylib window
  assets/shaders/crt.fs   CRT fragment shader
  assets/cv/              Haar cascade for opencv's face_detect mode
  video_export_emscripten.js / video_export.h   MediaRecorder + FFmpeg.wasm export pipeline
  Makefile                emsdk + raylib(PLATFORM_WEB) + OpenCV(wasm) + ffmpeg.wasm build
Dockerfile                 3 stages: wasm-builder (native/Makefile) → build (tsc -b && vite build) → nginx (static + TLS)
docker-compose.yml          nginx (4443 TLS / 8081 HTTP, host-facing) + frontend (internal only)
```

## Effects

- **ASCII Renderer** (`ascii`) — image → character grid; `normal`
  (grayscale ramp) and `matrix` (falling-character rain) modes.
- **Particle System** (`particles`) — count, spawn rate, gravity,
  lifetime, size + falloff, color, spread.
- **CRT** (`crt`) — scanlines, barrel distortion, noise, chromatic
  aberration, vignette, flicker. Fragment shader (`assets/shaders/crt.fs`).
- **OpenCV Vision** (`opencv`) — CPU-only, no shader. Five modes: `edges`
  (Canny), `contours`, `optical_flow`, `bg_subtract`, `face_detect` (Haar
  cascade). The only effect compiled as C++ instead of plain C.

Every effect follows the same 5-function C contract
(`<Name>Effect_Init/_SetParams/_Update/_Draw/_Unload`) and the same
frontend shape (`EffectDefinition` + codegen + thumbnail) — see
[Adding a new effect](#adding-a-new-effect).

## Adding a new effect

Full walkthrough, verified end-to-end against this repo, in
[`docs/adding-a-new-effect.md`](./docs/adding-a-new-effect.md). Short version:
one new folder in `native/effects/<name>/`, one new file in
`src/effects/<name>.ts`, and 3 one-line registrations (`EFFECT_LIST` in
`native/effects/effect_common.h`, one `#include` in `native/main.c`, one
entry in `EFFECT_MODULES` in `src/effects/index.ts`) — everything else
(`EffectId`, the Inspector, the dispatch switch, the code panel) is
generated from those lists, so a missing step fails loudly at compile
time instead of silently at runtime.

## Theme

Dark, professional creative-tool aesthetic. Accent `#44D4FF`. Tokens live
in `tailwind.config.ts` and `src/index.css`.
