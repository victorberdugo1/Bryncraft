# CRT + VHS

Cathode-ray-tube screen post-processing and VHS tape artifacts via a GLSL
100 fragment shader (WebGL1/ES-compatible): animated scanlines, barrel
curvature, vignette, noise, chromatic aberration, flicker, tracking error,
tape warp, dropout lines, jitter, vertical roll, ghosting — plus a
VHS-player-style overlay (PLAY/PAUSE/REW/FF + date/time), all controlled
from a single `CRT_Params`.

> **Unlike `ascii`/`particles`/`opencv`, this is the only effect in the
> family that runs on a real GLSL shader** (`CRT_FS_SOURCE`, compiled in
> `CrtEffect_Init()` with `LoadShaderFromMemory`/`LoadShader`).
> The others process the image on the CPU or with plain `raylib`; here all
> the heavy lifting (scanlines, curvature, ghosting, etc.) goes through the
> GPU via a fragment shader. This matters for anyone touching the code:
> the `CRT_Params` fields aren't just flags the CPU reads — they're
> uniforms uploaded every frame with `SetShaderValue`/`SetShaderValueTexture`
> — if you add a new field, you need to declare it in the GLSL, map it
> with `GetShaderLocation` in `CrtEffect_Init()`, and upload it in
> `CrtEffect_Draw()`.

## Files in this folder

| File | What it is |
|---|---|
| `crt_effect.h` | Single-header: `CrtEffect_Init/SetParams/Update/Draw/Unload` + the GLSL embedded in `CRT_FS_SOURCE` |
| `main002.c` | Minimal standalone demo — just this effect, without the rest of Bryncraft |

## Using an external `crt.fs` (edit the shader without recompiling)

`CrtEffect_Init()` looks for a `crt.fs` file next to the executable:

- **If it exists**, it loads it with `LoadShader(NULL, "crt.fs")`. This is
  useful for iterating on the shader by hand (edit the `.fs`, rerun the
  program, see the change) without having to recompile the `.c`.
- **If it doesn't exist**, it uses the GLSL embedded in the `CRT_FS_SOURCE`
  string inside `crt_effect.h` (via `LoadShaderFromMemory`).

