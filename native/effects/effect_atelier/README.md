# Element Burst

A ready-to-use magic/element particle effect for [raylib](https://www.raylib.com/)
games — fireballs, lightning, shields, water rings, heal auras, and more,
all in a single `effect_atelier.h` file. No engine, no dependencies beyond
raylib.

Try it live: click or press SPACE in the demo window to fire the effect.

## 1. Add it to your game

Copy `effect_atelier.h` into your project and include it once, next to raylib:

```c
#include "raylib.h"
#include "effect_atelier.h"
```

## 2. The 5 functions you need

```c
EffectAtelierEffect_Init();                       // call once, after InitWindow()
EffectAtelierEffect_Trigger();                     // call whenever you want the effect to fire
EffectAtelierEffect_Update(GetFrameTime());        // call every frame
EffectAtelierEffect_Draw(scene, screenW, screenH);  // call every frame, between BeginDrawing()/EndDrawing()
EffectAtelierEffect_Unload();                      // call once, before CloseWindow()
```

That's the whole API. See `main.c` for a complete, working example — it
fires the effect on a mouse click or the SPACE key.

```c
#include "raylib.h"
#include "effect_atelier.h"

int main(void) {
    InitWindow(800, 600, "My Game");
    SetTargetFPS(60);
    RenderTexture2D scene = LoadRenderTexture(800, 600);
    EffectAtelierEffect_Init();

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            EffectAtelierEffect_Trigger();

        EffectAtelierEffect_Update(GetFrameTime());

        BeginDrawing();
            ClearBackground(BLACK);
            EffectAtelierEffect_Draw(scene, 800, 600);
        EndDrawing();
    }

    EffectAtelierEffect_Unload();
    UnloadRenderTexture(scene);
    CloseWindow();
    return 0;
}
```

## 3. Changing what the effect looks like

Open `effect_atelier.h` and find `EB_g_params` near the top of the file —
it's one big struct literal that describes the effect (shape, element,
colors, speed, particle count, etc). Edit the values directly and
recompile. There's no need to understand the rest of the file.

The two fields you'll change most:

```c
.element = EB_ELEM_FIRE,      // what color palette to use
.shape   = EB_SHAPE_SPHERE,   // what the effect looks like
```

**Elements** (color palettes): `EB_ELEM_NEUTRAL`, `EB_ELEM_FIRE`, `EB_ELEM_WATER`,
`EB_ELEM_EARTH`, `EB_ELEM_WIND`, `EB_ELEM_LIGHTNING`, `EB_ELEM_DARK`,
`EB_ELEM_POISON`, `EB_ELEM_LIGHT`, `EB_ELEM_ICE`

**Shapes** (what gets drawn): `EB_SHAPE_SPHERE`, `EB_SHAPE_RING`, `EB_SHAPE_SPIRAL`,
`EB_SHAPE_BEAM`, `EB_SHAPE_PILLAR`, `EB_SHAPE_RAIN`, `EB_SHAPE_WAVE`,
`EB_SHAPE_PROJECTILE`, `EB_SHAPE_JUMP`, `EB_SHAPE_SHIELD`, `EB_SHAPE_FIELD`,
`EB_SHAPE_FIRE_ORBS`, `EB_SHAPE_WIND_SPIN`, `EB_SHAPE_FIRE_WIND`,
`EB_SHAPE_WATER_RING`, `EB_SHAPE_EARTH_BURST`, `EB_SHAPE_FIRE_BURST`,
`EB_SHAPE_LIGHTNING_BURST`, `EB_SHAPE_POISON_BURST`, `EB_SHAPE_HEAL_AURA`,
`EB_SHAPE_DARK_SLASH`, `EB_SHAPE_BARRIER`, `EB_SHAPE_FIREBALL`,
`EB_SHAPE_WIND_SLASH`, `EB_SHAPE_ROCK_THROW`, `EB_SHAPE_LIGHTNING_BOLT`,
`EB_SHAPE_WATER_JET`, `EB_SHAPE_ICE_SHARD`, `EB_SHAPE_POISON_ORB`,
`EB_SHAPE_DARK_ORB`, `EB_SHAPE_LIGHT_ARROW`

Other useful fields:

| Field | What it does |
|---|---|
| `particleCount` | How many particles per burst |
| `speedMin` / `speedMax` | How fast particles fly out |
| `lifeMin` / `lifeMax` | How long particles last (seconds) |
| `colorCore` / `colorMid` / `colorOuter` | The 3-layer particle color gradient |
| `loopInterval` | Seconds between automatic re-triggers (on top of manual `Trigger()` calls) |
| `showGrid` | Show a reference floor grid |

## 4. Combining effects (combo layers)

`EB_g_params.extraLayers` is a list of 3 extra layers that play at the same
time as the main effect. Each layer has its own shape, color, and particle
count. To turn one on:

```c
.extraLayers = {
    { .enabled = 1, .shape = EB_SHAPE_WATER_RING, .particleCount = 24,
      .colorCore = (Color){255, 224, 140, 255},
      .colorMid  = (Color){255, 120,  24, 255},
      .colorOuter= (Color){255,  60,   0, 255} },
},
```

Set `.enabled = 0` (or leave it out) to keep a layer off. With all 3
enabled you get up to 4 shapes playing together — that's how effects like
"fire jump stomp" (a burst + an orbit + a ring) are built.

## 5. Exporting a preset from the web tool

If you designed an effect in the Bryncraft web tool, it exports a full
`EB_g_params` struct literal. Just paste it over the existing `EB_g_params`
definition in `effect_atelier.h` — no other changes needed.
