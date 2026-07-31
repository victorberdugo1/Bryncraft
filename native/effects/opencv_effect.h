/*
 * opencv_effect.h -- bridge between main.c (plain C) and the OpenCV-powered
 * effect implementation, kept in the SAME file (single-header style, same
 * idiom as stb_image.h): the top half is the plain-C declarations main.c
 * parses; everything below OPENCV_EFFECT_IMPLEMENTATION is real OpenCV C++
 * code that main.c never sees.
 *
 * This header intentionally follows the SAME three-entry-point contract
 * described in effect_common.h (SetParams / Update / Draw) so main.c's
 * dispatch switch treats EFFECT_OPENCV exactly like ascii/particles/crt.
 *
 * NOTHING above the OPENCV_EFFECT_IMPLEMENTATION guard includes <opencv2/...>
 * or uses C++ types -- that part is parsed by main.c (a .c file) exactly
 * like every other effects/*.h, so it must stay plain C.
 *
 * There is no companion .cpp for the implementation below. It's compiled
 * by pointing a C++ compiler straight at THIS file (see the OPENCV_OBJ rule
 * in the Makefile):
 *     em++ -DOPENCV_EFFECT_IMPLEMENTATION -x c++ -c effects/opencv_effect.h -o opencv_effect.o
 * `-x c++` forces the C++ frontend regardless of the .h extension, and the
 * -D flag unlocks the block below instead of just the plain-C declarations
 * main.c sees. That command line is the ONLY reason a C++ compiler is
 * invoked at all -- OpenCV has no C API, so *some* translation unit has to
 * be C++ -- but there's nothing left to maintain outside this header: every
 * line of the real effect lives here now.
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
// OpencvEffect_SetParams() further down in this same file (implementation
// section) for the full field list — kept in sync with
// src/types/effects.ts's OPENCV_EFFECT param schema.
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

#ifndef __EMSCRIPTEN__
// ----------------------------------------------------------------------
// Native camera capture (desktop builds only, e.g. main003.c). The web
// build gets its frames from the browser (getUserMedia) via JavaScript ->
// js_set_video_frame() in main.c instead, so none of this compiles under
// Emscripten -- there's no cv::VideoCapture in the trimmed wasm OpenCV
// build (core/imgproc/video/objdetect only, no videoio) and no camera to
// open from inside a wasm sandbox anyway.
// ----------------------------------------------------------------------

// Opens system camera `deviceIndex` (0 = default camera). Returns true on
// success; false if no camera could be opened. Safe to call again after a
// failed attempt or after OcvCamera_Close().
bool OcvCamera_Open(int deviceIndex);

// True once OcvCamera_Open() has succeeded and the camera hasn't been
// closed since.
bool OcvCamera_IsOpen(void);

// Grabs the next camera frame and draws it into `target` (a RenderTexture2D
// -- the same `scene` OpencvEffect_Draw reads from), stretched to fill it.
// Safe to call every frame even if the camera isn't open or a grab fails:
// `target` is simply left with whatever it already had in that case, so
// callers don't need to check OcvCamera_IsOpen() first.
void OcvCamera_CaptureInto(RenderTexture2D target);

// Releases the camera and the internal texture used to stage its frames.
// Safe to call even if OcvCamera_Open() was never called or already failed.
void OcvCamera_Close(void);
#endif

#ifdef __cplusplus
}
#endif

#endif // OPENCV_EFFECT_H

/* ============================================================================
 * IMPLEMENTATION -- only compiled when OPENCV_EFFECT_IMPLEMENTATION is
 * defined on the compiler command line (see the OPENCV_OBJ rule in the
 * Makefile, which compiles this exact file with `-x c++ -DOPENCV_EFFECT_IMPLEMENTATION`).
 * main.c never defines that macro, so it never sees anything past this
 * point -- not the OpenCV includes, not OcvParams, none of it.
 *
 * Pipeline shared by every mode:
 *   1. Downsample `scene` (the RenderTexture2D every effect reads from --
 *      decoded video/camera frame, or DrawBaseScene()'s procedural
 *      placeholder) into g_readTarget at g_params.processScale resolution.
 *      Working at less than full resolution is what keeps Canny/contours/
 *      optical flow/Haar cascades inside frame budget on a single wasm
 *      thread -- this is a deliberate quality/perf tradeoff exposed to the
 *      user as the "processScale" param, not an oversight.
 *   2. LoadImageFromTexture() it back to CPU (same technique
 *      ascii_effect.h/particles_effect.h already use), wrap as a cv::Mat,
 *      flip it right-side-up once (RenderTexture2D's GPU texture is
 *      stored upside-down -- see the long comment in OpencvEffect_Draw).
 *   3. Run whichever OpenCV pipeline is selected.
 *   4. Upload the RGBA result to a plain Texture2D (NOT a RenderTexture --
 *      so no upside-down storage this time) and blit it to the backbuffer.
 * ========================================================================== */
