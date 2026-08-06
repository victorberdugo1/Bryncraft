/* particles_effect.h — single-header particle system for raylib
 * Modes: fountain, rain, embers with optional reactivity (GPU→CPU downsampling)
 *
 * Part of Bryncraft (https://bryncraft.online/) — created by Victor Berdugo
 */

#ifndef PARTICLES_EFFECT_H
#define PARTICLES_EFFECT_H

#include "raylib.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include "../../json_mini.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __EMSCRIPTEN__
void ParticlesEffect_SetParams(const JsonValue *paramsObj);
#endif
void ParticlesEffect_Update(float dt);
void ParticlesEffect_Draw(RenderTexture2D scene, int screenW, int screenH);

#ifdef __cplusplus
}
#endif

#define PARTICLES_MAX 20000

#define PART_MODE_FOUNTAIN 0
#define PART_MODE_RAIN     1
#define PART_MODE_EMBERS   2

#define PART_SCENE_GRID_W 64
#define PART_SCENE_GRID_H 36

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float age;
    float lifetime;
    float phase;
    bool alive;
} PART_Particle;

typedef struct {
    int mode;
    int count;
    float spawnRate;
    float gravity;
    float lifetime;
    float size;
    float sizeFalloff;
    Color color;
    float spreadDeg;
    float spawnX;
    float spawnY;
    float windX;
    int reactive;
    float reactiveStrength;
    float flowStrength;
} PART_ParticleParams;

static PART_ParticleParams PART_g_params = {
    .mode = PART_MODE_RAIN,
    .count = 2000,
    .spawnRate = 120.0f,
    .gravity = 9.8f,
    .lifetime = 2.5f,
    .size = 4.0f,
    .sizeFalloff = 0.6f,
    .color = (Color){ 68, 212, 255, 255 },
    .spreadDeg = 45.0f,
    .spawnX = 0.5f,
    .spawnY = 0.8f,
    .windX = 0.0f,
    .reactive = 1,
    .reactiveStrength = 0.6f,
    .flowStrength = 0.8f,
};

static PART_Particle PART_g_pool[PARTICLES_MAX];
static int PART_g_aliveCount = 0;
static float PART_g_spawnAccumulator = 0.0f;
static int PART_g_lastScreenW = 1;
static int PART_g_lastScreenH = 1;

static RenderTexture2D PART_g_sceneSmall;
static bool PART_g_sceneSmallReady = false;
static Image PART_g_sceneImg = { 0 };
static Image PART_g_scenePrevImg = { 0 };
static Image PART_g_motionImg = { 0 };
static bool PART_g_hasPrevFrame = false;

