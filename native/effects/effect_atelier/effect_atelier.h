#ifndef EFFECT_ATELIER_H
#define EFFECT_ATELIER_H

#include "raylib.h"
#include "rlgl.h"
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

void EffectAtelierEffect_Init(void);
#ifdef __EMSCRIPTEN__
void EffectAtelierEffect_SetParams(const JsonValue *paramsObj);
#endif
void EffectAtelierEffect_Update(float dt);
void EffectAtelierEffect_Draw(RenderTexture2D scene, int screenW, int screenH);
void EffectAtelierEffect_Unload(void);

#ifdef __cplusplus
}
#endif

#define EB_PARTICLES_PER_LAYER 64
#define EB_MAX_LAYERS 4
#define EB_MAX_PARTICLES (EB_PARTICLES_PER_LAYER * EB_MAX_LAYERS)
#define EB_NAME_MAX 32

#define EB_ELEM_NEUTRAL   0
#define EB_ELEM_FIRE      1
#define EB_ELEM_WATER     2
#define EB_ELEM_EARTH     3
#define EB_ELEM_WIND      4
#define EB_ELEM_LIGHTNING 5
#define EB_ELEM_DARK      6
#define EB_ELEM_POISON    7
#define EB_ELEM_LIGHT     8
#define EB_ELEM_ICE       9

#define EB_SHAPE_SPHERE     0
#define EB_SHAPE_RING       1
#define EB_SHAPE_SPIRAL     2
#define EB_SHAPE_BEAM       3
#define EB_SHAPE_PILLAR     4
#define EB_SHAPE_RAIN       5
#define EB_SHAPE_WAVE       6
#define EB_SHAPE_PROJECTILE 7
#define EB_SHAPE_JUMP       8
#define EB_SHAPE_SHIELD     9
#define EB_SHAPE_FIRE_ORBS  10
#define EB_SHAPE_WIND_SPIN  11
#define EB_SHAPE_FIRE_WIND  12
#define EB_SHAPE_FIELD      13

#define EB_MAX_ORBS      8
#define EB_TRAIL_LEN     24
#define EB_FUNNEL_LINES  10
#define EB_FUNNEL_SAMPLES 40

typedef struct {
    Vector3 pos, vel;
    float   age, life, size;
    Color   colorCore, colorMid, colorOuter;
    bool    active;
} EB_Particle;

typedef struct {
    int   enabled;
    int   element;
    int   shape;
    Color colorCore, colorMid, colorOuter;
    int   particleCount;
} EB_ExtraLayer;

typedef struct {
    int   element;
    int   shape;
    char  presetName[EB_NAME_MAX];

    int   particleCount;
    float spawnRadiusMin, spawnRadiusMax;
    float radius;
    float directionYaw;
    float speedMin, speedMax;
    float lifeMin, lifeMax;
    float loopInterval;

    float gravity, drag;

    Color colorCore, colorMid, colorOuter;
    int   additive;

    EB_ExtraLayer extraLayers[3];

    float shieldFacingDeg;
    int   shieldAutoRotate;
    float shieldRotateSpeedDeg;
    float shieldArchWidthDeg, shieldArchHeightDeg;
    float shieldHexSize;
    float shieldFlickerSpeed;
    float shieldImpactInterval;

    float fieldHexSize;
    float fieldFlickerSpeed;
    float fieldPulseSpeed, fieldPulseAmount;
    float fieldRotationSpeedDeg;

    int   orbCount;
    float orbSize, orbitRadius, orbitSpeedDeg, orbBobAmount, orbBobSpeed;
    float trailFade;

    float windHelixHeight, windHelixTurns, windRibbonWidth;
    int   windFunnelLines;

    float cameraDistance;
    float cameraOrbitSpeed;
    int   showGrid;
} EB_Params;

static EB_Params EB_g_params = {
    .element = EB_ELEM_FIRE,
    .shape = EB_SHAPE_SPHERE,
    .presetName = "custom_burst",
    .particleCount = 20,
    .spawnRadiusMin = 0.3f, .spawnRadiusMax = 1.0f,
    .radius = 1.0f,
    .directionYaw = 0.0f,
    .speedMin = 1.2f, .speedMax = 2.8f,
    .lifeMin = 0.35f, .lifeMax = 0.8f,
    .loopInterval = 1.2f,
    .gravity = 1.2f, .drag = 0.6f,
    .colorCore  = (Color){255, 224, 140, 255},
    .colorMid   = (Color){255, 120,  24, 255},
    .colorOuter = (Color){255,  60,   0, 255},
    .additive = 1,

    .shieldFacingDeg = 0.0f,
    .shieldAutoRotate = 1,
    .shieldRotateSpeedDeg = 25.0f,
    .shieldArchWidthDeg = 110.0f, .shieldArchHeightDeg = 80.0f,
    .shieldHexSize = 0.16f,
    .shieldFlickerSpeed = 3.0f,
    .shieldImpactInterval = 2.0f,

    .fieldHexSize = 0.18f,
    .fieldFlickerSpeed = 2.0f,
    .fieldPulseSpeed = 1.2f, .fieldPulseAmount = 0.04f,
    .fieldRotationSpeedDeg = 12.0f,

    .orbCount = 4,
    .orbSize = 0.14f, .orbitRadius = 1.1f, .orbitSpeedDeg = 140.0f,
    .orbBobAmount = 0.2f, .orbBobSpeed = 2.2f,
    .trailFade = 0.9f,

    .windHelixHeight = 2.4f, .windHelixTurns = 2.5f, .windRibbonWidth = 0.06f,
    .windFunnelLines = 6,

    .cameraDistance = 5.5f,
    .cameraOrbitSpeed = 18.0f,
    .showGrid = 1,
};

static EB_Particle EB_g_particles[EB_MAX_PARTICLES];
static float        EB_g_loopAccum   = 0.0f;
static float        EB_g_orbitAngle  = 0.0f;
static float        EB_g_orbAngle    = 0.0f;
static float        EB_g_time        = 0.0f;
static float        EB_g_impactAccum = 0.0f;
static float        EB_g_shieldFacing     = 0.0f;
static float        EB_g_shieldImpactFlash = 0.0f;
static float        EB_g_fieldYaw    = 0.0f;
static Camera3D     EB_g_camera;

static int EB_g_shieldActive   = 0;
static int EB_g_fieldActive    = 0;
static int EB_g_fireOrbsActive = 0;
static int EB_g_windSpinActive = 0;
static int EB_g_fireWindActive = 0;

static Color EB_g_shieldColorCore, EB_g_shieldColorMid, EB_g_shieldColorOuter;
static Color EB_g_fieldColorCore, EB_g_fieldColorMid, EB_g_fieldColorOuter;
static Color EB_g_fireOrbsColorCore, EB_g_fireOrbsColorMid, EB_g_fireOrbsColorOuter;
static Color EB_g_windSpinColorCore, EB_g_windSpinColorMid, EB_g_windSpinColorOuter;
static Color EB_g_fireWindColorCore, EB_g_fireWindColorMid, EB_g_fireWindColorOuter;

typedef struct { Vector3 pos; float life, maxLife; } EB_Trail;
static EB_Trail EB_g_trails[EB_MAX_ORBS][EB_TRAIL_LEN];
static int       EB_g_trailHead[EB_MAX_ORBS];
static float      EB_g_orbPhase[EB_MAX_ORBS];

static inline float EB_RandF(float lo, float hi) {
    return lo + (hi - lo) * ((float)rand() / (float)RAND_MAX);
}

static inline float EB_Clamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline Vector3 EB_V3Add(Vector3 a, Vector3 b)   { return (Vector3){ a.x+b.x, a.y+b.y, a.z+b.z }; }
static inline Vector3 EB_V3Sub(Vector3 a, Vector3 b)   { return (Vector3){ a.x-b.x, a.y-b.y, a.z-b.z }; }
static inline Vector3 EB_V3Scale(Vector3 a, float s)   { return (Vector3){ a.x*s, a.y*s, a.z*s }; }
static inline Vector3 EB_V3Cross(Vector3 a, Vector3 b) { return (Vector3){ a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x }; }
static inline float   EB_V3Dot(Vector3 a, Vector3 b)   { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline float   EB_V3Length(Vector3 a)           { return sqrtf(EB_V3Dot(a, a)); }
static inline float   EB_V3Distance(Vector3 a, Vector3 b) { return EB_V3Length(EB_V3Sub(a, b)); }
static inline Vector3 EB_V3Normalize(Vector3 a) {
    float len = EB_V3Length(a);
    if (len <= 0.00001f) return (Vector3){0, 0, 0};
    return EB_V3Scale(a, 1.0f / len);
}

static inline float EB_FadeAlpha(float age, float life, float fadeIn, float fadeOut) {
    if (life <= 0.0f || age < 0.0f) return 0.0f;
    if (age < fadeIn) return EB_Clamp(age / fadeIn, 0.0f, 1.0f);
    if (age > life - fadeOut) return EB_Clamp((life - age) / fadeOut, 0.0f, 1.0f);
    return 1.0f;
}

static inline Color EB_LerpColor(Color a, Color b, float t) {
    t = EB_Clamp(t, 0.0f, 1.0f);
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t),
    };
}

