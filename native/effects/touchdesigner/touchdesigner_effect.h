#ifndef TOUCHDESIGNER_EFFECT_H
#define TOUCHDESIGNER_EFFECT_H

#include "raylib.h"
#include "rlgl.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include "../../json_mini.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void TouchdesignerEffect_Init(void);
#ifdef __EMSCRIPTEN__
void TouchdesignerEffect_SetParams(const JsonValue *paramsObj);
#endif
void TouchdesignerEffect_Update(float dt);
void TouchdesignerEffect_Draw(RenderTexture2D scene, int screenW, int screenH);
void TouchdesignerEffect_Unload(void);
void TouchdesignerEffect_SetHandData(
    float h0x, float h0y, float h0pinch, float h0fist, int h0present,
    float h1x, float h1y, float h1pinch, float h1fist, int h1present);

#ifdef __cplusplus
}
#endif

typedef enum {
    HAND_GESTURE_NONE = 0,
    HAND_GESTURE_OPEN = 1,
    HAND_GESTURE_CLOSED = 2,
    HAND_GESTURE_PINCH = 3,
    HAND_GESTURE_POINTING = 4,
    HAND_GESTURE_PEACE = 5,
} HandGesture;

typedef struct {
    Vector2 pos;
    float confidence;
    HandGesture gesture;
    int fingerCount;
    float palmSize;
    float rotation;
} Hand;

#ifdef __cplusplus
extern "C" {
#endif

void TD_HandTracking_Init(void);
void TD_HandTracking_ProcessFrame(const unsigned char *rgba, int width, int height, bool forceReanchor);
void TD_HandTracking_SetMirror(bool mirror);
int  TD_HandTracking_GetHandCount(void);
Hand TD_HandTracking_GetHand(int index);
void TD_HandTracking_Unload(void);

#ifndef __EMSCRIPTEN__
bool TD_HandCamera_Open(int deviceIndex);
bool TD_HandCamera_IsOpen(void);
void TD_HandCamera_CaptureInto(RenderTexture2D target);
void TD_HandCamera_Close(void);
#endif

#ifdef __cplusplus
}
#endif

#define TD_MAX_HANDS 2

typedef struct {
    float x, y;
    float size;
    float rotation;
    bool  present;
} TD_Hand;

typedef struct {
    bool   showCameraBg;
    bool   mirror;
    bool   autoDetectHands;
    float  handProcessScale;
    int    handReanchorInterval;
    int    handDetectSkip;
    Color  glassColor;
    float  glassSize;
    bool   glassEnabled;
    float  slimeDistance;
    bool   slimeEnabled;
    bool   showHandCount;
    Color  bgFallbackColor;
    bool   forceFallback;
} TD_Params;

static TD_Params TD_g_params = {
    .showCameraBg = true,
    .mirror = false,
    .autoDetectHands = true,
    .handProcessScale = 0.16f,
    .handReanchorInterval = 24,
    .handDetectSkip = 3,
    .glassColor = (Color){ 140, 235, 255, 255 },
    .glassSize = 2.0f,
    .glassEnabled = true,
    .slimeDistance = 3.0f,
    .slimeEnabled = true,
    .showHandCount = false,
    .bgFallbackColor = (Color){ 0, 0, 0, 255 },
    .forceFallback = false,
};

static TD_Hand TD_g_hands[TD_MAX_HANDS];
static TD_Hand TD_g_handsTarget[TD_MAX_HANDS];
static float TD_g_time = 0.0f;
static float TD_g_dt = 1.0f / 60.0f;

#define TD_LENS_CAPTURE_SIZE 192
static RenderTexture2D TD_g_lensCapture[TD_MAX_HANDS];
static bool TD_g_lensCaptureReady[TD_MAX_HANDS] = { false, false };

#define TD_BRIDGE_CAPTURE_W 256
#define TD_BRIDGE_CAPTURE_H 160
static RenderTexture2D TD_g_bridgeLensCapture;
static bool TD_g_bridgeLensCaptureReady = false;

static RenderTexture2D TD_g_handReadTarget;
static bool TD_g_handReadReady = false;
static int  TD_g_handReadW = 0, TD_g_handReadH = 0;
static int  TD_g_handFrameCounter = 0;

static bool TD_IsSceneBlack(RenderTexture2D rt) {
    (void)rt;
    return TD_g_params.forceFallback;
}

static RenderTexture2D TD_s_mirrorTarget;
static bool TD_s_mirrorReady = false;
static int  TD_s_mirrorW = 0, TD_s_mirrorH = 0;

static RenderTexture2D TD_GetEffectiveScene(RenderTexture2D scene, int screenW, int screenH) {
    if (!TD_g_params.mirror) return scene;

    if (!TD_s_mirrorReady || TD_s_mirrorW != scene.texture.width || TD_s_mirrorH != scene.texture.height) {
        if (TD_s_mirrorReady) UnloadRenderTexture(TD_s_mirrorTarget);
        TD_s_mirrorTarget = LoadRenderTexture(scene.texture.width, scene.texture.height);
        TD_s_mirrorReady = true;
        TD_s_mirrorW = scene.texture.width;
        TD_s_mirrorH = scene.texture.height;
    }

    BeginTextureMode(TD_s_mirrorTarget);
        ClearBackground(BLACK);
        DrawTexturePro(scene.texture,
            (Rectangle){ 0, 0, -(float)scene.texture.width, -(float)scene.texture.height },
            (Rectangle){ 0, 0, (float)scene.texture.width, (float)scene.texture.height },
            (Vector2){ 0, 0 }, 0.0f, WHITE);
    EndTextureMode();

    return TD_s_mirrorTarget;
}

#ifndef TOUCHDESIGNER_EFFECT_OBJ_BUILD
void TouchdesignerEffect_Init(void) {
    for (int i = 0; i < TD_MAX_HANDS; i++) {
        TD_g_hands[i] = (TD_Hand){ 0.5f, 0.5f, 0.0f, 0.0f, false };
        TD_g_handsTarget[i] = TD_g_hands[i];
    }

    TD_HandTracking_Init();
    TD_HandTracking_SetMirror(TD_g_params.mirror);
}