static void PART_UpdateSceneSample(RenderTexture2D scene) {
    if (!PART_g_sceneSmallReady) {
        PART_g_sceneSmall = LoadRenderTexture(PART_SCENE_GRID_W, PART_SCENE_GRID_H);
        PART_g_sceneSmallReady = true;
    }
    BeginTextureMode(PART_g_sceneSmall);
    ClearBackground(BLACK);
    DrawTexturePro(
        scene.texture,
        (Rectangle){ 0, 0, (float)scene.texture.width, -(float)scene.texture.height },
        (Rectangle){ 0, 0, (float)PART_SCENE_GRID_W, (float)PART_SCENE_GRID_H },
        (Vector2){ 0, 0 }, 0.0f, WHITE
    );
    EndTextureMode();

    if (PART_g_sceneImg.data) {
        if (PART_g_scenePrevImg.data) UnloadImage(PART_g_scenePrevImg);
        PART_g_scenePrevImg = PART_g_sceneImg; // toma ownership
        PART_g_hasPrevFrame = true;
    }
    PART_g_sceneImg = LoadImageFromTexture(PART_g_sceneSmall.texture);

    if (PART_g_hasPrevFrame) {
        if (PART_g_motionImg.data) UnloadImage(PART_g_motionImg);
        PART_g_motionImg = GenImageColor(PART_SCENE_GRID_W, PART_SCENE_GRID_H, BLACK);
        unsigned char *cur_pixels = (unsigned char *)PART_g_sceneImg.data;
        unsigned char *prev_pixels = (unsigned char *)PART_g_scenePrevImg.data;
        unsigned char *motion_pixels = (unsigned char *)PART_g_motionImg.data;
        bool isCurRGBA8 = (PART_g_sceneImg.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        bool isPrevRGBA8 = (PART_g_scenePrevImg.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        bool isMotionRGBA8 = (PART_g_motionImg.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        for (int y = 0; y < PART_SCENE_GRID_H; y++) {
            for (int x = 0; x < PART_SCENE_GRID_W; x++) {
                int idx = y * PART_SCENE_GRID_W + x;
                float diff;
                if (isCurRGBA8 && isPrevRGBA8) {
                    unsigned char *cp = cur_pixels + idx * 4;
                    unsigned char *pp = prev_pixels + idx * 4;
                    diff = (fabsf((float)cp[0] - (float)pp[0]) +
                            fabsf((float)cp[1] - (float)pp[1]) +
                            fabsf((float)cp[2] - (float)pp[2])) / (3.0f * 255.0f);
                } else {
                    Color cur = GetImageColor(PART_g_sceneImg, x, y);
                    Color prev = GetImageColor(PART_g_scenePrevImg, x, y);
                    diff = (fabsf((float)cur.r - (float)prev.r) +
                            fabsf((float)cur.g - (float)prev.g) +
                            fabsf((float)cur.b - (float)prev.b)) / (3.0f * 255.0f);
                }
                float m = diff * 4.0f;
                if (m > 1.0f) m = 1.0f;
                unsigned char mb = (unsigned char)(m * 255.0f);
                if (isMotionRGBA8) {
                    unsigned char *mp = motion_pixels + idx * 4;
                    mp[0] = mb; mp[1] = mb; mp[2] = mb; mp[3] = 255;
                } else {
                    ImageDrawPixel(&PART_g_motionImg, x, y, (Color){ mb, mb, mb, 255 });
                }
            }
        }
    }
}

static inline float PART_Luma(Color c) {
    return (c.r + c.g + c.b) / (3.0f * 255.0f);
}

static Color PART_SampleScene(float normX, float normY) {
    if (!PART_g_sceneImg.data) return WHITE;
    int x = (int)(normX * PART_SCENE_GRID_W);
    int y = (int)(normY * PART_SCENE_GRID_H);
    if (x < 0) x = 0; if (x >= PART_SCENE_GRID_W) x = PART_SCENE_GRID_W - 1;
    if (y < 0) y = 0; if (y >= PART_SCENE_GRID_H) y = PART_SCENE_GRID_H - 1;
    if (PART_g_sceneImg.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
        unsigned char *p = (unsigned char *)PART_g_sceneImg.data + (y * PART_SCENE_GRID_W + x) * 4;
        return (Color){ p[0], p[1], p[2], p[3] };
    }
    return GetImageColor(PART_g_sceneImg, x, y);
}

static Color PART_SampleSceneAvg(float normX, float normY) {
    if (!PART_g_sceneImg.data) return WHITE;
    int cx = (int)(normX * PART_SCENE_GRID_W);
    int cy = (int)(normY * PART_SCENE_GRID_H);
    int rSum = 0, gSum = 0, bSum = 0, n = 0;
    bool isRGBA8 = (PART_g_sceneImg.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    unsigned char *pixels = (unsigned char *)PART_g_sceneImg.data;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int x = cx + dx, y = cy + dy;
            if (x < 0 || x >= PART_SCENE_GRID_W || y < 0 || y >= PART_SCENE_GRID_H) continue;
            if (isRGBA8) {
                unsigned char *p = pixels + (y * PART_SCENE_GRID_W + x) * 4;
                rSum += p[0]; gSum += p[1]; bSum += p[2]; n++;
            } else {
                Color c = GetImageColor(PART_g_sceneImg, x, y);
                rSum += c.r; gSum += c.g; bSum += c.b; n++;
            }
        }
    }
    if (n == 0) return WHITE;
    return (Color){ (unsigned char)(rSum / n), (unsigned char)(gSum / n), (unsigned char)(bSum / n), 255 };
}

static float PART_SampleLuma(float normX, float normY) {
    return PART_Luma(PART_SampleScene(normX, normY));
}

static float PART_SampleMotion(float normX, float normY) {
    if (!PART_g_motionImg.data) return 0.0f;
    int x = (int)(normX * PART_SCENE_GRID_W);
    int y = (int)(normY * PART_SCENE_GRID_H);
    if (x < 0) x = 0; if (x >= PART_SCENE_GRID_W) x = PART_SCENE_GRID_W - 1;
    if (y < 0) y = 0; if (y >= PART_SCENE_GRID_H) y = PART_SCENE_GRID_H - 1;
    if (PART_g_motionImg.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
        unsigned char *p = (unsigned char *)PART_g_motionImg.data + (y * PART_SCENE_GRID_W + x) * 4;
        return p[0] / 255.0f;
    }
    return GetImageColor(PART_g_motionImg, x, y).r / 255.0f;
}

static Vector2 PART_SampleGradient(float normX, float normY) {
    if (!PART_g_sceneImg.data) return (Vector2){ 0, 0 };
    int x = (int)(normX * PART_SCENE_GRID_W);
    int y = (int)(normY * PART_SCENE_GRID_H);
    if (x < 1) x = 1; if (x > PART_SCENE_GRID_W - 2) x = PART_SCENE_GRID_W - 2;
    if (y < 1) y = 1; if (y > PART_SCENE_GRID_H - 2) y = PART_SCENE_GRID_H - 2;

    float lumaL, lumaR, lumaU, lumaD;
    if (PART_g_sceneImg.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
        unsigned char *pixels = (unsigned char *)PART_g_sceneImg.data;
        unsigned char *pL = pixels + (y * PART_SCENE_GRID_W + x - 1) * 4;
        unsigned char *pR = pixels + (y * PART_SCENE_GRID_W + x + 1) * 4;
        unsigned char *pU = pixels + ((y - 1) * PART_SCENE_GRID_W + x) * 4;
        unsigned char *pD = pixels + ((y + 1) * PART_SCENE_GRID_W + x) * 4;
        lumaL = (pL[0] + pL[1] + pL[2]) / (3.0f * 255.0f);
        lumaR = (pR[0] + pR[1] + pR[2]) / (3.0f * 255.0f);
        lumaU = (pU[0] + pU[1] + pU[2]) / (3.0f * 255.0f);
        lumaD = (pD[0] + pD[1] + pD[2]) / (3.0f * 255.0f);
    } else {
        lumaL = PART_Luma(GetImageColor(PART_g_sceneImg, x - 1, y));
        lumaR = PART_Luma(GetImageColor(PART_g_sceneImg, x + 1, y));
        lumaU = PART_Luma(GetImageColor(PART_g_sceneImg, x, y - 1));
        lumaD = PART_Luma(GetImageColor(PART_g_sceneImg, x, y + 1));
    }
    return (Vector2){ lumaR - lumaL, lumaD - lumaU };
}

static Vector2 PART_SampleMotionGradient(float normX, float normY) {
    if (!PART_g_motionImg.data) return (Vector2){ 0, 0 };
    int x = (int)(normX * PART_SCENE_GRID_W);
    int y = (int)(normY * PART_SCENE_GRID_H);
    if (x < 1) x = 1; if (x > PART_SCENE_GRID_W - 2) x = PART_SCENE_GRID_W - 2;
    if (y < 1) y = 1; if (y > PART_SCENE_GRID_H - 2) y = PART_SCENE_GRID_H - 2;

    float mL, mR, mU, mD;
    if (PART_g_motionImg.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
        unsigned char *pixels = (unsigned char *)PART_g_motionImg.data;
        mL = pixels[(y * PART_SCENE_GRID_W + x - 1) * 4] / 255.0f;
        mR = pixels[(y * PART_SCENE_GRID_W + x + 1) * 4] / 255.0f;
        mU = pixels[((y - 1) * PART_SCENE_GRID_W + x) * 4] / 255.0f;
        mD = pixels[((y + 1) * PART_SCENE_GRID_W + x) * 4] / 255.0f;
    } else {
        mL = GetImageColor(PART_g_motionImg, x - 1, y).r / 255.0f;
        mR = GetImageColor(PART_g_motionImg, x + 1, y).r / 255.0f;
        mU = GetImageColor(PART_g_motionImg, x, y - 1).r / 255.0f;
        mD = GetImageColor(PART_g_motionImg, x, y + 1).r / 255.0f;
    }
    return (Vector2){ mR - mL, mD - mU };
}

#ifdef __EMSCRIPTEN__
static Color PART_HexToColor(const char *hex, Color fallback) {
    if (!hex || hex[0] != '#' || strlen(hex) < 7) return fallback;
    unsigned int r, g, b;
    if (sscanf(hex + 1, "%02x%02x%02x", &r, &g, &b) != 3) return fallback;
    return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
}

static int PART_ParseMode(const char *s, int fallback) {
    if (!s) return fallback;
    if (strcmp(s, "rain") == 0) return PART_MODE_RAIN;
    if (strcmp(s, "embers") == 0) return PART_MODE_EMBERS;
    if (strcmp(s, "fountain") == 0) return PART_MODE_FOUNTAIN;
    return fallback;
}

void ParticlesEffect_SetParams(const JsonValue *paramsObj) {
    if (!paramsObj) return;
    int newCount = (int)JsonAsNumber(JsonObjectGet(paramsObj, "count"), PART_g_params.count);
    PART_g_params.count = newCount > PARTICLES_MAX ? PARTICLES_MAX : newCount;

    int newMode = PART_ParseMode(JsonAsString(JsonObjectGet(paramsObj, "mode"), NULL), PART_g_params.mode);
    if (newMode != PART_g_params.mode) {
        PART_g_aliveCount = 0;
        PART_g_spawnAccumulator = 0.0f;
    }
    PART_g_params.mode = newMode;

    PART_g_params.spawnRate   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "spawnRate"), PART_g_params.spawnRate);
    PART_g_params.gravity     = (float)JsonAsNumber(JsonObjectGet(paramsObj, "gravity"), PART_g_params.gravity);
    PART_g_params.lifetime    = (float)JsonAsNumber(JsonObjectGet(paramsObj, "lifetime"), PART_g_params.lifetime);
    PART_g_params.size        = (float)JsonAsNumber(JsonObjectGet(paramsObj, "size"), PART_g_params.size);
    PART_g_params.sizeFalloff = (float)JsonAsNumber(JsonObjectGet(paramsObj, "sizeFalloff"), PART_g_params.sizeFalloff);
    PART_g_params.spreadDeg   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "spread"), PART_g_params.spreadDeg);
    PART_g_params.spawnX      = (float)JsonAsNumber(JsonObjectGet(paramsObj, "spawnX"), PART_g_params.spawnX);
    PART_g_params.spawnY      = (float)JsonAsNumber(JsonObjectGet(paramsObj, "spawnY"), PART_g_params.spawnY);
    PART_g_params.windX       = (float)JsonAsNumber(JsonObjectGet(paramsObj, "wind"), PART_g_params.windX);
    PART_g_params.reactive    = JsonAsBool(JsonObjectGet(paramsObj, "reactive"), PART_g_params.reactive) ? 1 : 0;
    PART_g_params.reactiveStrength = (float)JsonAsNumber(JsonObjectGet(paramsObj, "reactiveStrength"), PART_g_params.reactiveStrength);
    PART_g_params.flowStrength = (float)JsonAsNumber(JsonObjectGet(paramsObj, "flowStrength"), PART_g_params.flowStrength);
    PART_g_params.color = PART_HexToColor(JsonAsString(JsonObjectGet(paramsObj, "color"), NULL), PART_g_params.color);
}
#endif

