// ============================================================================
// Procedural VFX Atelier — Raylib/Emscripten renderer
//
// React owns every widget; this program owns only the canvas. It receives
// JSON effect/param messages from React (see src/lib/wasmBridge.ts) through
// js_set_effect_json(), and reports live stats back through
// js_get_stats_json(). No UI is ever drawn here.
// ============================================================================

#include "raylib.h"
#include "json_mini.h"
#include "video_export.h"
#include "effects/effect_common.h"

// Único paso manual que EFFECT_LIST (effects/effect_common.h) no puede
// automatizar: el #include del header de cada efecto. El resto del
// dispatch (enum, SetParams, Init/Update/Draw/Unload, ClearBackground
// condicional) sale solo de agregar una línea a EFFECT_LIST.
#include "effects/ascii/ascii_effect.h"
#include "effects/particles/particles_effect.h"
#include "effects/crt/crt_effect.h"
#include "effects/opencv/opencv_effect.h"
#include "effects/touchdesigner/touchdesigner_effect.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

// ============================================================================
// STATE
// ============================================================================

static int g_screenW = 1280;
static int g_screenH = 720;
static EffectKind g_activeEffect = EFFECT_ASCII;
static RenderTexture2D g_sceneTarget;
static int g_frameCount = 0;
static double g_lastStatsTime = 0.0;
static float g_lastGpuFrameTimeMs = 0.0f;
static bool g_captureLocked = false;
static int g_presentedFrames = 0;

// Video-as-source-texture bridge state (see js_set_video_frame below).
static Texture2D g_videoTexture;
static bool g_videoTextureLoaded = false;
static int g_videoTexW = 0;
static int g_videoTexH = 0;

// ============================================================================
// PROCEDURAL BASE SCENE
// ============================================================================
static void DrawWaveBand(int screenW, int screenH, float t, int i, int bands, Color color) {
	const int steps = 32;
	float baseY = screenH * ((i + 0.5f) / bands);
	float amp = screenH * (0.06f + i * 0.01f);
	float phase = t * 0.4f + i * 1.3f;

	Vector2 points[2 * (33)]; // steps + 1, times 2
	int idx = 0;
	for (int s = 0; s <= steps; s++) {
		float x = (screenW * (float)s) / steps;
		float y = baseY + sinf(x * 0.008f + phase) * amp;
		points[idx++] = (Vector2){ x, y };
		points[idx++] = (Vector2){ x, (float)screenH };
	}
	DrawTriangleStrip(points, idx, color);
}

