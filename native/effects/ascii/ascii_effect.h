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
#include <stdint.h>
#ifdef __EMSCRIPTEN__
#include "../../json_mini.h"
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

void js_set_matrix_font_data(size_t bufSize, uint8_t *buf);
const char *js_get_ascii_grid_text(void);
#ifdef __cplusplus
}
#endif
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
    int mode;
    char matrixChars[ASCII_MATRIX_CHARS_MAX];
    int matrixDirection;
    float matrixSpeed;
    float matrixDensity;
    int matrixTrailLength;
    Color matrixHeadColor;
    bool matrixReactive;
    float matrixReactiveStrength;
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
    .matrixChars = "0123456789アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲン",
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
typedef struct {
    float head;
    int dir;
    float speed;
    bool active;
} ASCII_MatrixStream;
static int *ASCII_g_matrixGlyphs = NULL;
static float *ASCII_g_colLuma = NULL;
static float *ASCII_g_colLumaPrev = NULL;
static float *ASCII_g_cellLuma = NULL;
static int *ASCII_g_baseGlyphs = NULL;
static bool ASCII_g_baseGlyphsSeeded = false;
static ASCII_MatrixStream *ASCII_g_matrixStreams = NULL;
static int ASCII_g_matrixCodepoints[ASCII_MATRIX_CODEPOINTS_MAX];
static int ASCII_g_matrixCodepointCount = 0;
static char ASCII_g_matrixCharsParsed[ASCII_MATRIX_CHARS_MAX] = "";
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
static uint8_t *ASCII_g_matrixFontBuffer = NULL;
static size_t ASCII_g_matrixFontBufferSize = 0;
static bool ASCII_g_matrixFontDirty = false;
static Font ASCII_g_matrixFont = { 0 };
static bool ASCII_g_matrixFontIsCustom = false;
static int ASCII_g_matrixFontBuiltSize = -1;
static char ASCII_g_matrixFontBuiltChars[ASCII_MATRIX_CHARS_MAX] = "";
void js_set_matrix_font_data(size_t bufSize, uint8_t *buf) {
    free(ASCII_g_matrixFontBuffer);
    ASCII_g_matrixFontBuffer = NULL;
    ASCII_g_matrixFontBufferSize = 0;
    if (bufSize > 0 && buf) {
        ASCII_g_matrixFontBuffer = (uint8_t *)malloc(bufSize);
        if (ASCII_g_matrixFontBuffer) {
            memcpy(ASCII_g_matrixFontBuffer, buf, bufSize);
            ASCII_g_matrixFontBufferSize = bufSize;
        } else {
            fprintf(stderr, "[ascii_matrix] Failed to allocate memory for font buffer\n");
        }
    }
    ASCII_g_matrixFontDirty = true;
}
static void ASCII_MatrixEnsureFont(int fontSize) {
    bool sizeChanged = (fontSize != ASCII_g_matrixFontBuiltSize);
    bool charsChanged = (strcmp(ASCII_g_matrixFontBuiltChars, ASCII_g_params.matrixChars) != 0);
    if (!ASCII_g_matrixFontDirty && !sizeChanged && !charsChanged && ASCII_g_matrixFont.texture.id != 0) return;
    if (ASCII_g_matrixFontIsCustom && ASCII_g_matrixFont.texture.id != 0) {
        UnloadFont(ASCII_g_matrixFont);
    }
    ASCII_g_matrixFont = GetFontDefault();
    ASCII_g_matrixFontIsCustom = false;
    if (ASCII_g_matrixFontBuffer && ASCII_g_matrixFontBufferSize > 0 && ASCII_g_matrixCodepointCount > 0) {
        Font custom = LoadFontFromMemory(".ttf", ASCII_g_matrixFontBuffer, (int)ASCII_g_matrixFontBufferSize,
            fontSize, ASCII_g_matrixCodepoints, ASCII_g_matrixCodepointCount);
        if (custom.texture.id != 0 && custom.glyphCount > 0) {
            ASCII_g_matrixFont = custom;
            ASCII_g_matrixFontIsCustom = true;
        } else {
            fprintf(stderr, "[ascii_matrix] LoadFontFromMemory failed, falling back to GetFontDefault()\n");
        }
    }
    ASCII_g_matrixFontDirty = false;
    ASCII_g_matrixFontBuiltSize = fontSize;
    strncpy(ASCII_g_matrixFontBuiltChars, ASCII_g_params.matrixChars, ASCII_MATRIX_CHARS_MAX - 1);
    ASCII_g_matrixFontBuiltChars[ASCII_MATRIX_CHARS_MAX - 1] = '\0';
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
        ASCII_MatrixEnsureFont(fontSize);
        Font font = ASCII_g_matrixFont;
        float dt = GetFrameTime();
        int trail = ASCII_g_params.matrixTrailLength > 1 ? ASCII_g_params.matrixTrailLength : 2;
        bool reactive = ASCII_g_params.matrixReactive;
        bool wantBoth = (ASCII_g_params.matrixDirection == ASCII_MATRIX_DIR_BOTH);
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
// Vuelca la rejilla de caracteres actual (la que se está dibujando este
// frame) a un string con salto de línea real tras cada fila, para
// exportarla tal cual a un .txt. cols/rows son los mismos que determinan
// el render (screenW/fontSize, screenH/fontSize), así que el .txt respeta
// la resolución real del efecto sin necesidad de que JS conozca cols/rows
// por separado.
//
// En modo matrix se usa ASCII_g_baseGlyphs (la capa base con shimmer, con
// la que se compone casi toda la rejilla en cualquier instante) en vez de
// ASCII_g_matrixGlyphs (que solo tiene valor válido en las celdas que un
// trail activo ha pisado en algún momento, y queda obsoleto en el resto).
// Es una aproximación honesta del "qué se ve ahora mismo" y no una
// réplica exacta del compositing de streams frame a frame — precisar eso
// pediría rastrear qué celdas cubre cada trail en este instante exacto.
//
// No se declara con EMSCRIPTEN_KEEPALIVE (la macro de <emscripten.h>) a
// propósito: main.c incluye este header ANTES de <emscripten.h>, así que
// esa macro todavía no existiría en este punto. __attribute__((used)) es
// exactamente a lo que se expande esa macro, así que el efecto es
// idéntico sin depender del orden de includes.
#ifdef __EMSCRIPTEN__
__attribute__((used))
#endif
const char *js_get_ascii_grid_text(void) {
    static char *buf = NULL;
    free(buf);
    buf = NULL;

    int cols = ASCII_g_gridCols, rows = ASCII_g_gridRows;
    if (cols <= 0 || rows <= 0) {
        buf = (char *)malloc(1);
        buf[0] = '\0';
        return buf;
    }

    // Peor caso: 4 bytes UTF-8 por glyph (kana matrix) + '\n' por fila + NUL.
    size_t cap = (size_t)cols * (size_t)rows * 4 + (size_t)rows + 1;
    buf = (char *)malloc(cap);
    if (!buf) return "";

    size_t pos = 0;
    bool matrix = (ASCII_g_params.mode == ASCII_MODE_MATRIX);
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            int idx = y * cols + x;
            if (matrix) {
                int cp = ASCII_g_baseGlyphs ? ASCII_g_baseGlyphs[idx] : ' ';
                if (cp <= 0) cp = ' ';
                int size = 0;
                const char *enc = CodepointToUTF8(cp, &size);
                if (size > 0) { memcpy(buf + pos, enc, (size_t)size); pos += (size_t)size; }
            } else {
                buf[pos++] = ASCII_g_glyphCache ? ASCII_g_glyphCache[idx] : ' ';
            }
        }
        buf[pos++] = '\n';
    }
    buf[pos] = '\0';
    return buf;
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
    if (ASCII_g_matrixFontIsCustom && ASCII_g_matrixFont.texture.id != 0) UnloadFont(ASCII_g_matrixFont);
    ASCII_g_matrixFont = (Font){ 0 };
    ASCII_g_matrixFontIsCustom = false;
    ASCII_g_matrixFontBuiltSize = -1;
    ASCII_g_matrixFontBuiltChars[0] = '\0';
    free(ASCII_g_matrixFontBuffer); ASCII_g_matrixFontBuffer = NULL;
    ASCII_g_matrixFontBufferSize = 0;
}
#endif /* ASCII_EFFECT_H */