#ifdef __EMSCRIPTEN__
static Color TD_HexToColor(const char *hex, Color fallback) {
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

void TouchdesignerEffect_SetParams(const JsonValue *paramsObj) {
    if (!paramsObj) return;
    TD_g_params.showCameraBg = JsonAsBool(JsonObjectGet(paramsObj, "showCameraBg"), TD_g_params.showCameraBg);
    TD_g_params.handReanchorInterval = (int)JsonAsNumber(JsonObjectGet(paramsObj, "handReanchorInterval"), TD_g_params.handReanchorInterval);
    TD_g_params.handProcessScale = (float)JsonAsNumber(JsonObjectGet(paramsObj, "handProcessScale"), TD_g_params.handProcessScale);
    TD_g_params.handDetectSkip = (int)JsonAsNumber(JsonObjectGet(paramsObj, "handDetectSkip"), TD_g_params.handDetectSkip);
    TD_g_params.glassColor = TD_HexToColor(JsonAsString(JsonObjectGet(paramsObj, "glassColor"), NULL), TD_g_params.glassColor);
    TD_g_params.glassSize = (float)JsonAsNumber(JsonObjectGet(paramsObj, "glassSize"), TD_g_params.glassSize);
    TD_g_params.glassEnabled = JsonAsBool(JsonObjectGet(paramsObj, "glassEnabled"), TD_g_params.glassEnabled);
    TD_g_params.slimeDistance = (float)JsonAsNumber(JsonObjectGet(paramsObj, "slimeDistance"), TD_g_params.slimeDistance);
    TD_g_params.slimeEnabled = JsonAsBool(JsonObjectGet(paramsObj, "slimeEnabled"), TD_g_params.slimeEnabled);
    TD_g_params.showHandCount = JsonAsBool(JsonObjectGet(paramsObj, "showHandCount"), TD_g_params.showHandCount);

    bool prevMirror = TD_g_params.mirror;
    TD_g_params.mirror = JsonAsBool(JsonObjectGet(paramsObj, "mirror"), TD_g_params.mirror);
    if (TD_g_params.mirror != prevMirror) TD_HandTracking_SetMirror(TD_g_params.mirror);

    TD_g_params.bgFallbackColor = TD_HexToColor(JsonAsString(JsonObjectGet(paramsObj, "bgFallbackColor"), NULL), TD_g_params.bgFallbackColor);
    TD_g_params.forceFallback = JsonAsBool(JsonObjectGet(paramsObj, "forceFallback"), TD_g_params.forceFallback);
}
#endif

static void TD_DetectHands(RenderTexture2D scene, int screenW, int screenH) {
    TD_g_handFrameCounter++;

    int detectSkip = TD_g_params.handDetectSkip;
    if (detectSkip < 1) detectSkip = 1;
    if (TD_g_handFrameCounter != 1 && (TD_g_handFrameCounter % detectSkip) != 0) {
        return;
    }

    int interval = TD_g_params.handReanchorInterval;
    if (interval < 4) interval = 4;
    if (TD_HandTracking_GetHandCount() > 0) interval *= 4;
    bool forceReanchor = (TD_g_handFrameCounter == 1) || (TD_g_handFrameCounter % interval == 0);

    float scale = TD_g_params.handProcessScale;
    if (scale < 0.1f) scale = 0.1f;
    if (scale > 1.0f) scale = 1.0f;
    int workW = (int)(screenW * scale);
    int workH = (int)(screenH * scale);
    if (workW < 2) workW = 2;
    if (workH < 2) workH = 2;

    if (!TD_g_handReadReady || workW != TD_g_handReadW || workH != TD_g_handReadH) {
        if (TD_g_handReadReady) UnloadRenderTexture(TD_g_handReadTarget);
        TD_g_handReadTarget = LoadRenderTexture(workW, workH);
        TD_g_handReadReady = true;
        TD_g_handReadW = workW;
        TD_g_handReadH = workH;
    }

    BeginTextureMode(TD_g_handReadTarget);
        ClearBackground(BLACK);
        DrawTexturePro(scene.texture,
            (Rectangle){ 0, 0, (float)scene.texture.width, (float)scene.texture.height },
            (Rectangle){ 0, 0, (float)workW, (float)workH },
            (Vector2){ 0, 0 }, 0.0f, WHITE);
    EndTextureMode();

    Image img = LoadImageFromTexture(TD_g_handReadTarget.texture);
    TD_HandTracking_ProcessFrame((const unsigned char *)img.data, workW, workH, forceReanchor);
    UnloadImage(img);

    int count = TD_HandTracking_GetHandCount();
    Hand detected[TD_MAX_HANDS];
    int n = count < TD_MAX_HANDS ? count : TD_MAX_HANDS;
    for (int i = 0; i < n; i++) detected[i] = TD_HandTracking_GetHand(i);
    if (n == 2 && detected[0].pos.x > detected[1].pos.x) {
        Hand tmp = detected[0]; detected[0] = detected[1]; detected[1] = tmp;
    }

    int maxWork = workW > workH ? workW : workH;
    for (int i = 0; i < TD_MAX_HANDS; i++) {
        if (i < n) {
            TD_g_handsTarget[i].x = workW > 0 ? detected[i].pos.x / (float)workW : 0.5f;
            TD_g_handsTarget[i].y = workH > 0 ? detected[i].pos.y / (float)workH : 0.5f;
            TD_g_handsTarget[i].size = maxWork > 0 ? detected[i].palmSize / (float)maxWork : 0.12f;
            TD_g_handsTarget[i].rotation = detected[i].rotation;
            TD_g_handsTarget[i].present = true;
        } else {
            TD_g_handsTarget[i].present = false;
        }
    }
}

void TouchdesignerEffect_SetHandData(
    float h0x, float h0y, float h0pinch, float h0fist, int h0present,
    float h1x, float h1y, float h1pinch, float h1fist, int h1present) {
    (void)h0pinch; (void)h0fist; (void)h1pinch; (void)h1fist;
    TD_g_handsTarget[0].x = h0x; TD_g_handsTarget[0].y = h0y; TD_g_handsTarget[0].rotation = 0.0f;
    TD_g_handsTarget[0].present = (h0present != 0);
    TD_g_handsTarget[1].x = h1x; TD_g_handsTarget[1].y = h1y; TD_g_handsTarget[1].rotation = 0.0f;
    TD_g_handsTarget[1].present = (h1present != 0);
}

#define TD_HAND_SMOOTH_RATE 14.0f

void TouchdesignerEffect_Update(float dt) {
    TD_g_time += dt;
    TD_g_dt = dt;

    float smoothFactor = 1.0f - expf(-TD_HAND_SMOOTH_RATE * dt);
    for (int i = 0; i < TD_MAX_HANDS; i++) {
        TD_Hand *cur = &TD_g_hands[i];
        TD_Hand *tgt = &TD_g_handsTarget[i];

        if (!tgt->present) {
            cur->present = false;
            continue;
        }

        if (!cur->present) {
            *cur = *tgt;
            continue;
        }

        cur->x += (tgt->x - cur->x) * smoothFactor;
        cur->y += (tgt->y - cur->y) * smoothFactor;
        cur->size += (tgt->size - cur->size) * smoothFactor;

        float dRot = tgt->rotation - cur->rotation;
        while (dRot > PI) dRot -= 2.0f * PI;
        while (dRot < -PI) dRot += 2.0f * PI;
        cur->rotation += dRot * smoothFactor;
        cur->present = true;
    }
}

#define TD_BLOB_PTS 40
#define TD_BLOB_ELONGATE 1.12f

#define TD_BLOB_ELONGATE_TAUT 1.4f

static Vector2 TD_BlobLocalOffsetAtAngle(float a, float baseR, float elongate, float sideScale,
                                          float time, float phase) {
    float fwd = cosf(a);

    float wobble =
        0.03f * sinf(a * 3.0f + time * 1.1f + phase) +
        0.012f * sinf(a * 5.0f - time * 1.7f + phase * 1.6f);

    float lobes = 0.0f;
    if (fwd > 0.1f) {
        float fwdT = (fwd - 0.1f) / 0.9f;
        lobes = 0.045f * fwdT * fwdT * sinf(a * 4.0f + phase * 0.5f + time * 0.35f);
    }

    float rr = baseR * (1.0f + wobble + lobes);
    return (Vector2){ fwd * rr * elongate, sinf(a) * rr * sideScale };
}

static void TD_BuildHandBlobOutline(Vector2 center, float baseR, float rotation, float elongate,
                                     float time, float phase, Vector2 *outPts) {
    float ca = cosf(rotation), sa = sinf(rotation);
    float sideScale = 2.0f - elongate;
    if (sideScale < 0.6f) sideScale = 0.6f;

    for (int i = 0; i <= TD_BLOB_PTS; i++) {
        float a = (2.0f * PI * (float)i) / (float)TD_BLOB_PTS;
        Vector2 off = TD_BlobLocalOffsetAtAngle(a, baseR, elongate, sideScale, time, phase);
        outPts[i] = (Vector2){ center.x + off.x * ca - off.y * sa, center.y + off.x * sa + off.y * ca };
    }
}

static float TD_BlobRadiusAtDir(float baseR, float rotation, float elongate, float time, float phase, Vector2 worldDir) {
    float sideScale = 2.0f - elongate;
    if (sideScale < 0.6f) sideScale = 0.6f;

    float ca = cosf(rotation), sa = sinf(rotation);
    float lux = worldDir.x * ca + worldDir.y * sa;
    float luy = -worldDir.x * sa + worldDir.y * ca;

    float ux = lux / elongate;
    float uy = luy / sideScale;
    if (ux == 0.0f && uy == 0.0f) return baseR;
    float a = atan2f(uy, ux);

    Vector2 off = TD_BlobLocalOffsetAtAngle(a, baseR, elongate, sideScale, time, phase);
    return sqrtf(off.x * off.x + off.y * off.y);
}

static Vector2 TD_LocalToWorld(Vector2 c, float ca, float sa, float lx, float ly) {
    return (Vector2){ c.x + lx * ca - ly * sa, c.y + lx * sa + ly * ca };
}

static void TD_DrawSoftGlint(Vector2 center, float radiusX, float radiusY, Color peak) {
    if (radiusX < 0.5f) radiusX = 0.5f;
    if (radiusY < 0.5f) radiusY = 0.5f;

    const int steps = 8;
    for (int i = steps; i >= 1; i--) {
        float t = (float)i / (float)steps;
        float fade = 1.0f - t;
        Color c = peak;
        c.a = (unsigned char)(peak.a * fade * fade);
        DrawEllipse((int)center.x, (int)center.y, radiusX * t, radiusY * t, c);
    }
}

static void TD_CaptureLensRegion(int handIndex, RenderTexture2D scene, int screenW, int screenH,
                                  Vector2 centerScreen, float captureRadiusScreen) {
    if (!TD_g_lensCaptureReady[handIndex]) {
        TD_g_lensCapture[handIndex] = LoadRenderTexture(TD_LENS_CAPTURE_SIZE, TD_LENS_CAPTURE_SIZE);
        TD_g_lensCaptureReady[handIndex] = true;
    }

    float scaleX = (float)scene.texture.width / (float)screenW;
    float scaleY = (float)scene.texture.height / (float)screenH;

    float sx = centerScreen.x * scaleX;
    float sy = ((float)screenH - centerScreen.y) * scaleY;
    float srx = captureRadiusScreen * scaleX;
    float sry = captureRadiusScreen * scaleY;

    Rectangle src = { sx - srx, sy - sry, srx * 2.0f, sry * 2.0f };
    Rectangle dst = { 0, 0, (float)TD_LENS_CAPTURE_SIZE, (float)TD_LENS_CAPTURE_SIZE };

    BeginTextureMode(TD_g_lensCapture[handIndex]);
        ClearBackground(BLANK);
        DrawTexturePro(scene.texture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
    EndTextureMode();
}

static void TD_CaptureBridgeLensRegion(RenderTexture2D scene, int screenW, int screenH,
                                        float minX, float minY, float maxX, float maxY) {
    if (!TD_g_bridgeLensCaptureReady) {
        TD_g_bridgeLensCapture = LoadRenderTexture(TD_BRIDGE_CAPTURE_W, TD_BRIDGE_CAPTURE_H);
        TD_g_bridgeLensCaptureReady = true;
    }

    float scaleX = (float)scene.texture.width / (float)screenW;
    float scaleY = (float)scene.texture.height / (float)screenH;

    float sy0 = ((float)screenH - maxY) * scaleY;
    float sy1 = ((float)screenH - minY) * scaleY;

    Rectangle src = { minX * scaleX, sy0, (maxX - minX) * scaleX, sy1 - sy0 };
    Rectangle dst = { 0, 0, (float)TD_BRIDGE_CAPTURE_W, (float)TD_BRIDGE_CAPTURE_H };

    BeginTextureMode(TD_g_bridgeLensCapture);
        ClearBackground(BLANK);
        DrawTexturePro(scene.texture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
    EndTextureMode();
}

#define TD_LENS_RINGS 7

#define TD_SHARED_LENS_ZOOM 0.5f

static void TD_DrawLensDome(Vector2 c, Vector2 *outline, Texture2D lensTex, float captureRadiusScreen, Color tint,
                             bool useSharedRect, float capMinX, float capMinY, float capMaxX, float capMaxY,
                             const float *suppress) {
    static Vector2 ringPts[TD_LENS_RINGS][TD_BLOB_PTS + 1];
    static Vector2 ringUV[TD_LENS_RINGS][TD_BLOB_PTS + 1];

    const float gamma = 1.7f;
    float capW = capMaxX - capMinX; if (capW < 1.0f) capW = 1.0f;
    float capH = capMaxY - capMinY; if (capH < 1.0f) capH = 1.0f;

    for (int ring = 0; ring < TD_LENS_RINGS; ring++) {
        float t = (float)ring / (float)(TD_LENS_RINGS - 1);
        float sampleT = powf(t, gamma);

        for (int i = 0; i <= TD_BLOB_PTS; i++) {
            float offX = outline[i].x - c.x;
            float offY = outline[i].y - c.y;

            ringPts[ring][i] = (Vector2){ c.x + offX * t, c.y + offY * t };

            float u, v;
            if (useSharedRect) {
                float zoomT = sampleT * TD_SHARED_LENS_ZOOM;
                float sampleX = c.x + offX * zoomT;
                float sampleY = c.y + offY * zoomT;
                u = (sampleX - capMinX) / capW;
                v = (sampleY - capMinY) / capH;
            } else {
                u = 0.5f + (offX * sampleT / captureRadiusScreen) * 0.5f;
                v = 0.5f + (offY * sampleT / captureRadiusScreen) * 0.5f;
            }
            if (u < 0.0f) u = 0.0f; else if (u > 1.0f) u = 1.0f;
            if (v < 0.0f) v = 0.0f; else if (v > 1.0f) v = 1.0f;
            ringUV[ring][i] = (Vector2){ u, v };
        }
    }

    rlSetTexture(lensTex.id);
    rlBegin(RL_QUADS);
        rlColor4ub(255, 255, 255, 255);
        for (int ring = 0; ring < TD_LENS_RINGS - 1; ring++) {
            for (int i = 0; i < TD_BLOB_PTS; i++) {
                rlTexCoord2f(ringUV[ring][i].x, ringUV[ring][i].y);
                rlVertex2f(ringPts[ring][i].x, ringPts[ring][i].y);

                rlTexCoord2f(ringUV[ring][i + 1].x, ringUV[ring][i + 1].y);
                rlVertex2f(ringPts[ring][i + 1].x, ringPts[ring][i + 1].y);

                rlTexCoord2f(ringUV[ring + 1][i + 1].x, ringUV[ring + 1][i + 1].y);
                rlVertex2f(ringPts[ring + 1][i + 1].x, ringPts[ring + 1][i + 1].y);

                rlTexCoord2f(ringUV[ring + 1][i].x, ringUV[ring + 1][i].y);
                rlVertex2f(ringPts[ring + 1][i].x, ringPts[ring + 1][i].y);
            }
        }
    rlEnd();
    rlSetTexture(0);

    for (int ring = 0; ring < TD_LENS_RINGS - 1; ring++) {
        float t0 = (float)ring / (float)(TD_LENS_RINGS - 1);
        float t1 = (float)(ring + 1) / (float)(TD_LENS_RINGS - 1);
        float tm = (t0 + t1) * 0.5f;
        float shade = powf(tm, 2.2f);
        float baseA = 8.0f + shade * 60.0f;

        for (int i = 0; i < TD_BLOB_PTS; i++) {
            float segSuppress = suppress ? (suppress[i] + suppress[i + 1]) * 0.5f : 1.0f;
            Color band = tint;
            band.a = (unsigned char)(baseA * segSuppress);
            DrawTriangle(ringPts[ring][i], ringPts[ring][i + 1], ringPts[ring + 1][i + 1], band);
            DrawTriangle(ringPts[ring][i], ringPts[ring + 1][i + 1], ringPts[ring + 1][i], band);
        }
    }
}

static float TD_MergeSuppression(Vector2 outDir, Vector2 mergeDir, float mergeStrength) {
    if (mergeStrength <= 0.0f) return 1.0f;
    float dot = outDir.x * mergeDir.x + outDir.y * mergeDir.y;
    const float cosFull = 0.30f;
    const float cosZero = -0.15f;
    float factor = (dot - cosZero) / (cosFull - cosZero);
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;
    return 1.0f - factor;
}

static void TD_DrawHandGlass(Texture2D lensTex, float captureRadiusScreen, Vector2 c, float r, float rotation,
                              Color tint, float time, float phase, float elongate,
                              Vector2 mergeDir, float mergeStrength,
                              bool useSharedRect, float capMinX, float capMinY, float capMaxX, float capMaxY) {
    if (r < 6.0f) r = 6.0f;

    Vector2 outline[TD_BLOB_PTS + 1];
    TD_BuildHandBlobOutline(c, r, rotation, elongate, time, phase, outline);

    float suppress[TD_BLOB_PTS + 1];
    for (int i = 0; i <= TD_BLOB_PTS; i++) {
        Vector2 d = { outline[i].x - c.x, outline[i].y - c.y };
        float len = sqrtf(d.x * d.x + d.y * d.y);
        Vector2 outDir = len > 0.0001f ? (Vector2){ d.x / len, d.y / len } : (Vector2){ 0, 0 };
        suppress[i] = TD_MergeSuppression(outDir, mergeDir, mergeStrength);
    }

    Vector2 fan[TD_BLOB_PTS + 2];
    fan[0] = c;
    for (int i = 0; i <= TD_BLOB_PTS; i++) {
        Vector2 d = { outline[i].x - c.x, outline[i].y - c.y };
        fan[i + 1] = (Vector2){ c.x + d.x * 1.22f, c.y + d.y * 1.22f };
    }
    Color glow = tint; glow.a = 16;
    for (int i = 0; i < TD_BLOB_PTS; i++) {
        float segSuppress = (suppress[i] + suppress[i + 1]) * 0.5f;
        Color segColor = glow; segColor.a = (unsigned char)(glow.a * segSuppress);
        DrawTriangle(fan[0], fan[i + 1], fan[i + 2], segColor);
    }

    fan[0] = c;
    for (int i = 0; i <= TD_BLOB_PTS; i++) {
        Vector2 d = { outline[i].x - c.x, outline[i].y - c.y };
        fan[i + 1] = (Vector2){ c.x + d.x * 1.08f, c.y + d.y * 1.08f };
    }
    Color haloTight = tint; haloTight.a = 26;
    for (int i = 0; i < TD_BLOB_PTS; i++) {
        float segSuppress = (suppress[i] + suppress[i + 1]) * 0.5f;
        Color segColor = haloTight; segColor.a = (unsigned char)(haloTight.a * segSuppress);
        DrawTriangle(fan[0], fan[i + 1], fan[i + 2], segColor);
    }

    TD_DrawLensDome(c, outline, lensTex, captureRadiusScreen, tint, useSharedRect, capMinX, capMinY, capMaxX, capMaxY, suppress);

    float ca = cosf(rotation), sa = sinf(rotation);

    Vector2 spec = TD_LocalToWorld(c, ca, sa, -r * 0.05f, -r * 0.4f);
    Color specColor = WHITE; specColor.a = 200;
    TD_DrawSoftGlint(spec, r * 0.3f, r * 0.16f, specColor);

    Vector2 spec2 = TD_LocalToWorld(c, ca, sa, r * 0.3f, r * 0.28f);
    Color spec2Color = WHITE; spec2Color.a = 110;
    TD_DrawSoftGlint(spec2, r * 0.1f, r * 0.1f, spec2Color);
}

#define TD_BRIDGE_SEGS 20
#define TD_BRIDGE_RINGS 7

static float TD_Smooth01(float x) {
    if (x < 0.0f) x = 0.0f;
    if (x > 1.0f) x = 1.0f;
    return x * x * (3.0f - 2.0f * x);
}

static float TD_BridgeEdgeFade(float t, float fadeSpan0, float fadeSpan1) {
    return TD_Smooth01(t / fadeSpan0) * TD_Smooth01((1.0f - t) / fadeSpan1);
}

static void TD_DrawSlimeBridge(Vector2 c0, float r0, float rot0, float phase0, float elongate0,
                                Vector2 c1, float r1, float rot1, float phase1, float elongate1,
                                Color tint, float strength, float time, float sag,
                                Texture2D lensTex, float capMinX, float capMinY, float capMaxX, float capMaxY) {
    if (strength <= 0.02f) return;
    if (strength > 1.0f) strength = 1.0f;

    float dx = c1.x - c0.x;
    float dy = c1.y - c0.y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 1.0f) dist = 1.0f;
    Vector2 dir = { dx / dist, dy / dist };
    Vector2 nrm = { -dir.y, dir.x };
    Vector2 negNrm = { -nrm.x, -nrm.y };

    float edge0Top = TD_BlobRadiusAtDir(r0, rot0, elongate0, time, phase0, nrm) / r0;
    float edge0Bot = TD_BlobRadiusAtDir(r0, rot0, elongate0, time, phase0, negNrm) / r0;
    float edge1Top = TD_BlobRadiusAtDir(r1, rot1, elongate1, time, phase1, nrm) / r1;
    float edge1Bot = TD_BlobRadiusAtDir(r1, rot1, elongate1, time, phase1, negNrm) / r1;

    Vector2 center[TD_BRIDGE_SEGS + 1];
    float   halfWTop[TD_BRIDGE_SEGS + 1];
    float   halfWBot[TD_BRIDGE_SEGS + 1];
    Vector2 top[TD_BRIDGE_SEGS + 1];
    Vector2 bot[TD_BRIDGE_SEGS + 1];

    for (int i = 0; i <= TD_BRIDGE_SEGS; i++) {
        float t = (float)i / (float)TD_BRIDGE_SEGS;
        float sagAmt = sag * sinf(t * PI);
        Vector2 p = { c0.x + dx * t + nrm.x * sagAmt, c0.y + dy * t + nrm.y * sagAmt };
        center[i] = p;

        float baseR = r0 + (r1 - r0) * t;
        float waistShape = sinf(t * PI);
        float waist = 1.0f - (1.0f - strength) * 0.45f * waistShape;
        if (waist < -0.1f) waist = -0.1f;
        float noise = 0.05f * strength * sinf(t * 9.0f + time * 3.0f);

        float edgeTopFrac = edge0Top + (edge1Top - edge0Top) * t;
        float edgeBotFrac = edge0Bot + (edge1Bot - edge0Bot) * t;
        float edgeFrac = (edgeTopFrac + edgeBotFrac) * 0.5f;

        float hwTop = baseR * (waist + noise) * edgeFrac;
        float hwBot = baseR * (waist + noise) * edgeFrac;
        if (hwTop < 1.5f) hwTop = 1.5f;
        if (hwBot < 1.5f) hwBot = 1.5f;
        halfWTop[i] = hwTop;
        halfWBot[i] = hwBot;

        top[i] = (Vector2){ p.x + nrm.x * hwTop, p.y + nrm.y * hwTop };
        bot[i] = (Vector2){ p.x - nrm.x * hwBot, p.y - nrm.y * hwBot };
    }

    float capW = capMaxX - capMinX; if (capW < 1.0f) capW = 1.0f;
    float capH = capMaxY - capMinY; if (capH < 1.0f) capH = 1.0f;
    const float gamma = 1.7f;

    float fadeSpan0 = (r0 * 0.5f) / dist; if (fadeSpan0 < 0.05f) fadeSpan0 = 0.05f; if (fadeSpan0 > 0.45f) fadeSpan0 = 0.45f;
    float fadeSpan1 = (r1 * 0.5f) / dist; if (fadeSpan1 < 0.05f) fadeSpan1 = 0.05f; if (fadeSpan1 > 0.45f) fadeSpan1 = 0.45f;

    float glowExt[TD_BRIDGE_SEGS + 1];
    float haloExt[TD_BRIDGE_SEGS + 1];
    for (int i = 0; i <= TD_BRIDGE_SEGS; i++) {
        float t = (float)i / (float)TD_BRIDGE_SEGS;
        float fade = TD_BridgeEdgeFade(t, fadeSpan0, fadeSpan1);
        glowExt[i] = 1.0f + 0.15f * fade;
        haloExt[i] = 1.0f + 0.05f * fade;
    }

    Color glow = tint; glow.a = (unsigned char)(16.0f * strength);
    Vector2 glowStrip[(TD_BRIDGE_SEGS + 1) * 2];
    for (int i = 0; i <= TD_BRIDGE_SEGS; i++) {
        glowStrip[i * 2]     = (Vector2){ center[i].x + nrm.x * halfWTop[i] * glowExt[i], center[i].y + nrm.y * halfWTop[i] * glowExt[i] };
        glowStrip[i * 2 + 1] = center[i];
    }
    DrawTriangleStrip(glowStrip, (TD_BRIDGE_SEGS + 1) * 2, glow);
    for (int i = 0; i <= TD_BRIDGE_SEGS; i++) {
        glowStrip[i * 2]     = center[i];
        glowStrip[i * 2 + 1] = (Vector2){ center[i].x - nrm.x * halfWBot[i] * glowExt[i], center[i].y - nrm.y * halfWBot[i] * glowExt[i] };
    }
    DrawTriangleStrip(glowStrip, (TD_BRIDGE_SEGS + 1) * 2, glow);

    Color haloTight = tint; haloTight.a = (unsigned char)(26.0f * strength);
    for (int i = 0; i <= TD_BRIDGE_SEGS; i++) {
        glowStrip[i * 2]     = (Vector2){ center[i].x + nrm.x * halfWTop[i] * haloExt[i], center[i].y + nrm.y * halfWTop[i] * haloExt[i] };
        glowStrip[i * 2 + 1] = center[i];
    }
    DrawTriangleStrip(glowStrip, (TD_BRIDGE_SEGS + 1) * 2, haloTight);
    for (int i = 0; i <= TD_BRIDGE_SEGS; i++) {
        glowStrip[i * 2]     = center[i];
        glowStrip[i * 2 + 1] = (Vector2){ center[i].x - nrm.x * halfWBot[i] * haloExt[i], center[i].y - nrm.y * halfWBot[i] * haloExt[i] };
    }
    DrawTriangleStrip(glowStrip, (TD_BRIDGE_SEGS + 1) * 2, haloTight);

    static Vector2 ringPts[TD_BRIDGE_RINGS][TD_BRIDGE_SEGS + 1];
    static Vector2 ringUV[TD_BRIDGE_RINGS][TD_BRIDGE_SEGS + 1];

    unsigned char imgAlpha = (unsigned char)(230.0f * TD_Smooth01(strength / 0.15f));

    rlDisableBackfaceCulling();

    for (int side = 0; side < 2; side++) {
        float sideSign = side == 0 ? 1.0f : -1.0f;

        for (int ring = 0; ring < TD_BRIDGE_RINGS; ring++) {
            float rt = (float)ring / (float)(TD_BRIDGE_RINGS - 1);
            float sampleT = powf(rt, gamma);

            for (int i = 0; i <= TD_BRIDGE_SEGS; i++) {
                float hw = side == 0 ? halfWTop[i] : halfWBot[i];
                ringPts[ring][i] = (Vector2){
                    center[i].x + nrm.x * hw * rt * sideSign,
                    center[i].y + nrm.y * hw * rt * sideSign };

                Vector2 samplePos = {
                    center[i].x + nrm.x * hw * sampleT * TD_SHARED_LENS_ZOOM * sideSign,
                    center[i].y + nrm.y * hw * sampleT * TD_SHARED_LENS_ZOOM * sideSign };

                float u = (samplePos.x - capMinX) / capW;
                float v = (samplePos.y - capMinY) / capH;
                if (u < 0.0f) u = 0.0f; else if (u > 1.0f) u = 1.0f;
                if (v < 0.0f) v = 0.0f; else if (v > 1.0f) v = 1.0f;
                ringUV[ring][i] = (Vector2){ u, v };
            }
        }

        rlSetTexture(lensTex.id);
        rlBegin(RL_QUADS);
            rlColor4ub(255, 255, 255, imgAlpha);
            for (int ring = 0; ring < TD_BRIDGE_RINGS - 1; ring++) {
                for (int i = 0; i < TD_BRIDGE_SEGS; i++) {
                    rlTexCoord2f(ringUV[ring][i].x, ringUV[ring][i].y);
                    rlVertex2f(ringPts[ring][i].x, ringPts[ring][i].y);

                    rlTexCoord2f(ringUV[ring][i + 1].x, ringUV[ring][i + 1].y);
                    rlVertex2f(ringPts[ring][i + 1].x, ringPts[ring][i + 1].y);

                    rlTexCoord2f(ringUV[ring + 1][i + 1].x, ringUV[ring + 1][i + 1].y);
                    rlVertex2f(ringPts[ring + 1][i + 1].x, ringPts[ring + 1][i + 1].y);

                    rlTexCoord2f(ringUV[ring + 1][i].x, ringUV[ring + 1][i].y);
                    rlVertex2f(ringPts[ring + 1][i].x, ringPts[ring + 1][i].y);
                }
            }
        rlEnd();
        rlSetTexture(0);

        for (int ring = 0; ring < TD_BRIDGE_RINGS - 1; ring++) {
            float t0 = (float)ring / (float)(TD_BRIDGE_RINGS - 1);
            float t1 = (float)(ring + 1) / (float)(TD_BRIDGE_RINGS - 1);
            float tm = (t0 + t1) * 0.5f;
            float shade = powf(tm, 2.2f);
            float baseA = 8.0f + shade * 60.0f;

            for (int i = 0; i < TD_BRIDGE_SEGS; i++) {
                float segT = ((float)i + 0.5f) / (float)TD_BRIDGE_SEGS;
                float fade = TD_BridgeEdgeFade(segT, fadeSpan0, fadeSpan1);
                Color band = tint;
                band.a = (unsigned char)(baseA * fade);
                DrawTriangle(ringPts[ring][i], ringPts[ring][i + 1], ringPts[ring + 1][i + 1], band);
                DrawTriangle(ringPts[ring][i], ringPts[ring + 1][i + 1], ringPts[ring + 1][i], band);
            }
        }
    }

    rlEnableBackfaceCulling();

}

static void TD_DrawSlimeBridgeSpecular(Vector2 c0, float r0, float rot0, float phase0, float elongate0,
                                        Vector2 c1, float r1, float rot1, float phase1, float elongate1,
                                        Color tint, float strength, float time) {
    if (strength <= 0.02f) return;
    if (strength > 1.0f) strength = 1.0f;

    float dx = c1.x - c0.x;
    float dy = c1.y - c0.y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 1.0f) dist = 1.0f;
    Vector2 dir = { dx / dist, dy / dist };
    Vector2 nrm = { -dir.y, dir.x };

    float edge0Top = TD_BlobRadiusAtDir(r0, rot0, elongate0, time, phase0, nrm) / r0;
    float edge1Top = TD_BlobRadiusAtDir(r1, rot1, elongate1, time, phase1, nrm) / r1;

    float fadeSpan0 = (r0 * 0.5f) / dist; if (fadeSpan0 < 0.05f) fadeSpan0 = 0.05f; if (fadeSpan0 > 0.45f) fadeSpan0 = 0.45f;
    float fadeSpan1 = (r1 * 0.5f) / dist; if (fadeSpan1 < 0.05f) fadeSpan1 = 0.05f; if (fadeSpan1 > 0.45f) fadeSpan1 = 0.45f;

    float t = 0.5f;
    float fade = TD_BridgeEdgeFade(t, fadeSpan0, fadeSpan1);

    Vector2 p = { c0.x + dx * t, c0.y + dy * t };
    float baseR = r0 + (r1 - r0) * t;
    float waistShape = sinf(t * PI);
    float waist = 1.0f - (1.0f - strength) * 0.45f * waistShape;
    if (waist < 0.55f) waist = 0.55f;
    float noise = 0.05f * strength * sinf(t * 9.0f + time * 3.0f);
    float edgeTopFrac = edge0Top + (edge1Top - edge0Top) * t;
    float hwTop = baseR * (waist + noise) * edgeTopFrac;
    if (hwTop < 1.5f) hwTop = 1.5f;

    Vector2 sp = { p.x + nrm.x * hwTop * 0.85f, p.y + nrm.y * hwTop * 0.85f };
    Color specColor = WHITE; specColor.a = (unsigned char)(180.0f * strength * fade);
    TD_DrawSoftGlint(sp, hwTop * 0.26f, hwTop * 0.12f, specColor);
}

static void TD_DrawSlimeDrips(Vector2 c0, float r0, Vector2 c1, float r1, Color tint, float strength, float time) {
    if (strength <= 0.0f) return;
    if (strength > 1.0f) strength = 1.0f;

    float dripP = TD_Smooth01((strength - 0.35f) / 0.35f);
    if (dripP <= 0.0f) return;

    float dx = c1.x - c0.x;
    float dy = c1.y - c0.y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 1.0f) dist = 1.0f;
    Vector2 nrm = { -dy / dist, dx / dist };

    float dripT[2] = { 0.4f, 0.6f };
    for (int k = 0; k < 2; k++) {
        float t = dripT[k];
        Vector2 p = { c0.x + dx * t, c0.y + dy * t };
        float baseR = r0 + (r1 - r0) * t;
        float sway = sinf(time * 2.0f + (float)k * 3.1f) * baseR * 0.08f;
        float drop = baseR * 0.35f * dripP;
        Vector2 dripCenter = { p.x + nrm.x * sway, p.y + nrm.y * drop };
        float dripR = baseR * 0.16f * dripP;
        Color dripColor = tint; dripColor.a = (unsigned char)(140.0f * dripP);
        TD_DrawSoftGlint(dripCenter, dripR, dripR, dripColor);

        Vector2 dripSpec = { dripCenter.x - dripR * 0.35f, dripCenter.y - dripR * 0.35f };
        Color dripSpecColor = WHITE; dripSpecColor.a = (unsigned char)(80.0f * dripP);
        TD_DrawSoftGlint(dripSpec, dripR * 0.4f, dripR * 0.4f, dripSpecColor);
    }
}

