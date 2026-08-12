/*
 * crt_effect.h — single-header CRT + VHS post-processing effect for raylib
 * Dependencia JSON solo en __EMSCRIPTEN__
 *
 * Part of Bryncraft (https://bryncraft.online/) — created by Victor Berdugo
 */

#ifndef CRT_EFFECT_H
#define CRT_EFFECT_H

#include "raylib.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifdef __EMSCRIPTEN__
#include "../../json_mini.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void CrtEffect_Init(void);
#ifdef __EMSCRIPTEN__
void CrtEffect_SetParams(const JsonValue *paramsObj);
#endif
void CrtEffect_Update(float dt);
void CrtEffect_Draw(RenderTexture2D scene, int screenW, int screenH);
void CrtEffect_Prepare(RenderTexture2D scene, int screenW, int screenH);
void CrtEffect_DrawFinal(RenderTexture2D scene, int screenW, int screenH);
void CrtEffect_Unload(void);

#ifdef __cplusplus
}
#endif

#define CRT_VHS_ICON_NONE  0
#define CRT_VHS_ICON_PLAY  1
#define CRT_VHS_ICON_PAUSE 2
#define CRT_VHS_ICON_REW   3
#define CRT_VHS_ICON_FF    4
#define CRT_VHS_ICON_STOP  5
#define CRT_VHS_ICON_REC   6

typedef struct {
	float scanlineIntensity, scanlineCount, scanlineSpeed;
	float curvature, vignette, noise, chromaticAberration, flicker;

	float trackingGlitch;
	float waveDistortion;
	float waveSpeed;
	float dropoutLines;
	float jitter;
	float verticalRoll;
	float ghosting;

	bool  vhsOverlay;
	int   vhsIcon;
	char  vhsTimestamp[32];
	char  vhsLabel[8];
} CRT_Params;

static CRT_Params CRT_g_params = {
	0.35f, 480.0f, 0.0f, 0.15f, 0.3f, 0.05f, 0.4f, 0.1f,
	0.0f, 0.0f, 1.5f, 0.0f, 0.0f, 0.0f, 0.0f,
	false, CRT_VHS_ICON_NONE, "", "SP"
};

static Shader CRT_g_shader;
static bool CRT_g_shaderLoaded = false;
static bool CRT_g_shaderIsExternal = false;
static float CRT_g_time = 0.0f;

static int CRT_g_locTime, CRT_g_locScanlineIntensity, CRT_g_locScanlineCount, CRT_g_locScanlineSpeed;
static int CRT_g_locCurvature, CRT_g_locVignette, CRT_g_locNoise, CRT_g_locAberration, CRT_g_locFlicker;
static int CRT_g_locTrackingGlitch, CRT_g_locWaveDistortion, CRT_g_locWaveSpeed;
static int CRT_g_locDropoutLines, CRT_g_locJitter, CRT_g_locVerticalRoll;
#define CRT_GHOST_COUNT 10
#define CRT_GHOST_SAMPLE_INTERVAL 0.07f

static int CRT_g_locGhostTex[CRT_GHOST_COUNT];
static int CRT_g_locGhostOffset[CRT_GHOST_COUNT];
static int CRT_g_locGhostAlpha[CRT_GHOST_COUNT];

static RenderTexture2D CRT_g_ghostHistory[CRT_GHOST_COUNT];
static bool  CRT_g_ghostSlotReady[CRT_GHOST_COUNT];
static int   CRT_g_ghostHead = 0;
static int   CRT_g_ghostFilled = 0;
static float CRT_g_ghostSampleTimer = 0.0f;
static float CRT_g_lastDt = 0.0f;

