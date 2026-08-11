# Adding a new effect to Bryncraft

Verified by hand against the real repo (created, compiled, reverted). The
example below is a minimal effect called `brightness` — a single slider,
no shader.

---

## Where each file goes

```
native/effects/brightness/
├── brightness_effect.h   ← the effect itself (C logic)
├── main.c                ← standalone demo (optional)
└── README.md             ← effect docs (optional)

src/effects/
└── brightness.ts         ← params + codegen + thumbnail, all in one file
```

Note about `main.c`: the 4 existing effects have their demos numbered
(`main000.c`, `main001.c`, etc.) as an old convention, but `make` (the
actual WASM build) **doesn't use them for anything** — it only compiles
`native/main.c` + `json_mini.c` (you can check this yourself in
`native/Makefile`, the `SRC_C := main.c json_mini.c` line). Since each
demo already lives in its own per-effect folder, there's no risk of name
collisions — just call it `main.c` plain and simple, no need to track
which number comes next.

Plus 3 lines in files that already exist:

```
native/effects/effect_common.h   → 1 new line in EFFECT_LIST
native/main.c                    → 1 new #include
src/effects/index.ts             → 1 import + 1 entry in EFFECT_MODULES
```

---

## Step 1 — `native/effects/brightness/brightness_effect.h`

Every effect implements these 5 functions, always with these exact names
(swapping `Brightness` for whatever prefix you pick):

```c
#ifndef BRIGHTNESS_EFFECT_H
#define BRIGHTNESS_EFFECT_H

#include "raylib.h"

#ifdef __EMSCRIPTEN__
#include "../../json_mini.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void BrightnessEffect_Init(void);
#ifdef __EMSCRIPTEN__
void BrightnessEffect_SetParams(const JsonValue *paramsObj);
#endif
void BrightnessEffect_Update(float dt);
void BrightnessEffect_Draw(RenderTexture2D scene, int screenW, int screenH);
void BrightnessEffect_Unload(void);

#ifdef __cplusplus
}
#endif

typedef struct {
    float intensity; // 0..2 — 1.0 is normal brightness
} Brightness_Params;

static Brightness_Params BRI_g_params = { 1.0f };

void BrightnessEffect_Init(void) { }

#ifdef __EMSCRIPTEN__
void BrightnessEffect_SetParams(const JsonValue *paramsObj) {
    if (!paramsObj) return;
    BRI_g_params.intensity = (float)JsonAsNumber(JsonObjectGet(paramsObj, "intensity"), BRI_g_params.intensity);
}
#endif

void BrightnessEffect_Update(float dt) { (void)dt; }

void BrightnessEffect_Draw(RenderTexture2D scene, int screenW, int screenH) {
    (void)screenW; (void)screenH;
    unsigned char v = (unsigned char)(BRI_g_params.intensity * 255.0f > 255.0f ? 255.0f : BRI_g_params.intensity * 255.0f);
    Color tint = (Color){ v, v, v, 255 };
    DrawTextureRec(scene.texture,
        (Rectangle){0,0,(float)scene.texture.width,-(float)scene.texture.height},
        (Vector2){0,0}, tint);
}

void BrightnessEffect_Unload(void) { }

#endif /* BRIGHTNESS_EFFECT_H */
```

`Init`/`Unload` are empty because this effect doesn't need to
create/release anything (no shader, no buffers). They still have to
exist with those exact names — the dispatcher in `native/main.c`
(step 4) calls them without checking whether they actually do anything.

## Step 2 — `native/effects/brightness/main.c` (demo, optional)

Lets you try the effect on its own, without spinning up the whole
frontend. `make` doesn't use it — it's just for you.

```c
// main.c — minimal example: Brightness

#include "raylib.h"
#include "brightness_effect.h"

int main(void) {
    const int screenW = 800, screenH = 600;
    InitWindow(screenW, screenH, "Brightness — minimal example");
    SetTargetFPS(60);

    RenderTexture2D scene = LoadRenderTexture(screenW, screenH);
    BrightnessEffect_Init();

    while (!WindowShouldClose()) {
        BeginTextureMode(scene);
            ClearBackground(DARKGRAY);
            DrawCircle(screenW/2, screenH/2, 120, RED);
        EndTextureMode();

        BrightnessEffect_Update(GetFrameTime());
        BeginDrawing();
            ClearBackground(BLACK);
            BrightnessEffect_Draw(scene, screenW, screenH);
        EndDrawing();
    }

    BrightnessEffect_Unload();
    UnloadRenderTexture(scene);
    CloseWindow();
    return 0;
}
```