#if defined(OPENCV_EFFECT_IMPLEMENTATION) && !defined(OPENCV_EFFECT_IMPLEMENTATION_INCLUDED)
#define OPENCV_EFFECT_IMPLEMENTATION_INCLUDED

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>
#include <opencv2/objdetect.hpp>

#include <vector>
#include <cstring>
#include <cmath>
#include <algorithm>

// ============================================================================
// PARAMS
// ============================================================================

enum OcvMode {
    OCV_MODE_EDGES = 0,
    OCV_MODE_CONTOURS,
    OCV_MODE_FLOW,
    OCV_MODE_BG,
    OCV_MODE_FACE,
};

// Plain aggregate — SIN inicializadores por-campo dentro de la declaración
// (eso era C++-only y es justo lo que impedía a generateRaylibCode.ts
// sustituir los valores actuales del Inspector: no había un único bloque
// inicializador equivalente al de ASCII_g_params / CRT_g_params /
// PART_g_params que localizar por regex). Los defaults ahora viven en el
// bloque de abajo (ver g_params más adelante), con el mismo formato que
// esos otros tres — así este archivo se comporta igual que sus .h
// hermanos: main.c nunca ve esta diferencia (sigue incluyendo solo
// effects/opencv_effect.h), pero el panel "Code" ya refleja los valores
// reales del Inspector.
struct OcvParams {
    OcvMode mode;
    float processScale;   // internal working resolution, relative to the canvas
    bool mirror;          // horizontal flip — handy with a front-facing camera

    // edges
    float cannyLow, cannyHigh;
    int blur;
    bool edgeOnSource;
    Color edgeColor;

    // contours
    float contourMinArea;
    int contourThickness;
    bool contourFill;
    Color contourColor;

    // optical flow
    float flowStrength;
    bool flowArrows;
    int flowArrowStep;

    // background subtraction
    int bgHistory;
    float bgVarThreshold;
    bool bgShadows;
    bool bgMaskOnly;

    // face detection
    float faceScaleFactor;
    int faceMinNeighbors;
    float faceMinSizeFraction;
    Color faceBoxColor;
    bool faceShowCount;
};

static OcvParams g_params = {
    .mode = OCV_MODE_EDGES,
    .processScale = 0.5f,
    .mirror = false,

    .cannyLow = 60.0f,
    .cannyHigh = 160.0f,
    .blur = 1,
    .edgeOnSource = false,
    .edgeColor = (Color){ 68, 212, 255, 255 },

    .contourMinArea = 80.0f,
    .contourThickness = 2,
    .contourFill = false,
    .contourColor = (Color){ 68, 212, 255, 255 },

    .flowStrength = 1.0f,
    .flowArrows = false,
    .flowArrowStep = 16,

    .bgHistory = 120,
    .bgVarThreshold = 16.0f,
    .bgShadows = true,
    .bgMaskOnly = false,

    .faceScaleFactor = 1.1f,
    .faceMinNeighbors = 4,
    .faceMinSizeFraction = 0.08f,
    .faceBoxColor = (Color){ 120, 255, 120, 255 },
    .faceShowCount = true,
};
static OcvMode g_lastMode = OCV_MODE_EDGES;

// ============================================================================
// INTERNAL STATE
// ============================================================================

static RenderTexture2D g_readTarget;
static bool g_readTargetReady = false;
static int g_readW = 0, g_readH = 0;

static Texture2D g_outputTexture;
static bool g_outputTextureReady = false;
static int g_outW = 0, g_outH = 0;