It's literally the same code in both cases — once you want to "lock in"
your changes from the loose `.fs`, just paste its content back into
`CRT_FS_SOURCE` (respecting C's `"` and `\n` escaping) and you're back to
having everything in a single header.

Unlike `ascii`/`particles`/`opencv`, this effect does have a real shader:
the app's "Extra" tab doesn't point to a separate `.fs` — the GLSL is
extracted live from `CRT_FS_SOURCE`, so what gets downloaded is always
exactly what gets compiled. If you put your own `crt.fs` next to the
executable locally, it overrides the embedded one only in your native
build.

## Quick start (copy & paste)

```c
#include "raylib.h"
#include "crt_effect.h"

int main(void) {
    InitWindow(800, 600, "my game");
    SetTargetFPS(60);

    RenderTexture2D scene = LoadRenderTexture(800, 600);
    CrtEffect_Init(); // compiles the shader (external or embedded) — call once, after InitWindow

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        BeginTextureMode(scene);
            ClearBackground(DARKGRAY);
            // ... your normal drawing goes here ...
        EndTextureMode();

        CrtEffect_Update(dt);
        BeginDrawing();
            ClearBackground(BLACK);
            CrtEffect_Draw(scene, 800, 600);
        EndDrawing();
    }

    CrtEffect_Unload();
    UnloadRenderTexture(scene);
    CloseWindow();
    return 0;
}
```

## Parameters

Everything is controlled by tweaking the global variable `CRT_g_params`
(struct `CRT_Params`, declared in `crt_effect.h`).

### Classic CRT

| Field | What it does |
|---|---|
| `scanlineIntensity` | Strength of the horizontal lines |
| `scanlineCount` | Number of lines |
| `scanlineSpeed` | Scanline scroll speed |
| `curvature` | Barrel curvature (convex-screen-style deformation) |
| `vignette` | Darkening toward the edges |
| `noise` | General static-like noise |
| `chromaticAberration` | R/B channel separation (RGB split) |
| `flicker` | Global brightness flicker |

### VHS — off (`0`) by default, enable as desired

| Field | Effect it simulates | What it does |
|---|---|---|
| `trackingGlitch` | Tracking Error / Horizontal Sync Glitch | Horizontal band that appears and cuts/shakes, like when tape tracking gets misaligned |
| `waveDistortion` + `waveSpeed` | Tape Warp / Time Base Error | Smooth horizontal wave that distorts the image |
| `dropoutLines` | Dropout Lines / Tape Noise | Intermittent white-noise streaks, like signal loss on tape |
| `jitter` | Frame Jitter | Global horizontal shake of the whole image |
| `verticalRoll` | Vertical Hold Error | The image "rolls" vertically nonstop (0 = still) |
| `ghosting` | Ghosting / Signal Echo | Trailing/echo of previous frames over the current one |

Effects from the original list that **were not added** because they were
already covered or didn't add enough value for the cost: *Analog
Blur/Tape Blur* and *Generation Loss* (already covered by that
degradation feel from `noise` + `dropoutLines`), *Analog Compression
Artifacts* (requires macroblocks, doesn't add anything to this
lightweight shader), and *RF Noise/Chroma Delay* (redundant with
`chromaticAberration` + `noise`).

### VHS overlay (icon + date/time)

This is drawn with plain `raylib` after the shader — it's not part of the
GLSL:

| Field | What it does |
|---|---|
| `vhsOverlay` | `true`/`false` — show the overlay |
| `vhsIcon` | `CRT_VHS_ICON_PLAY` / `_PAUSE` / `_REW` / `_FF` / `_STOP` / `_REC` / `_NONE` |
| `vhsTimestamp` | Free text (up to 31 chars), e.g. `"JUL-15-1998  12:34 PM"` |
| `vhsLabel` | Short text (up to 7 chars) next to the icon, e.g. `"SP"` / `"LP"` |

```c
CRT_g_params.vhsOverlay = true;
CRT_g_params.vhsIcon = CRT_VHS_ICON_PLAY;
strcpy(CRT_g_params.vhsLabel, "SP");
strcpy(CRT_g_params.vhsTimestamp, "JUL-15-1998  12:34 PM");
```

The icon flickers gently (like a real VCR's OSD) and the timestamp stays
fixed at the bottom-left with a drop shadow behind it so it's readable on
any background. If you want the real system date/time, build the string
yourself (with `time.h`/`strftime`, or whatever you use) and copy it into
`vhsTimestamp` every so often — it doesn't need to happen every frame, a
couple of times per second is enough.

## Typical combinations

```c
// Good old clean CRT
CRT_g_params.scanlineIntensity = 0.35f;
CRT_g_params.curvature = 0.15f;
CRT_g_params.vignette = 0.3f;

// Old, beat-up VHS tape
CRT_g_params.trackingGlitch = 0.6f;
CRT_g_params.waveDistortion = 0.5f;
CRT_g_params.dropoutLines = 0.4f;
CRT_g_params.jitter = 0.3f;
CRT_g_params.ghosting = 0.35f;
CRT_g_params.vhsOverlay = true;
CRT_g_params.vhsIcon = CRT_VHS_ICON_PLAY;
strcpy(CRT_g_params.vhsLabel, "SP");
strcpy(CRT_g_params.vhsTimestamp, "JUL-15-1998  12:34 PM");

// Tape rewinding
CRT_g_params.vhsIcon = CRT_VHS_ICON_REW;
CRT_g_params.trackingGlitch = 0.8f;
CRT_g_params.jitter = 0.6f;
```

## JSON contract (`CrtEffect_SetParams`)

```json
{ "effect": "crt", "params": {
  "scanlineIntensity": 0.3, "scanlineCount": 240.0, "scanlineSpeed": 1.0,
  "curvature": 0.1, "vignette": 0.3, "noise": 0.05,
  "chromaticAberration": 0.002, "flicker": 0.02,

  "trackingGlitch": 0.0, "waveDistortion": 0.0, "waveSpeed": 1.5,
  "dropoutLines": 0.0, "jitter": 0.0, "verticalRoll": 0.0, "ghosting": 0.0,

  "vhsOverlay": false, "vhsIcon": "none | play | pause | rew | ff | stop | rec",
  "vhsTimestamp": "JUL-15-1998  12:34 PM", "vhsLabel": "SP"
} }
```

## Building the standalone demo

`raylib.h` / `libraylib.a` are one level up, in `native/effects/` (shared
by the 4 demos):

```bash
gcc -o crt_demo.exe main002.c -I.. -L.. -lraylib -lgdi32 -lwinmm   # Windows
gcc -o crt_demo main002.c -I.. -L.. -lraylib -lm -lpthread -ldl -lrt -lX11   # Linux
```

## Full build (WASM, inside Bryncraft)

This is built as part of the `native/` build (`make` from `native/`, see
`native/README.md`); nothing needs to be touched here for that.
