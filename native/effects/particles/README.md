# Particle System

Simulación de partículas con pool fijo (sin allocs en el hot path). Tres
modos — `fountain` (brota de un punto), `rain` (cae desde arriba, con
viento) y `embers` (sube con vaivén) — todos con un modo "reactive"
opcional: cada frame se hace un downsample de la escena a una textura
pequeña, se lee a CPU, y con eso se modula color/brillo, se desvía el
movimiento por gradiente de luminancia (flow field) y se sesga dónde nacen
las partículas. También compara contra el downsample del frame anterior
para reaccionar a movimiento real en el vídeo, no solo a zonas claras/oscuras
estáticas.

## Archivos de esta carpeta

| Archivo | Qué es |
|---|---|
| `particles_effect.h` | Single-header: `ParticlesEffect_SetParams/Update/Draw/Unload`, todo el efecto |
| `main001.c` | Demo standalone mínima — solo este efecto, sin el resto de Bryncraft |

Este efecto no tiene archivos extra (no usa shader propio ni assets
externos) — el tab "Extra" de la app lo indica.

## Contrato JSON (`ParticlesEffect_SetParams`)

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

## Compilar la demo standalone

`raylib.h` / `libraylib.a` están un nivel arriba, en `native/effects/`
(compartidos por las 4 demos):

```bash
gcc -o particles_demo.exe main001.c -I.. -L.. -lraylib -lgdi32 -lwinmm   # Windows
```

## Build completo (WASM, dentro de Bryncraft)

Se compila como parte del build de `native/` (`make` desde `native/`, ver
`native/README.md`); no hace falta tocar nada aquí para eso.