/* Dibuja un hexágono relleno (fan) + contorno, en la posición 3D 'p',
   orientado en el plano tangente definido por (tU, tV), con "punta arriba".
   Portado 1:1 de _CVFX_DrawHexPanel en effects.h — usado tanto por el
   shape "shield" (arco frontal) como por "field" (cúpula completa). */
static void EB_DrawHexPanel(Vector3 p, Vector3 tU, Vector3 tV, float size, Color fillCol, Color edgeCol) {
    Vector3 verts[6];
    for (int k = 0; k < 6; k++) {
        float ang = DEG2RAD * (60.0f * k + 30.0f);
        float ox = cosf(ang) * size, oy = sinf(ang) * size;
        verts[k] = (Vector3){
            p.x + tU.x * ox + tV.x * oy,
            p.y + tU.y * ox + tV.y * oy,
            p.z + tU.z * ox + tV.z * oy
        };
    }
    for (int k = 0; k < 6; k++) {
        Vector3 a = verts[k], b = verts[(k + 1) % 6];
        DrawTriangle3D(p, a, b, fillCol);
    }
    for (int k = 0; k < 6; k++)
        DrawLine3D(verts[k], verts[(k + 1) % 6], edgeCol);
}

static void EB_ResetOrbits(void) {
    for (int i = 0; i < EB_MAX_ORBS; i++) {
        EB_g_orbPhase[i] = ((float)i / (float)EB_g_params.orbCount) * 2.0f * PI;
        EB_g_trailHead[i] = 0;
        for (int t = 0; t < EB_TRAIL_LEN; t++) EB_g_trails[i][t] = (EB_Trail){0};
    }
}

static void EB_PushTrailSample(int orbIndex, Vector3 pos) {
    int head = EB_g_trailHead[orbIndex];
    EB_g_trails[orbIndex][head] = (EB_Trail){ .pos = pos, .life = 1.0f, .maxLife = 1.0f };
    EB_g_trailHead[orbIndex] = (head + 1) % EB_TRAIL_LEN;
}

static inline Vector3 EB_RandomOnSphere(float minR, float maxR) {
    float theta = EB_RandF(0.0f, 2.0f * PI);
    float phi   = EB_RandF(0.15f, PI - 0.15f);
    float r     = EB_RandF(minR, maxR);
    return (Vector3){ r * sinf(phi) * cosf(theta), r * cosf(phi), r * sinf(phi) * sinf(theta) };
}

static void EB_SpawnLayer(EB_Particle *particles, int shape, int count,
                           float radius, float directionYaw,
                           float speedMin, float speedMax,
                           float lifeMin, float lifeMax,
                           float spawnRadiusMin, float spawnRadiusMax,
                           Color core, Color mid, Color outer)
{
    if (count > EB_PARTICLES_PER_LAYER) count = EB_PARTICLES_PER_LAYER;
    Vector3 center = (Vector3){0, 0.05f, 0};

    switch (shape) {
        case EB_SHAPE_RING:
            for (int i = 0; i < count; i++) {
                EB_Particle *p = &particles[i];
                float a = (float)i / count * 2.0f * PI + EB_RandF(-0.15f, 0.15f);
                float spd = EB_RandF(speedMin, speedMax);
                p->pos    = (Vector3){ center.x + cosf(a) * radius, center.y, center.z + sinf(a) * radius };
                p->vel    = (Vector3){ cosf(a) * spd * 0.35f, spd * 0.6f, sinf(a) * spd * 0.35f };
                p->age    = 0.0f;
                p->life   = EB_RandF(lifeMin, lifeMax);
                p->size   = EB_RandF(0.05f, 0.12f);
                p->active = true;
            }
            break;

        case EB_SHAPE_SPIRAL:
            for (int i = 0; i < count; i++) {
                EB_Particle *p = &particles[i];
                float t = (float)i / count;
                float a = t * 2.0f * PI * 2.5f;
                float r = radius * t;
                float spd = EB_RandF(speedMin, speedMax);
                p->pos    = (Vector3){ center.x + cosf(a) * r, center.y, center.z + sinf(a) * r };
                p->vel    = (Vector3){ cosf(a) * 0.3f, spd, sinf(a) * 0.3f };
                p->age    = 0.0f;
                p->life   = EB_RandF(lifeMin, lifeMax);
                p->size   = EB_RandF(0.05f, 0.12f);
                p->active = true;
            }
            break;

        case EB_SHAPE_BEAM: {
            float yaw = directionYaw * DEG2RAD;
            Vector3 dir = (Vector3){ sinf(yaw), 0.0f, cosf(yaw) };
            for (int i = 0; i < count; i++) {
                EB_Particle *p = &particles[i];
                float spd = EB_RandF(speedMin, speedMax);
                Vector3 v = { dir.x + EB_RandF(-0.15f, 0.15f), dir.y + EB_RandF(-0.15f, 0.15f), dir.z + EB_RandF(-0.15f, 0.15f) };
                v = EB_V3Scale(EB_V3Normalize(v), spd);
                p->pos    = center;
                p->vel    = v;
                p->age    = 0.0f;
                p->life   = EB_RandF(lifeMin, lifeMax);
                p->size   = EB_RandF(0.05f, 0.12f);
                p->active = true;
            }
            break;
        }

        case EB_SHAPE_PILLAR:
            for (int i = 0; i < count; i++) {
                EB_Particle *p = &particles[i];
                float a = EB_RandF(0.0f, 2.0f * PI), r = EB_RandF(0.0f, radius);
                float spd = EB_RandF(speedMin, speedMax);
                p->pos    = (Vector3){ center.x + cosf(a) * r, center.y, center.z + sinf(a) * r };
                p->vel    = (Vector3){ EB_RandF(-0.1f, 0.1f), spd, EB_RandF(-0.1f, 0.1f) };
                p->age    = 0.0f;
                p->life   = EB_RandF(lifeMin, lifeMax);
                p->size   = EB_RandF(0.05f, 0.12f);
                p->active = true;
            }
            break;

        case EB_SHAPE_RAIN:
            for (int i = 0; i < count; i++) {
                EB_Particle *p = &particles[i];
                float a = EB_RandF(0.0f, 2.0f * PI), r = EB_RandF(0.0f, radius);
                float spd = EB_RandF(speedMin, speedMax);
                p->pos    = (Vector3){ center.x + cosf(a) * r, center.y + EB_RandF(1.5f, 3.0f), center.z + sinf(a) * r };
                p->vel    = (Vector3){ EB_RandF(-0.1f, 0.1f), -spd, EB_RandF(-0.1f, 0.1f) };
                p->age    = 0.0f;
                p->life   = EB_RandF(lifeMin, lifeMax);
                p->size   = EB_RandF(0.05f, 0.12f);
                p->active = true;
            }
            break;

        case EB_SHAPE_WAVE:
            for (int i = 0; i < count; i++) {
                EB_Particle *p = &particles[i];
                float t = (float)i / count;
                float a = t * 2.0f * PI;
                float crest = 0.5f + 0.5f * sinf(a * 3.0f);
                float spd = EB_RandF(speedMin, speedMax);
                p->pos    = (Vector3){ center.x + cosf(a) * radius, center.y, center.z + sinf(a) * radius };
                p->vel    = (Vector3){ cosf(a) * spd * 0.4f, spd * (0.3f + crest * 0.9f), sinf(a) * spd * 0.4f };
                p->age    = 0.0f;
                p->life   = EB_RandF(lifeMin, lifeMax);
                p->size   = EB_RandF(0.05f, 0.12f);
                p->active = true;
            }
            break;

        case EB_SHAPE_PROJECTILE: {
            float yaw = directionYaw * DEG2RAD;
            Vector3 dir = (Vector3){ sinf(yaw), 0.0f, cosf(yaw) };
            for (int i = 0; i < count; i++) {
                EB_Particle *p = &particles[i];
                float t = (float)i / count;
                float spd = EB_RandF(speedMin, speedMax);
                p->pos    = EB_V3Add(center, EB_V3Scale(dir, -radius * 0.5f * t));
                p->vel    = (Vector3){ dir.x * spd + EB_RandF(-0.04f, 0.04f), EB_RandF(-0.03f, 0.03f), dir.z * spd + EB_RandF(-0.04f, 0.04f) };
                p->age    = 0.0f;
                p->life   = EB_RandF(lifeMin, lifeMax);
                p->size   = EB_RandF(0.04f, 0.09f);
                p->active = true;
            }
            break;
        }

        case EB_SHAPE_JUMP:
            for (int i = 0; i < count; i++) {
                EB_Particle *p = &particles[i];
                float t = (float)i / count;
                float a = t * 2.0f * PI * 3.0f;
                float r = radius * 0.35f;
                float spd = EB_RandF(speedMin, speedMax);
                p->pos    = (Vector3){ center.x + cosf(a) * r, center.y, center.z + sinf(a) * r };
                p->vel    = (Vector3){ cosf(a) * 0.5f, spd * 1.4f, sinf(a) * 0.5f };
                p->age    = 0.0f;
                p->life   = EB_RandF(lifeMin, lifeMax);
                p->size   = EB_RandF(0.05f, 0.12f);
                p->active = true;
            }
            break;

        case EB_SHAPE_SHIELD:
        case EB_SHAPE_FIELD:
        case EB_SHAPE_FIRE_ORBS:
        case EB_SHAPE_WIND_SPIN:
        case EB_SHAPE_FIRE_WIND:
            break;

        case EB_SHAPE_SPHERE:
        default:
            for (int i = 0; i < count; i++) {
                EB_Particle *p = &particles[i];
                Vector3 dir = EB_RandomOnSphere(spawnRadiusMin, spawnRadiusMax);
                float spd = EB_RandF(speedMin, speedMax);
                p->pos    = center;
                p->vel    = EB_V3Scale(EB_V3Normalize(dir), spd);
                p->age    = 0.0f;
                p->life   = EB_RandF(lifeMin, lifeMax);
                p->size   = EB_RandF(0.05f, 0.12f);
                p->active = true;
            }
            break;
    }

    for (int i = 0; i < count; i++) {
        particles[i].colorCore  = core;
        particles[i].colorMid   = mid;
        particles[i].colorOuter = outer;
    }
}