void TouchdesignerEffect_Draw(RenderTexture2D scene, int screenW, int screenH) {
    RenderTexture2D effScene = TD_GetEffectiveScene(scene, screenW, screenH);

    if (TD_g_params.autoDetectHands) {
        TD_DetectHands(effScene, screenW, screenH);
    }

    bool useCamera = TD_g_params.showCameraBg && !TD_IsSceneBlack(effScene);
    if (useCamera) {
        DrawTexturePro(effScene.texture,
            (Rectangle){ 0, 0, (float)effScene.texture.width, -(float)effScene.texture.height },
            (Rectangle){ 0, 0, (float)screenW, (float)screenH },
            (Vector2){ 0, 0 }, 0.0f, WHITE);
    } else {
        ClearBackground(TD_g_params.bgFallbackColor);
    }

    Vector2 centers[TD_MAX_HANDS] = {0};
    float   radii[TD_MAX_HANDS] = {0};
    float   rotations[TD_MAX_HANDS] = {0};
    bool    present[TD_MAX_HANDS] = {0};
    int visible = 0;

    for (int i = 0; i < TD_MAX_HANDS; i++) {
        TD_Hand *h = &TD_g_hands[i];
        present[i] = h->present;
        if (!h->present) continue;
        visible++;

        float radius = h->size * TD_g_params.glassSize * (float)screenW * 0.5f;
        if (radius < 12.0f) radius = 12.0f;
        centers[i] = (Vector2){ h->x * screenW, h->y * screenH };
        radii[i] = radius;
        rotations[i] = h->rotation;
    }

    static float TD_s_elongateSmooth[TD_MAX_HANDS] = { TD_BLOB_ELONGATE, TD_BLOB_ELONGATE };
    static float TD_s_jiggleTime[TD_MAX_HANDS] = { -1.0f, -1.0f };
    static float TD_s_jiggleAmp[TD_MAX_HANDS] = { 0.0f, 0.0f };
    static bool  TD_s_wasBridged = false;
    float elongateTarget[TD_MAX_HANDS] = { TD_BLOB_ELONGATE, TD_BLOB_ELONGATE };
    float elongate[TD_MAX_HANDS];

    float bridgeStrength = 0.0f;
    float capMinX = 0.0f, capMinY = 0.0f, capMaxX = 0.0f, capMaxY = 0.0f;
    bool hasBridgeRect = false;
    Vector2 bridgeCenter0 = {0}, bridgeCenter1 = {0};
    if (TD_g_params.slimeEnabled && present[0] && present[1]) {
        float dx = centers[1].x - centers[0].x;
        float dy = centers[1].y - centers[0].y;
        float dist = sqrtf(dx * dx + dy * dy);
        float maxDist = (radii[0] + radii[1]) * TD_g_params.slimeDistance;
        bridgeStrength = maxDist > 0.0f ? (1.0f - dist / maxDist) : 0.0f;
        if (bridgeStrength > 0.0f) {
            float taut = 1.0f - bridgeStrength;
            float dynElongate = 1.0f + taut * (TD_BLOB_ELONGATE_TAUT - 1.0f);
            elongateTarget[0] = dynElongate;
            elongateTarget[1] = dynElongate;

            float minX = fminf(centers[0].x - radii[0] * 1.55f, centers[1].x - radii[1] * 1.55f);
            float maxX = fmaxf(centers[0].x + radii[0] * 1.55f, centers[1].x + radii[1] * 1.55f);
            float minY = fminf(centers[0].y - radii[0] * 1.55f, centers[1].y - radii[1] * 1.55f);
            float maxY = fmaxf(centers[0].y + radii[0] * 1.55f, centers[1].y + radii[1] * 1.55f);
            if (minX < 0.0f) minX = 0.0f;
            if (minY < 0.0f) minY = 0.0f;
            if (maxX > (float)screenW) maxX = (float)screenW;
            if (maxY > (float)screenH) maxY = (float)screenH;
            capMinX = minX; capMinY = minY; capMaxX = maxX; capMaxY = maxY;
            hasBridgeRect = true;
        }
    }

    const float TD_ELONGATE_SMOOTH_RATE = 8.0f;
    const float TD_JIGGLE_FREQ = 18.0f;
    const float TD_JIGGLE_DECAY = 9.0f;
    const float TD_JIGGLE_DURATION = 1.2f;

    if (hasBridgeRect) {
        float smoothFactor = 1.0f - expf(-TD_ELONGATE_SMOOTH_RATE * TD_g_dt);
        for (int i = 0; i < TD_MAX_HANDS; i++) {
            TD_s_elongateSmooth[i] += (elongateTarget[i] - TD_s_elongateSmooth[i]) * smoothFactor;
            elongate[i] = TD_s_elongateSmooth[i];
            TD_s_jiggleTime[i] = -1.0f;
        }
    } else {
        for (int i = 0; i < TD_MAX_HANDS; i++) {
            if (TD_s_wasBridged) {
                TD_s_jiggleAmp[i] = TD_s_elongateSmooth[i] - TD_BLOB_ELONGATE;
                TD_s_jiggleTime[i] = 0.0f;
                TD_s_elongateSmooth[i] = TD_BLOB_ELONGATE;
            }

            float wobble = 0.0f;
            if (TD_s_jiggleTime[i] >= 0.0f) {
                wobble = TD_s_jiggleAmp[i] * expf(-TD_JIGGLE_DECAY * TD_s_jiggleTime[i]) *
                          cosf(TD_JIGGLE_FREQ * TD_s_jiggleTime[i]);
                TD_s_jiggleTime[i] += TD_g_dt;
                if (TD_s_jiggleTime[i] > TD_JIGGLE_DURATION) TD_s_jiggleTime[i] = -1.0f;
            }
            elongate[i] = TD_BLOB_ELONGATE + wobble;
        }
    }
    TD_s_wasBridged = hasBridgeRect;

    if (hasBridgeRect) {
        TD_CaptureBridgeLensRegion(effScene, screenW, screenH, capMinX, capMinY, capMaxX, capMaxY);

        float bdx = centers[1].x - centers[0].x;
        float bdy = centers[1].y - centers[0].y;
        float bdist = sqrtf(bdx * bdx + bdy * bdy);
        if (bdist < 1.0f) bdist = 1.0f;
        Vector2 bnrm = { -bdy / bdist, bdx / bdist };

        const float topSagFactor = -0.14f;
        float topSag = (radii[0] + radii[1]) * topSagFactor;
        const float redOffsetFactor = 0.44f;
        float redOffset = (radii[0] + radii[1]) * redOffsetFactor;
        Vector2 redC0 = { centers[0].x + bnrm.x * redOffset, centers[0].y + bnrm.y * redOffset };
        Vector2 redC1 = { centers[1].x + bnrm.x * redOffset, centers[1].y + bnrm.y * redOffset };
        TD_DrawSlimeBridge(redC0, radii[0], rotations[0], 0.0f, elongate[0],
                            redC1, radii[1], rotations[1], 2.4f, elongate[1],
                            TD_g_params.glassColor, bridgeStrength, TD_g_time, topSag,
                            TD_g_bridgeLensCapture.texture, capMinX, capMinY, capMaxX, capMaxY);

        const float archOffsetFactor = 0.14f;
        const float archSagFactor = 0.04f;
        float archOffset = (radii[0] + radii[1]) * archOffsetFactor;
        Vector2 archC0 = { centers[0].x + bnrm.x * archOffset, centers[0].y + bnrm.y * archOffset };
        Vector2 archC1 = { centers[1].x + bnrm.x * archOffset, centers[1].y + bnrm.y * archOffset };
        float archSag = (radii[0] + radii[1]) * archSagFactor;
        TD_DrawSlimeBridge(archC0, radii[0], rotations[0], 0.0f, elongate[0],
                            archC1, radii[1], rotations[1], 2.4f, elongate[1],
                            TD_g_params.glassColor, bridgeStrength, TD_g_time, -archSag,
                            TD_g_bridgeLensCapture.texture, capMinX, capMinY, capMaxX, capMaxY);

        const float centerLiftFactor = 0.22f;
        float centerLift = (radii[0] + radii[1]) * centerLiftFactor;
        bridgeCenter0 = (Vector2){ centers[0].x - bnrm.x * centerLift, centers[0].y - bnrm.y * centerLift };
        bridgeCenter1 = (Vector2){ centers[1].x - bnrm.x * centerLift, centers[1].y - bnrm.y * centerLift };
    }

    if (TD_g_params.glassEnabled) {
        for (int i = 0; i < TD_MAX_HANDS; i++) {
            if (!present[i]) continue;

            Vector2 mergeDir = { 0, 0 };
            float mergeStrength = 0.0f;
            if (bridgeStrength > 0.0f) {
                int other = i == 0 ? 1 : 0;
                float mdx = centers[other].x - centers[i].x;
                float mdy = centers[other].y - centers[i].y;
                float mdist = sqrtf(mdx * mdx + mdy * mdy);
                if (mdist > 0.001f) {
                    mergeDir = (Vector2){ mdx / mdist, mdy / mdist };
                    mergeStrength = bridgeStrength;
                }
            }

            if (bridgeStrength > 0.0f) {

                TD_DrawHandGlass(TD_g_bridgeLensCapture.texture, 0.0f, centers[i], radii[i], rotations[i],
                                  TD_g_params.glassColor, TD_g_time, (float)i * 2.4f, elongate[i],
                                  mergeDir, mergeStrength,
                                  true, capMinX, capMinY, capMaxX, capMaxY);
            } else {

                float captureRadius = radii[i] * 0.62f;
                if (captureRadius < 8.0f) captureRadius = 8.0f;

                TD_CaptureLensRegion(i, effScene, screenW, screenH, centers[i], captureRadius);
                TD_DrawHandGlass(TD_g_lensCapture[i].texture, captureRadius, centers[i], radii[i], rotations[i],
                                  TD_g_params.glassColor, TD_g_time, (float)i * 2.4f, elongate[i],
                                  mergeDir, mergeStrength,
                                  false, 0.0f, 0.0f, 0.0f, 0.0f);
            }
        }

        if (bridgeStrength > 0.0f) {
            TD_DrawSlimeBridgeSpecular(bridgeCenter0, radii[0], rotations[0], 0.0f, elongate[0],
                                        bridgeCenter1, radii[1], rotations[1], 2.4f, elongate[1],
                                        TD_g_params.glassColor, bridgeStrength, TD_g_time);
            TD_DrawSlimeDrips(bridgeCenter0, radii[0], bridgeCenter1, radii[1],
                               TD_g_params.glassColor, bridgeStrength, TD_g_time);
        }
    }

    if (TD_g_params.showHandCount) {
        char label[32];
        snprintf(label, sizeof(label), "hands: %d", visible);
        DrawText(label, 8, 22, 20, TD_g_params.glassColor);
    }
}