static const char *CRT_FS_SOURCE =
	"#version 100\n"
	"precision mediump float;\n"
	"varying vec2 fragTexCoord;\n"
	"varying vec4 fragColor;\n"
	"uniform sampler2D texture0;\n"
	"uniform sampler2D textureGhost[10];\n"
	"uniform vec2 uGhostOffset[10];\n"
	"uniform float uGhostAlpha[10];\n"
	"uniform float uTime;\n"
	"uniform float uScanlineIntensity;\n"
	"uniform float uScanlineCount;\n"
	"uniform float uScanlineSpeed;\n"
	"uniform float uCurvature;\n"
	"uniform float uVignette;\n"
	"uniform float uNoise;\n"
	"uniform float uAberration;\n"
	"uniform float uFlicker;\n"
	"uniform float uTrackingGlitch;\n"
	"uniform float uWaveDistortion;\n"
	"uniform float uWaveSpeed;\n"
	"uniform float uDropoutLines;\n"
	"uniform float uJitter;\n"
	"uniform float uVerticalRoll;\n"
	"float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233)))*43758.5453); }\n"
	"vec2 barrel(vec2 uv, float amount) { vec2 cc = uv-0.5; return uv + cc * dot(cc,cc) * amount; }\n"
	"void main() {\n"
	"    vec2 uvBase = barrel(fragTexCoord, uCurvature);\n"
	"    if (uvBase.x<0.0 || uvBase.x>1.0 || uvBase.y<0.0 || uvBase.y>1.0) { gl_FragColor=vec4(0.0); return; }\n"
	"    vec2 uv = uvBase;\n"
	"    uv.y = fract(uv.y + uTime*uVerticalRoll);\n"
	"    float trackBandY = fract(uTime*0.15 + rand(vec2(floor(uTime*0.37), 0.0))*0.7);\n"
	"    float trackBandH = 0.02 + rand(vec2(floor(uTime*0.53), 1.0))*0.05;\n"
	"    if (uTrackingGlitch > 0.0 && abs(uv.y - trackBandY) < trackBandH) {\n"
	"        float shiftN = rand(vec2(floor(uv.y*60.0), floor(uTime*24.0))) - 0.5;\n"
	"        uv.x += shiftN * uTrackingGlitch * 0.2;\n"
	"    }\n"
	"    uv.x += sin(uv.y*12.0 + uTime*uWaveSpeed) * uWaveDistortion * 0.01;\n"
	"    uv.x += (rand(vec2(floor(uTime*30.0), 7.0)) - 0.5) * uJitter * 0.01;\n"
	"    float ab = uAberration*0.002;\n"
	"    float r = texture2D(texture0, uv+vec2(ab,0.0)).r;\n"
	"    vec4 center = texture2D(texture0, uv);\n"
	"    float g = center.g;\n"
	"    float b = texture2D(texture0, uv-vec2(ab,0.0)).b;\n"
	"    vec3 color = vec3(r,g,b);\n"
	"    float scanY = uv.y - uTime*uScanlineSpeed*0.2;\n"
	"    float scan = sin(scanY*uScanlineCount*3.14159)*0.5+0.5;\n"
	"    color *= mix(1.0, scan, uScanlineIntensity);\n"
	"    if (uDropoutLines > 0.0) {\n"
	"        float dropRand = rand(vec2(floor(uv.y*450.0), floor(uTime*28.0)));\n"
	"        if (dropRand > 1.0 - uDropoutLines*0.06) { color = mix(color, vec3(1.0), 0.85); }\n"
	"    }\n"
	"    color += (rand(uv*uTime)-0.5)*uNoise;\n"
	"    float d = distance(uv, vec2(0.5));\n"
	"    color *= mix(1.0, 1.0-d, uVignette);\n"
	"    color += rand(vec2(uTime,0.0))*uFlicker*0.1;\n"
	"    for (int i = 0; i < 10; i++) {\n"
	"        vec3 ghostSample = texture2D(textureGhost[i], uv - uGhostOffset[i]).rgb;\n"
	"        color = mix(color, ghostSample, uGhostAlpha[i]);\n"
	"    }\n"
	"    color = clamp(color, 0.0, 1.0);\n"
	"    gl_FragColor = vec4(color, center.a);\n"
	"}\n";