static inline int EB_IsMeshShape(int shape) {
    return shape == EB_SHAPE_SHIELD || shape == EB_SHAPE_FIELD || shape == EB_SHAPE_FIRE_ORBS ||
           shape == EB_SHAPE_WIND_SPIN || shape == EB_SHAPE_FIRE_WIND;
}

static void EB_UpdateActiveMeshShapes(void) {
    EB_g_shieldActive = EB_g_fieldActive = EB_g_fireOrbsActive = EB_g_windSpinActive = EB_g_fireWindActive = 0;

    int shapes[4], count = 0;
    Color cores[4], mids[4], outers[4];

    shapes[count] = EB_g_params.shape;
    cores[count]  = EB_g_params.colorCore;
    mids[count]   = EB_g_params.colorMid;
    outers[count] = EB_g_params.colorOuter;
    count++;

    for (int L = 0; L < 3; L++) {
        const EB_ExtraLayer *layer = &EB_g_params.extraLayers[L];
        if (!layer->enabled) continue;
        shapes[count] = layer->shape;
        cores[count]  = layer->colorCore;
        mids[count]   = layer->colorMid;
        outers[count] = layer->colorOuter;
        count++;
    }

    for (int i = 0; i < count; i++) {
        switch (shapes[i]) {
            case EB_SHAPE_SHIELD:
                if (!EB_g_shieldActive) {
                    EB_g_shieldColorCore = cores[i]; EB_g_shieldColorMid = mids[i]; EB_g_shieldColorOuter = outers[i];
                }
                EB_g_shieldActive = 1;
                break;
            case EB_SHAPE_FIELD:
                if (!EB_g_fieldActive) {
                    EB_g_fieldColorCore = cores[i]; EB_g_fieldColorMid = mids[i]; EB_g_fieldColorOuter = outers[i];
                }
                EB_g_fieldActive = 1;
                break;
            case EB_SHAPE_FIRE_ORBS:
                if (!EB_g_fireOrbsActive) {
                    EB_g_fireOrbsColorCore = cores[i]; EB_g_fireOrbsColorMid = mids[i]; EB_g_fireOrbsColorOuter = outers[i];
                }
                EB_g_fireOrbsActive = 1;
                break;
            case EB_SHAPE_WIND_SPIN:
                if (!EB_g_windSpinActive) {
                    EB_g_windSpinColorCore = cores[i]; EB_g_windSpinColorMid = mids[i]; EB_g_windSpinColorOuter = outers[i];
                }
                EB_g_windSpinActive = 1;
                break;
            case EB_SHAPE_FIRE_WIND:
                if (!EB_g_fireWindActive) {
                    EB_g_fireWindColorCore = cores[i]; EB_g_fireWindColorMid = mids[i]; EB_g_fireWindColorOuter = outers[i];
                }
                EB_g_fireWindActive = 1;
                break;
            default: break;
        }
    }
}

static void EB_SpawnBurst(void) {
    for (int i = 0; i < EB_MAX_PARTICLES; i++) EB_g_particles[i].active = false;

    EB_SpawnLayer(&EB_g_particles[0], EB_g_params.shape, EB_g_params.particleCount,
                  EB_g_params.radius, EB_g_params.directionYaw,
                  EB_g_params.speedMin, EB_g_params.speedMax,
                  EB_g_params.lifeMin, EB_g_params.lifeMax,
                  EB_g_params.spawnRadiusMin, EB_g_params.spawnRadiusMax,
                  EB_g_params.colorCore, EB_g_params.colorMid, EB_g_params.colorOuter);

    for (int L = 0; L < 3; L++) {
        const EB_ExtraLayer *layer = &EB_g_params.extraLayers[L];
        if (!layer->enabled) continue;
        EB_SpawnLayer(&EB_g_particles[(L + 1) * EB_PARTICLES_PER_LAYER], layer->shape, layer->particleCount,
                      EB_g_params.radius, EB_g_params.directionYaw,
                      EB_g_params.speedMin, EB_g_params.speedMax,
                      EB_g_params.lifeMin, EB_g_params.lifeMax,
                      EB_g_params.spawnRadiusMin, EB_g_params.spawnRadiusMax,
                      layer->colorCore, layer->colorMid, layer->colorOuter);
    }
}

static void EB_UpdateShield(float dt) {
    if (EB_g_params.shieldAutoRotate) EB_g_shieldFacing += EB_g_params.shieldRotateSpeedDeg * DEG2RAD * dt;

    EB_g_impactAccum += dt;
    if (EB_g_impactAccum >= EB_g_params.shieldImpactInterval) {
        EB_g_impactAccum = 0.0f;
        EB_g_shieldImpactFlash = 1.0f;
    }
    EB_g_shieldImpactFlash = fmaxf(0.0f, EB_g_shieldImpactFlash - dt * 2.5f);
}

static void EB_UpdateField(float dt) {
    EB_g_fieldYaw += EB_g_params.fieldRotationSpeedDeg * DEG2RAD * dt;
}

static void EB_UpdateFireOrbs(float dt) {
    EB_g_orbAngle += EB_g_params.orbitSpeedDeg * DEG2RAD * dt;
    for (int i = 0; i < EB_g_params.orbCount; i++) {
        float a = EB_g_orbAngle + EB_g_orbPhase[i];
        float bob = sinf(EB_g_time * EB_g_params.orbBobSpeed + EB_g_orbPhase[i]) * EB_g_params.orbBobAmount;
        Vector3 pos = (Vector3){
            cosf(a) * EB_g_params.orbitRadius,
            0.7f + bob,
            sinf(a) * EB_g_params.orbitRadius,
        };
        EB_PushTrailSample(i, pos);
    }
    for (int i = 0; i < EB_g_params.orbCount; i++)
        for (int t = 0; t < EB_TRAIL_LEN; t++)
            if (EB_g_trails[i][t].life > 0.0f)
                EB_g_trails[i][t].life -= dt * (1.0f / fmaxf(EB_g_params.trailFade, 0.05f));
}

