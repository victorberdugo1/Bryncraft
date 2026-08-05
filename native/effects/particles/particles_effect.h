/*
 * particles_effect.h — single-header particle system for raylib
 * Dependencia JSON solo en __EMSCRIPTEN__
 *
 * Modos:
 *   fountain — comportamiento original: brota de un punto (spawnX/spawnY)
 *   rain     — cae desde el borde superior, con viento y velocidad terminal
 *   embers   — sube desde abajo con vaivén, brilla más sobre zonas claras de la escena
 *
 * "reactive" (los 3 modos) hace un downsample de la escena a una textura
 * pequeña una vez por frame y la lee a CPU para:
 *   1) teñir/modular el color y brillo de las partículas según lo que hay debajo
 *   2) desviar su movimiento siguiendo el gradiente de luminancia de la escena
 *      (flow field: las partículas se curvan hacia zonas brillantes)
 *   3) sesgar dónde nacen (embers preferentemente sobre zonas claras, rain sobre
 *      zonas oscuras) — en fountain no aplica, porque el punto de nacimiento es fijo
 *
 * Además de luminancia estática, ahora se compara cada frame contra el
 * anterior (downsample vs downsample) para obtener un campo de "movimiento".
 * Esto es lo que hace que la interacción se sienta real: las partículas no
 * solo reaccionan a que algo esté claro/oscuro, reaccionan a que algo se
 * esté MOVIENDO en el vídeo (el gradiente de movimiento empuja el flow
 * field, y el spawn bias / tinte / brillo también lo tienen en cuenta).
 *
 * Apagado por defecto en el header (el toggle real lo pone effects.ts); el
 * readback GPU->CPU tiene coste, sobre todo en wasm, pero a esta resolución
 * es barato comparado con el grid de ASCII.
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

/* =========================================================
 * Implementación (prefijo PART_ para evitar conflictos)
 * ========================================================= */
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
    float phase; // fase aleatoria para el vaivén de embers
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
    float spawnX; // normalizado 0..1 (solo fountain)
    float spawnY; // normalizado 0..1 (solo fountain)
    float windX;  // solo rain/embers
    int reactive; // 0/1, ahora afecta a los 3 modos
    float reactiveStrength; // 0..1, cuánto pesa el tinte/sesgo de spawn
    float flowStrength;     // 0..2, cuánto empuja el gradiente de luz/movimiento al movimiento
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
/* Últimas dimensiones vistas en Draw(); Update() las necesita para normalizar
 * posiciones al muestrear el flow field, y puede llamarse antes que Draw() en
 * el primer frame o correr a una cadencia distinta de la escena offscreen. */
static int PART_g_lastScreenW = 1;
static int PART_g_lastScreenH = 1;