void CrtEffect_Init(void) {
	const char *externalPath = "crt.fs";
	if (FileExists(externalPath)) {
		CRT_g_shader = LoadShader(NULL, externalPath);
		CRT_g_shaderIsExternal = true;
	} else {
		CRT_g_shader = LoadShaderFromMemory(NULL, CRT_FS_SOURCE);
		CRT_g_shaderIsExternal = false;
	}
	CRT_g_shaderLoaded = (CRT_g_shader.id != 0);
	TraceLog(CRT_g_shaderLoaded ? LOG_INFO : LOG_WARNING,
		"[CrtEffect] %s crt.fs %s (id=%d)",
		CRT_g_shaderIsExternal ? "external" : "embedded",
		CRT_g_shaderLoaded ? "compiled" : "FAILED", CRT_g_shader.id);

	CRT_g_locTime              = GetShaderLocation(CRT_g_shader, "uTime");
	CRT_g_locScanlineIntensity = GetShaderLocation(CRT_g_shader, "uScanlineIntensity");
	CRT_g_locScanlineCount     = GetShaderLocation(CRT_g_shader, "uScanlineCount");
	CRT_g_locScanlineSpeed     = GetShaderLocation(CRT_g_shader, "uScanlineSpeed");
	CRT_g_locCurvature         = GetShaderLocation(CRT_g_shader, "uCurvature");
	CRT_g_locVignette          = GetShaderLocation(CRT_g_shader, "uVignette");
	CRT_g_locNoise             = GetShaderLocation(CRT_g_shader, "uNoise");
	CRT_g_locAberration        = GetShaderLocation(CRT_g_shader, "uAberration");
	CRT_g_locFlicker           = GetShaderLocation(CRT_g_shader, "uFlicker");
	CRT_g_locTrackingGlitch    = GetShaderLocation(CRT_g_shader, "uTrackingGlitch");
	CRT_g_locWaveDistortion    = GetShaderLocation(CRT_g_shader, "uWaveDistortion");
	CRT_g_locWaveSpeed         = GetShaderLocation(CRT_g_shader, "uWaveSpeed");
	CRT_g_locDropoutLines      = GetShaderLocation(CRT_g_shader, "uDropoutLines");
	CRT_g_locJitter            = GetShaderLocation(CRT_g_shader, "uJitter");
	CRT_g_locVerticalRoll      = GetShaderLocation(CRT_g_shader, "uVerticalRoll");

	for (int i = 0; i < CRT_GHOST_COUNT; i++) {
		char nameTex[24], nameOffset[24], nameAlpha[24];
		snprintf(nameTex,    sizeof(nameTex),    "textureGhost[%d]", i);
		snprintf(nameOffset, sizeof(nameOffset), "uGhostOffset[%d]", i);
		snprintf(nameAlpha,  sizeof(nameAlpha),  "uGhostAlpha[%d]",  i);
		CRT_g_locGhostTex[i]    = GetShaderLocation(CRT_g_shader, nameTex);
		CRT_g_locGhostOffset[i] = GetShaderLocation(CRT_g_shader, nameOffset);
		CRT_g_locGhostAlpha[i]  = GetShaderLocation(CRT_g_shader, nameAlpha);
	}
}

#ifdef __EMSCRIPTEN__
static int CRT_ParseVhsIcon(const char *s, int fallback) {
	if (!s) return fallback;
	if (strcmp(s, "play") == 0)  return CRT_VHS_ICON_PLAY;
	if (strcmp(s, "pause") == 0) return CRT_VHS_ICON_PAUSE;
	if (strcmp(s, "rew") == 0)   return CRT_VHS_ICON_REW;
	if (strcmp(s, "ff") == 0)    return CRT_VHS_ICON_FF;
	if (strcmp(s, "stop") == 0)  return CRT_VHS_ICON_STOP;
	if (strcmp(s, "rec") == 0)   return CRT_VHS_ICON_REC;
	if (strcmp(s, "none") == 0)  return CRT_VHS_ICON_NONE;
	return fallback;
}