static void PART_SpawnParticle(int screenW, int screenH) {
    if (PART_g_aliveCount >= PART_g_params.count || PART_g_aliveCount >= PARTICLES_MAX) return;
    PART_Particle *p = &PART_g_pool[PART_g_aliveCount++];
    p->age = 0.0f;
    p->phase = ((float)rand() / RAND_MAX) * 2.0f * PI;

    bool reactive = PART_g_params.reactive && PART_g_sceneImg.data;

    if (PART_g_params.mode == PART_MODE_RAIN) {
        float fallSpeed = 200.0f + PART_g_params.gravity * 20.0f;

        /* El viento desplaza cada gota "windX*40" por segundo mientras cae.
         * Si el spawn se queda fijo en [0, screenW], el lado contrario al
         * viento va quedando vacío a medida que baja (embudo de aire libre).
         * Para compensarlo, extendemos el rango de aparición hacia ese lado
         * en proporción a cuánto va a derivar durante toda la caída, así al
         * llegar abajo la nube de gotas vuelve a cubrir toda la pantalla. */
        float fallTime = (screenH + 20.0f) / fmaxf(fallSpeed, 1.0f);
        float drift = PART_g_params.windX * 40.0f * fallTime;
        float spawnMinX = drift >= 0.0f ? -drift : 0.0f;
        float spawnMaxX = drift >= 0.0f ? (float)screenW : (float)screenW - drift;
        float spawnRangeW = spawnMaxX - spawnMinX;

        float spawnFrac = (float)rand() / RAND_MAX;
        if (reactive) {
            for (int tries = 0; tries < 4; tries++) {
                float candidate = (float)rand() / RAND_MAX;
                float darkness = 1.0f - PART_SampleLuma(candidate, 0.05f);
                float motion = PART_SampleMotion(candidate, 0.05f);
                float bias = darkness * 0.6f + motion * 0.4f;
                float weight = 1.0f - PART_g_params.reactiveStrength + PART_g_params.reactiveStrength * bias;
                if (((float)rand() / RAND_MAX) <= weight) { spawnFrac = candidate; break; }
                spawnFrac = candidate;
            }
        }
        p->position = (Vector2){ spawnMinX + spawnFrac * spawnRangeW, -10.0f };
        p->velocity = (Vector2){ PART_g_params.windX * 40.0f, fallSpeed };
        p->lifetime = PART_g_params.lifetime; // límite de seguridad, normalmente muere al salir por abajo
        p->alive = true;
    } else if (PART_g_params.mode == PART_MODE_EMBERS) {
        float riseSpeed = 20.0f + PART_g_params.gravity * 4.0f;

        /* Misma idea que en rain: el viento va corriendo la brasa hacia un
         * lado mientras sube, así que ensanchamos el spawn hacia el lado
         * contrario para que no se note un hueco arriba. Usamos el tiempo
         * de vida (o el de subir toda la pantalla, lo que sea menor) para
         * estimar cuánto va a derivar. */
        float riseTime = fminf((float)screenH / fmaxf(riseSpeed, 1.0f), PART_g_params.lifetime);
        float drift = PART_g_params.windX * 20.0f * riseTime;
        float spawnMinX = drift >= 0.0f ? -drift : 0.0f;
        float spawnMaxX = drift >= 0.0f ? (float)screenW : (float)screenW - drift;
        float spawnRangeW = spawnMaxX - spawnMinX;

        float spawnFrac = (float)rand() / RAND_MAX;
        if (reactive) {
            for (int tries = 0; tries < 4; tries++) {
                float candidate = (float)rand() / RAND_MAX;
                float brightness = PART_SampleLuma(candidate, 0.9f);
                float motion = PART_SampleMotion(candidate, 0.9f);
                float bias = brightness * 0.6f + motion * 0.4f;
                float weight = 1.0f - PART_g_params.reactiveStrength + PART_g_params.reactiveStrength * bias;
                if (((float)rand() / RAND_MAX) <= weight) { spawnFrac = candidate; break; }
                spawnFrac = candidate;
            }
        }
        p->position = (Vector2){ spawnMinX + spawnFrac * spawnRangeW, screenH + ((float)rand() / RAND_MAX) * 20.0f };
        p->velocity = (Vector2){ 0.0f, -riseSpeed };
        p->lifetime = PART_g_params.lifetime * (0.7f + ((float)rand() / RAND_MAX) * 0.6f);
        p->alive = true;
    } else { 
        float spreadRad = PART_g_params.spreadDeg * DEG2RAD;
        float angle = -PI / 2.0f + ((float)rand() / RAND_MAX - 0.5f) * spreadRad;
        float speed = 60.0f + ((float)rand() / RAND_MAX) * 120.0f;
        p->position = (Vector2){ PART_g_params.spawnX * screenW, PART_g_params.spawnY * screenH };
        p->velocity = (Vector2){ cosf(angle) * speed, sinf(angle) * speed };
        p->lifetime = PART_g_params.lifetime * (0.6f + ((float)rand() / RAND_MAX) * 0.4f);
        p->alive = true;
    }
}

