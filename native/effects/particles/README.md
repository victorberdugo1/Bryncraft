# Particle System (single-header, raylib)

A particle system with a fixed pool (no allocs in the hot path) that
draws on top of whatever you've rendered into a `RenderTexture2D`.
Three modes included:

- **fountain**: spawns from a fixed point, with angular spread.
- **rain**: falls from the top of the screen, with lateral wind.
- **embers**: rises with a side-to-side sway, like embers/sparks.

All three have an optional **reactive** mode: every frame the effect
downsamples your scene, reads it back on the CPU, and uses that to
modulate particle color/brightness, deflect movement via a luminance
gradient (flow field), and bias where particles spawn — it also
compares against the previous frame's downsample to react to actual
motion in the video, not just static bright/dark areas.

## Quick start (copy & paste)

1. Copy `particles_effect.h` into your project.
2. In your code:

```c
#include "raylib.h"
#include "particles_effect.h"

int main(void) {
    InitWindow(800, 600, "my game");
    SetTargetFPS(60);

    RenderTexture2D scene = LoadRenderTexture(800, 600);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // 1. Render your game as usual, but into "scene"
        BeginTextureMode(scene);
            ClearBackground(DARKGRAY);
            // ... your regular drawing goes here ...
        EndTextureMode();

        // 2. Apply the effect
        ParticlesEffect_Update(dt);
        BeginDrawing();
            ClearBackground(BLACK);
            ParticlesEffect_Draw(scene, 800, 600);
        EndDrawing();
    }

    UnloadRenderTexture(scene);
    CloseWindow();
    return 0;
}
```

That's it. No JSON or extra libraries needed for the basics.

`ParticlesEffect_Draw` already draws the background scene (with a
slight darkening overlay) and the particles on top of it — you don't
need to draw `scene` yourself separately, just pass it in as a
parameter.

## Changing parameters (mode, color, count...)

Everything is controlled by editing the global `PART_g_params`
variable directly (declared inside `particles_effect.h`). For example,
before your loop:

```c
PART_g_params.mode = PART_MODE_RAIN;          // PART_MODE_FOUNTAIN / PART_MODE_RAIN / PART_MODE_EMBERS
PART_g_params.count = 2000;                   // max alive particles (cap: PARTICLES_MAX = 20000)
PART_g_params.spawnRate = 120.0f;             // new particles per second
PART_g_params.gravity = 9.8f;
PART_g_params.lifetime = 2.5f;
PART_g_params.size = 4.0f;
PART_g_params.sizeFalloff = 0.6f;             // 0 = constant size, 1 = shrinks to nothing by death
PART_g_params.color = (Color){ 68, 212, 255, 255 };
PART_g_params.spreadDeg = 45.0f;              // fountain only: spread angle
PART_g_params.spawnX = 0.5f;                  // fountain only: normalized origin (0..1)
PART_g_params.spawnY = 0.8f;
PART_g_params.windX = 0.0f;                   // rain and embers
PART_g_params.reactive = 1;                   // 0/1
PART_g_params.reactiveStrength = 0.6f;
PART_g_params.flowStrength = 0.8f;            // how much the flow field deflects particles
```

You can see every available field at the top of the `.h` file, in the
`PART_ParticleParams` struct. The values above are the defaults
(`rain` mode, reactive enabled).

## Modes in detail

| Mode | Behavior | Relevant params |
|---|---|---|
| `PART_MODE_FOUNTAIN` | Spawns at `(spawnX, spawnY)` and shoots upward at a random angle within `spreadDeg`, affected by `gravity`. | `spawnX`, `spawnY`, `spreadDeg`, `gravity` |
| `PART_MODE_RAIN` | Falls straight down from the top of the screen, with `windX` pushing it sideways. Drawn as a line (drop), not a circle. | `windX`, `gravity` doesn't apply (fall velocity is already fixed) |
| `PART_MODE_EMBERS` | Rises from the bottom with a sinusoidal side-to-side sway. `gravity` here controls rise speed, not fall speed. | `windX`, `gravity` |

If `reactive` is `1`, in `fountain`/`rain` particles spawn more where
the scene is dark or moving, and in `embers` where it's bright or
moving; in all three modes color and/or brightness get modulated by
what's underneath.

## Building

```bash
# Windows (MinGW)
gcc main.c -o game.exe -I. -L. -lraylib -lgdi32 -lwinmm

# Linux
gcc main.c -o game -I. -L. -lraylib -lm -lpthread -ldl -lrt -lX11
```

(`raylib.h` / `libraylib.a` need to be reachable through those
`-I`/`-L` flags; adjust the paths to wherever you have raylib
installed)

## Included demo

`main001.c` is a minimal standalone example (a red circle with the
particle effect on top). Build it the same way as above.

## Feeding the effect with video or a camera

Same as with any other effect in this family: `ParticlesEffect_Draw`
doesn't care where `scene`'s content came from, it just needs a
`RenderTexture2D` with something drawn into it.

- **Video**: if you decode a video frame by frame (e.g. with an
  external video library), just draw each decoded frame inside
  `BeginTextureMode(scene)/EndTextureMode()` before calling
  `ParticlesEffect_Draw`, exactly like you would with any other
  drawing.
- **Camera**: raylib has no built-in webcam capture, so you'll need a
  separate library to read frames from the camera (OpenCV, for
  example). Once you have each frame as an `Image`/`Texture2D`, load it
  into `scene` with `UpdateTexture`, or draw it with `DrawTexturePro`
  inside `BeginTextureMode(scene)`.

Bottom line: the pattern is always the same, only where `scene`'s
content comes from changes.

## Optional JSON usage (only if you embed this in an Emscripten/JS
runtime, e.g. inside a website)

If you build with `__EMSCRIPTEN__` defined and add your own minimal
JSON parser (not included in this folder), `ParticlesEffect_SetParams`
becomes available, accepting this contract:

```json
{ "effect": "particles", "params": {
  "mode": "fountain | rain | embers",
  "count": 500, "spawnRate": 20.0,
  "gravity": 9.8, "lifetime": 2.0,
  "size": 4.0, "sizeFalloff": 1.0,
  "color": "#RRGGBB", "spread": 30.0,
  "spawnX": 0.5, "spawnY": 0.5, "wind": 0.0,
  "reactive": false, "reactiveStrength": 1.0, "flowStrength": 1.0
} }
```

If you build natively without Emscripten, ignore this section
entirely: everything is configured by touching `PART_g_params` as
shown above.