void CrtEffect_SetParams(const JsonValue *paramsObj) {
	if (!paramsObj) return;
	CRT_g_params.scanlineIntensity = (float)JsonAsNumber(JsonObjectGet(paramsObj, "scanlineIntensity"), CRT_g_params.scanlineIntensity);
	CRT_g_params.scanlineCount     = (float)JsonAsNumber(JsonObjectGet(paramsObj, "scanlineCount"), CRT_g_params.scanlineCount);
	CRT_g_params.scanlineSpeed     = (float)JsonAsNumber(JsonObjectGet(paramsObj, "scanlineSpeed"), CRT_g_params.scanlineSpeed);
	CRT_g_params.curvature         = (float)JsonAsNumber(JsonObjectGet(paramsObj, "curvature"), CRT_g_params.curvature);
	CRT_g_params.vignette          = (float)JsonAsNumber(JsonObjectGet(paramsObj, "vignette"), CRT_g_params.vignette);
	CRT_g_params.noise             = (float)JsonAsNumber(JsonObjectGet(paramsObj, "noise"), CRT_g_params.noise);
	CRT_g_params.chromaticAberration = (float)JsonAsNumber(JsonObjectGet(paramsObj, "chromaticAberration"), CRT_g_params.chromaticAberration);
	CRT_g_params.flicker           = (float)JsonAsNumber(JsonObjectGet(paramsObj, "flicker"), CRT_g_params.flicker);

	CRT_g_params.trackingGlitch = (float)JsonAsNumber(JsonObjectGet(paramsObj, "trackingGlitch"), CRT_g_params.trackingGlitch);
	CRT_g_params.waveDistortion = (float)JsonAsNumber(JsonObjectGet(paramsObj, "waveDistortion"), CRT_g_params.waveDistortion);
	CRT_g_params.waveSpeed      = (float)JsonAsNumber(JsonObjectGet(paramsObj, "waveSpeed"), CRT_g_params.waveSpeed);
	CRT_g_params.dropoutLines   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "dropoutLines"), CRT_g_params.dropoutLines);
	CRT_g_params.jitter         = (float)JsonAsNumber(JsonObjectGet(paramsObj, "jitter"), CRT_g_params.jitter);
	CRT_g_params.verticalRoll   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "verticalRoll"), CRT_g_params.verticalRoll);
	CRT_g_params.ghosting       = (float)JsonAsNumber(JsonObjectGet(paramsObj, "ghosting"), CRT_g_params.ghosting);

	CRT_g_params.vhsOverlay = JsonAsBool(JsonObjectGet(paramsObj, "vhsOverlay"), CRT_g_params.vhsOverlay);
	CRT_g_params.vhsIcon = CRT_ParseVhsIcon(JsonAsString(JsonObjectGet(paramsObj, "vhsIcon"), NULL), CRT_g_params.vhsIcon);
	const char *ts = JsonAsString(JsonObjectGet(paramsObj, "vhsTimestamp"), CRT_g_params.vhsTimestamp);
	strncpy(CRT_g_params.vhsTimestamp, ts, sizeof(CRT_g_params.vhsTimestamp) - 1);
	CRT_g_params.vhsTimestamp[sizeof(CRT_g_params.vhsTimestamp) - 1] = '\0';
	const char *lbl = JsonAsString(JsonObjectGet(paramsObj, "vhsLabel"), CRT_g_params.vhsLabel);
	strncpy(CRT_g_params.vhsLabel, lbl, sizeof(CRT_g_params.vhsLabel) - 1);
	CRT_g_params.vhsLabel[sizeof(CRT_g_params.vhsLabel) - 1] = '\0';
}
#endif

void CrtEffect_Update(float dt) { CRT_g_time += dt; CRT_g_lastDt = dt; }

static void CRT_EnsureGhostSlot(int i, int screenW, int screenH) {
	if (!CRT_g_ghostSlotReady[i]) {
		CRT_g_ghostHistory[i] = LoadRenderTexture(screenW, screenH);
		CRT_g_ghostSlotReady[i] = true;
	}
}

static void CRT_UpdateGhostHistory(RenderTexture2D scene, int screenW, int screenH, float dt) {
	CRT_g_ghostSampleTimer += dt;
	if (CRT_g_ghostSampleTimer < CRT_GHOST_SAMPLE_INTERVAL) return;
	CRT_g_ghostSampleTimer = 0.0f;

	CRT_g_ghostHead = (CRT_g_ghostHead + 1) % CRT_GHOST_COUNT;
	CRT_EnsureGhostSlot(CRT_g_ghostHead, screenW, screenH);
	BeginTextureMode(CRT_g_ghostHistory[CRT_g_ghostHead]);
		DrawTextureRec(scene.texture,
			(Rectangle){ 0, 0, (float)scene.texture.width, -(float)scene.texture.height },
			(Vector2){ 0, 0 }, WHITE);
	EndTextureMode();
	if (CRT_g_ghostFilled < CRT_GHOST_COUNT) CRT_g_ghostFilled++;
}