void ParticlesEffect_Update(float dt) {
    bool reactive = PART_g_params.reactive && PART_g_sceneImg.data && PART_g_params.flowStrength > 0.0f;

    for (int i = 0; i < PART_g_aliveCount; i++) {
        PART_Particle *p = &PART_g_pool[i];
        p->age += dt;

        bool dead = (p->age >= p->lifetime);

        if (PART_g_params.mode == PART_MODE_RAIN) {
            p->position.x += p->velocity.x * dt;
            p->position.y += p->velocity.y * dt;
        } else if (PART_g_params.mode == PART_MODE_EMBERS) {
            p->velocity.x = sinf(p->phase + p->age * 2.0f) * 12.0f + PART_g_params.windX * 20.0f;
            p->position.x += p->velocity.x * dt;
            p->position.y += p->velocity.y * dt;
        } else {
            p->velocity.y += PART_g_params.gravity * dt * 20.0f;
            p->position.x += p->velocity.x * dt;
            p->position.y += p->velocity.y * dt;
        }

        if (reactive) {
            float nx = p->position.x / (float)PART_g_lastScreenW;
            float ny = p->position.y / (float)PART_g_lastScreenH;

            Vector2 gradLuma = PART_SampleGradient(nx, ny);
            Vector2 gradMotion = PART_SampleMotionGradient(nx, ny);

            float pull = PART_g_params.flowStrength * 60.0f;
            float motionPull = PART_g_params.flowStrength * 90.0f;

            p->position.x += gradLuma.x * pull * dt;
            p->position.y += gradLuma.y * pull * dt;
            p->position.x += gradMotion.x * motionPull * dt;
            p->position.y += gradMotion.y * motionPull * dt;
        }

        if (dead) {
            PART_g_pool[i] = PART_g_pool[PART_g_aliveCount - 1];
            PART_g_aliveCount--;
            i--;
        }
    }
}

