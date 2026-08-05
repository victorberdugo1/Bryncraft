# CRT

Post-procesado de pantalla de tubo catódico vía fragment shader GLSL 100
(WebGL1/ES-compatible): scanlines animadas, curvatura de barril, viñeta,
ruido, aberración cromática y flicker — todo controlado por uniforms desde
un único `CRT_Params`.

## Archivos de esta carpeta

| Archivo | Qué es |
|---|---|
| `crt_effect.h` | Single-header: `CrtEffect_Init/SetParams/Update/Draw/Unload` + el GLSL embebido en `CRT_FS_SOURCE` |
| `main002.c` | Demo standalone mínima — solo este efecto, sin el resto de Bryncraft |

**Extra:** a diferencia de `ascii`/`particles`/`opencv`, este efecto sí
tiene shader real. El tab "Extra" de la app no apunta a un archivo `.fs`
separado — el GLSL se extrae en vivo de la cadena `CRT_FS_SOURCE` dentro de
`crt_effect.h`, así lo que se descarga es siempre exactamente lo que se
compila, nunca una copia mantenida a mano aparte.

## Contrato JSON (`CrtEffect_SetParams`)

```json
{ "effect": "crt", "params": {
  "scanlineIntensity": 0.3, "scanlineCount": 240.0, "scanlineSpeed": 1.0,
  "curvature": 0.1, "vignette": 0.3, "noise": 0.05,
  "chromaticAberration": 0.002, "flicker": 0.02
} }
```

## Compilar la demo standalone

`raylib.h` / `libraylib.a` están un nivel arriba, en `native/effects/`
(compartidos por las 4 demos):

```bash
gcc -o crt_demo.exe main002.c -I.. -L.. -lraylib -lgdi32 -lwinmm   # Windows
```

## Build completo (WASM, dentro de Bryncraft)

Se compila como parte del build de `native/` (`make` desde `native/`, ver
`native/README.md`); no hace falta tocar nada aquí para eso.