static void CRT_DrawVhsOverlay(int screenW, int screenH) {
	(void)screenW;
	if (!CRT_g_params.vhsOverlay) return;

	const char *iconText = NULL;
	switch (CRT_g_params.vhsIcon) {
		case CRT_VHS_ICON_PLAY:  iconText = "> PLAY";   break;
		case CRT_VHS_ICON_PAUSE: iconText = "|| PAUSE"; break;
		case CRT_VHS_ICON_REW:   iconText = "<< REW";   break;
		case CRT_VHS_ICON_FF:    iconText = ">> FF";    break;
		case CRT_VHS_ICON_STOP:  iconText = "[] STOP";  break;
		case CRT_VHS_ICON_REC:   iconText = "* REC";    break;
		default: break;
	}

	int pad = 18;
	float blink = sinf((float)GetTime() * 3.0f) * 0.5f + 0.5f;
	Color fg = (Color){ 255, 255, 255, (unsigned char)(150 + 105 * blink) };
	Color shadow = (Color){ 0, 0, 0, 160 };

	if (iconText) {
		int fontSize = 22;
		DrawText(iconText, pad + 2, pad + 2, fontSize, shadow);
		DrawText(iconText, pad, pad, fontSize, fg);
		if (CRT_g_params.vhsLabel[0] != '\0') {
			char label[16];
			snprintf(label, sizeof(label), "[%s]", CRT_g_params.vhsLabel);
			DrawText(label, pad + 2, pad + fontSize + 6, fontSize - 4, shadow);
			DrawText(label, pad, pad + fontSize + 4, fontSize - 4, (Color){ 255, 255, 255, 220 });
		}
	}

	if (CRT_g_params.vhsTimestamp[0] != '\0') {
		int tsSize = 20;
		int y = screenH - pad - tsSize;
		DrawText(CRT_g_params.vhsTimestamp, pad + 2, y + 2, tsSize, shadow);
		DrawText(CRT_g_params.vhsTimestamp, pad, y, tsSize, (Color){ 255, 255, 255, 220 });
	}
}

void CrtEffect_Prepare(RenderTexture2D scene, int screenW, int screenH) {
	if (!CRT_g_shaderLoaded) return;
	if (CRT_g_params.ghosting > 0.0f) CRT_UpdateGhostHistory(scene, screenW, screenH, CRT_g_lastDt);
}

