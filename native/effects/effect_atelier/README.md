# Element Burst

A 3D world-space particle burst rendered with an orbiting `Camera3D`,
using the same billboard-circle technique (`GetWorldToScreen` + nested
`DrawCircleV`) as Enuma Ichor's `CardVFX_Render*Particles` functions in
`inc/effects.h`. Loops automatically on `loopInterval` so it plays as a
live preview.

## Parameters

| Field | Range | What it does |
|---|---|---|
| `element` | select (neutral/fire/water/earth/wind/lightning/dark/poison) | Matches Enuma Ichor's `Element` enum, purely descriptive here |
| `presetName` | string | Name used for the exported preset and the in-game `VFX_CUSTOM_<name>` trigger |
| `particleCount` | 4..64 | Particles per burst |
| `spawnRadiusMin` / `spawnRadiusMax` | 0..3 | Seed sphere radius the burst launches from |
| `speedMin` / `speedMax` | 0..10 | Initial particle speed range |
| `lifeMin` / `lifeMax` | 0.05..3 | Particle lifetime range (seconds) |
| `loopInterval` | 0.2..5 | Seconds between automatic re-triggers in the preview |
| `gravity` | -5..5 | Downward acceleration |
| `drag` | 0..4 | Velocity damping per second |
| `colorCore` / `colorMid` / `colorOuter` | color | Three-layer billboard gradient |
| `additive` | bool | Additive vs alpha blending |
| `cameraDistance` | 2..15 | Orbit camera radius |
| `cameraOrbitSpeed` | -180..180 | Orbit camera speed (deg/sec) |
| `showGrid` | bool | Reference floor grid — turn off before a transparent-video export |

## Combo layers and mesh shapes

`mode` (the base shape) and each of the 3 combo layers can independently be
set to `shield`, `fire_orbs`, `wind_spin`, or `fire_wind`. Any of those set
anywhere (base or an enabled layer) renders — so `shield` on the base with
`fire_wind` on Combo Layer 2 draws both at once. `fire_orbs`, `wind_spin`,
and `fire_wind` share a single orb pool, so only one of them animates if
more than one is selected at the same time (`fire_wind` takes priority,
then `wind_spin`, then `fire_orbs`); `shield` has its own state and always
combines cleanly with any of them. All other shapes (`sphere`, `ring`,
`spiral`, etc.) still spawn their own independent particle burst per layer.

## JSON contract (`EffectAtelierEffect_SetParams`)

```json
{
  "effect": "effect_atelier",
  "params": {
    "element": "fire",
    "presetName": "meteor_impact",
    "particleCount": 24,
    "spawnRadiusMin": 0.4, "spawnRadiusMax": 1.0,
    "speedMin": 1.2, "speedMax": 3.0,
    "lifeMin": 0.4, "lifeMax": 0.9,
    "loopInterval": 1.2,
    "gravity": 2.0, "drag": 1.5,
    "colorCore": "#FFE08C", "colorMid": "#FF7818", "colorOuter": "#FF3C00",
    "additive": true,
    "cameraDistance": 5.5, "cameraOrbitSpeed": 18, "showGrid": true
  }
}
```

## Exporting into Enuma Ichor

The "Enuma Ichor preset" extra generated alongside the effect code is a
`CustomBurstPreset` struct literal ready to paste into
`inc/effects_custom.h`, inside `CUSTOM_BURST_PRESETS[]`. Once pasted,
trigger it from any card's `VFX_` field with `VFX_CUSTOM_<presetName>` —
no other game code changes needed. The same preset struct is plain C and
has no Bryncraft/Emscripten dependency, so it drops into any raylib
project that defines a matching `CustomBurstPreset` shape.