void TouchdesignerEffect_Unload(void) {
    if (TD_g_handReadReady) { UnloadRenderTexture(TD_g_handReadTarget); TD_g_handReadReady = false; }
    if (TD_s_mirrorReady) { UnloadRenderTexture(TD_s_mirrorTarget); TD_s_mirrorReady = false; }
    for (int i = 0; i < TD_MAX_HANDS; i++) {
        if (TD_g_lensCaptureReady[i]) { UnloadRenderTexture(TD_g_lensCapture[i]); TD_g_lensCaptureReady[i] = false; }
    }
    if (TD_g_bridgeLensCaptureReady) { UnloadRenderTexture(TD_g_bridgeLensCapture); TD_g_bridgeLensCaptureReady = false; }
    TD_HandTracking_Unload();
}
#endif

#endif

#if defined(TOUCHDESIGNER_EFFECT_IMPLEMENTATION) && !defined(TOUCHDESIGNER_EFFECT_IMPLEMENTATION_INCLUDED)
#define TOUCHDESIGNER_EFFECT_IMPLEMENTATION_INCLUDED

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/video/tracking.hpp>

#include <vector>
#include <cstring>
#include <cmath>
#include <algorithm>

static const int PALM_INPUT_SIZE = 192;
static const int PALM_NUM_LANDMARKS = 7;