/* --- downsample de la escena para reactividad --- */
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

    // Antes de reemplazar el frame, lo guardamos como "anterior" para poder
    // calcular movimiento (diferencia entre frames) en vez de solo brillo
    // estático. Sin esto las partículas solo podían teñirse/sesgarse por
    // luz, nunca "seguir" algo que se mueve.
    if (PART_g_sceneImg.data) {
        if (PART_g_scenePrevImg.data) UnloadImage(PART_g_scenePrevImg);
        PART_g_scenePrevImg = PART_g_sceneImg; // toma ownership
        PART_g_hasPrevFrame = true;
    }
    PART_g_sceneImg = LoadImageFromTexture(PART_g_sceneSmall.texture);

    if (PART_g_hasPrevFrame) {
        if (PART_g_motionImg.data) UnloadImage(PART_g_motionImg);
        PART_g_motionImg = GenImageColor(PART_SCENE_GRID_W, PART_SCENE_GRID_H, BLACK);
        for (int y = 0; y < PART_SCENE_GRID_H; y++) {
            for (int x = 0; x < PART_SCENE_GRID_W; x++) {
                Color cur = GetImageColor(PART_g_sceneImg, x, y);
                Color prev = GetImageColor(PART_g_scenePrevImg, x, y);
                float diff = (fabsf((float)cur.r - (float)prev.r) +
                              fabsf((float)cur.g - (float)prev.g) +
                              fabsf((float)cur.b - (float)prev.b)) / (3.0f * 255.0f);
                float m = diff * 4.0f; // realce para que el movimiento se note claramente
                if (m > 1.0f) m = 1.0f;
                unsigned char mb = (unsigned char)(m * 255.0f);
                ImageDrawPixel(&PART_g_motionImg, x, y, (Color){ mb, mb, mb, 255 });
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
    return GetImageColor(PART_g_sceneImg, x, y);
}

/* Promedio 3x3 en vez de un solo texel: menos "ruidoso" para teñir partículas. */
static Color PART_SampleSceneAvg(float normX, float normY) {
    if (!PART_g_sceneImg.data) return WHITE;
    int cx = (int)(normX * PART_SCENE_GRID_W);
    int cy = (int)(normY * PART_SCENE_GRID_H);
    int rSum = 0, gSum = 0, bSum = 0, n = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int x = cx + dx, y = cy + dy;
            if (x < 0 || x >= PART_SCENE_GRID_W || y < 0 || y >= PART_SCENE_GRID_H) continue;
            Color c = GetImageColor(PART_g_sceneImg, x, y);
            rSum += c.r; gSum += c.g; bSum += c.b; n++;
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
    return GetImageColor(PART_g_motionImg, x, y).r / 255.0f;
}

/* Gradiente de luminancia en espacio normalizado (0..1). Apunta hacia donde
 * la escena se vuelve más brillante — se usa para que las partículas "fluyan"
 * siguiendo el contenido del vídeo en vez de moverse a ciegas. */
static Vector2 PART_SampleGradient(float normX, float normY) {
    if (!PART_g_sceneImg.data) return (Vector2){ 0, 0 };
    int x = (int)(normX * PART_SCENE_GRID_W);
    int y = (int)(normY * PART_SCENE_GRID_H);
    if (x < 1) x = 1; if (x > PART_SCENE_GRID_W - 2) x = PART_SCENE_GRID_W - 2;
    if (y < 1) y = 1; if (y > PART_SCENE_GRID_H - 2) y = PART_SCENE_GRID_H - 2;

    float lumaL = PART_Luma(GetImageColor(PART_g_sceneImg, x - 1, y));
    float lumaR = PART_Luma(GetImageColor(PART_g_sceneImg, x + 1, y));
    float lumaU = PART_Luma(GetImageColor(PART_g_sceneImg, x, y - 1));
    float lumaD = PART_Luma(GetImageColor(PART_g_sceneImg, x, y + 1));

    return (Vector2){ lumaR - lumaL, lumaD - lumaU };
}

/* Igual que el gradiente de luminancia, pero sobre el campo de movimiento.
 * Esto es lo que hace que las partículas se sientan atraídas hacia lo que
 * se está moviendo en el vídeo, no solo hacia lo que está iluminado. */
static Vector2 PART_SampleMotionGradient(float normX, float normY) {
    if (!PART_g_motionImg.data) return (Vector2){ 0, 0 };
    int x = (int)(normX * PART_SCENE_GRID_W);
    int y = (int)(normY * PART_SCENE_GRID_H);
    if (x < 1) x = 1; if (x > PART_SCENE_GRID_W - 2) x = PART_SCENE_GRID_W - 2;
    if (y < 1) y = 1; if (y > PART_SCENE_GRID_H - 2) y = PART_SCENE_GRID_H - 2;

    float mL = GetImageColor(PART_g_motionImg, x - 1, y).r / 255.0f;
    float mR = GetImageColor(PART_g_motionImg, x + 1, y).r / 255.0f;
    float mU = GetImageColor(PART_g_motionImg, x, y - 1).r / 255.0f;
    float mD = GetImageColor(PART_g_motionImg, x, y + 1).r / 255.0f;

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
        /* Las partículas vivas del modo anterior quedaban "atrapadas": Update()
         * les aplicaba la física del modo nuevo (p.ej. gotas de rain con la
         * aceleración de fountain) usando posiciones/velocidades pensadas para
         * el modo viejo, así que el cambio de modo se veía roto o parecía no
         * aplicarse. Al vaciar el pool, el modo nuevo arranca limpio. */
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
        float spawnNormX = (float)rand() / RAND_MAX;
        if (reactive) {
            /* La lluvia se concentra sobre las zonas oscuras Y sobre las
             * zonas donde algo se mueve en la escena, como si cayera sobre
             * las sombras/siluetas y siguiera la acción del vídeo. */
            for (int tries = 0; tries < 4; tries++) {
                float candidate = (float)rand() / RAND_MAX;
                float darkness = 1.0f - PART_SampleLuma(candidate, 0.05f);
                float motion = PART_SampleMotion(candidate, 0.05f);
                float bias = darkness * 0.6f + motion * 0.4f;
                float weight = 1.0f - PART_g_params.reactiveStrength + PART_g_params.reactiveStrength * bias;
                if (((float)rand() / RAND_MAX) <= weight) { spawnNormX = candidate; break; }
                spawnNormX = candidate;
            }
        }
        p->position = (Vector2){ spawnNormX * screenW, -10.0f };
        p->velocity = (Vector2){ PART_g_params.windX * 40.0f, fallSpeed };
        p->lifetime = PART_g_params.lifetime; // límite de seguridad, normalmente muere al salir por abajo
        p->alive = true;
    } else if (PART_g_params.mode == PART_MODE_EMBERS) {
        float riseSpeed = 20.0f + PART_g_params.gravity * 4.0f;
        float spawnNormX = (float)rand() / RAND_MAX;
        if (reactive) {
            /* Los embers nacen preferentemente bajo zonas brillantes o con
             * movimiento, como si emanaran de un punto de luz/acción real. */
            for (int tries = 0; tries < 4; tries++) {
                float candidate = (float)rand() / RAND_MAX;
                float brightness = PART_SampleLuma(candidate, 0.9f);
                float motion = PART_SampleMotion(candidate, 0.9f);
                float bias = brightness * 0.6f + motion * 0.4f;
                float weight = 1.0f - PART_g_params.reactiveStrength + PART_g_params.reactiveStrength * bias;
                if (((float)rand() / RAND_MAX) <= weight) { spawnNormX = candidate; break; }
                spawnNormX = candidate;
            }
        }
        p->position = (Vector2){ spawnNormX * screenW, screenH + ((float)rand() / RAND_MAX) * 20.0f };
        p->velocity = (Vector2){ 0.0f, -riseSpeed };
        p->lifetime = PART_g_params.lifetime * (0.7f + ((float)rand() / RAND_MAX) * 0.6f);
        p->alive = true;
    } else { // FOUNTAIN (original)
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
            // la muerte real por salir del encuadre se resuelve en Draw()
        } else if (PART_g_params.mode == PART_MODE_EMBERS) {
            p->velocity.x = sinf(p->phase + p->age * 2.0f) * 12.0f + PART_g_params.windX * 20.0f;
            p->position.x += p->velocity.x * dt;
            p->position.y += p->velocity.y * dt;
        } else {
            p->velocity.y += PART_g_params.gravity * dt * 20.0f;
            p->position.x += p->velocity.x * dt;
            p->position.y += p->velocity.y * dt;
        }

        /* Flow field: el gradiente de luminancia Y el gradiente de movimiento
         * de la escena empujan a la partícula — esto es lo que hace que de
         * verdad "sigan" el contenido del vídeo (incluida la acción), no solo
         * se tiñan de color. El tirón por movimiento pesa más porque es lo
         * que se percibe como interacción "real" con lo que pasa en pantalla. */
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

    // Muerte por salir del encuadre (rain hacia abajo, embers hacia arriba)
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

    // Dibuja la escena de fondo (textura) ligeramente atenuada
    DrawTextureRec(
        scene.texture,
        (Rectangle){ 0, 0, (float)scene.texture.width, -(float)scene.texture.height },
        (Vector2){ 0, 0 },
        WHITE
    );
    DrawRectangle(0, 0, screenW, screenH, (Color){ 11, 11, 14, 100 });

    float rs = PART_g_params.reactiveStrength;

    for (int i = 0; i < PART_g_aliveCount; i++) {
        PART_Particle *p = &PART_g_pool[i];
        float lifeRatio = 1.0f - (p->age / p->lifetime);
        if (lifeRatio < 0.0f) lifeRatio = 0.0f;

        Color base = PART_g_params.color;

        if (PART_g_params.mode == PART_MODE_RAIN) {
            Color tint = base;
            if (reactive) {
                float nx = p->position.x / screenW, ny = p->position.y / screenH;
                Color scn = PART_SampleSceneAvg(nx, ny);
                float luma = PART_Luma(scn);
                float motion = PART_SampleMotion(nx, ny);
                float boost = (luma * 60.0f + motion * 140.0f) * rs; // el movimiento resalta mucho más que el brillo estático
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
                float nx = p->position.x / screenW, ny = p->position.y / screenH;
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

        } else { // FOUNTAIN
            float radius = PART_g_params.size * (1.0f - PART_g_params.sizeFalloff * (1.0f - lifeRatio));
            if (radius < 0.2f) radius = 0.2f;
            Color c = base;
            if (reactive) {
                float nx = p->position.x / screenW, ny = p->position.y / screenH;
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
