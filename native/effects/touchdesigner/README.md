# TouchDesigner Hand Tracker

Detección de manos en vivo (OpenCV, todo en C/C++ — sin JS/modelos
externos salvo la carga inicial del `.onnx`): la cámara se ve completa,
sin negro, y cada mano detectada se cubre con un blob de vidrio líquido;
si acercás las dos manos, un puente de slime las conecta.

Hay dos estilos independientes, cada uno seleccionable por dropdown:

| Param | Dónde se aplica |
|---|---|
| `handStyle` | Adentro del vidrio/slime de las manos |
| `bgStyle` | El fondo de cámara, afuera del vidrio/slime |

Opciones para ambos: `none` (passthrough, sin filtro) / `ascii` / `matrix`
/ `crt` / `edges`.

Estos 4 estilos **no se reimplementan acá**: `touchdesigner_effect.h` los
reutiliza tal cual llamando a `AsciiEffect_Draw`/`AsciiEffect_SetMode`
(`ascii_effect.h`), `CrtEffect_Draw` (`crt_effect.h`) y
`OpencvEffect_Draw`/`OpencvEffect_SetEdgesMode` (`opencv_effect.h`) — solo
declara esos símbolos como `extern`, no incluye esos headers. Para que
compilen juntos hace falta que los tres `.h` estén presentes en el build
(ver "Files in this folder" abajo).

## Dónde vive la detección de manos

**Todo en un solo archivo: `touchdesigner_effect.h`.** OpenCV (`cv::dnn`),
sin JS de por medio salvo la carga inicial del modelo ONNX
(`src/utils/palmModelLoader.ts`). El flujo:

1. La cámara/video llega como siempre a `scene` vía `js_set_video_frame`
   (`native/main.c`, alimentado por `wasmBridge.pushCameraFrame` cada rAF).
2. `TouchdesignerEffect_Draw()` llama a `TD_DetectHands(scene, ...)`
   cada `handDetectSkip` frames: reescala `scene` a baja resolución
   (`handProcessScale`), la baja a CPU con `LoadImageFromTexture` (mismo
   patrón que `RunFaceDetect()` en `opencv_effect.h`) y se la pasa a
   `TD_HandTracking_ProcessFrame()` — detector de palma SSD (MP-PalmDet),
   implementado en la sección `TOUCHDESIGNER_EFFECT_IMPLEMENTATION` al
   final de este mismo `touchdesigner_effect.h` (C++, compilada aparte con
   em++, ver `TOUCHDESIGNER_OBJ` en `native/Makefile`): hasta 2 manos con
   posición, tamaño de palma y orientación aproximada.
3. `TD_DetectHands` lee `TD_HandTracking_GetHandCount/GetHand` y llena
   `TD_g_hands[0..1]` directamente — sin pasar por ningún canal externo.
4. `TouchdesignerEffect_Draw` calcula `bgScene = TD_ApplyStyle(bgStyle,
   effScene)` y la dibuja de fondo; calcula `handScene =
   TD_ApplyStyle(handStyle, effScene)` y esa es la textura que
   `TD_CaptureLensRegion`/`TD_CaptureBridgeLensRegion` recortan para
   mostrar adentro del vidrio líquido / puente de slime de cada mano.
   `TD_ApplyStyle` cachea el resultado por frame (si `handStyle ==
   bgStyle` no vuelve a renderizar el mismo estilo dos veces).

`TouchdesignerEffect_SetHandData()` sigue existiendo como override manual
(usado por el demo `main.c` con el mouse) para cuando `autoDetectHands` es
`false` — la app real nunca la llama, `autoDetectHands` es `true` por
defecto.

## Files in this folder

| File | What it is |
|---|---|
| `touchdesigner_effect.h` | Single-header: arriba `TouchdesignerEffect_Init/SetParams/Update/Draw/Unload` + `TouchdesignerEffect_SetHandData` + `TD_DetectHands` + `TD_ApplyStyle` (C puro, compilado dentro de `main.c`); al final, bajo el guard `TOUCHDESIGNER_EFFECT_IMPLEMENTATION`, el detector de palma en C++ (`cv::dnn::Net`), compilado aparte como su propio `.o` |
| `main.c` | Demo standalone con `autoDetectHands = false` — simula una mano con el mouse (click izq = pinch, click der = fist); teclas 1-5 cambian `handStyle`, 6-0 cambian `bgStyle`. Incluye `ascii_effect.h`/`crt_effect.h`/`opencv_effect.h` directamente (paths planos: `"ascii_effect.h"`, no `"../ascii/ascii_effect.h"`) para funcionar igual desde el repo o desde una carpeta de descarga plana |
| `touchdesigner_build_and_run.sh` | Builda y corre el demo en Linux/macOS (requiere `libopencv-dev` >= 4.7, por `cv::dnn::blobFromImageWithParams`) |
| `touchdesigner_build_and_run.bat` | Descarga un OpenCV-MinGW prearmado si hace falta, builda y corre el demo en Windows |

Además de estos, para que el ejecutable compile hace falta bajar (tab
"Extra" en la app) los `.h` de los estilos que quieras usar en
`handStyle`/`bgStyle`: `ascii_effect.h` (cubre `ascii` y `matrix`),
`crt_effect.h` (`crt`) y `opencv_effect.h` (`edges`) — y el modelo
`palm_detection_mediapipe_2023feb.onnx`, obligatorio siempre (es el
detector de manos). Los build scripts ya asumen que los tres `.h` están
presentes.

> **Igual que `opencv_effect.h`, este no es un `gcc` plano.** La sección
> `TOUCHDESIGNER_EFFECT_IMPLEMENTATION` necesita un compilador de C++
> apuntando al header (`-x c++`) y linkear contra OpenCV (`core`,
> `imgproc`, `videoio`, `dnn`, `video`) — de ahí los scripts de build en
> vez de un `gcc` de una línea. `opencv_effect.h` (necesario si usás el
> estilo `edges`) también se compila aparte, igual que en
> `native/Makefile` (`OPENCV_OBJ`).

## Params (`TouchdesignerEffect_SetParams`)

| Param | Qué hace |
|---|---|
| `handStyle` | `"none"` / `"ascii"` / `"matrix"` / `"crt"` / `"edges"` — estilo adentro del vidrio/slime de las manos |
| `bgStyle` | Igual, pero para el fondo de cámara |
| `mirror` | Espejo horizontal antes de detectar manos (cámara frontal = selfie) |
| `handReanchorInterval`, `handProcessScale`, `handDetectSkip` | Ajustan costo/latencia de la detección |
| `glassColor`, `glassEnabled`, `glassSize` | Vidrio líquido sobre cada mano |
| `slimeEnabled`, `slimeDistance` | Puente de slime entre las dos manos |
| `showHandCount` | Overlay con la cantidad de manos detectadas |
| `showCameraBg`, `bgFallbackColor` | Fondo: cámara (con el `bgStyle` elegido) o color liso |

Part of [Bryncraft](https://bryncraft.online/) — created by Victor Berdugo.