static cv::Mat g_prevGray;                          // optical flow
static cv::Mat g_lastFlowVis;                        // reused between throttled recomputes
static cv::Ptr<cv::BackgroundSubtractorMOG2> g_bgSub; // background subtraction
static int g_bgHistoryBuilt = -1;
static float g_bgVarThresholdBuilt = -1.0f;
static bool g_bgShadowsBuilt = false;

static cv::CascadeClassifier g_faceCascade;
static bool g_faceCascadeAttempted = false;
static bool g_faceCascadeOk = false;
static std::vector<cv::Rect> g_lastFaces;

// Cascade data passed from JavaScript via js_set_cascade_data()
static uint8_t *g_cascadeBuffer = NULL;
static size_t g_cascadeBufferSize = 0;

static int g_frameCounter = 0;

// ============================================================================
// PARSING HELPERS (same conventions as ascii_effect.h / particles_effect.h)
// ============================================================================

static Color OCV_HexToColor(const char *hex, Color fallback) {
    if (!hex || hex[0] != '#') return fallback;
    size_t len = strlen(hex);
    unsigned int r, g, b, a;
    if (len >= 9) {
        if (sscanf(hex + 1, "%02x%02x%02x%02x", &r, &g, &b, &a) != 4) return fallback;
        return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
    }
    if (len >= 7) {
        if (sscanf(hex + 1, "%02x%02x%02x", &r, &g, &b) != 3) return fallback;
        return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
    }
    return fallback;
}

static OcvMode OcvModeFromString(const char *s, OcvMode fallback) {
    if (!s) return fallback;
    if (strcmp(s, "edges") == 0) return OCV_MODE_EDGES;
    if (strcmp(s, "contours") == 0) return OCV_MODE_CONTOURS;
    if (strcmp(s, "optical_flow") == 0) return OCV_MODE_FLOW;
    if (strcmp(s, "bg_subtract") == 0) return OCV_MODE_BG;
    if (strcmp(s, "face_detect") == 0) return OCV_MODE_FACE;
    return fallback;
}

void OpencvEffect_SetParams(const JsonValue *paramsObj) {
    if (!paramsObj) return;

    g_params.mode = OcvModeFromString(JsonAsString(JsonObjectGet(paramsObj, "mode"), NULL), g_params.mode);
    g_params.processScale = (float)JsonAsNumber(JsonObjectGet(paramsObj, "processScale"), g_params.processScale);
    g_params.mirror = JsonAsBool(JsonObjectGet(paramsObj, "mirror"), g_params.mirror);

    g_params.cannyLow = (float)JsonAsNumber(JsonObjectGet(paramsObj, "cannyLow"), g_params.cannyLow);
    g_params.cannyHigh = (float)JsonAsNumber(JsonObjectGet(paramsObj, "cannyHigh"), g_params.cannyHigh);
    g_params.blur = (int)JsonAsNumber(JsonObjectGet(paramsObj, "blur"), g_params.blur);
    g_params.edgeOnSource = JsonAsBool(JsonObjectGet(paramsObj, "edgeOnSource"), g_params.edgeOnSource);
    g_params.edgeColor = OCV_HexToColor(JsonAsString(JsonObjectGet(paramsObj, "edgeColor"), NULL), g_params.edgeColor);

    g_params.contourMinArea = (float)JsonAsNumber(JsonObjectGet(paramsObj, "contourMinArea"), g_params.contourMinArea);
    g_params.contourThickness = (int)JsonAsNumber(JsonObjectGet(paramsObj, "contourThickness"), g_params.contourThickness);
    g_params.contourFill = JsonAsBool(JsonObjectGet(paramsObj, "contourFill"), g_params.contourFill);
    g_params.contourColor = OCV_HexToColor(JsonAsString(JsonObjectGet(paramsObj, "contourColor"), NULL), g_params.contourColor);

    g_params.flowStrength = (float)JsonAsNumber(JsonObjectGet(paramsObj, "flowStrength"), g_params.flowStrength);
    g_params.flowArrows = JsonAsBool(JsonObjectGet(paramsObj, "flowArrows"), g_params.flowArrows);
    g_params.flowArrowStep = (int)JsonAsNumber(JsonObjectGet(paramsObj, "flowArrowStep"), g_params.flowArrowStep);

    g_params.bgHistory = (int)JsonAsNumber(JsonObjectGet(paramsObj, "bgHistory"), g_params.bgHistory);
    g_params.bgVarThreshold = (float)JsonAsNumber(JsonObjectGet(paramsObj, "bgVarThreshold"), g_params.bgVarThreshold);
    g_params.bgShadows = JsonAsBool(JsonObjectGet(paramsObj, "bgShadows"), g_params.bgShadows);
    g_params.bgMaskOnly = JsonAsBool(JsonObjectGet(paramsObj, "bgMaskOnly"), g_params.bgMaskOnly);

    g_params.faceScaleFactor = (float)JsonAsNumber(JsonObjectGet(paramsObj, "faceScaleFactor"), g_params.faceScaleFactor);
    g_params.faceMinNeighbors = (int)JsonAsNumber(JsonObjectGet(paramsObj, "faceMinNeighbors"), g_params.faceMinNeighbors);
    g_params.faceMinSizeFraction = (float)JsonAsNumber(JsonObjectGet(paramsObj, "faceMinSizeFraction"), g_params.faceMinSizeFraction);
    g_params.faceBoxColor = OCV_HexToColor(JsonAsString(JsonObjectGet(paramsObj, "faceBoxColor"), NULL), g_params.faceBoxColor);
    g_params.faceShowCount = JsonAsBool(JsonObjectGet(paramsObj, "faceShowCount"), g_params.faceShowCount);

    // Switching modes invalidates state that only makes sense for the
    // previous pipeline (a stale optical-flow reference frame, a
    // background model built from a completely different scene, cached
    // face boxes from before). Dropping it avoids visible glitches on
    // mode switch and matches what a fresh start of each pipeline expects.
    if (g_params.mode != g_lastMode) {
        g_prevGray.release();
        g_lastFlowVis.release();
        g_bgSub.release();
        g_bgHistoryBuilt = -1;
        g_lastFaces.clear();
        g_lastMode = g_params.mode;
    }
}