struct HandParams {
    bool mirror;
    float scoreThreshold;
    float nmsThreshold;
};

static HandParams g_params = {
    .mirror = true,
    .scoreThreshold = 0.5f,
    .nmsThreshold = 0.3f,
};

static std::vector<Hand> g_detectedHands;
static bool g_initialized = false;

static cv::dnn::Net g_palmNet;
static bool g_palmNetAttempted = false;
static bool g_palmNetOk = false;
static std::vector<cv::Point2f> g_anchors;

#ifdef __EMSCRIPTEN__
static uint8_t *g_palmModelBuffer = NULL;
static size_t g_palmModelBufferSize = 0;
#endif

#ifndef __EMSCRIPTEN__
static cv::VideoCapture g_camera;
#endif

static std::vector<cv::Point2f> BuildPalmAnchors(int inputSize) {
    std::vector<cv::Point2f> anchors;
    struct Layer { int stride; int anchorsPerCell; };
    const Layer layers[] = { { 8, 2 }, { 16, 6 } };

    for (const auto &layer : layers) {
        int gridSize = inputSize / layer.stride;
        for (int y = 0; y < gridSize; y++) {
            float cy = (y + 0.5f) / (float)gridSize;
            for (int x = 0; x < gridSize; x++) {
                float cx = (x + 0.5f) / (float)gridSize;
                for (int a = 0; a < layer.anchorsPerCell; a++) {
                    anchors.push_back(cv::Point2f(cx, cy));
                }
            }
        }
    }
    return anchors;
}