void ParticlesEffect_Draw(RenderTexture2D scene, int screenW, int screenH) {
    PART_g_lastScreenW = screenW > 0 ? screenW : 1;
    PART_g_lastScreenH = screenH > 0 ? screenH : 1;

    bool reactive = PART_g_params.reactive;
    if (reactive) PART_UpdateSceneSample(scene);

    PART_g_spawnAccumulator += PART_g_params.spawnRate * GetFrameTime();
    while (PART_g_spawnAccumulator >= 1.0f && PART_g_aliveCount < PART_g_params.count) {
        PART_SpawnParticle(screenW, screenH);
        PART_g_spawnAccumulator -= 1.0f;
    }

    for (int i = 0; i < PART_g_aliveCount; i++) {
        PART_Particle *p = &PART_g_pool[i];
        bool outOfBounds =
            (PART_g_params.mode == PART_MODE_RAIN && p->position.y > screenH + 10.0f) ||
            (PART_g_params.mode == PART_MODE_EMBERS && p->position.y < -10.0f);
        if (outOfBounds) {
            PART_g_pool[i] = PART_g_pool[PART_g_aliveCount - 1];
            PART_g_aliveCount--;
            i--;
        }
    }

    ClearBackground((Color){ 0, 0, 0, 0 });

    DrawTextureRec(
        scene.texture,
        (Rectangle){ 0, 0, (float)scene.texture.width, -(float)scene.texture.height },
        (Vector2){ 0, 0 },
        WHITE
    );
    DrawRectangle(0, 0, screenW, screenH, (Color){ 11, 11, 14, 100 });

    float rs = PART_g_params.reactiveStrength;
    float invScreenW = 1.0f / (float)screenW;
    float invScreenH = 1.0f / (float)screenH;

    for (int i = 0; i < PART_g_aliveCount; i++) {
        PART_Particle *p = &PART_g_pool[i];
        float lifeRatio = 1.0f - (p->age / p->lifetime);
        if (lifeRatio < 0.0f) lifeRatio = 0.0f;

        Color base = PART_g_params.color;

        if (PART_g_params.mode == PART_MODE_RAIN) {
            Color tint = base;
            if (reactive) {
                float nx = p->position.x * invScreenW, ny = p->position.y * invScreenH;
                Color scn = PART_SampleSceneAvg(nx, ny);
                float luma = PART_Luma(scn);
                float motion = PART_SampleMotion(nx, ny);
                float boost = (luma * 60.0f + motion * 140.0f) * rs;
                tint = (Color){
                    (unsigned char)fminf(255, base.r + boost),
                    (unsigned char)fminf(255, base.g + boost),
                    (unsigned char)fminf(255, base.b + boost),
                    base.a
                };
            }
            tint.a = (unsigned char)(180 * lifeRatio + 40);
            float len = 0.045f; // longitud de la gota relativa a la velocidad
            Vector2 tail = { p->position.x - p->velocity.x * len, p->position.y - p->velocity.y * len };
            DrawLineEx(tail, p->position, PART_g_params.size * 0.5f, tint);

        } else if (PART_g_params.mode == PART_MODE_EMBERS) {
            float glow = 1.0f;
            if (reactive) {
                float nx = p->position.x * invScreenW, ny = p->position.y * invScreenH;
                Color scn = PART_SampleSceneAvg(nx, ny);
                float motion = PART_SampleMotion(nx, ny);
                glow = 1.0f + (PART_Luma(scn) * 1.0f + motion * 1.4f) * rs;
            }
            float radius = PART_g_params.size * (1.0f - PART_g_params.sizeFalloff * (1.0f - lifeRatio)) * glow;
            if (radius < 0.2f) radius = 0.2f;
            Color c = base;
            c.a = (unsigned char)(255 * lifeRatio);
            DrawCircleV(p->position, radius * 1.8f, (Color){ c.r, c.g, c.b, (unsigned char)(c.a * 0.35f) }); // halo
            DrawCircleV(p->position, radius, c);

        } else {
            float radius = PART_g_params.size * (1.0f - PART_g_params.sizeFalloff * (1.0f - lifeRatio));
            if (radius < 0.2f) radius = 0.2f;
            Color c = base;
            if (reactive) {
                float nx = p->position.x * invScreenW, ny = p->position.y * invScreenH;
                Color scn = PART_SampleSceneAvg(nx, ny);
                float motion = PART_SampleMotion(nx, ny);
                c = (Color){
                    (unsigned char)(base.r * (1.0f - rs) + scn.r * rs),
                    (unsigned char)(base.g * (1.0f - rs) + scn.g * rs),
                    (unsigned char)(base.b * (1.0f - rs) + scn.b * rs),
                    base.a
                };
                float boost = motion * 60.0f * rs;
                c.r = (unsigned char)fminf(255, c.r + boost);
                c.g = (unsigned char)fminf(255, c.g + boost);
                c.b = (unsigned char)fminf(255, c.b + boost);
            }
            c.a = (unsigned char)(255 * fmaxf(0.0f, lifeRatio));
            DrawCircleV(p->position, radius, c);
        }
    }
}

#endif /* PARTICLES_EFFECT_H */