void OpencvEffect_Update(float dt) {
    (void)dt;
}

// ============================================================================
// PIPELINES — each takes the working-resolution RGBA frame and returns an
// RGBA cv::Mat of the same size to upload/display.
// ============================================================================

static cv::Mat RunEdges(const cv::Mat &frame) {
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_RGBA2GRAY);
    if (g_params.blur > 0) {
        int k = g_params.blur * 2 + 1;
        cv::GaussianBlur(gray, gray, cv::Size(k, k), 0);
    }
    cv::Mat edges;
    cv::Canny(gray, edges, g_params.cannyLow, g_params.cannyHigh);

    cv::Mat out;
    if (g_params.edgeOnSource) {
        out = frame.clone();
    } else {
        out = cv::Mat::zeros(frame.size(), CV_8UC4);
    }
    cv::Scalar col(g_params.edgeColor.r, g_params.edgeColor.g, g_params.edgeColor.b, 255);
    out.setTo(col, edges);
    return out;
}

static cv::Mat RunContours(const cv::Mat &frame) {
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_RGBA2GRAY);
    if (g_params.blur > 0) {
        int k = g_params.blur * 2 + 1;
        cv::GaussianBlur(gray, gray, cv::Size(k, k), 0);
    }
    cv::Mat edges;
    cv::Canny(gray, edges, g_params.cannyLow, g_params.cannyHigh);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    cv::Mat out = cv::Mat::zeros(frame.size(), CV_8UC4);
    cv::Scalar col(g_params.contourColor.r, g_params.contourColor.g, g_params.contourColor.b, 255);
    int thickness = g_params.contourFill ? cv::FILLED : g_params.contourThickness;
    for (size_t i = 0; i < contours.size(); i++) {
        if (cv::contourArea(contours[i]) < g_params.contourMinArea) continue;
        cv::drawContours(out, contours, (int)i, col, thickness);
    }
    return out;
}