static void EB_UpdateWindSpin(float dt) {
    EB_g_orbAngle += EB_g_params.orbitSpeedDeg * DEG2RAD * dt;
    for (int i = 0; i < EB_g_params.orbCount; i++) {
        float loopT = fmodf(EB_g_time * 0.4f + EB_g_orbPhase[i] / (2.0f * PI), 1.0f);
        float y = loopT * EB_g_params.windHelixHeight;
        float taper = 1.0f - 0.6f * loopT;
        float a = EB_g_orbAngle * EB_g_params.windHelixTurns + EB_g_orbPhase[i];
        Vector3 pos = (Vector3){
            cosf(a) * EB_g_params.orbitRadius * taper,
            y,
            sinf(a) * EB_g_params.orbitRadius * taper,
        };
        EB_PushTrailSample(i, pos);
    }
    for (int i = 0; i < EB_g_params.orbCount; i++)
        for (int t = 0; t < EB_TRAIL_LEN; t++)
            if (EB_g_trails[i][t].life > 0.0f)
                EB_g_trails[i][t].life -= dt * (1.0f / fmaxf(EB_g_params.trailFade, 0.05f));
}

static void EB_UpdateFireWind(float dt) {
    EB_g_orbAngle += EB_g_params.orbitSpeedDeg * DEG2RAD * dt;
    for (int i = 0; i < EB_g_params.orbCount; i++) {
        float loopT = fmodf(EB_g_time * 0.4f + EB_g_orbPhase[i] / (2.0f * PI), 1.0f);
        float bob = sinf(EB_g_time * EB_g_params.orbBobSpeed + EB_g_orbPhase[i]) * EB_g_params.orbBobAmount;
        float y = loopT * EB_g_params.windHelixHeight + bob;
        float taper = 1.0f - 0.6f * loopT;
        float a = EB_g_orbAngle * EB_g_params.windHelixTurns + EB_g_orbPhase[i];
        Vector3 pos = (Vector3){
            cosf(a) * EB_g_params.orbitRadius * taper,
            y,
            sinf(a) * EB_g_params.orbitRadius * taper,
        };
        EB_PushTrailSample(i, pos);
    }
    for (int i = 0; i < EB_g_params.orbCount; i++)
        for (int t = 0; t < EB_TRAIL_LEN; t++)
            if (EB_g_trails[i][t].life > 0.0f)
                EB_g_trails[i][t].life -= dt * (1.0f / fmaxf(EB_g_params.trailFade, 0.05f));
}

static void EB_DrawShieldMesh(void) {
    Vector3 c = { 0.0f, 0.95f, 0.0f };
    float r = EB_g_params.radius * 0.85f;

    Vector3 fwd   = { sinf(EB_g_shieldFacing), 0.0f, cosf(EB_g_shieldFacing) };
    Vector3 right = { cosf(EB_g_shieldFacing), 0.0f, -sinf(EB_g_shieldFacing) };
    Vector3 up    = { 0.0f, 1.0f, 0.0f };

    float maxH = EB_g_params.shieldArchWidthDeg  * 0.5f * DEG2RAD;
    float maxV = EB_g_params.shieldArchHeightDeg * 0.5f * DEG2RAD;
    float hexSize = EB_g_params.shieldHexSize;
    float colStep = hexSize * 1.6f;
    float rowStep = hexSize * 1.5f;

    int cols = (int)(maxH / colStep) + 2;
    int rows = (int)(maxV / rowStep) + 2;

    for (int rv = -rows; rv <= rows; rv++) {
        float angV = rv * rowStep;
        if (fabsf(angV) > maxV) continue;
        int parity = ((rv % 2) + 2) % 2;
        float rowOffset = parity ? colStep * 0.5f : 0.0f;

        for (int rh = -cols; rh <= cols; rh++) {
            float angH = rh * colStep + rowOffset;
            if (fabsf(angH) > maxH) continue;

            float nh = angH / maxH, nv = angV / maxV;
            if (nh * nh + nv * nv > 1.05f) continue;

            Vector3 dir = {
                fwd.x * cosf(angV) * cosf(angH) + right.x * cosf(angV) * sinf(angH) + up.x * sinf(angV),
                fwd.y * cosf(angV) * cosf(angH) + right.y * cosf(angV) * sinf(angH) + up.y * sinf(angV),
                fwd.z * cosf(angV) * cosf(angH) + right.z * cosf(angV) * sinf(angH) + up.z * sinf(angV)
            };
            float dl = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            if (dl > 0.00001f) { dir.x /= dl; dir.y /= dl; dir.z /= dl; }
            Vector3 p = { c.x + dir.x * r, c.y + dir.y * r, c.z + dir.z * r };

            Vector3 tU = {
                right.x * cosf(angH) - fwd.x * sinf(angH),
                right.y * cosf(angH) - fwd.y * sinf(angH),
                right.z * cosf(angH) - fwd.z * sinf(angH)
            };
            float tuLen = sqrtf(tU.x * tU.x + tU.y * tU.y + tU.z * tU.z);
            if (tuLen > 0.00001f) { tU.x /= tuLen; tU.y /= tuLen; tU.z /= tuLen; }
            Vector3 tV = {
                dir.y * tU.z - dir.z * tU.y,
                dir.z * tU.x - dir.x * tU.z,
                dir.x * tU.y - dir.y * tU.x
            };

            float seed = (float)(rh * 13 + rv * 7);
            float flicker = 0.75f + 0.25f * sinf(EB_g_time * EB_g_params.shieldFlickerSpeed + seed);
            float edgeFromCenter = sqrtf(nh * nh + nv * nv);
            float cellA = flicker * (0.55f + 0.45f * edgeFromCenter);
            cellA = fminf(1.0f, fmaxf(0.0f, cellA + EB_g_shieldImpactFlash * 0.5f));

            Color fillCol = EB_g_shieldColorMid; fillCol.a = (unsigned char)(cellA * 32.0f);
            Color edgeCol = EB_g_shieldColorMid; edgeCol.a = (unsigned char)(cellA * 170.0f);

            float cellSize = hexSize * r * 0.95f;
            EB_DrawHexPanel(p, tU, tV, cellSize, fillCol, edgeCol);
        }
    }

    if (EB_g_shieldImpactFlash > 0.02f) {
        Color glowCol = EB_g_shieldColorCore;
        glowCol.a = (unsigned char)(fminf(1.0f, EB_g_shieldImpactFlash) * 200.0f);
        Vector3 flashPos = { c.x + fwd.x * r, c.y + fwd.y * r, c.z + fwd.z * r };
        rlSetBlendMode(BLEND_ADDITIVE);
        DrawSphere(flashPos, 0.18f + EB_g_shieldImpactFlash * 0.25f, glowCol);
        rlSetBlendMode(BLEND_ALPHA);
    }
}

static void EB_DrawFieldMesh(void) {
    Vector3 c = { 0.0f, 0.9f, 0.0f };
    float pulse = 1.0f + sinf(EB_g_time * EB_g_params.fieldPulseSpeed) * EB_g_params.fieldPulseAmount;
    float r = EB_g_params.radius * pulse;

    Vector3 fwd   = { sinf(EB_g_fieldYaw), 0.0f, cosf(EB_g_fieldYaw) };
    Vector3 right = { cosf(EB_g_fieldYaw), 0.0f, -sinf(EB_g_fieldYaw) };
    Vector3 up    = { 0.0f, 1.0f, 0.0f };

    float maxH = PI;
    float maxV = 0.5f * PI;
    float hexSize = EB_g_params.fieldHexSize;
    float colStep = hexSize * 1.6f;
    float rowStep = hexSize * 1.5f;

    int cols = (int)(maxH / colStep) + 2;
    int rows = (int)(maxV / rowStep) + 2;

    for (int rv = -rows; rv <= rows; rv++) {
        float angV = rv * rowStep;
        if (fabsf(angV) > maxV) continue;
        int parity = ((rv % 2) + 2) % 2;
        float rowOffset = parity ? colStep * 0.5f : 0.0f;

        for (int rh = -cols; rh <= cols; rh++) {
            float angH = rh * colStep + rowOffset;
            if (angH < -maxH || angH > maxH) continue;

            Vector3 dir = {
                fwd.x * cosf(angV) * cosf(angH) + right.x * cosf(angV) * sinf(angH) + up.x * sinf(angV),
                fwd.y * cosf(angV) * cosf(angH) + right.y * cosf(angV) * sinf(angH) + up.y * sinf(angV),
                fwd.z * cosf(angV) * cosf(angH) + right.z * cosf(angV) * sinf(angH) + up.z * sinf(angV)
            };
            float dl = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            if (dl > 0.00001f) { dir.x /= dl; dir.y /= dl; dir.z /= dl; }
            Vector3 p = { c.x + dir.x * r, c.y + dir.y * r, c.z + dir.z * r };

            Vector3 tU = {
                right.x * cosf(angH) - fwd.x * sinf(angH),
                right.y * cosf(angH) - fwd.y * sinf(angH),
                right.z * cosf(angH) - fwd.z * sinf(angH)
            };
            float tuLen = sqrtf(tU.x * tU.x + tU.y * tU.y + tU.z * tU.z);
            if (tuLen > 0.00001f) { tU.x /= tuLen; tU.y /= tuLen; tU.z /= tuLen; }
            Vector3 tV = {
                dir.y * tU.z - dir.z * tU.y,
                dir.z * tU.x - dir.x * tU.z,
                dir.x * tU.y - dir.y * tU.x
            };

            float seed = (float)(rh * 13 + rv * 7);
            float flicker = 0.7f + 0.3f * sinf(EB_g_time * EB_g_params.fieldFlickerSpeed + seed);
            float latFade = 0.5f + 0.5f * cosf(angV);
            float cellA = fminf(1.0f, fmaxf(0.0f, flicker * latFade));

            Color fillCol = EB_g_fieldColorMid; fillCol.a = (unsigned char)(cellA * 26.0f);
            Color edgeCol = EB_g_fieldColorMid; edgeCol.a = (unsigned char)(cellA * 150.0f);

            float cellSize = hexSize * r * 0.95f;
            EB_DrawHexPanel(p, tU, tV, cellSize, fillCol, edgeCol);
        }
    }
}

