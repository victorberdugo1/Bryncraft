# CRT + VHS

Post-procesado de pantalla de tubo catódico y artefactos de cinta VHS vía
fragment shader GLSL 100 (WebGL1/ES-compatible): scanlines animadas,
curvatura de barril, viñeta, ruido, aberración cromática, flicker, tracking
error, tape warp, dropout lines, jitter, vertical roll, ghosting — y un
overlay estilo reproductor VHS (PLAY/PAUSE/REW/FF + fecha/hora), todo
controlado desde un único `CRT_Params`.

> **A diferencia de `ascii`/`particles`/`opencv`, este es el único efecto
> de la familia que corre en un shader GLSL real** (`CRT_FS_SOURCE`,
> compilado en `CrtEffect_Init()` con `LoadShaderFromMemory`/`LoadShader`).
> Los demás procesan la imagen por CPU o con `raylib` puro; acá todo el
> trabajo pesado (scanlines, curvatura, ghosting, etc.) pasa por la GPU
> vía fragment shader. Esto importa para quien vaya a tocar el código:
> los parámetros de `CRT_Params` no son solo flags que la CPU lee, son
> uniforms que se suben cada frame con `SetShaderValue`/`SetShaderValueTexture`
> — si agregás un campo nuevo, tenés que declararlo en el GLSL, mapearlo
> con `GetShaderLocation` en `CrtEffect_Init()`, y subirlo en `CrtEffect_Draw()`.

## Archivos de esta carpeta

| Archivo | Qué es |
|---|---|
| `crt_effect.h` | Single-header: `CrtEffect_Init/SetParams/Update/Draw/Unload` + el GLSL embebido en `CRT_FS_SOURCE` |
| `main002.c` | Demo standalone mínima — solo este efecto, sin el resto de Bryncraft |

## Usar un `crt.fs` externo (editar el shader sin recompilar)

`CrtEffect_Init()` busca un archivo `crt.fs` al lado del ejecutable:

- **Si existe**, lo carga con `LoadShader(NULL, "crt.fs")`. Sirve para
  iterar el shader a mano (tocar el `.fs`, volver a correr el programa,
  ver el cambio) sin tener que recompilar el `.c`.
- **Si no existe**, usa el GLSL embebido en la cadena `CRT_FS_SOURCE`
  dentro de `crt_effect.h` (vía `LoadShaderFromMemory`).

Es literalmente el mismo código en los dos casos — cuando quieras "fijar"
tus cambios del `.fs` suelto, simplemente pegá su contenido de vuelta en
`CRT_FS_SOURCE` (respetando el escapado de `"` y `\n` de C) y volvés a
tener todo en un solo header.

A diferencia de `ascii`/`particles`/`opencv`, este efecto sí tiene shader
real: el tab "Extra" de la app no apunta a un `.fs` separado — el GLSL se
extrae en vivo de `CRT_FS_SOURCE`, así lo que se descarga es siempre
exactamente lo que se compila. Si vos ponés tu propio `crt.fs` al lado del
ejecutable en local, eso pisa el embebido solo en tu build nativo.

## Quick start (copy & paste)

