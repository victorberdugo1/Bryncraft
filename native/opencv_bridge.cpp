/*
 * opencv_bridge.cpp — OpenCV-powered implementation of the "opencv" effect.
 *
 * This is the ONLY .cpp file in native/ — everything else (main.c,
 * json_mini.c, effects/*.h) stays plain C, compiled with emcc as before.
 * This file is compiled separately with em++ (see Makefile: CXXFLAGS /
 * the `%.o: %.cpp` rule) and the two object sets are linked together with
 * em++ as the final link driver, which is the only change needed to the
 * link step to make a C project consume a C++-only library like OpenCV.
 *
 * Pipeline shared by every mode:
 *   1. Downsample `scene` (the RenderTexture2D every effect reads from —
 *      decoded video/camera frame, or DrawBaseScene()'s procedural
 *      placeholder) into g_readTarget at g_params.processScale resolution.
 *      Working at less than full resolution is what keeps Canny/contours/
 *      optical flow/Haar cascades inside frame budget on a single wasm
 *      thread — this is a deliberate quality/perf tradeoff exposed to the
 *      user as the "processScale" param, not an oversight.
 *   2. LoadImageFromTexture() it back to CPU (same technique
 *      ascii_effect.h/particles_effect.h already use), wrap as a cv::Mat,
 *      flip it right-side-up once (RenderTexture2D's GPU texture is
 *      stored upside-down — see the long comment in OpencvEffect_Draw).
 *   3. Run whichever OpenCV pipeline is selected.
 *   4. Upload the RGBA result to a plain Texture2D (NOT a RenderTexture —
 *      so no upside-down storage this time) and blit it to the backbuffer.
 */
#include "effects/opencv_effect.h"
#include "json_mini.h"

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

struct OcvParams {
    OcvMode mode = OCV_MODE_EDGES;
    float processScale = 0.5f;   // internal working resolution, relative to the canvas
    bool mirror = false;         // horizontal flip — handy with a front-facing camera

    // edges
    float cannyLow = 60.0f, cannyHigh = 160.0f;
    int blur = 1;
    bool edgeOnSource = false;
    Color edgeColor = { 68, 212, 255, 255 };

    // contours
    float contourMinArea = 80.0f;
    int contourThickness = 2;
    bool contourFill = false;
    Color contourColor = { 68, 212, 255, 255 };

    // optical flow
    float flowStrength = 1.0f;
    bool flowArrows = false;
    int flowArrowStep = 16;

    // background subtraction
    int bgHistory = 120;
    float bgVarThreshold = 16.0f;
    bool bgShadows = true;
    bool bgMaskOnly = false;

    // face detection
    float faceScaleFactor = 1.1f;
    int faceMinNeighbors = 4;
    float faceMinSizeFraction = 0.08f;
    Color faceBoxColor = { 120, 255, 120, 255 };
    bool faceShowCount = true;
};

static OcvParams g_params;
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
        // Preloaded by Emscripten's --preload-file assets (see Makefile) —
        // lands in the virtual FS at this same relative path.
        g_faceCascadeOk = g_faceCascade.load("assets/cv/haarcascade_frontalface_default.xml");
    }

    cv::Mat out = frame.clone();
    if (!g_faceCascadeOk) return out; // asset missing — passthrough, never crash

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
    switch (g_params.mode) {
        case OCV_MODE_EDGES:    out = RunEdges(frame); break;
        case OCV_MODE_CONTOURS: out = RunContours(frame); break;
        case OCV_MODE_FLOW:     out = RunOpticalFlow(frame); break;
        case OCV_MODE_BG:       out = RunBgSubtract(frame); break;
        case OCV_MODE_FACE:     out = RunFaceDetect(frame); break;
        default:                out = frame; break;
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