static void EB_DrawOrbGlow(Vector3 pos, float size, float screenW, float screenH, Color mid, Color core) {
    Vector2 sp = GetWorldToScreen(pos, EB_g_camera);
    if (sp.x < -80 || sp.x > screenW + 80 || sp.y < -80 || sp.y > screenH + 80) return;
    float d  = EB_V3Distance(EB_g_camera.position, pos);
    float sz = EB_Clamp(size * 320.0f / (d * d + 0.1f), 2.0f, 60.0f);
    DrawCircleV(sp, sz, mid);
    DrawCircleV(sp, sz * 0.55f, core);
}

static void EB_DrawOrbTrail(int orbIndex, float orbSize, float screenW, float screenH, Color outer) {
    for (int t = 0; t < EB_TRAIL_LEN; t++) {
        EB_Trail *sample = &EB_g_trails[orbIndex][t];
        if (sample->life <= 0.0f) continue;
        Vector2 sp = GetWorldToScreen(sample->pos, EB_g_camera);
        if (sp.x < -80 || sp.x > screenW + 80 || sp.y < -80 || sp.y > screenH + 80) continue;
        float d  = EB_V3Distance(EB_g_camera.position, sample->pos);
        float fade = sample->life / sample->maxLife;
        float sz = EB_Clamp(orbSize * 220.0f / (d * d + 0.1f) * fade, 1.0f, 40.0f);
        Color c = outer;
        c.a = (unsigned char)(fade * 160);
        DrawCircleV(sp, sz, c);
    }
}

static void EB_DrawFireOrbsMesh(float screenW, float screenH) {
    rlSetBlendMode(EB_g_params.additive ? BLEND_ADDITIVE : BLEND_ALPHA);
    for (int i = 0; i < EB_g_params.orbCount; i++) EB_DrawOrbTrail(i, EB_g_params.orbSize, screenW, screenH, EB_g_fireOrbsColorOuter);

    BeginMode3D(EB_g_camera);
    for (int i = 0; i < EB_g_params.orbCount; i++) {
        int head = (EB_g_trailHead[i] - 1 + EB_TRAIL_LEN) % EB_TRAIL_LEN;
        Vector3 pos = EB_g_trails[i][head].pos;
        DrawSphere(pos, EB_g_params.orbSize, EB_g_fireOrbsColorCore);
    }
    EndMode3D();

    for (int i = 0; i < EB_g_params.orbCount; i++) {
        int head = (EB_g_trailHead[i] - 1 + EB_TRAIL_LEN) % EB_TRAIL_LEN;
        Vector3 pos = EB_g_trails[i][head].pos;
        EB_DrawOrbGlow(pos, EB_g_params.orbSize, screenW, screenH, EB_g_fireOrbsColorMid, EB_g_fireOrbsColorCore);
    }
    rlSetBlendMode(BLEND_ALPHA);
}

static void EB_DrawWindFunnel(Color outer) {
    rlSetLineWidth(1.5f);
    for (int line = 0; line < EB_g_params.windFunnelLines; line++) {
        float basePhase = ((float)line / (float)EB_g_params.windFunnelLines) * 2.0f * PI;
        rlBegin(RL_LINES);
        for (int s = 0; s < EB_FUNNEL_SAMPLES - 1; s++) {
            float t0 = (float)s / (EB_FUNNEL_SAMPLES - 1);
            float t1 = (float)(s + 1) / (EB_FUNNEL_SAMPLES - 1);
            float y0 = t0 * EB_g_params.windHelixHeight, y1 = t1 * EB_g_params.windHelixHeight;
            float taper0 = 1.0f - 0.6f * t0, taper1 = 1.0f - 0.6f * t1;
            float a0 = basePhase + t0 * EB_g_params.windHelixTurns * 2.0f * PI - EB_g_time * 2.5f;
            float a1 = basePhase + t1 * EB_g_params.windHelixTurns * 2.0f * PI - EB_g_time * 2.5f;
            float r0 = EB_g_params.orbitRadius * taper0, r1 = EB_g_params.orbitRadius * taper1;
            unsigned char alpha = (unsigned char)(60 * (1.0f - t0));
            rlColor4ub(outer.r, outer.g, outer.b, alpha);
            rlVertex3f(cosf(a0)*r0, y0, sinf(a0)*r0);
            rlVertex3f(cosf(a1)*r1, y1, sinf(a1)*r1);
        }
        rlEnd();
    }
    rlSetLineWidth(1.0f);
}

/* Dibuja una hojita como billboard orientado a cámara (fill + contorno,
   mismo esquema que EB_DrawHexPanel): gira sobre su propio eje con
   'spinAngle' y se "voltea" con 'flipAngle' (que angosta el ancho al pasar
   por el filo), simulando una hoja dando tumbos en el viento. */
static void EB_DrawGrassBlade(Vector3 pos, Vector3 dir, float size, float sway, float spinAngle, float phase, Color core, Color mid, Color outer) {
    Vector3 fwd = EB_V3Normalize(EB_V3Sub(EB_g_camera.position, pos));
    Vector3 axis = (EB_V3Length(dir) < 0.0001f) ? (Vector3){ 0.0f, 1.0f, 0.0f } : EB_V3Normalize(dir);
    Vector3 right0 = EB_V3Cross(axis, fwd);
    if (EB_V3Length(right0) < 0.0001f) right0 = (Vector3){ 1.0f, 0.0f, 0.0f };
    else right0 = EB_V3Normalize(right0);
    Vector3 up0 = EB_V3Cross(fwd, right0);

    float cs = cosf(spinAngle), sn = sinf(spinAngle);
    Vector3 right = EB_V3Add(EB_V3Scale(right0, cs), EB_V3Scale(up0, sn));
    Vector3 growth = EB_V3Add(EB_V3Scale(right0, -sn), EB_V3Scale(up0, cs));

    const int segments = 5;
    float baseWidth = size * 0.09f;
    float curve = size * 0.35f * sway;

    Vector3 prevL = pos, prevR = pos;
    for (int s = 1; s <= segments; s++) {
        float t = (float)s / segments;
        float wobble = sinf(phase + t * 3.0f + EB_g_time * 5.0f) * size * 0.05f * t;
        float bend = curve * t * t + wobble;
        float width = baseWidth * (1.0f - t);

        Vector3 center = EB_V3Add(pos, EB_V3Add(
            EB_V3Scale(growth, t * size),
            EB_V3Scale(right, bend)));
        Vector3 curL = EB_V3Add(center, EB_V3Scale(right, -width));
        Vector3 curR = EB_V3Add(center, EB_V3Scale(right,  width));

        Color colA = EB_LerpColor(mid, core, t * 0.6f);
        Color colB = EB_LerpColor(outer, mid, t * 0.6f);
        DrawTriangle3D(prevL, prevR, curR, colA);
        DrawTriangle3D(prevL, curR, curL, colB);

        prevL = curL;
        prevR = curR;
    }

    Color edge = core;
    edge.a = 180;
    DrawLine3D(pos, prevL, edge);
}

/* Dibuja la estela de un orb como una polilínea (solo líneas) conectando
   las últimas 'sampleCount' muestras del trail en orden cronológico, con
   alpha decreciente. Cuando el movimiento es un bucle que envuelve (como el
   ascenso en hélice de wind_spin/fire_wind), la muestra más nueva puede
   caer de golpe de arriba a abajo del ciclo — en ese caso se corta la
   polilínea en vez de dibujar una línea que atraviese todo el remolino. */
