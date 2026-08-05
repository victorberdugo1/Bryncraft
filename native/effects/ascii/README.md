# ASCII Renderer

Convierte la escena 3D (renderizada a `RenderTexture2D`) en una grilla de
caracteres ASCII, dibujada carácter a carácter con `DrawTextEx`. Incluye un
modo "Matrix" (lluvia de caracteres cayendo, con estela y color de cabeza
independiente) además del modo normal en escala de grises con rampa de
caracteres configurable.

## Archivos de esta carpeta

| Archivo | Qué es |
|---|---|
| `ascii_effect.h` | Single-header: `AsciiEffect_SetParams/Update/Draw/Unload`, todo el efecto |
| `main000.c` | Demo standalone mínima — solo este efecto, sin el resto de Bryncraft |
| `NotoSansJP-Kana.ttf` | **Extra.** Fuente usada en modo Matrix (`matrixChars` con kana). Sin ella el modo Matrix cae a la fuente por defecto de raylib |
| `raylib.h` / `libraylib.a` | **Extra.** Copia de las de `native/effects/` (un nivel arriba) — para poder compilar `main000.c` sin ir a buscarlas a otra carpeta |

## Contrato JSON (`AsciiEffect_SetParams`)

```json
{ "effect": "ascii", "params": {
  "characters": " .:-=+*#%@",
  "fontSize": 12,
  "brightness": 1.0, "contrast": 1.0, "gamma": 1.0,
  "foreground": "#RRGGBB", "background": "#RRGGBBAA",
  "invert": false,
  "mode": "normal | matrix",
  "matrixChars": "...", "matrixDirection": "down | up | both",
  "matrixSpeed": 1.0, "matrixDensity": 1.0, "matrixTrailLength": 10,
  "matrixHeadColor": "#RRGGBB",
  "matrixReactive": false, "matrixReactiveStrength": 1.0,
  "matrixImageStrength": 1.0
} }
```

## Compilar la demo standalone

`raylib.h` / `libraylib.a` están un nivel arriba, en `native/effects/`
(compartidos por las 4 demos), de ahí el `-I..`/`-L..`:

```bash
# Windows (MinGW)
gcc main000.c -o ascii_demo.exe -I.. -L.. -lraylib -lgdi32 -lwinmm

# Linux
gcc main000.c -o ascii_demo -I.. -L.. -lraylib -lm -lpthread -ldl -lrt -lX11
```

Para que el modo Matrix use la fuente kana, `NotoSansJP-Kana.ttf` debe estar
en el mismo directorio desde el que se ejecuta el binario (ya está, junto a
`main000.c`, en esta carpeta).

## Build completo (WASM, dentro de Bryncraft)

Este header se compila como parte del build de `native/` (`make` desde
`native/`, ver `native/README.md`); no hace falta tocar nada aquí para eso.
La fuente Matrix se sirve vía `js_set_matrix_font_data()` desde
`public/assets/fonts/NotoSansJP-Kana.ttf` (copia idéntica a la de esta
carpeta, cargada por React, no por Emscripten `--preload-file`).
