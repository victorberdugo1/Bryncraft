# TouchDesigner Hand Portal

Portal de "liquid glass" cuyo borde sigue tus manos — estética
TouchDesigner/Motion-Lab hecha a medida para Bryncraft (no un remix de
ningún proyecto ajeno): donde el blob se solapa con la imagen se ve el
efecto elegido (`ascii`, `matrix` o `crt` — los mismos de siempre,
reutilizados tal cual, nada reimplementado); afuera, la cámara atenuada (o
negro liso). Pinch encoge el portal, fist lo cierra casi del todo. Dos
manos se fusionan en un solo blob (metaball / smooth-min entre sus dos SDFs
de círculo).

## Dónde vive la detección de manos

**Todo en un solo archivo: `touchdesigner_effect.h`.** OpenCV (`cv::dnn`),
sin JS de por medio salvo la carga inicial del modelo ONNX
(`src/utils/palmModelLoader.ts`). El flujo:

1. La cámara/video llega como siempre a `scene` vía `js_set_video_frame`
   (`native/main.c`, alimentado por `wasmBridge.pushCameraFrame` cada rAF).
2. `TouchdesignerEffect_Draw()` llama a `TD_DetectHands(scene, ...)` cada 2
   frames: reescala `scene` a baja resolución (`handProcessScale`, 0.4 por
   defecto), la baja a CPU con `LoadImageFromTexture` (mismo patrón que
   `RunFaceDetect()` en `opencv_effect.h`) y se la pasa a
   `TD_HandTracking_ProcessFrame()` — detector de palma SSD (MP-PalmDet),
   implementado en la sección `TOUCHDESIGNER_EFFECT_IMPLEMENTATION` al
   final de este mismo `touchdesigner_effect.h` (C++, compilada aparte con
   em++, ver `TOUCHDESIGNER_OBJ` en `native/Makefile`): hasta 2 manos con
   posición, tamaño de palma y orientación aproximada.
3. `TD_DetectHands` lee `TD_HandTracking_GetHandCount/GetHand`, ordena las
   manos de izquierda a derecha (el detector no garantiza orden estable
   entre frames) y llena `TD_g_hands[0..1]` directamente — sin pasar por
   ningún canal externo.
4. El shader hace el resto cada `Draw()`, igual que antes.

`TouchdesignerEffect_SetHandData()` sigue existiendo como override manual
(usado por el demo `main.c` con el mouse) para cuando `autoDetectHands` es
`false` — la app real nunca la llama, `autoDetectHands` es `true` por
defecto.

## Files in this folder

| File | What it is |
|---|---|
| `touchdesigner_effect.h` | Single-header, un solo archivo para todo: arriba `TouchdesignerEffect_Init/SetParams/Update/Draw/Unload` + `TouchdesignerEffect_SetHandData` + `TD_DetectHands` + el shader de portal embebido (C puro, compilado dentro de `main.c`); al final, bajo el guard `TOUCHDESIGNER_EFFECT_IMPLEMENTATION`, el detector de palma en C++ (`cv::dnn::Net`), compilado aparte como su propio `.o` |
| `main.c` | Demo standalone con `autoDetectHands = false` — simula una mano con el mouse (click izq = pinch, click der = fist) para iterar el shader sin cámara |

## Cómo compone el frame (`TouchdesignerEffect_Draw`)

1. Renderiza el efecto interno elegido (`ascii`/`matrix` vía
   `AsciiEffect_Draw`, o `crt` vía `CrtEffect_Draw`) a una textura propia
   — llamando a las funciones de siempre, no hay copia/fork del código.
2. Renderiza la cámara atenuada (o negro) a otra textura, para tener algo
   fuera del portal.
3. Un solo shader (`TD_FS_SOURCE`, GLSL 100 / WebGL1-ES, mismo estilo que
   `crt_effect.h`) mezcla ambas texturas según un SDF de círculo(s) por
   mano — `smin()` (smooth-min) fusiona las dos manos en un blob, un rim
   glow marca el borde, y una distorsión barata (offset de UV a lo largo
   del gradiente del SDF) da la sensación de vidrio/refracción.

`ASCII_g_params.mode` se pisa cada frame según `innerEffect` — este efecto
comparte el estado global de `ascii_effect.h` (mismo patrón que main.c usa
para todos los efectos), así que si volvés al efecto `ascii` standalone
puede que herede el `mode` que dejó `touchdesigner`.

## Params (`TouchdesignerEffect_SetParams`)

| Param | Qué hace |
|---|---|
| `innerEffect` | `"ascii"` / `"matrix"` / `"crt"` — qué se ve dentro del portal |
| `radiusBase` | Radio del blob a mano abierta, fracción del alto de pantalla |
| `radiusPinchScale` / `radiusFistScale` | Cuánto se encoge el radio con pinch=1 / fist=1 |
| `blendK` | Qué tan "gomosa" es la fusión entre las dos manos |
| `edgeWidth`, `glowColor`, `glowIntensity` | Rim glow del borde |
| `refractionStrength` | Distorsión de vidrio cerca del borde |
| `showCameraBg`, `cameraBgDim` | Fondo fuera del portal: cámara atenuada o negro liso |
| `idlePulse` | Si no hay ninguna mano, deja un blob fantasma pulsante en el centro (0 = nada) |
| `mirror` | Espejo horizontal antes de detectar manos (cámara frontal = selfie) |

Part of [Bryncraft](https://bryncraft.online/) — created by Victor Berdugo.