static void EB_DrawOrbTrailLines(int orbIndex, Color baseColor, int sampleCount) {
    if (sampleCount > EB_TRAIL_LEN) sampleCount = EB_TRAIL_LEN;
    int start = EB_TRAIL_LEN - sampleCount;
    int head = EB_g_trailHead[orbIndex];
    Vector3 prev = { 0 };
    int havePrev = 0;
    for (int k = start; k < EB_TRAIL_LEN; k++) {
        EB_Trail *sample = &EB_g_trails[orbIndex][(head + k) % EB_TRAIL_LEN];
        if (sample->life <= 0.0f) { havePrev = 0; continue; }
        if (havePrev) {
            float dy = sample->pos.y - prev.y;
            if (dy > -EB_g_params.windHelixHeight * 0.3f) {
                Color c = baseColor;
                c.a = (unsigned char)((sample->life / sample->maxLife) * 160);
                DrawLine3D(prev, sample->pos, c);
            }
        }
        prev = sample->pos;
        havePrev = 1;
    }
}

/* Color "hada" para las bolitas de fire_wind: va rotando entre colorCore,
   colorMid y colorOuter por índice de partícula y tiempo, para que el
   racimo de bolitas titile con tonos distintos como luciérnagas. */
static Color EB_FairyTint(int i, float t, Color core, Color mid, Color outer) {
    float phase = fmodf((float)i / 3.0f + t * 0.15f, 1.0f);
    if (phase < 0.3333f) return EB_LerpColor(core, mid, phase * 3.0f);
    if (phase < 0.6667f) return EB_LerpColor(mid, outer, (phase - 0.3333f) * 3.0f);
    return EB_LerpColor(outer, core, (phase - 0.6667f) * 3.0f);
}

static void EB_DrawFairyGlow(Vector3 pos, float size, Color tint, float screenW, float screenH) {
    Vector2 sp = GetWorldToScreen(pos, EB_g_camera);
    if (sp.x < -80 || sp.x > screenW + 80 || sp.y < -80 || sp.y > screenH + 80) return;
    float d  = EB_V3Distance(EB_g_camera.position, pos);
    float sz = EB_Clamp(size * 320.0f / (d * d + 0.1f), 2.0f, 60.0f);
    Color outer = tint; outer.a = 150;
    DrawCircleV(sp, sz, outer);
    DrawCircleV(sp, sz * 0.5f, EB_LerpColor(tint, WHITE, 0.6f));
}

static void EB_DrawFairyTrail(int orbIndex, float orbSize, Color tint, float screenW, float screenH) {
    for (int t = 0; t < EB_TRAIL_LEN; t++) {
        EB_Trail *sample = &EB_g_trails[orbIndex][t];
        if (sample->life <= 0.0f) continue;
        Vector2 sp = GetWorldToScreen(sample->pos, EB_g_camera);
        if (sp.x < -80 || sp.x > screenW + 80 || sp.y < -80 || sp.y > screenH + 80) continue;
        float d  = EB_V3Distance(EB_g_camera.position, sample->pos);
        float fade = sample->life / sample->maxLife;
        float sz = EB_Clamp(orbSize * 220.0f / (d * d + 0.1f) * fade, 1.0f, 40.0f);
        Color c = tint;
        c.a = (unsigned char)(fade * 160);
        DrawCircleV(sp, sz, c);
    }
}

static void EB_DrawWindSpinMesh(float screenW, float screenH) {
    (void)screenW; (void)screenH;
    BeginMode3D(EB_g_camera);
    rlSetBlendMode(EB_g_params.additive ? BLEND_ADDITIVE : BLEND_ALPHA);
    EB_DrawWindFunnel(EB_g_windSpinColorOuter);

    rlSetLineWidth(1.5f);
    for (int i = 0; i < EB_g_params.orbCount; i++) {
        EB_DrawOrbTrailLines(i, EB_g_windSpinColorOuter, EB_TRAIL_LEN / 4);

        int head = (EB_g_trailHead[i] - 1 + EB_TRAIL_LEN) % EB_TRAIL_LEN;
        int prevIdx = (head - 1 + EB_TRAIL_LEN) % EB_TRAIL_LEN;
        Vector3 pos = EB_g_trails[i][head].pos;
        Vector3 prevPos = EB_g_trails[i][prevIdx].life > 0.0f ? EB_g_trails[i][prevIdx].pos : pos;
        Vector3 velocity = EB_V3Sub(pos, prevPos);
        Vector3 worldUp = (Vector3){ 0.0f, 1.0f, 0.0f };
        Vector3 dir = EB_V3Add(EB_V3Scale(velocity, 0.7f), EB_V3Scale(worldUp, 0.3f));
        float phase = EB_g_orbPhase[i];
        float sway = sinf(EB_g_time * 2.4f + phase * 2.0f) * 0.6f;
        float spin = EB_g_time * 2.2f + phase * 3.0f;
        EB_DrawGrassBlade(pos, dir, EB_g_params.orbSize * 1.0f, sway, spin, phase,
                           EB_g_windSpinColorCore, EB_g_windSpinColorMid, EB_g_windSpinColorOuter);
    }
    rlSetLineWidth(1.0f);
    rlSetBlendMode(BLEND_ALPHA);
    EndMode3D();
}

static void EB_DrawFireWindMesh(float screenW, float screenH) {
    BeginMode3D(EB_g_camera);
    rlSetBlendMode(EB_g_params.additive ? BLEND_ADDITIVE : BLEND_ALPHA);
    EB_DrawWindFunnel(EB_g_fireWindColorOuter);
    rlSetBlendMode(BLEND_ALPHA);
    EndMode3D();

    rlSetBlendMode(EB_g_params.additive ? BLEND_ADDITIVE : BLEND_ALPHA);
    for (int i = 0; i < EB_g_params.orbCount; i++)
        EB_DrawFairyTrail(i, EB_g_params.orbSize * 0.7f,
                           EB_FairyTint(i, EB_g_time, EB_g_fireWindColorCore, EB_g_fireWindColorMid, EB_g_fireWindColorOuter),
                           screenW, screenH);

    BeginMode3D(EB_g_camera);
    for (int i = 0; i < EB_g_params.orbCount; i++) {
        int head = (EB_g_trailHead[i] - 1 + EB_TRAIL_LEN) % EB_TRAIL_LEN;
        Vector3 pos = EB_g_trails[i][head].pos;
        float twinkle = 0.75f + 0.25f * sinf(EB_g_time * 6.0f + EB_g_orbPhase[i] * 3.0f);
        DrawSphere(pos, EB_g_params.orbSize * 0.55f * twinkle,
                   EB_FairyTint(i, EB_g_time, EB_g_fireWindColorCore, EB_g_fireWindColorMid, EB_g_fireWindColorOuter));
    }
    EndMode3D();

    for (int i = 0; i < EB_g_params.orbCount; i++) {
        int head = (EB_g_trailHead[i] - 1 + EB_TRAIL_LEN) % EB_TRAIL_LEN;
        Vector3 pos = EB_g_trails[i][head].pos;
        float twinkle = 0.75f + 0.25f * sinf(EB_g_time * 6.0f + EB_g_orbPhase[i] * 3.0f);
        EB_DrawFairyGlow(pos, EB_g_params.orbSize * twinkle,
                          EB_FairyTint(i, EB_g_time, EB_g_fireWindColorCore, EB_g_fireWindColorMid, EB_g_fireWindColorOuter),
                          screenW, screenH);
    }
    rlSetBlendMode(BLEND_ALPHA);
}

void EffectAtelierEffect_Init(void) {
    EB_g_camera.position   = (Vector3){ EB_g_params.cameraDistance, EB_g_params.cameraDistance * 0.55f, EB_g_params.cameraDistance };
    EB_g_camera.target     = (Vector3){ 0.0f, 0.6f, 0.0f };
    EB_g_camera.up         = (Vector3){ 0.0f, 1.0f, 0.0f };
    EB_g_camera.fovy       = 45.0f;
    EB_g_camera.projection = CAMERA_PERSPECTIVE;
    EB_g_time = 0.0f;
    EB_g_orbitAngle = 0.0f;
    EB_g_orbAngle = 0.0f;
    EB_g_impactAccum = 0.0f;
    EB_g_shieldFacing = EB_g_params.shieldFacingDeg * DEG2RAD;
    EB_g_shieldImpactFlash = 0.0f;
    EB_g_fieldYaw = 0.0f;
    EB_ResetOrbits();
    EB_SpawnBurst();
}