static cv::Mat PreprocessPalm(const cv::Mat &bgr, cv::Point2i &padBias, float &ratio) {
    ratio = std::min((float)PALM_INPUT_SIZE / bgr.cols, (float)PALM_INPUT_SIZE / bgr.rows);

    cv::Mat processed;
    padBias = cv::Point2i(0, 0);

    if (bgr.cols != PALM_INPUT_SIZE || bgr.rows != PALM_INPUT_SIZE) {
        cv::Size ratioSize((int)(bgr.cols * ratio), (int)(bgr.rows * ratio));
        if (ratioSize.width < 1) ratioSize.width = 1;
        if (ratioSize.height < 1) ratioSize.height = 1;
        cv::resize(bgr, processed, ratioSize);

        int padH = PALM_INPUT_SIZE - ratioSize.height;
        int padW = PALM_INPUT_SIZE - ratioSize.width;
        padBias.x = padW / 2;
        padBias.y = padH / 2;

        cv::copyMakeBorder(processed, processed, padBias.y, padH - padBias.y,
                            padBias.x, padW - padBias.x, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    } else {
        processed = bgr;
    }

    cv::dnn::Image2BlobParams params;
    params.datalayout = cv::dnn::DNN_LAYOUT_NHWC;
    params.ddepth = CV_32F;
    params.mean = cv::Scalar::all(0);
    params.scalefactor = cv::Scalar::all(1.0 / 255.0);
    params.size = cv::Size(PALM_INPUT_SIZE, PALM_INPUT_SIZE);
    params.swapRB = true;
    params.paddingmode = cv::dnn::DNN_PMODE_NULL;

    padBias.x = (int)(padBias.x / ratio);
    padBias.y = (int)(padBias.y / ratio);

    return cv::dnn::blobFromImageWithParams(processed, params);
}

static void PostprocessPalm(const std::vector<cv::Mat> &outputs, const cv::Size &originalSize,
                             const cv::Point2i &padBias, std::vector<Hand> &out) {
    out.clear();
    if (outputs.size() < 2) return;

    cv::Mat boxes = outputs[0].reshape(1, (int)(outputs[0].total() / 18));
    cv::Mat scores = outputs[1].reshape(1, (int)(outputs[1].total() / 1));

    if (boxes.rows != (int)g_anchors.size() || scores.rows != (int)g_anchors.size()) {
        return;
    }

    float scale = (float)std::max(originalSize.width, originalSize.height);

    std::vector<float> scoreVec;
    std::vector<cv::Rect2f> boxVec;
    std::vector<std::vector<cv::Point2f>> landmarkVec;

    for (int i = 0; i < scores.rows; i++) {
        float score = 1.0f / (1.0f + std::exp(-scores.at<float>(i, 0)));
        if (score <= g_params.scoreThreshold) continue;

        cv::Mat boxDelta = boxes.row(i).colRange(0, 4);
        cv::Mat landmarkDelta = boxes.row(i).colRange(4, 18);
        const cv::Point2f &anchor = g_anchors[i];

        cv::Point2f cxyDelta(boxDelta.at<float>(0) / PALM_INPUT_SIZE, boxDelta.at<float>(1) / PALM_INPUT_SIZE);
        cv::Point2f whDelta(boxDelta.at<float>(2) / PALM_INPUT_SIZE, boxDelta.at<float>(3) / PALM_INPUT_SIZE);

        cv::Point2f xy1((cxyDelta.x - whDelta.x / 2 + anchor.x) * scale - padBias.x,
                        (cxyDelta.y - whDelta.y / 2 + anchor.y) * scale - padBias.y);
        cv::Point2f xy2((cxyDelta.x + whDelta.x / 2 + anchor.x) * scale - padBias.x,
                        (cxyDelta.y + whDelta.y / 2 + anchor.y) * scale - padBias.y);

        scoreVec.push_back(score);
        boxVec.push_back(cv::Rect2f(xy1.x, xy1.y, xy2.x - xy1.x, xy2.y - xy1.y));

        std::vector<cv::Point2f> landmarks;
        landmarks.reserve(PALM_NUM_LANDMARKS);
        for (int j = 0; j < PALM_NUM_LANDMARKS; j++) {
            float dx = landmarkDelta.at<float>(j * 2) / PALM_INPUT_SIZE + anchor.x;
            float dy = landmarkDelta.at<float>(j * 2 + 1) / PALM_INPUT_SIZE + anchor.y;
            landmarks.push_back(cv::Point2f(dx * scale - padBias.x, dy * scale - padBias.y));
        }
        landmarkVec.push_back(landmarks);
    }

    std::vector<cv::Rect> boxesInt;
    boxesInt.reserve(boxVec.size());
    for (const auto &b : boxVec) {
        boxesInt.push_back(cv::Rect((int)b.x, (int)b.y, (int)b.width, (int)b.height));
    }

    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxesInt, scoreVec, g_params.scoreThreshold, g_params.nmsThreshold, indices);

    out.reserve(indices.size());
    for (int idx : indices) {
        const cv::Rect2f &b = boxVec[idx];
        const auto &lm = landmarkVec[idx];

        Hand hand = {0};
        hand.pos = (Vector2){ b.x + b.width / 2.0f, b.y + b.height / 2.0f };
        hand.palmSize = std::max(b.width, b.height);
        hand.confidence = scoreVec[idx];

        if (lm.size() >= 3) {
            float dx = lm[2].x - lm[0].x;
            float dy = lm[2].y - lm[0].y;
            hand.rotation = std::atan2(dy, dx);
        } else {
            hand.rotation = 0.0f;
        }

        hand.gesture = HAND_GESTURE_NONE;
        hand.fingerCount = 0;
        out.push_back(hand);
    }
}