static cv::Mat RunOpticalFlow(const cv::Mat &frame) {
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_RGBA2GRAY);

    if (g_prevGray.empty() || g_prevGray.size() != gray.size()) {
        g_prevGray = gray.clone();
        g_lastFlowVis = cv::Mat::zeros(frame.size(), CV_8UC4);
        return g_lastFlowVis.clone();
    }

    // Farneback dense flow is the most expensive step in this file — only
    // recompute every other frame and reuse the last visualization the
    // frames in between, same idea as the face-detect throttle below.
    if (g_frameCounter % 2 == 0) {
        cv::Mat flow;
        cv::calcOpticalFlowFarneback(g_prevGray, gray, flow, 0.5, 3, 15, 3, 5, 1.2, 0);

        std::vector<cv::Mat> ch(2);
        cv::split(flow, ch);
        cv::Mat magnitude, angle;
        cv::cartToPolar(ch[0], ch[1], magnitude, angle, true);

        if (g_params.flowArrows) {
            cv::Mat out = cv::Mat::zeros(frame.size(), CV_8UC4);
            cv::Scalar col(g_params.edgeColor.r, g_params.edgeColor.g, g_params.edgeColor.b, 255);
            int step = g_params.flowArrowStep > 2 ? g_params.flowArrowStep : 2;
            for (int y = step / 2; y < flow.rows; y += step) {
                for (int x = step / 2; x < flow.cols; x += step) {
                    cv::Point2f fxy = flow.at<cv::Point2f>(y, x) * g_params.flowStrength;
                    cv::Point p1(x, y);
                    cv::Point p2(cvRound(x + fxy.x), cvRound(y + fxy.y));
                    cv::arrowedLine(out, p1, p2, col, 1, cv::LINE_AA, 0, 0.35);
                }
            }
            g_lastFlowVis = out;
        } else {
            cv::Mat hue = angle * (180.0 / 360.0); // 0..360deg -> 0..180 (OpenCV hue range)
            hue.convertTo(hue, CV_8UC1);
            cv::Mat value;
            cv::normalize(magnitude, value, 0, 255 * std::min(std::max(g_params.flowStrength, 0.1f), 3.0f), cv::NORM_MINMAX);
            value.convertTo(value, CV_8UC1);
            cv::Mat sat(hue.size(), CV_8UC1, cv::Scalar(220));
            std::vector<cv::Mat> hsvCh = { hue, sat, value };
            cv::Mat hsv;
            cv::merge(hsvCh, hsv);
            cv::Mat bgr;
            cv::cvtColor(hsv, bgr, cv::COLOR_HSV2BGR);
            cv::cvtColor(bgr, g_lastFlowVis, cv::COLOR_BGR2RGBA);
        }
    }

    g_prevGray = gray.clone();
    return g_lastFlowVis.clone();
}

static void OCV_EnsureBgSubtractor() {
    bool needsNew = g_bgSub.empty()
        || g_bgHistoryBuilt != g_params.bgHistory
        || g_bgVarThresholdBuilt != g_params.bgVarThreshold
        || g_bgShadowsBuilt != g_params.bgShadows;
    if (!needsNew) return;
    g_bgSub = cv::createBackgroundSubtractorMOG2(g_params.bgHistory, g_params.bgVarThreshold, g_params.bgShadows);
    g_bgHistoryBuilt = g_params.bgHistory;
    g_bgVarThresholdBuilt = g_params.bgVarThreshold;
    g_bgShadowsBuilt = g_params.bgShadows;
}

static cv::Mat RunBgSubtract(const cv::Mat &frame) {
    OCV_EnsureBgSubtractor();
    cv::Mat bgr;
    cv::cvtColor(frame, bgr, cv::COLOR_RGBA2BGR);
    cv::Mat fgMask;
    g_bgSub->apply(bgr, fgMask);

    if (g_params.bgMaskOnly) {
        cv::Mat out;
        cv::cvtColor(fgMask, out, cv::COLOR_GRAY2RGBA);
        return out;
    }

    // Dim in BGR (3-channel) first, then convert both to RGBA — converting
    // the already-4-channel frame directly would scale the alpha channel
    // down along with color, leaving dimmed background pixels unintentionally
    // translucent once composited.
    cv::Mat dimBgr;
    bgr.convertTo(dimBgr, -1, 0.28, 0);
    cv::Mat out, dimRgba;
    cv::cvtColor(bgr, out, cv::COLOR_BGR2RGBA);
    cv::cvtColor(dimBgr, dimRgba, cv::COLOR_BGR2RGBA);
    dimRgba.copyTo(out, fgMask == 0);
    return out;
}

