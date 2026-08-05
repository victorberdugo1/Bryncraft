# OpenCV Vision

Corre entero en CPU vía OpenCV — sin fragment shader. Cinco modos: `edges`
(Canny), `contours`, `optical_flow`, `bg_subtract` (sustracción de fondo) y
`face_detect` (cascada Haar). Sobre escritorio real, además puede tomar la
cámara del sistema directamente (`OcvCamera_Open`/`OcvCamera_CaptureInto`)
en vez de la escena renderizada.

## Archivos de esta carpeta

| Archivo | Qué es |
|---|---|
| `opencv_effect.h` | Single-header estilo stb_image.h: interfaz plana en C arriba (lo que parsea `main.c`), pipeline real de OpenCV en C++ debajo de `OPENCV_EFFECT_IMPLEMENTATION` |
| `main003.c` | Demo standalone mínima — solo este efecto, sin el resto de Bryncraft |
| `opencv_build_and_run.sh` | **Extra.** Compila y corre la demo en Linux/macOS (requiere `libopencv-dev` vía pkg-config) |
| `opencv_build_and_run.bat` | **Extra.** Descarga OpenCV-MinGW prebuildeado si hace falta, compila y corre la demo en Windows |

A diferencia de `ascii`/`particles`/`crt`, `opencv_effect.h` no tiene
compañero `.cpp`: se compila apuntando un compilador C++ directamente a este
header (`-x c++`), porque OpenCV no tiene API en C puro.

## Contrato JSON (`OpencvEffect_SetParams`)

```json
{ "effect": "opencv", "params": {
  "mode": "edges | contours | optical_flow | bg_subtract | face_detect",
  "processScale": 0.5, "mirror": false,

  "cannyLow": 50.0, "cannyHigh": 150.0, "blur": 3,
  "edgeOnSource": true, "edgeColor": "#RRGGBB",

  "contourMinArea": 100.0, "contourThickness": 2, "contourFill": false,
  "contourColor": "#RRGGBB",

  "flowStrength": 1.0, "flowArrows": true, "flowArrowStep": 16,

  "bgHistory": 500, "bgVarThreshold": 16.0,
  "bgShadows": true, "bgMaskOnly": false,

  "faceScaleFactor": 1.1, "faceMinNeighbors": 5,
  "faceMinSizeFraction": 0.1, "faceBoxColor": "#RRGGBB",
  "faceShowCount": true
} }
```

## Compilar la demo standalone

`raylib.h` / `libraylib.a` están un nivel arriba, en `native/effects/`
(compartidos por las 4 demos). Lo más simple es usar el script incluido:

```bash
# Linux/macOS — requiere: sudo apt install libopencv-dev (o equivalente)
./opencv_build_and_run.sh
```

```bat
:: Windows — descarga OpenCV-MinGW la primera vez, luego compila con MinGW
opencv_build_and_run.bat
```

Ambos scripts asumen que se ejecutan **desde esta carpeta** (usan `-I..`/
`-L..` para encontrar `raylib.h`/`libraylib.a`, y compilan `main003.c` /
`opencv_effect.h` que están junto a ellos).

Manual, equivalente al script de Linux/macOS:

```bash
gcc  -c main003.c -o main003.o -I..
g++  -DOPENCV_EFFECT_IMPLEMENTATION -x c++ -c opencv_effect.h -o opencv_effect.o -I.. -std=c++20 $(pkg-config --cflags opencv4)
g++  main003.o opencv_effect.o -o opencv_demo -L.. -lraylib -lm -lpthread -ldl -lrt -lX11 $(pkg-config --libs opencv4)
```

## Build completo (WASM, dentro de Bryncraft)

Se compila como parte del build de `native/` (`make` desde `native/`, ver
`native/README.md`). La build WASM usa un OpenCV recortado (solo
core/imgproc/video/objdetect — sin `videoio`, no hay cámara del lado
Emscripten); la captura de cámara real (`OcvCamera_*`) solo existe fuera de
`__EMSCRIPTEN__`, o sea únicamente en esta demo standalone.