static bool EnsurePalmNetLoaded(void) {
    if (g_palmNetAttempted) return g_palmNetOk;
    g_palmNetAttempted = true;

    try {
#ifdef __EMSCRIPTEN__
        if (g_palmModelBuffer && g_palmModelBufferSize > 0) {
            std::vector<uchar> buf(g_palmModelBuffer, g_palmModelBuffer + g_palmModelBufferSize);
            g_palmNet = cv::dnn::readNetFromONNX(buf);
            g_palmNetOk = !g_palmNet.empty();
            if (!g_palmNetOk) {
                fprintf(stderr, "[touchdesigner] readNetFromONNX returned an empty net\n");
            }
        } else {
            fprintf(stderr, "[touchdesigner] Palm model buffer not set. Call js_set_palm_model_data() from JavaScript first.\n");
        }
#else
        const char *localPath = "../../assets/cv/palm_detection_mediapipe_2023feb.onnx";
        g_palmNet = cv::dnn::readNet(localPath);
        g_palmNetOk = !g_palmNet.empty();
        if (!g_palmNetOk) {
            fprintf(stderr, "[touchdesigner] Could not load %s\n", localPath);
        }
#endif
    } catch (const cv::Exception &e) {
        fprintf(stderr, "[touchdesigner] cv::Exception loading palm model: %s\n", e.what());
        g_palmNetOk = false;
    }

    if (g_palmNetOk && g_anchors.empty()) {
        g_anchors = BuildPalmAnchors(PALM_INPUT_SIZE);
    }

    return g_palmNetOk;
}