static cv::Mat RunFaceDetect(const cv::Mat &frame) {
    if (!g_faceCascadeAttempted) {
        g_faceCascadeAttempted = true;
        // Cascade data must be passed from JavaScript via js_set_cascade_data()
        // This avoids Emscripten's fragile --preload-file filesystem mounting.
        // JavaScript fetches the XML and passes its buffer directly to WASM.
        try {
            if (g_cascadeBuffer && g_cascadeBufferSize > 0) {
                // Create a temporary in-memory XML file by writing to /tmp
                FILE *tmpFile = fopen("/tmp/cascade.xml", "wb");
                if (tmpFile) {
                    fwrite(g_cascadeBuffer, 1, g_cascadeBufferSize, tmpFile);
                    fclose(tmpFile);
                    
                    g_faceCascadeOk = g_faceCascade.load("/tmp/cascade.xml");
                    if (!g_faceCascadeOk) {
                        fprintf(stderr, "[face_detect] Failed to load cascade from buffer (returned false)\n");
                    }
                } else {
                    fprintf(stderr, "[face_detect] Failed to write cascade buffer to /tmp\n");
                }
            } else {
                fprintf(stderr, "[face_detect] Cascade buffer not set. Call js_set_cascade_data() from JavaScript first.\n");
            }
        } catch (const cv::Exception &e) {
            fprintf(stderr, "[face_detect] cv::Exception loading cascade: %s\n", e.what());
            g_faceCascadeOk = false;
        }
    }

    cv::Mat out = frame.clone();
    if (!g_faceCascadeOk) return out; // cascade missing/failed — passthrough, never crash

    // Haar cascades are the most expensive pipeline here; detect on a
    // throttle and keep drawing the last known boxes in between so the
    // overlay still tracks roughly-in-place faces smoothly instead of
    // updating in visible steps.
    if (g_frameCounter % 4 == 0) {
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_RGBA2GRAY);
        cv::equalizeHist(gray, gray);
        int minSize = (int)(g_params.faceMinSizeFraction * frame.cols);
        if (minSize < 8) minSize = 8;
        std::vector<cv::Rect> faces;
        g_faceCascade.detectMultiScale(gray, faces, g_params.faceScaleFactor, g_params.faceMinNeighbors,
                                        0, cv::Size(minSize, minSize));
        g_lastFaces = faces;
    }

    cv::Scalar col(g_params.faceBoxColor.r, g_params.faceBoxColor.g, g_params.faceBoxColor.b, 255);
    for (const auto &r : g_lastFaces) {
        cv::rectangle(out, r, col, 2);
    }
    if (g_params.faceShowCount) {
        char label[32];
        snprintf(label, sizeof(label), "faces: %d", (int)g_lastFaces.size());
        cv::putText(out, label, cv::Point(8, 22), cv::FONT_HERSHEY_SIMPLEX, 0.6, col, 2, cv::LINE_AA);
    }
    return out;
}

// ============================================================================
// DRAW
// ============================================================================