static Color EB_HexToColor(const char *hex, Color fallback) {
    if (!hex || hex[0] != '#' || strlen(hex) < 7) return fallback;
    unsigned int r, g, b;
    if (sscanf(hex + 1, "%02x%02x%02x", &r, &g, &b) != 3) return fallback;
    return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
}

static int EB_ParseElement(const char *s, int fallback) {
    if (!s) return fallback;
    if (strcmp(s, "neutral")   == 0) return EB_ELEM_NEUTRAL;
    if (strcmp(s, "fire")      == 0) return EB_ELEM_FIRE;
    if (strcmp(s, "water")     == 0) return EB_ELEM_WATER;
    if (strcmp(s, "earth")     == 0) return EB_ELEM_EARTH;
    if (strcmp(s, "wind")      == 0) return EB_ELEM_WIND;
    if (strcmp(s, "lightning") == 0) return EB_ELEM_LIGHTNING;
    if (strcmp(s, "dark")      == 0) return EB_ELEM_DARK;
    if (strcmp(s, "poison")    == 0) return EB_ELEM_POISON;
    if (strcmp(s, "light")     == 0) return EB_ELEM_LIGHT;
    if (strcmp(s, "ice")       == 0) return EB_ELEM_ICE;
    return fallback;
}

static int EB_ParseShape(const char *s, int fallback) {
    if (!s) return fallback;
    if (strcmp(s, "sphere") == 0) return EB_SHAPE_SPHERE;
    if (strcmp(s, "ring")   == 0) return EB_SHAPE_RING;
    if (strcmp(s, "spiral") == 0) return EB_SHAPE_SPIRAL;
    if (strcmp(s, "beam")   == 0) return EB_SHAPE_BEAM;
    if (strcmp(s, "pillar") == 0) return EB_SHAPE_PILLAR;
    if (strcmp(s, "rain")   == 0) return EB_SHAPE_RAIN;
    if (strcmp(s, "wave")       == 0) return EB_SHAPE_WAVE;
    if (strcmp(s, "projectile") == 0) return EB_SHAPE_PROJECTILE;
    if (strcmp(s, "jump")       == 0) return EB_SHAPE_JUMP;
    if (strcmp(s, "shield")     == 0) return EB_SHAPE_SHIELD;
    if (strcmp(s, "field")      == 0) return EB_SHAPE_FIELD;
    if (strcmp(s, "fire_orbs")  == 0) return EB_SHAPE_FIRE_ORBS;
    if (strcmp(s, "wind_spin")  == 0) return EB_SHAPE_WIND_SPIN;
    if (strcmp(s, "fire_wind")  == 0) return EB_SHAPE_FIRE_WIND;
    return fallback;
}

#ifdef __EMSCRIPTEN__
void EffectAtelierEffect_SetParams(const JsonValue *paramsObj) {
    if (!paramsObj) return;
    EB_g_params.element = EB_ParseElement(JsonAsString(JsonObjectGet(paramsObj, "element"), NULL), EB_g_params.element);
    EB_g_params.shape   = EB_ParseShape(JsonAsString(JsonObjectGet(paramsObj, "mode"), NULL), EB_g_params.shape);

    const char *name = JsonAsString(JsonObjectGet(paramsObj, "presetName"), EB_g_params.presetName);
    strncpy(EB_g_params.presetName, name, EB_NAME_MAX - 1);
    EB_g_params.presetName[EB_NAME_MAX - 1] = '\0';

    EB_g_params.particleCount  = (int)JsonAsNumber(JsonObjectGet(paramsObj, "particleCount"), EB_g_params.particleCount);
    EB_g_params.spawnRadiusMin = (float)JsonAsNumber(JsonObjectGet(paramsObj, "spawnRadiusMin"), EB_g_params.spawnRadiusMin);
    EB_g_params.spawnRadiusMax = (float)JsonAsNumber(JsonObjectGet(paramsObj, "spawnRadiusMax"), EB_g_params.spawnRadiusMax);
    EB_g_params.radius         = (float)JsonAsNumber(JsonObjectGet(paramsObj, "radius"), EB_g_params.radius);
    EB_g_params.directionYaw   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "directionYaw"), EB_g_params.directionYaw);
    EB_g_params.speedMin       = (float)JsonAsNumber(JsonObjectGet(paramsObj, "speedMin"), EB_g_params.speedMin);
    EB_g_params.speedMax       = (float)JsonAsNumber(JsonObjectGet(paramsObj, "speedMax"), EB_g_params.speedMax);
    EB_g_params.lifeMin        = (float)JsonAsNumber(JsonObjectGet(paramsObj, "lifeMin"), EB_g_params.lifeMin);
    EB_g_params.lifeMax        = (float)JsonAsNumber(JsonObjectGet(paramsObj, "lifeMax"), EB_g_params.lifeMax);
    EB_g_params.loopInterval   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "loopInterval"), EB_g_params.loopInterval);

    EB_g_params.gravity = (float)JsonAsNumber(JsonObjectGet(paramsObj, "gravity"), EB_g_params.gravity);
    EB_g_params.drag    = (float)JsonAsNumber(JsonObjectGet(paramsObj, "drag"), EB_g_params.drag);

    EB_g_params.additive         = JsonAsBool(JsonObjectGet(paramsObj, "additive"), EB_g_params.additive != 0) ? 1 : 0;
    EB_g_params.cameraDistance   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "cameraDistance"), EB_g_params.cameraDistance);
    EB_g_params.cameraOrbitSpeed = (float)JsonAsNumber(JsonObjectGet(paramsObj, "cameraOrbitSpeed"), EB_g_params.cameraOrbitSpeed);
    EB_g_params.showGrid         = JsonAsBool(JsonObjectGet(paramsObj, "showGrid"), EB_g_params.showGrid != 0) ? 1 : 0;

    EB_g_params.colorCore  = EB_HexToColor(JsonAsString(JsonObjectGet(paramsObj, "colorCore"), NULL), EB_g_params.colorCore);
    EB_g_params.colorMid   = EB_HexToColor(JsonAsString(JsonObjectGet(paramsObj, "colorMid"), NULL), EB_g_params.colorMid);
    EB_g_params.colorOuter = EB_HexToColor(JsonAsString(JsonObjectGet(paramsObj, "colorOuter"), NULL), EB_g_params.colorOuter);

    EB_g_params.shieldFacingDeg      = (float)JsonAsNumber(JsonObjectGet(paramsObj, "shieldFacingDeg"), EB_g_params.shieldFacingDeg);
    EB_g_params.shieldAutoRotate     = JsonAsBool(JsonObjectGet(paramsObj, "shieldAutoRotate"), EB_g_params.shieldAutoRotate != 0) ? 1 : 0;
    EB_g_params.shieldRotateSpeedDeg = (float)JsonAsNumber(JsonObjectGet(paramsObj, "shieldRotateSpeedDeg"), EB_g_params.shieldRotateSpeedDeg);
    EB_g_params.shieldArchWidthDeg   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "shieldArchWidthDeg"), EB_g_params.shieldArchWidthDeg);
    EB_g_params.shieldArchHeightDeg  = (float)JsonAsNumber(JsonObjectGet(paramsObj, "shieldArchHeightDeg"), EB_g_params.shieldArchHeightDeg);
    EB_g_params.shieldHexSize        = (float)JsonAsNumber(JsonObjectGet(paramsObj, "shieldHexSize"), EB_g_params.shieldHexSize);
    EB_g_params.shieldFlickerSpeed   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "shieldFlickerSpeed"), EB_g_params.shieldFlickerSpeed);
    EB_g_params.shieldImpactInterval = (float)JsonAsNumber(JsonObjectGet(paramsObj, "shieldImpactInterval"), EB_g_params.shieldImpactInterval);
    EB_g_shieldFacing = EB_g_params.shieldFacingDeg * DEG2RAD;

    EB_g_params.fieldHexSize          = (float)JsonAsNumber(JsonObjectGet(paramsObj, "fieldHexSize"), EB_g_params.fieldHexSize);
    EB_g_params.fieldFlickerSpeed     = (float)JsonAsNumber(JsonObjectGet(paramsObj, "fieldFlickerSpeed"), EB_g_params.fieldFlickerSpeed);
    EB_g_params.fieldPulseSpeed       = (float)JsonAsNumber(JsonObjectGet(paramsObj, "fieldPulseSpeed"), EB_g_params.fieldPulseSpeed);
    EB_g_params.fieldPulseAmount      = (float)JsonAsNumber(JsonObjectGet(paramsObj, "fieldPulseAmount"), EB_g_params.fieldPulseAmount);
    EB_g_params.fieldRotationSpeedDeg = (float)JsonAsNumber(JsonObjectGet(paramsObj, "fieldRotationSpeedDeg"), EB_g_params.fieldRotationSpeedDeg);

    EB_g_params.orbCount      = (int)JsonAsNumber(JsonObjectGet(paramsObj, "orbCount"), EB_g_params.orbCount);
    EB_g_params.orbSize       = (float)JsonAsNumber(JsonObjectGet(paramsObj, "orbSize"), EB_g_params.orbSize);
    EB_g_params.orbitRadius   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "orbitRadius"), EB_g_params.orbitRadius);
    EB_g_params.orbitSpeedDeg = (float)JsonAsNumber(JsonObjectGet(paramsObj, "orbitSpeedDeg"), EB_g_params.orbitSpeedDeg);
    EB_g_params.orbBobAmount  = (float)JsonAsNumber(JsonObjectGet(paramsObj, "orbBobAmount"), EB_g_params.orbBobAmount);
    EB_g_params.orbBobSpeed   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "orbBobSpeed"), EB_g_params.orbBobSpeed);
    EB_g_params.trailFade     = (float)JsonAsNumber(JsonObjectGet(paramsObj, "trailFade"), EB_g_params.trailFade);

    EB_g_params.windHelixHeight = (float)JsonAsNumber(JsonObjectGet(paramsObj, "windHelixHeight"), EB_g_params.windHelixHeight);
    EB_g_params.windHelixTurns  = (float)JsonAsNumber(JsonObjectGet(paramsObj, "windHelixTurns"), EB_g_params.windHelixTurns);
    EB_g_params.windRibbonWidth = (float)JsonAsNumber(JsonObjectGet(paramsObj, "windRibbonWidth"), EB_g_params.windRibbonWidth);
    EB_g_params.windFunnelLines = (int)JsonAsNumber(JsonObjectGet(paramsObj, "windFunnelLines"), EB_g_params.windFunnelLines);

    if (EB_g_params.orbCount > EB_MAX_ORBS) EB_g_params.orbCount = EB_MAX_ORBS;
    if (EB_g_params.windFunnelLines > EB_FUNNEL_LINES) EB_g_params.windFunnelLines = EB_FUNNEL_LINES;
    EB_ResetOrbits();

    for (int L = 0; L < 3; L++) {
        EB_ExtraLayer *layer = &EB_g_params.extraLayers[L];
        int n = L + 2;
        char key[32];

        snprintf(key, sizeof(key), "layer%dEnabled", n);
        layer->enabled = JsonAsBool(JsonObjectGet(paramsObj, key), layer->enabled != 0) ? 1 : 0;

        snprintf(key, sizeof(key), "layer%dElement", n);
        layer->element = EB_ParseElement(JsonAsString(JsonObjectGet(paramsObj, key), NULL), layer->element);

        snprintf(key, sizeof(key), "layer%dShape", n);
        layer->shape = EB_ParseShape(JsonAsString(JsonObjectGet(paramsObj, key), NULL), layer->shape);

        snprintf(key, sizeof(key), "layer%dParticleCount", n);
        layer->particleCount = (int)JsonAsNumber(JsonObjectGet(paramsObj, key), layer->particleCount);

        snprintf(key, sizeof(key), "layer%dColorCore", n);
        layer->colorCore = EB_HexToColor(JsonAsString(JsonObjectGet(paramsObj, key), NULL), layer->colorCore);

        snprintf(key, sizeof(key), "layer%dColorMid", n);
        layer->colorMid = EB_HexToColor(JsonAsString(JsonObjectGet(paramsObj, key), NULL), layer->colorMid);

        snprintf(key, sizeof(key), "layer%dColorOuter", n);
        layer->colorOuter = EB_HexToColor(JsonAsString(JsonObjectGet(paramsObj, key), NULL), layer->colorOuter);
    }

    EB_SpawnBurst();
}
#endif