```c
#include "raylib.h"
#include "crt_effect.h"

int main(void) {
    InitWindow(800, 600, "my game");
    SetTargetFPS(60);

    RenderTexture2D scene = LoadRenderTexture(800, 600);
    CrtEffect_Init(); // compila el shader (externo o embebido) — llamar una vez, después de InitWindow

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        BeginTextureMode(scene);
            ClearBackground(DARKGRAY);
            // ... tu dibujo normal va acá ...
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

## Parámetros

Todo se controla tocando la variable global `CRT_g_params` (struct
`CRT_Params`, declarada en `crt_effect.h`).

### CRT clásico

| Campo | Qué hace |
|---|---|
| `scanlineIntensity` | Fuerza de las líneas horizontales |
| `scanlineCount` | Cantidad de líneas |
| `scanlineSpeed` | Velocidad de desplazamiento de las scanlines |
| `curvature` | Curvatura de barril (deformación tipo pantalla convexa) |
| `vignette` | Oscurecido hacia los bordes |
| `noise` | Ruido general tipo estática |
| `chromaticAberration` | Separación de canales R/B (RGB split) |
| `flicker` | Parpadeo global de brillo |

### VHS — apagados (`0`) por defecto, se activan a gusto

| Campo | Efecto que simula | Qué hace |
|---|---|---|
| `trackingGlitch` | Tracking Error / Horizontal Sync Glitch | Banda horizontal que aparece y se corta/tiembla, como cuando el tracking de la cinta se desajusta |
| `waveDistortion` + `waveSpeed` | Tape Warp / Time Base Error | Onda horizontal suave que deforma la imagen |
| `dropoutLines` | Dropout Lines / Tape Noise | Rayas de ruido blanco intermitentes, como pérdida de señal en la cinta |
| `jitter` | Frame Jitter | Temblor horizontal global de toda la imagen |
| `verticalRoll` | Vertical Hold Error | La imagen "rueda" verticalmente sin parar (0 = quieto) |
| `ghosting` | Ghosting / Signal Echo | Arrastre/eco de los frames anteriores sobre el actual |

Efectos de la lista original que **no se agregaron** porque ya estaban
cubiertos o no sumaban lo suficiente para el costo: *Analog Blur/Tape
Blur* y *Generation Loss* (ya hay `noise` + `dropoutLines` dando esa
sensación de degradación), *Analog Compression Artifacts* (requiere
macrobloques, no aporta a este shader liviano) y *RF Noise/Chroma Delay*
(redundante con `chromaticAberration` + `noise`).

### Overlay VHS (icono + fecha/hora)

Esto se dibuja con `raylib` normal después del shader, no es parte del
GLSL:

| Campo | Qué hace |
|---|---|
| `vhsOverlay` | `true`/`false` — mostrar el overlay |
| `vhsIcon` | `CRT_VHS_ICON_PLAY` / `_PAUSE` / `_REW` / `_FF` / `_STOP` / `_REC` / `_NONE` |
| `vhsTimestamp` | Texto libre (hasta 31 chars), p.ej. `"JUL-15-1998  12:34 PM"` |
| `vhsLabel` | Texto corto (hasta 7 chars) al lado del icono, p.ej. `"SP"` / `"LP"` |

```c
CRT_g_params.vhsOverlay = true;
CRT_g_params.vhsIcon = CRT_VHS_ICON_PLAY;
strcpy(CRT_g_params.vhsLabel, "SP");
strcpy(CRT_g_params.vhsTimestamp, "JUL-15-1998  12:34 PM");
```

El icono parpadea suavemente (como el OSD de un VCR real) y el timestamp
queda fijo abajo a la izquierda con una sombra atrás para que se lea sobre
cualquier fondo. Si querés la fecha/hora real del sistema, armá el string
vos mismo (con `time.h`/`strftime`, o lo que uses) y copialo a
`vhsTimestamp` cada tanto — no hace falta cada frame, un par de veces por
segundo alcanza.

## Combinaciones típicas

```c
// CRT limpio de toda la vida
CRT_g_params.scanlineIntensity = 0.35f;
CRT_g_params.curvature = 0.15f;
CRT_g_params.vignette = 0.3f;

// Cinta VHS vieja y maltratada
CRT_g_params.trackingGlitch = 0.6f;
CRT_g_params.waveDistortion = 0.5f;
CRT_g_params.dropoutLines = 0.4f;
CRT_g_params.jitter = 0.3f;
CRT_g_params.ghosting = 0.35f;
CRT_g_params.vhsOverlay = true;
CRT_g_params.vhsIcon = CRT_VHS_ICON_PLAY;
strcpy(CRT_g_params.vhsLabel, "SP");
strcpy(CRT_g_params.vhsTimestamp, "JUL-15-1998  12:34 PM");

// Cinta en rebobinado
CRT_g_params.vhsIcon = CRT_VHS_ICON_REW;
CRT_g_params.trackingGlitch = 0.8f;
CRT_g_params.jitter = 0.6f;
```

## Contrato JSON (`CrtEffect_SetParams`)

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

## Compilar la demo standalone

`raylib.h` / `libraylib.a` están un nivel arriba, en `native/effects/`
(compartidos por las 4 demos):

```bash
gcc -o crt_demo.exe main002.c -I.. -L.. -lraylib -lgdi32 -lwinmm   # Windows
gcc -o crt_demo main002.c -I.. -L.. -lraylib -lm -lpthread -ldl -lrt -lX11   # Linux
```

## Build completo (WASM, dentro de Bryncraft)

Se compila como parte del build de `native/` (`make` desde `native/`, ver
`native/README.md`); no hace falta tocar nada aquí para eso.
