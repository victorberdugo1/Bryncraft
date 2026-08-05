/*
 * crt_effect.h — single-header CRT post-processing effect for raylib
 * Dependencia JSON solo en __EMSCRIPTEN__
 */

#ifndef CRT_EFFECT_H
#define CRT_EFFECT_H

#include "raylib.h"
#include <string.h>

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
void CrtEffect_Unload(void);

#ifdef __cplusplus
}
#endif

/* =========================================================
 * Implementación (variables y funciones con prefijo CRT_)
 * ========================================================= */
typedef struct {
    float scanlineIntensity, scanlineCount, scanlineSpeed;
    float curvature, vignette, noise, chromaticAberration, flicker;
} CRT_Params;

static CRT_Params CRT_g_params = {
    0.35f, 480.0f, 0.0f, 0.15f, 0.3f, 0.05f, 0.4f, 0.1f
};

static Shader CRT_g_shader;
static bool CRT_g_shaderLoaded = false;
static float CRT_g_time = 0.0f;
static int CRT_g_locTime, CRT_g_locScanlineIntensity, CRT_g_locScanlineCount, CRT_g_locScanlineSpeed;
static int CRT_g_locCurvature, CRT_g_locVignette, CRT_g_locNoise, CRT_g_locAberration, CRT_g_locFlicker;

static const char *CRT_FS_SOURCE =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec2 fragTexCoord;\n"
    "varying vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform float uTime;\n"
    "uniform float uScanlineIntensity;\n"
    "uniform float uScanlineCount;\n"
    "uniform float uScanlineSpeed;\n"
    "uniform float uCurvature;\n"
    "uniform float uVignette;\n"
    "uniform float uNoise;\n"
    "uniform float uAberration;\n"
    "uniform float uFlicker;\n"
    "float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233)))*43758.5453); }\n"
    "vec2 barrel(vec2 uv, float amount) { vec2 cc = uv-0.5; return uv + cc * dot(cc,cc) * amount; }\n"
    "void main() {\n"
    "    vec2 uv = barrel(fragTexCoord, uCurvature);\n"
    "    if (uv.x<0.0 || uv.x>1.0 || uv.y<0.0 || uv.y>1.0) { gl_FragColor=vec4(0.0); return; }\n"
    "    float ab = uAberration*0.002;\n"
    "    float r = texture2D(texture0, uv+vec2(ab,0.0)).r;\n"
    "    vec4 center = texture2D(texture0, uv);\n"
    "    float g = center.g;\n"
    "    float b = texture2D(texture0, uv-vec2(ab,0.0)).b;\n"
    "    vec3 color = vec3(r,g,b);\n"
    "    float scanY = uv.y - uTime*uScanlineSpeed*0.2;\n"
    "    float scan = sin(scanY*uScanlineCount*3.14159)*0.5+0.5;\n"
    "    color *= mix(1.0, scan, uScanlineIntensity);\n"
    "    color += (rand(uv*uTime)-0.5)*uNoise;\n"
    "    float d = distance(uv, vec2(0.5));\n"
    "    color *= mix(1.0, 1.0-d, uVignette);\n"
    "    color += rand(vec2(uTime,0.0))*uFlicker*0.1;\n"
    "    gl_FragColor = vec4(color, center.a);\n"
    "}\n";

void CrtEffect_Init(void) {
    CRT_g_shader = LoadShaderFromMemory(NULL, CRT_FS_SOURCE);
    CRT_g_shaderLoaded = (CRT_g_shader.id != 0);
    TraceLog(CRT_g_shaderLoaded ? LOG_INFO : LOG_WARNING,
        "[CrtEffect] embedded crt.fs %s (id=%d)",
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
}

#ifdef __EMSCRIPTEN__
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
}
#endif

void CrtEffect_Update(float dt) { CRT_g_time += dt; }

void CrtEffect_Draw(RenderTexture2D scene, int screenW, int screenH) {
    (void)screenW; (void)screenH;
    if (!CRT_g_shaderLoaded) {
        DrawTextureRec(scene.texture,
            (Rectangle){0,0,(float)scene.texture.width,-(float)scene.texture.height},
            (Vector2){0,0}, WHITE);
        return;
    }
    SetShaderValue(CRT_g_shader, CRT_g_locTime, &CRT_g_time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(CRT_g_shader, CRT_g_locScanlineIntensity, &CRT_g_params.scanlineIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(CRT_g_shader, CRT_g_locScanlineCount, &CRT_g_params.scanlineCount, SHADER_UNIFORM_FLOAT);
    SetShaderValue(CRT_g_shader, CRT_g_locScanlineSpeed, &CRT_g_params.scanlineSpeed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(CRT_g_shader, CRT_g_locCurvature, &CRT_g_params.curvature, SHADER_UNIFORM_FLOAT);
    SetShaderValue(CRT_g_shader, CRT_g_locVignette, &CRT_g_params.vignette, SHADER_UNIFORM_FLOAT);
    SetShaderValue(CRT_g_shader, CRT_g_locNoise, &CRT_g_params.noise, SHADER_UNIFORM_FLOAT);
    SetShaderValue(CRT_g_shader, CRT_g_locAberration, &CRT_g_params.chromaticAberration, SHADER_UNIFORM_FLOAT);
    SetShaderValue(CRT_g_shader, CRT_g_locFlicker, &CRT_g_params.flicker, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(CRT_g_shader);
    DrawTextureRec(scene.texture,
        (Rectangle){0,0,(float)scene.texture.width,-(float)scene.texture.height},
        (Vector2){0,0}, (Color){255,255,255,255});
    EndShaderMode();
}

void CrtEffect_Unload(void) {
    if (CRT_g_shaderLoaded) UnloadShader(CRT_g_shader);
    CRT_g_shaderLoaded = false;
}
#endif /* CRT_EFFECT_H */