void EffectAtelierEffect_Update(float dt) {
    EB_g_time += dt;
    EB_g_orbitAngle += EB_g_params.cameraOrbitSpeed * DEG2RAD * dt;
    float dist = EB_g_params.cameraDistance;
    EB_g_camera.position = (Vector3){
        cosf(EB_g_orbitAngle) * dist,
        dist * 0.55f,
        sinf(EB_g_orbitAngle) * dist
    };

    EB_g_loopAccum += dt;
    if (EB_g_loopAccum >= EB_g_params.loopInterval) {
        EB_g_loopAccum = 0.0f;
        EB_SpawnBurst();
    }

    EB_UpdateActiveMeshShapes();

    if (EB_g_shieldActive) EB_UpdateShield(dt);
    if (EB_g_fieldActive)  EB_UpdateField(dt);
    if (EB_g_fireWindActive)      EB_UpdateFireWind(dt);
    else if (EB_g_windSpinActive) EB_UpdateWindSpin(dt);
    else if (EB_g_fireOrbsActive) EB_UpdateFireOrbs(dt);

    for (int i = 0; i < EB_MAX_PARTICLES; i++) {
        EB_Particle *p = &EB_g_particles[i];
        if (!p->active) continue;
        p->age += dt;
        if (p->age >= p->life) { p->active = false; continue; }
        p->vel.y -= EB_g_params.gravity * dt;
        p->vel    = EB_V3Scale(p->vel, 1.0f - EB_Clamp(EB_g_params.drag * dt, 0.0f, 0.9f));
        p->pos    = EB_V3Add(p->pos, EB_V3Scale(p->vel, dt));
    }
}

void EffectAtelierEffect_Draw(RenderTexture2D scene, int screenW, int screenH) {
    (void)scene;
    ClearBackground(BLANK);

    BeginMode3D(EB_g_camera);
    if (EB_g_params.showGrid) {
        rlPushMatrix();
        DrawGrid(10, 0.5f);
        rlPopMatrix();
    }
    EndMode3D();

    float sw = (float)screenW, sh = (float)screenH;
    Vector3 camFwd = EB_V3Normalize(EB_V3Sub(EB_g_camera.target, EB_g_camera.position));

    if (EB_g_shieldActive || EB_g_fieldActive) {
        BeginMode3D(EB_g_camera);
        rlDisableDepthMask();
        rlSetBlendMode(BLEND_ALPHA);
        if (EB_g_shieldActive) EB_DrawShieldMesh();
        if (EB_g_fieldActive)  EB_DrawFieldMesh();
        rlSetBlendMode(BLEND_ALPHA);
        rlEnableDepthMask();
        EndMode3D();
    }
    if (EB_g_fireWindActive) {
        EB_DrawFireWindMesh(sw, sh);
    } else if (EB_g_windSpinActive) {
        EB_DrawWindSpinMesh(sw, sh);
    } else if (EB_g_fireOrbsActive) {
        EB_DrawFireOrbsMesh(sw, sh);
    }

    rlSetBlendMode(EB_g_params.additive ? BLEND_ADDITIVE : BLEND_ALPHA);

    for (int i = 0; i < EB_MAX_PARTICLES; i++) {
        const EB_Particle *p = &EB_g_particles[i];
        if (!p->active) continue;
        if (EB_V3Dot(camFwd, EB_V3Sub(p->pos, EB_g_camera.position)) < 0.05f) continue;

        Vector2 sp = GetWorldToScreen(p->pos, EB_g_camera);
        if (sp.x < -80 || sp.x > sw + 80 || sp.y < -80 || sp.y > sh + 80) continue;

        float d  = EB_V3Distance(EB_g_camera.position, p->pos);
        float sz = EB_Clamp(0.12f * 300.0f / (d * d + 0.1f), 1.5f, 32.0f);
        float fa = EB_FadeAlpha(p->age, p->life, 0.04f, p->life * 0.55f);
        unsigned char alpha = (unsigned char)(fa * 220);

        Color core  = p->colorCore;  core.a  = alpha;
        Color mid   = p->colorMid;   mid.a   = (unsigned char)(alpha * 0.55f);
        Color outer = p->colorOuter; outer.a = (unsigned char)(alpha * 0.80f);

        DrawCircleV(sp, sz * 0.85f, mid);
        DrawCircleV(sp, sz * 0.50f, core);
        DrawCircleV((Vector2){ sp.x - sz * 0.16f, sp.y - sz * 0.16f }, fmaxf(1.5f, sz * 0.22f), outer);
    }

    rlSetBlendMode(BLEND_ALPHA);

    DrawText(TextFormat("%s", EB_g_params.presetName), 16, screenH - 32, 18, RAYWHITE);}

void EffectAtelierEffect_Unload(void) { }

#endif /* EFFECT_ATELIER_H */
