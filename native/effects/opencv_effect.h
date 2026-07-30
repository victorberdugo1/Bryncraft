/*
 * opencv_effect.h — bridge between main.c (plain C) and the OpenCV-powered
 * effect implementation (native/opencv_bridge.cpp, which is compiled as C++
 * because OpenCV has no C API).
 *
 * This header intentionally follows the SAME three-entry-point contract
 * described in effect_common.h (SetParams / Update / Draw) so main.c's
 * dispatch switch treats EFFECT_OPENCV exactly like ascii/particles/crt —
 * it just declares them here instead of implementing them inline, since a
 * single-header C implementation (like ascii_effect.h) isn't possible once
 * OpenCV is involved.
 *
 * NOTHING in this header includes <opencv2/...> or uses C++ types — it is
 * parsed by main.c (a .c file) exactly like every other effects/*.h, so it
 * must stay plain C. All OpenCV usage is confined to opencv_bridge.cpp.
 */
#ifndef OPENCV_EFFECT_H
#define OPENCV_EFFECT_H

#include "raylib.h"
#ifdef __EMSCRIPTEN__
#include "../json_mini.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __EMSCRIPTEN__
// paramsObj carries at least {"mode": "edges"|"contours"|"optical_flow"|
// "bg_subtract"|"face_detect", ...mode-specific fields}. See
// native/opencv_bridge.cpp OpencvParamsFromJson() for the full field list —
// kept in sync with src/types/effects.ts's OPENCV_EFFECT param schema.
void OpencvEffect_SetParams(const JsonValue *paramsObj);
#endif

// Runs the currently-selected OpenCV pipeline against whatever is in
// `scene` (the same RenderTexture2D every other effect reads from — the
// decoded video/camera frame, or DrawBaseScene()'s procedural placeholder
// when no source is loaded) and uploads the processed pixels to an
// internal texture for Draw() to blit. Cheap effects (edges/contours) run
// every call; face detection throttles itself internally (see
// OPENCV_FACE_DETECT_EVERY_N_FRAMES) since Haar cascades are comparatively
// expensive per frame.
void OpencvEffect_Update(float dt);
void OpencvEffect_Draw(RenderTexture2D scene, int screenW, int screenH);

// Releases the OpenCV-side Mats/state and the raylib textures this effect
// owns. Safe to call even if OpencvEffect_Update() was never called (e.g.
// the user never switched to this effect during the session).
void OpencvEffect_Unload(void);

#ifdef __cplusplus
}
#endif

#endif // OPENCV_EFFECT_H
