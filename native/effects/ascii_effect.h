/*
 * ascii_effect.h — single-header ASCII post‑processing effect for raylib
 * Dependencia JSON solo en __EMSCRIPTEN__
 */

#ifndef ASCII_EFFECT_H
#define ASCII_EFFECT_H

#include "raylib.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include "../json_mini.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __EMSCRIPTEN__
void AsciiEffect_SetParams(const JsonValue *paramsObj);
#endif
void AsciiEffect_Update(float dt);
void AsciiEffect_Draw(RenderTexture2D scene, int screenW, int screenH);
void AsciiEffect_Unload(void);

#ifdef __cplusplus
}
#endif

/* =========================================================
 * Implementación (variables y funciones con prefijo ASCII_)
 * ========================================================= */
#define ASCII_MAX_RAMP 64

typedef struct {
    char ramp[ASCII_MAX_RAMP];
    int fontSize;
    float brightness;
    float contrast;
    float gamma;
    Color foreground;
    Color background;
    bool invert;
} ASCII_Params;

static ASCII_Params ASCII_g_params = {
    .ramp = " .:-=+*#%@",
    .fontSize = 10,
    .brightness = 0.8f,
    .contrast = 1.2f,
    .gamma = 1.1f,
    .foreground = (Color){ 68, 212, 255, 255 },
    .background = (Color){ 11, 11, 14, 0 },
    .invert = false,
};

#ifdef __EMSCRIPTEN__
static Color ASCII_HexToColor(const char *hex, Color fallback) {
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

void AsciiEffect_SetParams(const JsonValue *paramsObj) {
    if (!paramsObj) return;
    const char *ramp = JsonAsString(JsonObjectGet(paramsObj, "characters"), ASCII_g_params.ramp);
    strncpy(ASCII_g_params.ramp, ramp, ASCII_MAX_RAMP - 1);
    ASCII_g_params.ramp[ASCII_MAX_RAMP - 1] = '\0';
    ASCII_g_params.fontSize   = (int)JsonAsNumber(JsonObjectGet(paramsObj, "fontSize"), ASCII_g_params.fontSize);
    ASCII_g_params.brightness = (float)JsonAsNumber(JsonObjectGet(paramsObj, "brightness"), ASCII_g_params.brightness);
    ASCII_g_params.contrast   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "contrast"), ASCII_g_params.contrast);
    ASCII_g_params.gamma      = (float)JsonAsNumber(JsonObjectGet(paramsObj, "gamma"), ASCII_g_params.gamma);
    ASCII_g_params.invert     = JsonAsBool(JsonObjectGet(paramsObj, "invert"), ASCII_g_params.invert);
    ASCII_g_params.foreground = ASCII_HexToColor(JsonAsString(JsonObjectGet(paramsObj, "foreground"), NULL), ASCII_g_params.foreground);
    ASCII_g_params.background = ASCII_HexToColor(JsonAsString(JsonObjectGet(paramsObj, "background"), NULL), ASCII_g_params.background);
}
#endif

void AsciiEffect_Update(float dt) { (void)dt; }

#define ASCII_READBACK_INTERVAL 3

static RenderTexture2D ASCII_g_gridTarget;
static int ASCII_g_gridCols = 0, ASCII_g_gridRows = 0;
static char *ASCII_g_glyphCache = NULL;
static int ASCII_g_frameCounter = 0;

void AsciiEffect_Draw(RenderTexture2D scene, int screenW, int screenH) {
    int fontSize = ASCII_g_params.fontSize > 0 ? ASCII_g_params.fontSize : 10;
    int cols = screenW / fontSize;
    int rows = screenH / fontSize;
    if (cols <= 0 || rows <= 0) return;

    if (cols != ASCII_g_gridCols || rows != ASCII_g_gridRows) {
        if (ASCII_g_gridCols > 0) UnloadRenderTexture(ASCII_g_gridTarget);
        ASCII_g_gridTarget = LoadRenderTexture(cols, rows);
        free(ASCII_g_glyphCache);
        ASCII_g_glyphCache = (char *)malloc((size_t)(cols * rows));
        memset(ASCII_g_glyphCache, ' ', (size_t)(cols * rows));
        ASCII_g_gridCols = cols;
        ASCII_g_gridRows = rows;
        ASCII_g_frameCounter = 0;
    }

    ClearBackground(ASCII_g_params.background);

    if (ASCII_g_frameCounter % ASCII_READBACK_INTERVAL == 0) {
        BeginTextureMode(ASCII_g_gridTarget);
        DrawTexturePro(scene.texture,
            (Rectangle){ 0, 0, (float)scene.texture.width, -(float)scene.texture.height },
            (Rectangle){ 0, 0, (float)cols, (float)rows },
            (Vector2){ 0, 0 }, 0.0f, WHITE);
        EndTextureMode();

        Image img = LoadImageFromTexture(ASCII_g_gridTarget.texture);
        int rampLen = (int)strlen(ASCII_g_params.ramp);
        if (rampLen == 0) rampLen = 1;

        for (int y = 0; y < rows; y++) {
            int srcY = rows - 1 - y;
            for (int x = 0; x < cols; x++) {
                Color px = GetImageColor(img, x, srcY);
                float lum = (px.r + px.g + px.b) / (3.0f * 255.0f);
                lum = powf(lum, 1.0f / fmaxf(0.01f, ASCII_g_params.gamma));
                lum = (lum - 0.5f) * ASCII_g_params.contrast + 0.5f + (ASCII_g_params.brightness - 1.0f);
                if (lum < 0) lum = 0; if (lum > 1) lum = 1;
                if (ASCII_g_params.invert) lum = 1.0f - lum;
                int idx = (int)(lum * (rampLen - 1));
                ASCII_g_glyphCache[y * cols + x] = ASCII_g_params.ramp[idx];
            }
        }
        UnloadImage(img);
    }
    ASCII_g_frameCounter++;

    for (int y = 0; y < rows; y++)
        for (int x = 0; x < cols; x++) {
            char glyph[2] = { ASCII_g_glyphCache[y * cols + x], '\0' };
            DrawText(glyph, x * fontSize, y * fontSize, fontSize, ASCII_g_params.foreground);
        }
}

void AsciiEffect_Unload(void) {
    if (ASCII_g_gridCols > 0) { UnloadRenderTexture(ASCII_g_gridTarget); ASCII_g_gridCols = 0; ASCII_g_gridRows = 0; }
    free(ASCII_g_glyphCache);
    ASCII_g_glyphCache = NULL;
}
#endif /* ASCII_EFFECT_H */