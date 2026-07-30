/*
 * ascii_effect.h — single-header ASCII post‑processing effect for raylib
 * Dependencia JSON solo en __EMSCRIPTEN__
 *
 * Modos:
 *   normal — comportamiento original: rampa de luminancia (glifo según brillo)
 *   matrix — "code rain" estilo Matrix: por columna, una racha de caracteres
 *            nace en un borde y avanza (abajo/arriba/ambas a la vez), con una
 *            cabeza brillante y una estela que se desvanece detrás.
 *
 * El modo matrix es reactivo al vídeo igual que particles_effect.h: se
 * reutiliza el mismo downsample de escena que ya hace el modo normal para
 * elegir el glifo por rampa, pero en vez de leer luminancia por celda se
 * promedia por columna, y se compara contra el frame anterior para estimar
 * "movimiento" por columna. Columnas más brillantes / con más movimiento
 * caen (o suben) más rápido y aparecen con más probabilidad — así el efecto
 * se siente enganchado a lo que pasa en el vídeo, no solo decorativo encima.
 *
 * Los caracteres del modo matrix se leen como codepoints UTF-8 y se dibujan
 * con DrawTextCodepoint usando la fuente por defecto de raylib (set por
 * defecto: hexadecimal, todo ASCII).
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
#define ASCII_MATRIX_CHARS_MAX 512
#define ASCII_MATRIX_CODEPOINTS_MAX 128
#define ASCII_MODE_NORMAL 0
#define ASCII_MODE_MATRIX 1
#define ASCII_MATRIX_DIR_DOWN 0
#define ASCII_MATRIX_DIR_UP   1
#define ASCII_MATRIX_DIR_BOTH 2
typedef struct {
    char ramp[ASCII_MAX_RAMP];
    int fontSize;
    float brightness;
    float contrast;
    float gamma;
    Color foreground;
    Color background;
    bool invert;
    int mode; // ASCII_MODE_NORMAL / ASCII_MODE_MATRIX
    char matrixChars[ASCII_MATRIX_CHARS_MAX]; // UTF-8, p.ej. hexadecimal
    int matrixDirection;   // ASCII_MATRIX_DIR_*
    float matrixSpeed;     // filas/seg base de cada racha
    float matrixDensity;   // 0..1, probabilidad de que nazcan rachas nuevas
    int matrixTrailLength; // longitud de la estela, en celdas
    Color matrixHeadColor; // color de la cabeza brillante de cada racha
    bool matrixReactive;
    float matrixReactiveStrength; // 0..1
    // Cuánto se nota la imagen/vídeo de fondo reconstruida con el brillo de
    // los glifos (0 = casi negro parejo, valores altos = imagen bien
    // reconocible detrás de las corrientes de código).
    float matrixImageStrength;
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
    .mode = ASCII_MODE_NORMAL,
    .matrixChars = "0123456789ABCDEF",
    .matrixDirection = ASCII_MATRIX_DIR_DOWN,
    .matrixSpeed = 14.0f,
    .matrixDensity = 0.97f,
    .matrixTrailLength = 24,
    .matrixHeadColor = (Color){ 207, 255, 224, 255 },
    .matrixReactive = true,
    .matrixReactiveStrength = 1.2f,
    .matrixImageStrength = 1.3f,
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
static int ASCII_ParseMode(const char *s, int fallback) {
    if (!s) return fallback;
    if (strcmp(s, "matrix") == 0) return ASCII_MODE_MATRIX;
    if (strcmp(s, "normal") == 0) return ASCII_MODE_NORMAL;
    return fallback;
}
static int ASCII_ParseDirection(const char *s, int fallback) {
    if (!s) return fallback;
    if (strcmp(s, "up") == 0) return ASCII_MATRIX_DIR_UP;
    if (strcmp(s, "both") == 0) return ASCII_MATRIX_DIR_BOTH;
    if (strcmp(s, "down") == 0) return ASCII_MATRIX_DIR_DOWN;
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
    ASCII_g_params.mode = ASCII_ParseMode(JsonAsString(JsonObjectGet(paramsObj, "mode"), NULL), ASCII_g_params.mode);
    const char *mchars = JsonAsString(JsonObjectGet(paramsObj, "matrixChars"), ASCII_g_params.matrixChars);
    strncpy(ASCII_g_params.matrixChars, mchars, ASCII_MATRIX_CHARS_MAX - 1);
    ASCII_g_params.matrixChars[ASCII_MATRIX_CHARS_MAX - 1] = '\0';
    ASCII_g_params.matrixDirection = ASCII_ParseDirection(JsonAsString(JsonObjectGet(paramsObj, "matrixDirection"), NULL), ASCII_g_params.matrixDirection);
    ASCII_g_params.matrixSpeed     = (float)JsonAsNumber(JsonObjectGet(paramsObj, "matrixSpeed"), ASCII_g_params.matrixSpeed);
    ASCII_g_params.matrixDensity   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "matrixDensity"), ASCII_g_params.matrixDensity);
    ASCII_g_params.matrixTrailLength = (int)JsonAsNumber(JsonObjectGet(paramsObj, "matrixTrailLength"), ASCII_g_params.matrixTrailLength);
    ASCII_g_params.matrixHeadColor = ASCII_HexToColor(JsonAsString(JsonObjectGet(paramsObj, "matrixHeadColor"), NULL), ASCII_g_params.matrixHeadColor);
    ASCII_g_params.matrixReactive  = JsonAsBool(JsonObjectGet(paramsObj, "matrixReactive"), ASCII_g_params.matrixReactive);
    ASCII_g_params.matrixReactiveStrength = (float)JsonAsNumber(JsonObjectGet(paramsObj, "matrixReactiveStrength"), ASCII_g_params.matrixReactiveStrength);
    ASCII_g_params.matrixImageStrength = (float)JsonAsNumber(JsonObjectGet(paramsObj, "matrixImageStrength"), ASCII_g_params.matrixImageStrength);
}
#endif
void AsciiEffect_Update(float dt) { (void)dt; }
#define ASCII_READBACK_INTERVAL 3
static RenderTexture2D ASCII_g_gridTarget;
static int ASCII_g_gridCols = 0, ASCII_g_gridRows = 0;
static char *ASCII_g_glyphCache = NULL;
static int ASCII_g_frameCounter = 0;
/* --- Estado del modo matrix --- */
typedef struct {
    float head;   // fila actual de la cabeza (fraccional)
    int dir;      // +1 baja, -1 sube
    float speed;  // filas/seg de esta racha en concreto
    bool active;
} ASCII_MatrixStream;
static int *ASCII_g_matrixGlyphs = NULL;      // codepoint por celda, cols*rows
static float *ASCII_g_colLuma = NULL;         // luminancia media por columna (frame actual)
static float *ASCII_g_colLumaPrev = NULL;     // luminancia media por columna (frame anterior, para "movimiento")
static float *ASCII_g_cellLuma = NULL;        // luminancia por celda (cols*rows) — para la capa base
static int *ASCII_g_baseGlyphs = NULL;        // codepoint por celda de la capa base (imagen reconstruida)
static bool ASCII_g_baseGlyphsSeeded = false; // true una vez que la capa base tiene glifos reales (no espacios)
static ASCII_MatrixStream *ASCII_g_matrixStreams = NULL; // 2 por columna (slot 1 solo se usa en "both")
static int ASCII_g_matrixCodepoints[ASCII_MATRIX_CODEPOINTS_MAX];
static int ASCII_g_matrixCodepointCount = 0;
static char ASCII_g_matrixCharsParsed[ASCII_MATRIX_CHARS_MAX] = "";
/* Decodifica ASCII_g_params.matrixChars (UTF-8) a una lista de codepoints,
 * cacheada: solo se vuelve a parsear si el string cambió desde la última vez. */