Compile and run (Windows/MinGW example):

```bash
gcc -o brightness_demo.exe native/effects/brightness/main.c -Inative/effects -Lnative/effects -lraylib -lgdi32 -lwinmm
```

## Step 3 — `native/effects/brightness/README.md` (optional)

````markdown
# Brightness

Multiplies image brightness via `DrawTextureRec`'s tint — no shader, no
GLSL.

## Parameters

| Field | Range | What it does |
|---|---|---|
| `intensity` | 0..2 (default 1.0) | Brightness multiplier |

## JSON contract (`BrightnessEffect_SetParams`)

```json
{ "effect": "brightness", "params": { "intensity": 1.0 } }
```
````

## Step 4 — register in `native/effects/effect_common.h` (1 line)

Add a line to the `EFFECT_LIST` macro (which already exists in that
file):

```c
#define EFFECT_LIST(X) \
    X(ASCII,     ascii,     Ascii,     false) \
    X(PARTICLES, particles, Particles, false) \
    X(CRT,       crt,       Crt,       true)  \
    X(OPENCV,    opencv,    Opencv,    true)  \
    X(BRIGHTNESS, brightness, Brightness, false)   /* ← this line */
```

The 4 fields are: enum name (uppercase), lowercase id (the same one used
in the JSON and as the folder name), C function prefix, and whether the
effect needs `ClearBackground(BLANK)` before drawing (only needed if your
effect can leave transparent areas; `brightness` doesn't).

## Step 5 — register in `native/main.c` (1 `#include`)

```c
#include "effects/opencv/opencv_effect.h"
#include "effects/brightness/brightness_effect.h"   /* ← this line */
```

This is the only manual step in `main.c` — the C preprocessor can't
build file paths on its own. Everything else in that file (the switches,
the `Init`/`Unload` calls) is generated straight from the step-4 list.

## Step 6 — `src/effects/brightness.ts` (the one frontend file)

This file combines the 3 things that used to live in 3 separate folders:
what controls the Inspector shows, how the downloadable `.h` is built
with those values, and how the thumbnail is drawn in the sidebar.

```ts
import {
  fmtFloat,
  type EffectDefinition,
  type EffectCodegenModule,
  type EffectParams,
  type EffectModule,
  type ThumbnailDrawFn,
} from "./shared";

import headerRaw from "../../native/effects/brightness/brightness_effect.h?raw";
import mainRaw from "../../native/effects/brightness/main.c?raw";
import readmeRaw from "../../native/effects/brightness/README.md?raw";

// --- 1. Parameter definition (Inspector) ---
// A single control: the intensity slider.
const definition: EffectDefinition<"brightness"> = {
  id: "brightness",
  name: "Brightness",
  description: "Simple brightness multiplier via texture tint — no shader needed.",
  params: [
    { key: "intensity", label: "Intensity", type: "float", default: 1.0, min: 0, max: 2, step: 0.01, group: "Brightness" },
  ],
};

// --- 2. Codegen (builds the downloadable .h with the slider's current value) ---
function buildParamsBlock(params: EffectParams): string {
  return `static Brightness_Params BRI_g_params = {
    .intensity = ${fmtFloat(params.intensity)},
};
`;
}

const codegen: EffectCodegenModule = {
  headerRaw,
  mainRaw,
  mainFilename: "main.c",
  readmeRaw,
  // Has to match the real block in the .h above EXACTLY (same struct name
  // and same global variable name) — this is what gets replaced when
  // downloading the code with the Inspector's current values.
  paramsRegex: /static Brightness_Params BRI_g_params = \{[\s\S]*?\};\r?\n/,
  buildParamsBlock,
  extras: [],
};

// --- 3. Thumbnail (animated preview, sidebar) ---
const thumbnail: ThumbnailDrawFn = (ctx, w, h, t) => {
  const pulse = (Math.sin(t * 2) + 1) / 2; // 0..1
  ctx.fillStyle = `rgba(255,255,255,${0.15 + pulse * 0.55})`;
  ctx.beginPath();
  ctx.arc(w / 2, h / 2, Math.min(w, h) * 0.28, 0, Math.PI * 2);
  ctx.fill();
};

// --- Final package ---
export const BRIGHTNESS_MODULE: EffectModule<"brightness"> = { definition, codegen, thumbnail };
```

`fmtFloat`, `fmtInt`, `hexToRgbComment`, `escapeCString`, `extractCString`
are C-formatting helpers already written in `src/effects/shared.ts` — no
need to reinvent them, just import them.

## Step 7 — register in `src/effects/index.ts` (1 import + 1 line)

```ts
import { BRIGHTNESS_MODULE } from "./brightness";   // ← new import

export const EFFECT_MODULES = [
  ASCII_MODULE,
  PARTICLES_MODULE,
  CRT_MODULE,
  OPENCV_MODULE,
  BRIGHTNESS_MODULE,   // ← new entry
] as const;
```

`EffectId`, `EFFECT_DEFINITIONS`, `CODEGEN_MODULES`, `THUMBNAILS`,
`defaultParamsFor()` — all of that is derived from this one list. No need
to touch `useAppStore.ts`, `LeftSidebar.tsx`, `EffectThumbnail.tsx`, or
`generateRaylibCode.ts`: they all read from here.

---

## Why you don't need to memorize the 3 steps above

If you forget step 7 (registering in `index.ts`), **the TypeScript build
breaks as soon as it compiles**, with a message that says exactly what's
missing:

```
Property 'brightness' is missing in type '...' but required in type
'Record<"ascii" | "particles" | "crt" | "opencv" | "brightness", ...>'
```

That's because `EFFECT_DEFINITIONS`, `CODEGEN_MODULES` and `THUMBNAILS`
in `src/effects/index.ts` are typed as `Record<EffectId, ...>` — a
`Record` requires ALL keys of the `EffectId` type, so if you add a new id
to `EFFECT_MODULES` but miss something, it won't fail silently.

The same happens on the C side: if you forget step 4 or step 5, the
macro loop in `native/main.c` references a function that doesn't exist
(`BrightnessEffect_SetParams`, etc.) and the WASM build won't compile.

---

## Verifying it worked (your flow: WSL + Docker at https://localhost:4443/)

```bash
# 1. from native/, in WSL — quick compile, just to check that the new
#    effect's C compiles (this isn't what serves the app; it's a cheap
#    check before paying for a full Docker rebuild)
cd native && make
```

```bash
# 2. from the repo root — rebuilds the frontend image (this re-runs the
#    WASM build inside Docker, with the new effect already included) and
#    restarts the containers
make wasm
```

With the containers up, check `https://localhost:4443/` (the port
`nginx` exposes, per `docker-compose.yml`) and the new effect should
already show up in the sidebar, with its slider and its thumbnail.

If `cd native && make` fails, the error will point straight at the
missing line (typically the step-5 `#include` or the step-4
`EFFECT_LIST` entry) — fix it there before spending time on the step-2
Docker rebuild, which is a lot slower.

## Checklist

- [ ] `brightness_effect.h` implements the 5 contract functions (`Init`/`Unload` can be empty `{}`)
- [ ] 1 line in `EFFECT_LIST` (`native/effects/effect_common.h`)
- [ ] 1 `#include` in `native/main.c`
- [ ] `src/effects/brightness.ts` with `definition` + `codegen` + `thumbnail`
- [ ] 1 import + 1 entry in `EFFECT_MODULES` (`src/effects/index.ts`)
- [ ] `paramsRegex`/`buildParamsBlock` reflect the real C struct (same struct and variable name)
- [ ] `make` compiles with no errors
- [ ] The effect shows up on localhost, in the sidebar, with its thumbnail
- [ ] Moving the slider in the Inspector updates the viewport live
- [ ] Downloading the generated `.c` includes the slider's current value, not the hardcoded default