void OpencvEffect_Draw(RenderTexture2D scene, int screenW, int screenH) {
    float scale = g_params.processScale;
    if (scale < 0.1f) scale = 0.1f;
    if (scale > 1.0f) scale = 1.0f;
    int workW = (int)(screenW * scale);
    int workH = (int)(screenH * scale);
    if (workW < 2) workW = 2;
    if (workH < 2) workH = 2;

    if (!g_readTargetReady || workW != g_readW || workH != g_readH) {
        if (g_readTargetReady) UnloadRenderTexture(g_readTarget);
        g_readTarget = LoadRenderTexture(workW, workH);
        g_readTargetReady = true;
        g_readW = workW;
        g_readH = workH;
    }

    // Downsample the scene into our working-resolution RenderTexture. The
    // negative source height is the same "un-flip while copying between
    // render textures" trick ascii_effect.h/particles_effect.h use —
    // RenderTexture2D's backing GL texture is stored upside-down relative
    // to normal screen orientation.
    BeginTextureMode(g_readTarget);
    ClearBackground(BLACK);
    DrawTexturePro(scene.texture,
        (Rectangle){ 0, 0, (float)scene.texture.width, -(float)scene.texture.height },
        (Rectangle){ 0, 0, (float)workW, (float)workH },
        (Vector2){ 0, 0 }, 0.0f, WHITE);
    EndTextureMode();

    Image img = LoadImageFromTexture(g_readTarget.texture);
    cv::Mat rgba(workH, workW, CV_8UC4, img.data);
    cv::Mat frame = rgba.clone();
    UnloadImage(img);

    // LoadImageFromTexture() reads the GL framebuffer bottom-up (same
    // reason ascii_effect.h indexes rows as `rows - 1 - y`). Flip once here
    // so every pipeline below works in normal top-down image orientation —
    // this matters for face-box coordinates and flow-arrow direction
    // looking right to a viewer, not just for symmetric filters like Canny.
    cv::flip(frame, frame, 0);
    if (g_params.mirror) cv::flip(frame, frame, 1);

    g_frameCounter++;

    cv::Mat out;
    try {
        switch (g_params.mode) {
            case OCV_MODE_EDGES:    out = RunEdges(frame); break;
            case OCV_MODE_CONTOURS: out = RunContours(frame); break;
            case OCV_MODE_FLOW:     out = RunOpticalFlow(frame); break;
            case OCV_MODE_BG:       out = RunBgSubtract(frame); break;
            case OCV_MODE_FACE:     out = RunFaceDetect(frame); break;
            default:                out = frame; break;
        }
    } catch (const cv::Exception &e) {
        // OpenCV throws cv::Exception (via CV_Assert/CV_Error) on plenty of
        // recoverable conditions — a malformed/missing cascade file, an
        // unexpected Mat shape at some internal assertion, etc. Emscripten
        // aborts the ENTIRE wasm runtime on an uncaught C++ exception
        // (needs -s DISABLE_EXCEPTION_CATCHING=0 at link time just to reach
        // this catch at all — see the Makefile), so letting any of these
        // propagate would take down the whole app over one bad frame in one
        // effect. Fall back to a plain passthrough instead.
        fprintf(stderr, "[opencv_effect] cv::Exception in mode %d: %s\n", (int)g_params.mode, e.what());
        out = frame;
    } catch (const std::exception &e) {
        fprintf(stderr, "[opencv_effect] std::exception in mode %d: %s\n", (int)g_params.mode, e.what());
        out = frame;
    }
    if (!out.isContinuous()) out = out.clone();

    // `out` is already top-down (we never flipped back), which is exactly
    // what a plain Texture2D (as opposed to a RenderTexture2D) expects —
    // no flip needed for this upload or the final DrawTexturePro below.
    if (!g_outputTextureReady || g_outW != workW || g_outH != workH) {
        if (g_outputTextureReady) UnloadTexture(g_outputTexture);
        Image outImg = {
            .data = out.data,
            .width = workW,
            .height = workH,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
        };
        g_outputTexture = LoadTextureFromImage(outImg);
        g_outW = workW;
        g_outH = workH;
        g_outputTextureReady = true;
    } else {
        UpdateTexture(g_outputTexture, out.data);
    }

    DrawTexturePro(g_outputTexture,
        (Rectangle){ 0, 0, (float)workW, (float)workH },
        (Rectangle){ 0, 0, (float)screenW, (float)screenH },
        (Vector2){ 0, 0 }, 0.0f, WHITE);
}

void OpencvEffect_Unload(void) {
    if (g_readTargetReady) { UnloadRenderTexture(g_readTarget); g_readTargetReady = false; }
    if (g_outputTextureReady) { UnloadTexture(g_outputTexture); g_outputTextureReady = false; }
    g_prevGray.release();
    g_lastFlowVis.release();
    g_bgSub.release();
    g_lastFaces.clear();
}