void CrtEffect_DrawFinal(RenderTexture2D scene, int screenW, int screenH) {
	if (!CRT_g_shaderLoaded) {
		DrawTextureRec(scene.texture,
			(Rectangle){0,0,(float)scene.texture.width,-(float)scene.texture.height},
			(Vector2){0,0}, WHITE);
		CRT_DrawVhsOverlay(screenW, screenH);
		return;
	}

	bool useGhost = CRT_g_params.ghosting > 0.0f;

	SetShaderValue(CRT_g_shader, CRT_g_locTime, &CRT_g_time, SHADER_UNIFORM_FLOAT);
	SetShaderValue(CRT_g_shader, CRT_g_locScanlineIntensity, &CRT_g_params.scanlineIntensity, SHADER_UNIFORM_FLOAT);
	SetShaderValue(CRT_g_shader, CRT_g_locScanlineCount, &CRT_g_params.scanlineCount, SHADER_UNIFORM_FLOAT);
	SetShaderValue(CRT_g_shader, CRT_g_locScanlineSpeed, &CRT_g_params.scanlineSpeed, SHADER_UNIFORM_FLOAT);
	SetShaderValue(CRT_g_shader, CRT_g_locCurvature, &CRT_g_params.curvature, SHADER_UNIFORM_FLOAT);
	SetShaderValue(CRT_g_shader, CRT_g_locVignette, &CRT_g_params.vignette, SHADER_UNIFORM_FLOAT);
	SetShaderValue(CRT_g_shader, CRT_g_locNoise, &CRT_g_params.noise, SHADER_UNIFORM_FLOAT);
	SetShaderValue(CRT_g_shader, CRT_g_locAberration, &CRT_g_params.chromaticAberration, SHADER_UNIFORM_FLOAT);
	SetShaderValue(CRT_g_shader, CRT_g_locFlicker, &CRT_g_params.flicker, SHADER_UNIFORM_FLOAT);
	SetShaderValue(CRT_g_shader, CRT_g_locTrackingGlitch, &CRT_g_params.trackingGlitch, SHADER_UNIFORM_FLOAT);
	SetShaderValue(CRT_g_shader, CRT_g_locWaveDistortion, &CRT_g_params.waveDistortion, SHADER_UNIFORM_FLOAT);
	SetShaderValue(CRT_g_shader, CRT_g_locWaveSpeed, &CRT_g_params.waveSpeed, SHADER_UNIFORM_FLOAT);
	SetShaderValue(CRT_g_shader, CRT_g_locDropoutLines, &CRT_g_params.dropoutLines, SHADER_UNIFORM_FLOAT);
	SetShaderValue(CRT_g_shader, CRT_g_locJitter, &CRT_g_params.jitter, SHADER_UNIFORM_FLOAT);
	SetShaderValue(CRT_g_shader, CRT_g_locVerticalRoll, &CRT_g_params.verticalRoll, SHADER_UNIFORM_FLOAT);

	int numGhosts = 0;
	if (useGhost) {
		numGhosts = (int)(CRT_g_params.ghosting * CRT_GHOST_COUNT + 0.5f);
		if (numGhosts > CRT_g_ghostFilled) numGhosts = CRT_g_ghostFilled;
		if (numGhosts > CRT_GHOST_COUNT) numGhosts = CRT_GHOST_COUNT;
	}
	for (int i = 0; i < CRT_GHOST_COUNT; i++) {
		float alpha = 0.0f;
		float offsetUV[2] = { 0.0f, 0.0f };
		if (i < numGhosts) {
			float frac = 1.0f - (float)i / (float)CRT_GHOST_COUNT;
			alpha = 0.55f * frac * fminf(1.0f, CRT_g_params.ghosting * 1.3f);
			if (alpha < 0.0f) alpha = 0.0f;
			float offsetPx = 3.0f + i * (2.0f + CRT_g_params.ghosting * 2.0f);
			offsetUV[0] = offsetPx / (float)screenW;
			offsetUV[1] = (offsetPx * 0.2f) / (float)screenH;

			int slot = ((CRT_g_ghostHead - i) % CRT_GHOST_COUNT + CRT_GHOST_COUNT) % CRT_GHOST_COUNT;
			if (CRT_g_ghostSlotReady[slot]) {
				SetShaderValueTexture(CRT_g_shader, CRT_g_locGhostTex[i], CRT_g_ghostHistory[slot].texture);
			}
		}
		SetShaderValue(CRT_g_shader, CRT_g_locGhostAlpha[i], &alpha, SHADER_UNIFORM_FLOAT);
		SetShaderValue(CRT_g_shader, CRT_g_locGhostOffset[i], offsetUV, SHADER_UNIFORM_VEC2);
	}

	BeginShaderMode(CRT_g_shader);
	DrawTextureRec(scene.texture,
		(Rectangle){0,0,(float)scene.texture.width,-(float)scene.texture.height},
		(Vector2){0,0}, (Color){255,255,255,255});
	EndShaderMode();

	CRT_DrawVhsOverlay(screenW, screenH);
}

void CrtEffect_Draw(RenderTexture2D scene, int screenW, int screenH) {
	CrtEffect_Prepare(scene, screenW, screenH);
	CrtEffect_DrawFinal(scene, screenW, screenH);
}

void CrtEffect_Unload(void) {
	if (CRT_g_shaderLoaded) UnloadShader(CRT_g_shader);
	CRT_g_shaderLoaded = false;
	for (int i = 0; i < CRT_GHOST_COUNT; i++) {
		if (CRT_g_ghostSlotReady[i]) {
			UnloadRenderTexture(CRT_g_ghostHistory[i]);
			CRT_g_ghostSlotReady[i] = false;
		}
	}
	CRT_g_ghostHead = 0;
	CRT_g_ghostFilled = 0;
	CRT_g_ghostSampleTimer = 0.0f;
}
#endif /* CRT_EFFECT_H */