static void ASCII_MatrixEnsureCodepoints(void) {
    if (ASCII_g_matrixCodepointCount > 0 &&
        strcmp(ASCII_g_matrixCharsParsed, ASCII_g_params.matrixChars) == 0) return;
    strncpy(ASCII_g_matrixCharsParsed, ASCII_g_params.matrixChars, ASCII_MATRIX_CHARS_MAX - 1);
    ASCII_g_matrixCharsParsed[ASCII_MATRIX_CHARS_MAX - 1] = '\0';
    ASCII_g_matrixCodepointCount = 0;
    const char *p = ASCII_g_params.matrixChars;
    while (*p != '\0' && ASCII_g_matrixCodepointCount < ASCII_MATRIX_CODEPOINTS_MAX) {
        int size = 0;
        int cp = GetCodepointNext(p, &size);
        if (size <= 0) break;
        if (cp != ' ') ASCII_g_matrixCodepoints[ASCII_g_matrixCodepointCount++] = cp;
        p += size;
    }
    if (ASCII_g_matrixCodepointCount == 0) {
        ASCII_g_matrixCodepoints[0] = '?';
        ASCII_g_matrixCodepointCount = 1;
    }
}
static inline int ASCII_RandomMatrixCodepoint(void) {
    if (ASCII_g_matrixCodepointCount <= 0) return ' ';
    return ASCII_g_matrixCodepoints[rand() % ASCII_g_matrixCodepointCount];
}
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
        free(ASCII_g_matrixGlyphs);
        ASCII_g_matrixGlyphs = (int *)malloc(sizeof(int) * (size_t)(cols * rows));
        for (int i = 0; i < cols * rows; i++) ASCII_g_matrixGlyphs[i] = ' ';
        free(ASCII_g_cellLuma);
        ASCII_g_cellLuma = (float *)calloc((size_t)(cols * rows), sizeof(float));
        free(ASCII_g_baseGlyphs);
        ASCII_g_baseGlyphs = (int *)malloc(sizeof(int) * (size_t)(cols * rows));
        for (int i = 0; i < cols * rows; i++) ASCII_g_baseGlyphs[i] = ' ';
        ASCII_g_baseGlyphsSeeded = false;
        free(ASCII_g_colLuma);
        free(ASCII_g_colLumaPrev);
        ASCII_g_colLuma = (float *)calloc((size_t)cols, sizeof(float));
        ASCII_g_colLumaPrev = (float *)calloc((size_t)cols, sizeof(float));
        free(ASCII_g_matrixStreams);
        ASCII_g_matrixStreams = (ASCII_MatrixStream *)malloc(sizeof(ASCII_MatrixStream) * (size_t)cols * 2);
        for (int c = 0; c < cols; c++) {
            for (int s = 0; s < 2; s++) {
                ASCII_MatrixStream *st = &ASCII_g_matrixStreams[c * 2 + s];
                st->head = 0.0f;
                st->dir = (s == 0) ? 1 : -1;
                st->speed = 0.0f;
                st->active = false;
            }
        }
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
        if (ASCII_g_params.mode == ASCII_MODE_MATRIX) {
            for (int x = 0; x < cols; x++) {
                float sum = 0.0f;
                for (int y = 0; y < rows; y++) {
                    int srcY = rows - 1 - y;
                    Color px = GetImageColor(img, x, srcY);
                    float luma = (px.r + px.g + px.b) / (3.0f * 255.0f);
                    ASCII_g_cellLuma[y * cols + x] = luma;
                    sum += luma;
                }
                float newLuma = sum / (float)rows;
                ASCII_g_colLumaPrev[x] = ASCII_g_colLuma[x];
                ASCII_g_colLuma[x] = newLuma;
            }
        } else {
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
        }
        UnloadImage(img);
    }
    ASCII_g_frameCounter++;
    if (ASCII_g_params.mode == ASCII_MODE_MATRIX) {
        ASCII_MatrixEnsureCodepoints();
        Font font = GetFontDefault();
        float dt = GetFrameTime();
        int trail = ASCII_g_params.matrixTrailLength > 1 ? ASCII_g_params.matrixTrailLength : 2;
        bool reactive = ASCII_g_params.matrixReactive;
        bool wantBoth = (ASCII_g_params.matrixDirection == ASCII_MATRIX_DIR_BOTH);

        // --- Capa base: reconstruye la imagen fuente (vídeo/escena) con la
        // propia densidad/brillo de los glifos en vez de mostrar el frame en
        // crudo — es lo que hace que se note el pasillo/gente/cables detrás
        // de la lluvia de código, igual que en la referencia. Antes de esto
        // el fondo quedaba directamente transparente/negro en modo matrix.
        if (ASCII_g_matrixCodepointCount > 0) {
            if (!ASCII_g_baseGlyphsSeeded) {
                for (int i = 0; i < cols * rows; i++) ASCII_g_baseGlyphs[i] = ASCII_RandomMatrixCodepoint();
                ASCII_g_baseGlyphsSeeded = true;
            }
            Color fg = ASCII_g_params.foreground;
            float shimmerChance = 0.06f * dt * (ASCII_g_params.matrixSpeed / 10.0f);
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    int idx = y * cols + x;
                    if (((float)rand() / RAND_MAX) < shimmerChance) {
                        ASCII_g_baseGlyphs[idx] = ASCII_RandomMatrixCodepoint();
                    }
                    float luma = ASCII_g_cellLuma[idx];
                    float alpha = luma * ASCII_g_params.matrixImageStrength;
                    if (alpha < 0.04f) alpha = 0.04f;
                    if (alpha > 1.0f) alpha = 1.0f;
                    Color tint = { (unsigned char)(fg.r * 0.7f), (unsigned char)(fg.g * 0.7f), (unsigned char)(fg.b * 0.7f), (unsigned char)(alpha * 255.0f) };
                    DrawTextCodepoint(font, ASCII_g_baseGlyphs[idx],
                        (Vector2){ (float)(x * fontSize), (float)(y * fontSize) },
                        (float)fontSize, tint);
                }
            }
        }

        for (int c = 0; c < cols; c++) {
            float luma = reactive ? ASCII_g_colLuma[c] : 0.0f;
            float motion = reactive ? fabsf(ASCII_g_colLuma[c] - ASCII_g_colLumaPrev[c]) * 6.0f : 0.0f;
            if (motion > 1.0f) motion = 1.0f;
            float boost = 1.0f + ASCII_g_params.matrixReactiveStrength * (luma * 0.5f + motion * 1.5f);
            for (int s = 0; s < 2; s++) {
                if (s == 1 && !wantBoth) continue;
                ASCII_MatrixStream *st = &ASCII_g_matrixStreams[c * 2 + s];
                int dir = wantBoth ? (s == 0 ? 1 : -1)
                                   : (ASCII_g_params.matrixDirection == ASCII_MATRIX_DIR_UP ? -1 : 1);
                st->dir = dir;
                if (!st->active) {
                    float chance = ASCII_g_params.matrixDensity * dt * 0.6f;
                    if (((float)rand() / RAND_MAX) < chance) {
                        st->head = dir > 0
                            ? -((float)rand() / RAND_MAX) * trail
                            : (float)(rows - 1) + ((float)rand() / RAND_MAX) * trail;
                        st->speed = ASCII_g_params.matrixSpeed * (0.6f + ((float)rand() / RAND_MAX) * 0.8f);
                        st->active = true;
                    } else {
                        continue;
                    }
                } else {
                    st->head += dir * st->speed * boost * dt;
                    bool outOfBounds = dir > 0 ? (st->head - trail > rows) : (st->head + trail < -1);
                    if (outOfBounds) { st->active = false; continue; }
                }
                for (int k = 0; k < trail; k++) {
                    int row = (int)roundf(st->head - dir * k);
                    if (row < 0 || row >= rows) continue;
                    int idx = row * cols + c;
                    if (k == 0 || ((float)rand() / RAND_MAX) < 0.12f) {
                        ASCII_g_matrixGlyphs[idx] = ASCII_RandomMatrixCodepoint();
                    }
                    float fadeLin = 1.0f - (float)k / (float)trail;
                    float fade = fadeLin * fadeLin;
                    Color tint = (k == 0) ? ASCII_g_params.matrixHeadColor : ASCII_g_params.foreground;
                    if (k > 0) tint.a = (unsigned char)(255.0f * fade);
                    DrawTextCodepoint(font, ASCII_g_matrixGlyphs[idx],
                        (Vector2){ (float)(c * fontSize), (float)(row * fontSize) },
                        (float)fontSize, tint);
                }
            }
        }
    } else {
        for (int y = 0; y < rows; y++)
            for (int x = 0; x < cols; x++) {
                char glyph[2] = { ASCII_g_glyphCache[y * cols + x], '\0' };
                DrawText(glyph, x * fontSize, y * fontSize, fontSize, ASCII_g_params.foreground);
            }
    }
}
void AsciiEffect_Unload(void) {
    if (ASCII_g_gridCols > 0) { UnloadRenderTexture(ASCII_g_gridTarget); ASCII_g_gridCols = 0; ASCII_g_gridRows = 0; }
    free(ASCII_g_glyphCache); ASCII_g_glyphCache = NULL;
    free(ASCII_g_matrixGlyphs); ASCII_g_matrixGlyphs = NULL;
    free(ASCII_g_cellLuma); ASCII_g_cellLuma = NULL;
    free(ASCII_g_baseGlyphs); ASCII_g_baseGlyphs = NULL;
    free(ASCII_g_colLuma); ASCII_g_colLuma = NULL;
    free(ASCII_g_colLumaPrev); ASCII_g_colLumaPrev = NULL;
    free(ASCII_g_matrixStreams); ASCII_g_matrixStreams = NULL;
}
#endif /* ASCII_EFFECT_H */