// ============================================================================
// CASCADE DATA RECEIVER (called from JavaScript)
// ============================================================================
// JavaScript fetches the cascade XML and passes it here as a byte buffer.
// This avoids Emscripten's --preload-file fragility.
//
// Usage from JS (via wasmBridge.ts):
//   fetch('/path/to/haarcascade_frontalface_default.xml')
//     .then(r => r.arrayBuffer())
//     .then(buf => Module.ccall('js_set_cascade_data', null, 
//             ['number', 'number'], [buf.byteLength, new Uint8Array(buf)]))
//
extern "C" {
    void js_set_cascade_data(size_t bufSize, uint8_t *buf) {
        if (g_cascadeBuffer) free(g_cascadeBuffer);
        g_cascadeBuffer = NULL;
        g_cascadeBufferSize = 0;
        
        if (bufSize > 0 && buf) {
            g_cascadeBuffer = (uint8_t *)malloc(bufSize);
            if (g_cascadeBuffer) {
                memcpy(g_cascadeBuffer, buf, bufSize);
                g_cascadeBufferSize = bufSize;
                // Reset attempted flag so next frame retries loading
                g_faceCascadeAttempted = false;
                g_faceCascadeOk = false;
            } else {
                fprintf(stderr, "[face_detect] Failed to allocate memory for cascade buffer\n");
            }
        }
    }
}

// ============================================================================
// NATIVE CAMERA CAPTURE (desktop builds only -- guarded out entirely under
// Emscripten, same as the declarations above, for the same reason: no
// cv::VideoCapture in the trimmed wasm OpenCV build and no camera to open
// from inside a wasm sandbox anyway).
// ============================================================================
#ifndef __EMSCRIPTEN__
#include <opencv2/videoio.hpp>

static cv::VideoCapture g_camera;
static bool g_cameraOpen = false;

static Texture2D g_cameraTexture;
static bool g_cameraTextureReady = false;
static int g_cameraTexW = 0, g_cameraTexH = 0;

bool OcvCamera_Open(int deviceIndex) {
    if (g_cameraOpen) return true;
    g_cameraOpen = g_camera.open(deviceIndex);
    if (!g_cameraOpen) {
        fprintf(stderr, "[opencv_effect] Could not open camera %d\n", deviceIndex);
    }
    return g_cameraOpen;
}

bool OcvCamera_IsOpen(void) {
    return g_cameraOpen;
}

void OcvCamera_CaptureInto(RenderTexture2D target) {
    if (!g_cameraOpen) return;

    cv::Mat bgr;
    if (!g_camera.read(bgr) || bgr.empty()) return; // dropped frame — leave target as-is

    cv::Mat rgba;
    cv::cvtColor(bgr, rgba, cv::COLOR_BGR2RGBA);
    if (!rgba.isContinuous()) rgba = rgba.clone();

    if (!g_cameraTextureReady || g_cameraTexW != rgba.cols || g_cameraTexH != rgba.rows) {
        if (g_cameraTextureReady) UnloadTexture(g_cameraTexture);
        Image img = {
            .data = rgba.data,
            .width = rgba.cols,
            .height = rgba.rows,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
        };
        g_cameraTexture = LoadTextureFromImage(img);
        g_cameraTexW = rgba.cols;
        g_cameraTexH = rgba.rows;
        g_cameraTextureReady = true;
    } else {
        UpdateTexture(g_cameraTexture, rgba.data);
    }

    // Same "external frame -> scene RenderTexture" wiring js_set_video_frame()
    // + DrawBaseScene() use for the web build's decoded video/camera frame —
    // stretch the camera texture to fill `target` so callers (main003.c) can
    // hand the result straight to OpencvEffect_Draw() as `scene`, exactly
    // like the procedural placeholder it replaces.
    BeginTextureMode(target);
        ClearBackground(BLACK);
        DrawTexturePro(g_cameraTexture,
            (Rectangle){ 0, 0, (float)g_cameraTexture.width, (float)g_cameraTexture.height },
            (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height },
            (Vector2){ 0, 0 }, 0.0f, WHITE);
    EndTextureMode();
}

void OcvCamera_Close(void) {
    if (g_cameraOpen) {
        g_camera.release();
        g_cameraOpen = false;
    }
    if (g_cameraTextureReady) {
        UnloadTexture(g_cameraTexture);
        g_cameraTextureReady = false;
    }
    g_cameraTexW = 0;
    g_cameraTexH = 0;
}
#endif // !__EMSCRIPTEN__

#endif // OPENCV_EFFECT_IMPLEMENTATION