static void RunPalmDetect(const cv::Mat &rgba) {
    g_detectedHands.clear();
    if (rgba.empty()) return;
    if (!EnsurePalmNetLoaded()) return;

    cv::Mat bgr;
    cv::cvtColor(rgba, bgr, cv::COLOR_RGBA2BGR);

    cv::Point2i padBias;
    float ratio;
    cv::Mat blob = PreprocessPalm(bgr, padBias, ratio);

    try {
        g_palmNet.setInput(blob);
        std::vector<cv::Mat> outputs;
        g_palmNet.forward(outputs, g_palmNet.getUnconnectedOutLayersNames());
        PostprocessPalm(outputs, bgr.size(), padBias, g_detectedHands);
    } catch (const cv::Exception &e) {
        fprintf(stderr, "[touchdesigner] cv::Exception during inference: %s\n", e.what());
    }
}

static cv::Mat g_lkPrevGray;
static bool g_lkPrevGrayValid = false;
static std::vector<cv::Point2f> g_lkPoints;
static bool g_lkPointsValid = false;

static const int   LK_WIN_SIZE  = 21;
static const int   LK_MAX_LEVEL = 2;
static const float LK_MAX_ERR   = 24.0f;
static const float LK_MAX_JUMP_FRAC = 0.35f;

static void TD_SeedTrackingFromDetections(void) {
    g_lkPoints.clear();
    size_t n = std::min(g_detectedHands.size(), (size_t)TD_MAX_HANDS);
    for (size_t i = 0; i < n; i++) {
        g_lkPoints.push_back(cv::Point2f(g_detectedHands[i].pos.x, g_detectedHands[i].pos.y));
    }
    g_lkPointsValid = true;
}

static bool TD_TrackHandsOpticalFlow(const cv::Mat &gray) {
    if (!g_lkPrevGrayValid || !g_lkPointsValid) return false;
    if (g_lkPoints.empty()) return true;
    if (gray.size() != g_lkPrevGray.size()) return false;

    std::vector<cv::Point2f> nextPts;
    std::vector<uchar> status;
    std::vector<float> err;
    cv::calcOpticalFlowPyrLK(
        g_lkPrevGray, gray, g_lkPoints, nextPts, status, err,
        cv::Size(LK_WIN_SIZE, LK_WIN_SIZE), LK_MAX_LEVEL,
        cv::TermCriteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 20, 0.03));

    float maxJump = std::max(gray.cols, gray.rows) * LK_MAX_JUMP_FRAC;
    bool anyValid = true;
    size_t n = std::min(g_lkPoints.size(), g_detectedHands.size());
    if (n == 0) return false;

    for (size_t i = 0; i < n; i++) {
        bool ok = status[i] != 0 && err[i] < LK_MAX_ERR;
        if (ok) {
            float dx = nextPts[i].x - g_lkPoints[i].x;
            float dy = nextPts[i].y - g_lkPoints[i].y;
            ok = std::sqrt(dx * dx + dy * dy) <= maxJump;
        }
        if (!ok) { anyValid = false; break; }
        g_detectedHands[i].pos = (Vector2){ nextPts[i].x, nextPts[i].y };
        g_lkPoints[i] = nextPts[i];
    }

    return anyValid;
}

void TD_HandTracking_Init(void) {
    if (g_initialized) return;
    g_initialized = true;
}

void TD_HandTracking_ProcessFrame(const unsigned char *rgba, int width, int height, bool forceReanchor) {
    if (!rgba || width <= 0 || height <= 0) return;

    cv::Mat frame(height, width, CV_8UC4, (void *)rgba);

    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_RGBA2GRAY);

    bool needsReanchor = forceReanchor || !g_lkPrevGrayValid || !g_lkPointsValid;
    if (!needsReanchor) {
        needsReanchor = !TD_TrackHandsOpticalFlow(gray);
    }

    if (needsReanchor) {
        RunPalmDetect(frame);
        TD_SeedTrackingFromDetections();
    }

    gray.copyTo(g_lkPrevGray);
    g_lkPrevGrayValid = true;
}

void TD_HandTracking_SetMirror(bool mirror) {
    g_params.mirror = mirror;
}

void TD_HandTracking_Unload(void) {
    g_detectedHands.clear();
    g_lkPoints.clear();
    g_lkPointsValid = false;
    g_lkPrevGray.release();
    g_lkPrevGrayValid = false;
    g_initialized = false;
}

extern "C" {
#ifdef __EMSCRIPTEN__
    void js_set_palm_model_data(size_t bufSize, uint8_t *buf) {
        if (g_palmModelBuffer) free(g_palmModelBuffer);
        g_palmModelBuffer = NULL;
        g_palmModelBufferSize = 0;

        if (bufSize > 0 && buf) {
            g_palmModelBuffer = (uint8_t *)malloc(bufSize);
            if (g_palmModelBuffer) {
                memcpy(g_palmModelBuffer, buf, bufSize);
                g_palmModelBufferSize = bufSize;
                g_palmNetAttempted = false;
                g_palmNetOk = false;
            } else {
                fprintf(stderr, "[touchdesigner] Failed to allocate memory for palm model buffer\n");
            }
        }
    }
#endif
}

#ifndef __EMSCRIPTEN__
bool TD_HandCamera_Open(int deviceIndex) {
    if (g_camera.isOpened()) {
        g_camera.release();
    }

    g_camera.open(deviceIndex);
    if (!g_camera.isOpened()) {
        fprintf(stderr, "[touchdesigner] Failed to open camera device %d\n", deviceIndex);
        return false;
    }

    g_camera.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    g_camera.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    g_camera.set(cv::CAP_PROP_FPS, 30);

    return true;
}

bool TD_HandCamera_IsOpen(void) {
    return g_camera.isOpened();
}

void TD_HandCamera_CaptureInto(RenderTexture2D target) {
    if (!g_camera.isOpened()) return;

    cv::Mat frame;
    g_camera >> frame;

    if (frame.empty()) return;

    if (frame.cols != target.texture.width || frame.rows != target.texture.height) {
        cv::Mat resized;
        cv::resize(frame, resized, cv::Size(target.texture.width, target.texture.height), 0, 0, cv::INTER_LINEAR);
        frame = resized;
    }

    cv::Mat rgba;
    cv::cvtColor(frame, rgba, cv::COLOR_BGR2RGBA);

    if (g_params.mirror) {
        cv::flip(rgba, rgba, 1);
    }

    RunPalmDetect(rgba);

    if (rgba.isContinuous()) {
        UpdateTexture(target.texture, rgba.data);
    }
}

void TD_HandCamera_Close(void) {
    if (g_camera.isOpened()) {
        g_camera.release();
    }
}
#endif

int TD_HandTracking_GetHandCount(void) {
    return static_cast<int>(g_detectedHands.size());
}

Hand TD_HandTracking_GetHand(int index) {
    if (index < 0 || index >= static_cast<int>(g_detectedHands.size())) {
        return (Hand){0};
    }
    return g_detectedHands[index];
}

#endif