static void DrawBaseScene(void) {
	ClearBackground((Color){ 0, 0, 0, 0 });

	if (g_videoTextureLoaded) {
		Rectangle src = { 0, 0, (float)g_videoTexture.width, (float)g_videoTexture.height };
		Rectangle dst = { 0, 0, (float)g_screenW, (float)g_screenH };
		DrawTexturePro(g_videoTexture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
		return;
	}

	float t = (float)GetTime();

	// Full-canvas vertical gradient background (no centered radial glow).
	DrawRectangleGradientV(0, 0, g_screenW, g_screenH, (Color){ 27, 58, 68, 255 }, (Color){ 11, 11, 14, 255 });

	const int bands = 5;
	for (int i = 0; i < bands; i++) {
		unsigned char alpha = (unsigned char)((0.22f - i * 0.03f) * 255.0f);
		Color c = (Color){ 68, 212, 255, alpha };
		DrawWaveBand(g_screenW, g_screenH, t, i, bands, c);
	}
}

// ============================================================================
// JS BRIDGE — EXPORTED FUNCTIONS
// ============================================================================

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void js_set_effect_json(const char *json) {
	JsonValue *root = JsonParse(json);
	if (!root) return;

	const char *effectName = JsonAsString(JsonObjectGet(root, "effect"), NULL);
	if (effectName) g_activeEffect = EffectKindFromString(effectName);

	const JsonValue *params = JsonObjectGet(root, "params");
	switch (g_activeEffect) {
#define X(ENUM, id, FnPrefix, needsClear) \
		case EFFECT_##ENUM: FnPrefix##Effect_SetParams(params); break;
							EFFECT_LIST(X)
#undef X
		default: break;
	}
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
const char *js_get_stats_json(void) {
	static char buf[256];
	double now = GetTime();
	double dt = now - g_lastStatsTime;
	g_lastStatsTime = now;
	int fps = GetFPS();

	snprintf(buf, sizeof(buf),
			"{\"fps\":%d,\"resolutionW\":%d,\"resolutionH\":%d,\"frame\":%d,\"effect\":\"%s\",\"gpuFrameTimeMs\":%.2f}",
			fps, g_screenW, g_screenH, g_frameCount, EffectKindToString(g_activeEffect), g_lastGpuFrameTimeMs);
	(void)dt;
	return buf;
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void js_start_export(int width, int height, int fps) {
	VideoExportStart(width, height);
#ifdef __EMSCRIPTEN__
	int safeFps = fps > 0 ? fps : 60;
	emscripten_set_main_loop_timing(EM_TIMING_SETTIMEOUT, 1000 / safeFps);
#else
	(void)fps;
#endif
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void js_stop_export(void) {
	VideoExportStop();
#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);
#endif
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void js_set_canvas_size(int width, int height) {
	if (width <= 0 || height <= 0) return;
	if (width == g_screenW && height == g_screenH) return;

	g_screenW = width;
	g_screenH = height;

	SetWindowSize(g_screenW, g_screenH);

	UnloadRenderTexture(g_sceneTarget);
	g_sceneTarget = LoadRenderTexture(g_screenW, g_screenH);
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void js_set_video_frame(const unsigned char *rgba, int width, int height) {
	if (!rgba || width <= 0 || height <= 0) return;

	if (g_videoTextureLoaded && (width != g_videoTexW || height != g_videoTexH)) {
		UnloadTexture(g_videoTexture);
		g_videoTextureLoaded = false;
	}

	if (!g_videoTextureLoaded) {
		Image img = {
			.data = (void *)rgba,
			.width = width,
			.height = height,
			.mipmaps = 1,
			.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
		};
		g_videoTexture = LoadTextureFromImage(img);
		g_videoTexW = width;
		g_videoTexH = height;
		g_videoTextureLoaded = true;
	} else {
		UpdateTexture(g_videoTexture, rgba);
	}
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void js_clear_video_frame(void) {
	if (g_videoTextureLoaded) {
		UnloadTexture(g_videoTexture);
		g_videoTextureLoaded = false;
	}
	g_videoTexW = 0;
	g_videoTexH = 0;
}

// ============================================================================
// FRAME CAPTURE SYNCHRONIZATION
// ============================================================================

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void js_lock_frame_capture(void) {
	g_captureLocked = true;
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void js_unlock_frame_capture(void) {
	g_captureLocked = false;
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
int js_get_presented_frame_count(void) {
	return g_presentedFrames;
}

// ============================================================================
// MAIN LOOP
// ============================================================================

static void UpdateDrawFrame(void) {
	if (g_captureLocked) {
		return;
	}

	float dt = GetFrameTime();
	double gpuStart = GetTime();

	switch (g_activeEffect) {
#define X(ENUM, id, FnPrefix, needsClear) \
		case EFFECT_##ENUM: { \
								BeginTextureMode(g_sceneTarget); \
								DrawBaseScene(); \
								EndTextureMode(); \
								BeginDrawing(); \
								if (needsClear) ClearBackground(BLANK); \
								FnPrefix##Effect_Update(dt); \
								FnPrefix##Effect_Draw(g_sceneTarget, g_screenW, g_screenH); \
								EndDrawing(); \
								g_presentedFrames++; \
								break; \
							}
		EFFECT_LIST(X)
#undef X
		default:
			break;
	}

	g_frameCount++;
	g_lastGpuFrameTimeMs = (float)((GetTime() - gpuStart) * 1000.0);
}

int main(void) {

	SetTraceLogLevel(LOG_NONE);

	SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
	InitWindow(g_screenW, g_screenH, "Procedural VFX Atelier");
	SetTargetFPS(60);

	g_sceneTarget = LoadRenderTexture(g_screenW, g_screenH);
#define X(ENUM, id, FnPrefix, needsClear) FnPrefix##Effect_Init();
	EFFECT_LIST(X)
#undef X
		VideoExportInit();

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
	while (!WindowShouldClose()) {
		UpdateDrawFrame();
	}
#endif

	VideoExportCleanup();
	if (g_videoTextureLoaded) UnloadTexture(g_videoTexture);
#define X(ENUM, id, FnPrefix, needsClear) FnPrefix##Effect_Unload();
	EFFECT_LIST(X)
#undef X
		UnloadRenderTexture(g_sceneTarget);
	CloseWindow();
	return 0;
}
