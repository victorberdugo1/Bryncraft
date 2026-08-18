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
void EffectAtelierEffect_Trigger(void);

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
#define EB_SHAPE_WATER_RING 14
#define EB_SHAPE_EARTH_BURST     15
#define EB_SHAPE_FIRE_BURST      16
#define EB_SHAPE_LIGHTNING_BURST 17
#define EB_SHAPE_POISON_BURST    18
#define EB_SHAPE_HEAL_AURA       19
#define EB_SHAPE_DARK_SLASH      20
#define EB_SHAPE_BARRIER         21

#define EB_SHAPE_FIREBALL       22
#define EB_SHAPE_WIND_SLASH     23
#define EB_SHAPE_ROCK_THROW     24
#define EB_SHAPE_LIGHTNING_BOLT 25
#define EB_SHAPE_WATER_JET      26
#define EB_SHAPE_ICE_SHARD      27
#define EB_SHAPE_POISON_ORB     28
#define EB_SHAPE_DARK_ORB       29
#define EB_SHAPE_LIGHT_ARROW    30
#define EB_SHAPE_BUBBLE_BURST   31

#define EB_MAX_ORBS      8
#define EB_TRAIL_LEN     24
#define EB_FUNNEL_LINES  10
#define EB_FUNNEL_SAMPLES 40
#define EB_WATER_RING_MAX 5
#define EB_EARTH_ROCK_MAX     24
#define EB_EARTH_CRACK_MAX    12
#define EB_FIRE_EMBER_MAX     30
#define EB_LIGHTNING_BOLT_MAX 8
#define EB_LIGHTNING_SEGS     10
#define EB_POISON_RING_MAX    3
#define EB_POISON_BUBBLE_MAX  30
#define EB_HEAL_CROSS_MAX     16
#define EB_DARK_TENTACLE_MAX  8
#define EB_DARK_ORB_MAX       16
#define EB_BARRIER_MOTE_MAX   20

#define EB_PROJ_TRAIL_SEGS       14
#define EB_FIREBALL_EMBER_MAX   16
#define EB_WIND_SLASH_TRAIL_MAX  5
#define EB_ROCK_DUST_MAX        16
#define EB_LIGHTNING_PROJ_SEGS  10
#define EB_LIGHTNING_PROJ_BRANCH_MAX 4
#define EB_WATER_JET_DROP_MAX   20
#define EB_ICE_SHARD_SPARK_MAX  12
#define EB_POISON_ORB_SPORE_MAX 16
#define EB_DARK_ORB_TENTACLE_MAX 4
#define EB_BUBBLE_BURST_TRAIL_MAX 16

typedef struct {
    Vector3 pos, vel;
    float   age, life, size;
    Color   colorCore, colorMid, colorOuter;
    bool    active;
    bool    isRain;
} EB_Particle;

#define EB_RAIN_SPLASH_MAX 24
#define EB_RAIN_GROUND_Y 0.05f

typedef struct {
    Vector3 pos;
    float   age, life, radius, maxRadius;
    Color   color;
    bool    active;
} EB_RainSplash;

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

    int   waterRingCount;
    float waterRingDuration, waterRingStagger;
    float waterRingCrestHeight, waterRingThickness;

    int   earthBurstRockCount, earthBurstCrackCount;
    float earthBurstDuration;

    int   fireBurstEmberCount;
    float fireBurstDuration, fireBurstColumnHeight;

    int   lightningBoltCount;
    float lightningBoltLife, lightningJitter, lightningHeight;

    int   poisonRingCount, poisonBubbleCount;
    float poisonDuration;

    int   healCrossCount;
    float healCycleDuration, healPillarHeight;

    int   darkTentacleCount, darkOrbCount;
    float darkDuration;

    int   barrierMoteCount;
    float barrierPulseSpeed;

    float fireballDuration;      int fireballEmberCount;
    float windSlashDuration;     int windSlashTrailCount;
    float rockThrowDuration;     int rockThrowDustCount;
    float lightningBoltDuration; int lightningBoltBranches;
    float waterJetDuration;      int waterJetDropCount;
    float iceShardDuration;      int iceShardSparkCount;
    float poisonOrbDuration;     int poisonOrbSporeCount;
    float darkOrbDuration;       int darkOrbTentacleCount;
    float lightArrowDuration;
    float bubbleBurstDuration;   int bubbleBurstTrailCount;

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

    .waterRingCount = 4,
    .waterRingDuration = 0.9f, .waterRingStagger = 0.09f,
    .waterRingCrestHeight = 0.16f, .waterRingThickness = 0.16f,

    .earthBurstRockCount = 14, .earthBurstCrackCount = 10,
    .earthBurstDuration = 0.9f,

    .fireBurstEmberCount = 18,
    .fireBurstDuration = 0.9f, .fireBurstColumnHeight = 2.2f,

    .lightningBoltCount = 5,
    .lightningBoltLife = 0.4f, .lightningJitter = 0.35f, .lightningHeight = 2.5f,

    .poisonRingCount = 3, .poisonBubbleCount = 20,
    .poisonDuration = 1.1f,

    .healCrossCount = 8,
    .healCycleDuration = 1.4f, .healPillarHeight = 2.0f,

    .darkTentacleCount = 8, .darkOrbCount = 10,
    .darkDuration = 0.7f,

    .barrierMoteCount = 14,
    .barrierPulseSpeed = 1.2f,

    .fireballDuration = 0.9f, .fireballEmberCount = 12,
    .windSlashDuration = 0.35f, .windSlashTrailCount = 3,
    .rockThrowDuration = 0.8f, .rockThrowDustCount = 10,
    .lightningBoltDuration = 0.25f, .lightningBoltBranches = 3,
    .waterJetDuration = 0.7f, .waterJetDropCount = 14,
    .iceShardDuration = 0.8f, .iceShardSparkCount = 10,
    .poisonOrbDuration = 1.0f, .poisonOrbSporeCount = 12,
    .darkOrbDuration = 0.8f, .darkOrbTentacleCount = 3,
    .lightArrowDuration = 0.35f,
    .bubbleBurstDuration = 0.9f, .bubbleBurstTrailCount = 10,

    .cameraDistance = 5.5f,
    .cameraOrbitSpeed = 18.0f,
    .showGrid = 1,
};

static EB_Particle    EB_g_particles[EB_MAX_PARTICLES];
static EB_RainSplash  EB_g_rainSplashes[EB_RAIN_SPLASH_MAX];
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
static int EB_g_waterRingActive = 0;
static int EB_g_earthBurstActive = 0;
static int EB_g_fireBurstActive = 0;
static int EB_g_lightningActive = 0;
static int EB_g_poisonActive = 0;
static int EB_g_healActive = 0;
static int EB_g_darkSlashActive = 0;
static int EB_g_barrierActive = 0;
static int EB_g_fireballActive = 0;
static int EB_g_windSlashActive = 0;
static int EB_g_rockThrowActive = 0;
static int EB_g_lightningBoltActive = 0;
static int EB_g_waterJetActive = 0;
static int EB_g_iceShardActive = 0;
static int EB_g_poisonOrbActive = 0;
static int EB_g_darkOrbActive = 0;
static int EB_g_lightArrowActive = 0;

static Color EB_g_shieldColorCore, EB_g_shieldColorMid, EB_g_shieldColorOuter;
static Color EB_g_fieldColorCore, EB_g_fieldColorMid, EB_g_fieldColorOuter;
static Color EB_g_fireOrbsColorCore, EB_g_fireOrbsColorMid, EB_g_fireOrbsColorOuter;
static Color EB_g_windSpinColorCore, EB_g_windSpinColorMid, EB_g_windSpinColorOuter;
static Color EB_g_fireWindColorCore, EB_g_fireWindColorMid, EB_g_fireWindColorOuter;
static Color EB_g_waterRingColorCore, EB_g_waterRingColorMid, EB_g_waterRingColorOuter;
static Color EB_g_earthBurstColorCore, EB_g_earthBurstColorMid, EB_g_earthBurstColorOuter;
static Color EB_g_fireBurstColorCore, EB_g_fireBurstColorMid, EB_g_fireBurstColorOuter;
static Color EB_g_lightningColorCore, EB_g_lightningColorMid, EB_g_lightningColorOuter;
static Color EB_g_poisonColorCore, EB_g_poisonColorMid, EB_g_poisonColorOuter;
static Color EB_g_healColorCore, EB_g_healColorMid, EB_g_healColorOuter;
static Color EB_g_darkSlashColorCore, EB_g_darkSlashColorMid, EB_g_darkSlashColorOuter;
static Color EB_g_barrierColorCore, EB_g_barrierColorMid, EB_g_barrierColorOuter;
static Color EB_g_fireballColorCore, EB_g_fireballColorMid, EB_g_fireballColorOuter;
static Color EB_g_windSlashColorCore, EB_g_windSlashColorMid, EB_g_windSlashColorOuter;
static Color EB_g_rockThrowColorCore, EB_g_rockThrowColorMid, EB_g_rockThrowColorOuter;
static Color EB_g_lightningBoltColorCore, EB_g_lightningBoltColorMid, EB_g_lightningBoltColorOuter;
static Color EB_g_waterJetColorCore, EB_g_waterJetColorMid, EB_g_waterJetColorOuter;
static Color EB_g_iceShardColorCore, EB_g_iceShardColorMid, EB_g_iceShardColorOuter;
static Color EB_g_poisonOrbColorCore, EB_g_poisonOrbColorMid, EB_g_poisonOrbColorOuter;
static Color EB_g_darkOrbColorCore, EB_g_darkOrbColorMid, EB_g_darkOrbColorOuter;
static Color EB_g_lightArrowColorCore, EB_g_lightArrowColorMid, EB_g_lightArrowColorOuter;
static int   EB_g_bubbleBurstActive = 0;
static Color EB_g_bubbleBurstColorCore, EB_g_bubbleBurstColorMid, EB_g_bubbleBurstColorOuter;

typedef struct { Vector3 pos; float life, maxLife; } EB_Trail;
static EB_Trail EB_g_trails[EB_MAX_ORBS][EB_TRAIL_LEN];
static int       EB_g_trailHead[EB_MAX_ORBS];
static float      EB_g_orbPhase[EB_MAX_ORBS];

static inline float EB_RandF(float lo, float hi) {
    return lo + (hi - lo) * ((float)rand() / (float)RAND_MAX);
}

/* Hash determinista 0..1 a partir de un entero — usado para dar variación
   estable por-índice a las partículas procedurales de los bursts (rocas,
   brasas, rayos, orbes...) sin necesitar arrays de estado persistente:
   cada partícula se recalcula analíticamente cada frame a partir de su
   índice + el tiempo transcurrido, así que no hay nada que actualizar,
   reservar ni desbordar. */
static inline float EB_Hash01(int seed) {
    unsigned int h = (unsigned int)seed;
    h = (h ^ 61u) ^ (h >> 16);
    h = h + (h << 3);
    h = h ^ (h >> 4);
    h = h * 0x27d4eb2du;
    h = h ^ (h >> 15);
    return (float)(h & 0x00FFFFFFu) / (float)0x01000000u;
}

static inline float EB_HashRange(int seed, float lo, float hi) {
    return lo + (hi - lo) * EB_Hash01(seed);
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
static inline Vector3 EB_V3Lerp(Vector3 a, Vector3 b, float t) {
    return (Vector3){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}

/* Origen/objetivo compartidos por todos los shapes de proyectil: parten del
   centro a altura de pecho y viajan `radius` unidades en la dirección
   `directionYaw` (mismos parámetros que ya usa el shape "projectile"). */
static inline void EB_ProjectileEndpoints(Vector3 *origin, Vector3 *target) {
    float yaw = EB_g_params.directionYaw * DEG2RAD;
    Vector3 dir = { sinf(yaw), 0.0f, cosf(yaw) };
    *origin = (Vector3){ 0.0f, 0.6f, 0.0f };
    *target = EB_V3Add(*origin, EB_V3Scale(dir, fmaxf(EB_g_params.radius, 0.1f)));
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
        case EB_SHAPE_WATER_RING:
        case EB_SHAPE_EARTH_BURST:
        case EB_SHAPE_FIRE_BURST:
        case EB_SHAPE_LIGHTNING_BURST:
        case EB_SHAPE_POISON_BURST:
        case EB_SHAPE_HEAL_AURA:
        case EB_SHAPE_DARK_SLASH:
        case EB_SHAPE_BARRIER:
        case EB_SHAPE_FIREBALL:
        case EB_SHAPE_WIND_SLASH:
        case EB_SHAPE_ROCK_THROW:
        case EB_SHAPE_LIGHTNING_BOLT:
        case EB_SHAPE_WATER_JET:
        case EB_SHAPE_ICE_SHARD:
        case EB_SHAPE_POISON_ORB:
        case EB_SHAPE_DARK_ORB:
        case EB_SHAPE_LIGHT_ARROW:
        case EB_SHAPE_BUBBLE_BURST:
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
        particles[i].isRain     = (shape == EB_SHAPE_RAIN);
    }
}

static void EB_SpawnRainSplash(Vector3 pos, Color color) {
    for (int i = 0; i < EB_RAIN_SPLASH_MAX; i++) {
        if (EB_g_rainSplashes[i].active) continue;
        EB_g_rainSplashes[i].pos       = pos;
        EB_g_rainSplashes[i].age       = 0.0f;
        EB_g_rainSplashes[i].life      = 0.35f;
        EB_g_rainSplashes[i].radius    = 0.02f;
        EB_g_rainSplashes[i].maxRadius = 0.28f;
        EB_g_rainSplashes[i].color     = color;
        EB_g_rainSplashes[i].active    = true;
        return;
    }
}

static inline int EB_IsMeshShape(int shape) {
    return shape == EB_SHAPE_SHIELD || shape == EB_SHAPE_FIELD || shape == EB_SHAPE_FIRE_ORBS ||
           shape == EB_SHAPE_WIND_SPIN || shape == EB_SHAPE_FIRE_WIND || shape == EB_SHAPE_WATER_RING ||
           shape == EB_SHAPE_EARTH_BURST || shape == EB_SHAPE_FIRE_BURST || shape == EB_SHAPE_LIGHTNING_BURST ||
           shape == EB_SHAPE_POISON_BURST || shape == EB_SHAPE_HEAL_AURA || shape == EB_SHAPE_DARK_SLASH ||
           shape == EB_SHAPE_BARRIER ||
           shape == EB_SHAPE_FIREBALL || shape == EB_SHAPE_WIND_SLASH || shape == EB_SHAPE_ROCK_THROW ||
           shape == EB_SHAPE_LIGHTNING_BOLT || shape == EB_SHAPE_WATER_JET || shape == EB_SHAPE_ICE_SHARD ||
           shape == EB_SHAPE_POISON_ORB || shape == EB_SHAPE_DARK_ORB || shape == EB_SHAPE_LIGHT_ARROW ||
           shape == EB_SHAPE_BUBBLE_BURST;
}

static void EB_UpdateActiveMeshShapes(void) {
    EB_g_shieldActive = EB_g_fieldActive = EB_g_fireOrbsActive = EB_g_windSpinActive = EB_g_fireWindActive = EB_g_waterRingActive = 0;
    EB_g_earthBurstActive = EB_g_fireBurstActive = EB_g_lightningActive = EB_g_poisonActive = 0;
    EB_g_healActive = EB_g_darkSlashActive = EB_g_barrierActive = 0;
    EB_g_fireballActive = EB_g_windSlashActive = EB_g_rockThrowActive = EB_g_lightningBoltActive = 0;
    EB_g_waterJetActive = EB_g_iceShardActive = EB_g_poisonOrbActive = EB_g_darkOrbActive = EB_g_lightArrowActive = 0;
    EB_g_bubbleBurstActive = 0;

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
            case EB_SHAPE_WATER_RING:
                if (!EB_g_waterRingActive) {
                    EB_g_waterRingColorCore = cores[i]; EB_g_waterRingColorMid = mids[i]; EB_g_waterRingColorOuter = outers[i];
                }
                EB_g_waterRingActive = 1;
                break;
            case EB_SHAPE_EARTH_BURST:
                if (!EB_g_earthBurstActive) {
                    EB_g_earthBurstColorCore = cores[i]; EB_g_earthBurstColorMid = mids[i]; EB_g_earthBurstColorOuter = outers[i];
                }
                EB_g_earthBurstActive = 1;
                break;
            case EB_SHAPE_FIRE_BURST:
                if (!EB_g_fireBurstActive) {
                    EB_g_fireBurstColorCore = cores[i]; EB_g_fireBurstColorMid = mids[i]; EB_g_fireBurstColorOuter = outers[i];
                }
                EB_g_fireBurstActive = 1;
                break;
            case EB_SHAPE_LIGHTNING_BURST:
                if (!EB_g_lightningActive) {
                    EB_g_lightningColorCore = cores[i]; EB_g_lightningColorMid = mids[i]; EB_g_lightningColorOuter = outers[i];
                }
                EB_g_lightningActive = 1;
                break;
            case EB_SHAPE_POISON_BURST:
                if (!EB_g_poisonActive) {
                    EB_g_poisonColorCore = cores[i]; EB_g_poisonColorMid = mids[i]; EB_g_poisonColorOuter = outers[i];
                }
                EB_g_poisonActive = 1;
                break;
            case EB_SHAPE_HEAL_AURA:
                if (!EB_g_healActive) {
                    EB_g_healColorCore = cores[i]; EB_g_healColorMid = mids[i]; EB_g_healColorOuter = outers[i];
                }
                EB_g_healActive = 1;
                break;
            case EB_SHAPE_DARK_SLASH:
                if (!EB_g_darkSlashActive) {
                    EB_g_darkSlashColorCore = cores[i]; EB_g_darkSlashColorMid = mids[i]; EB_g_darkSlashColorOuter = outers[i];
                }
                EB_g_darkSlashActive = 1;
                break;
            case EB_SHAPE_BARRIER:
                if (!EB_g_barrierActive) {
                    EB_g_barrierColorCore = cores[i]; EB_g_barrierColorMid = mids[i]; EB_g_barrierColorOuter = outers[i];
                }
                EB_g_barrierActive = 1;
                break;
            case EB_SHAPE_FIREBALL:
                if (!EB_g_fireballActive) {
                    EB_g_fireballColorCore = cores[i]; EB_g_fireballColorMid = mids[i]; EB_g_fireballColorOuter = outers[i];
                }
                EB_g_fireballActive = 1;
                break;
            case EB_SHAPE_WIND_SLASH:
                if (!EB_g_windSlashActive) {
                    EB_g_windSlashColorCore = cores[i]; EB_g_windSlashColorMid = mids[i]; EB_g_windSlashColorOuter = outers[i];
                }
                EB_g_windSlashActive = 1;
                break;
            case EB_SHAPE_ROCK_THROW:
                if (!EB_g_rockThrowActive) {
                    EB_g_rockThrowColorCore = cores[i]; EB_g_rockThrowColorMid = mids[i]; EB_g_rockThrowColorOuter = outers[i];
                }
                EB_g_rockThrowActive = 1;
                break;
            case EB_SHAPE_LIGHTNING_BOLT:
                if (!EB_g_lightningBoltActive) {
                    EB_g_lightningBoltColorCore = cores[i]; EB_g_lightningBoltColorMid = mids[i]; EB_g_lightningBoltColorOuter = outers[i];
                }
                EB_g_lightningBoltActive = 1;
                break;
            case EB_SHAPE_WATER_JET:
                if (!EB_g_waterJetActive) {
                    EB_g_waterJetColorCore = cores[i]; EB_g_waterJetColorMid = mids[i]; EB_g_waterJetColorOuter = outers[i];
                }
                EB_g_waterJetActive = 1;
                break;
            case EB_SHAPE_ICE_SHARD:
                if (!EB_g_iceShardActive) {
                    EB_g_iceShardColorCore = cores[i]; EB_g_iceShardColorMid = mids[i]; EB_g_iceShardColorOuter = outers[i];
                }
                EB_g_iceShardActive = 1;
                break;
            case EB_SHAPE_POISON_ORB:
                if (!EB_g_poisonOrbActive) {
                    EB_g_poisonOrbColorCore = cores[i]; EB_g_poisonOrbColorMid = mids[i]; EB_g_poisonOrbColorOuter = outers[i];
                }
                EB_g_poisonOrbActive = 1;
                break;
            case EB_SHAPE_DARK_ORB:
                if (!EB_g_darkOrbActive) {
                    EB_g_darkOrbColorCore = cores[i]; EB_g_darkOrbColorMid = mids[i]; EB_g_darkOrbColorOuter = outers[i];
                }
                EB_g_darkOrbActive = 1;
                break;
            case EB_SHAPE_LIGHT_ARROW:
                if (!EB_g_lightArrowActive) {
                    EB_g_lightArrowColorCore = cores[i]; EB_g_lightArrowColorMid = mids[i]; EB_g_lightArrowColorOuter = outers[i];
                }
                EB_g_lightArrowActive = 1;
                break;
            case EB_SHAPE_BUBBLE_BURST:
                if (!EB_g_bubbleBurstActive) {
                    EB_g_bubbleBurstColorCore = cores[i]; EB_g_bubbleBurstColorMid = mids[i]; EB_g_bubbleBurstColorOuter = outers[i];
                }
                EB_g_bubbleBurstActive = 1;
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

/* Olas de agua concéntricas que se alejan del jugador (stomp) — porteado
   de la pared volumétrica de CVT_WATER_BURST en effects.h: cada anillo es
   un cinturón de quads con cresta ondulada + espuma aditiva encima. Se
   dispara cada EB_g_params.loopInterval segundos (reutiliza EB_g_loopAccum,
   que ya se resetea a 0 en cada EB_SpawnBurst — no necesita estado propio). */
static void EB_DrawWaterRingMesh(void) {
    float dur   = fmaxf(EB_g_params.waterRingDuration, 0.05f);
    float stag  = fmaxf(EB_g_params.waterRingStagger, 0.0f);
    float maxR  = fmaxf(EB_g_params.radius, 0.05f);
    float crestBase = fmaxf(EB_g_params.waterRingCrestHeight, 0.01f);
    float thickBase = fmaxf(EB_g_params.waterRingThickness, 0.01f);
    int   n = EB_g_params.waterRingCount;
    if (n > EB_WATER_RING_MAX) n = EB_WATER_RING_MAX;
    if (n < 1) n = 1;
    float baseY = 0.02f;
    int segs = 48;

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();

    /* Flash inicial del impacto en el suelo */
    float flash = EB_FadeAlpha(EB_g_loopAccum, 0.16f, 0.02f, 0.10f);
    if (EB_g_loopAccum < 0.16f && flash > 0.02f) {
        rlSetBlendMode(BLEND_ADDITIVE);
        Color fc = EB_g_waterRingColorMid; fc.a = (unsigned char)(flash * 200.0f);
        DrawSphere((Vector3){ 0.0f, baseY + 0.04f, 0.0f }, 0.16f * (1.0f - flash * 0.4f), fc);
        rlSetBlendMode(BLEND_ALPHA);
    }

    for (int ri = 0; ri < n; ri++) {
        float delay = ri * stag;
        float lt = EB_g_loopAccum - delay;
        if (lt < 0.0f) continue;
        float rt = EB_Clamp(lt / dur, 0.0f, 1.0f);
        float r  = maxR * (1.0f - (1.0f - rt) * (1.0f - rt));
        float aBase = EB_FadeAlpha(lt, dur, 0.05f, 0.45f);
        if (aBase < 0.01f) continue;

        float falloff = EB_Clamp(r / maxR, 0.0f, 1.0f);
        float thick  = fmaxf(thickBase * (1.0f - falloff * 0.6f), thickBase * 0.25f);
        float crest  = fmaxf(crestBase * (1.0f - falloff * 0.6f), crestBase * 0.2f);
        float rInner = r, rOuter = r + thick;

        rlSetBlendMode(BLEND_ALPHA);
        rlBegin(RL_TRIANGLES);
        for (int s = 0; s < segs; s++) {
            float a0 = (float)s / segs * 2.0f * PI;
            float a1 = (float)(s + 1) / segs * 2.0f * PI;

            float c0 = crest * (1.0f + 0.18f * sinf(a0 * 6.0f + EB_g_time * 4.5f + ri * 1.3f));
            float c1 = crest * (1.0f + 0.18f * sinf(a1 * 6.0f + EB_g_time * 4.5f + ri * 1.3f));
            float ro0 = rOuter * (1.0f + 0.04f * sinf(a0 * 5.0f + EB_g_time * 3.0f));
            float ro1 = rOuter * (1.0f + 0.04f * sinf(a1 * 5.0f + EB_g_time * 3.0f));
            float ri0 = rInner * (1.0f + 0.03f * sinf(a0 * 5.0f + EB_g_time * 3.0f + 0.5f));
            float ri1 = rInner * (1.0f + 0.03f * sinf(a1 * 5.0f + EB_g_time * 3.0f + 0.5f));

            float x0o = cosf(a0) * ro0, z0o = sinf(a0) * ro0;
            float x1o = cosf(a1) * ro1, z1o = sinf(a1) * ro1;
            float x0i = cosf(a0) * ri0, z0i = sinf(a0) * ri0;
            float x1i = cosf(a1) * ri1, z1i = sinf(a1) * ri1;

            unsigned char ab  = (unsigned char)(aBase * 110);
            unsigned char am  = (unsigned char)(aBase * 75);
            unsigned char at_ = (unsigned char)(aBase * 100);

            Color oc = EB_g_waterRingColorOuter, mc = EB_g_waterRingColorMid, cc = EB_g_waterRingColorCore;

            /* pared exterior */
            rlColor4ub(oc.r, oc.g, oc.b, ab); rlVertex3f(x0o, baseY,      z0o);
            rlColor4ub(oc.r, oc.g, oc.b, ab); rlVertex3f(x1o, baseY,      z1o);
            rlColor4ub(mc.r, mc.g, mc.b, at_); rlVertex3f(x0o, baseY+c0,  z0o);

            rlColor4ub(oc.r, oc.g, oc.b, ab); rlVertex3f(x1o, baseY,      z1o);
            rlColor4ub(mc.r, mc.g, mc.b, at_); rlVertex3f(x1o, baseY+c1,  z1o);
            rlColor4ub(mc.r, mc.g, mc.b, at_); rlVertex3f(x0o, baseY+c0,  z0o);

            /* tapa/espuma superior */
            rlColor4ub(mc.r, mc.g, mc.b, am);  rlVertex3f(x0i, baseY+c0*0.75f, z0i);
            rlColor4ub(mc.r, mc.g, mc.b, am);  rlVertex3f(x1i, baseY+c1*0.75f, z1i);
            rlColor4ub(cc.r, cc.g, cc.b, at_); rlVertex3f(x0o, baseY+c0,       z0o);

            rlColor4ub(mc.r, mc.g, mc.b, am);  rlVertex3f(x1i, baseY+c1*0.75f, z1i);
            rlColor4ub(cc.r, cc.g, cc.b, at_); rlVertex3f(x1o, baseY+c1,       z1o);
            rlColor4ub(cc.r, cc.g, cc.b, at_); rlVertex3f(x0o, baseY+c0,       z0o);
        }
        rlEnd();

        rlSetBlendMode(BLEND_ADDITIVE);
        for (int s = 0; s < segs; s++) {
            float a0 = (float)s / segs * 2.0f * PI;
            float a1 = (float)(s + 1) / segs * 2.0f * PI;
            float c0 = crest * (1.0f + 0.18f * sinf(a0 * 6.0f + EB_g_time * 4.5f + ri * 1.3f));
            float c1 = crest * (1.0f + 0.18f * sinf(a1 * 6.0f + EB_g_time * 4.5f + ri * 1.3f));
            float ro0 = rOuter * (1.0f + 0.04f * sinf(a0 * 5.0f + EB_g_time * 3.0f));
            float ro1 = rOuter * (1.0f + 0.04f * sinf(a1 * 5.0f + EB_g_time * 3.0f));
            Vector3 p0 = { cosf(a0) * ro0, baseY + c0, sinf(a0) * ro0 };
            Vector3 p1 = { cosf(a1) * ro1, baseY + c1, sinf(a1) * ro1 };
            Color foam = EB_g_waterRingColorCore; foam.a = (unsigned char)(aBase * 130);
            DrawLine3D(p0, p1, foam);
        }
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Earth Burst — grietas radiales + rocas en trayectoria balística ────────
   Todo procedural: cada roca/grieta se recalcula desde su índice + el
   tiempo transcurrido desde el último disparo (EB_g_loopAccum), sin arrays
   propios de estado. */
static void EB_DrawEarthBurstMesh(void) {
    float dur = fmaxf(EB_g_params.earthBurstDuration, 0.05f);
    float t = EB_g_loopAccum;
    int rocks = EB_g_params.earthBurstRockCount; if (rocks > EB_EARTH_ROCK_MAX) rocks = EB_EARTH_ROCK_MAX; if (rocks < 0) rocks = 0;
    int cracks = EB_g_params.earthBurstCrackCount; if (cracks > EB_EARTH_CRACK_MAX) cracks = EB_EARTH_CRACK_MAX; if (cracks < 0) cracks = 0;
    float maxR = fmaxf(EB_g_params.radius, 0.05f);
    Color core = EB_g_earthBurstColorCore, mid = EB_g_earthBurstColorMid, outer = EB_g_earthBurstColorOuter;

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ALPHA);

    /* Grietas radiales que se desvanecen con el tiempo */
    float crackA = EB_FadeAlpha(t, dur, 0.03f, 0.55f);
    if (crackA > 0.01f) {
        for (int i = 0; i < cracks; i++) {
            float a = ((float)i / (float)cracks) * 2.0f * PI + EB_HashRange(i * 7 + 1, -0.1f, 0.1f);
            float len = maxR * EB_HashRange(i * 13 + 2, 0.5f, 1.0f);
            Color cc = mid; cc.a = (unsigned char)(crackA * 200.0f);
            DrawLine3D((Vector3){ 0.0f, 0.02f, 0.0f }, (Vector3){ cosf(a) * len, 0.02f, sinf(a) * len }, cc);
        }
    }

    /* Anillo de polvo expandiéndose */
    {
        float rt = EB_Clamp(t / dur, 0.0f, 1.0f);
        float r = maxR * (1.0f - (1.0f - rt) * (1.0f - rt));
        float ringA = EB_FadeAlpha(t, dur, 0.04f, 0.5f);
        if (ringA > 0.01f) {
            int segs = 40;
            Color rc = outer; rc.a = (unsigned char)(ringA * 180.0f);
            for (int s = 0; s < segs; s++) {
                float a0 = (float)s / segs * 2.0f * PI, a1 = (float)(s + 1) / segs * 2.0f * PI;
                DrawLine3D((Vector3){ cosf(a0) * r, 0.02f, sinf(a0) * r }, (Vector3){ cosf(a1) * r, 0.02f, sinf(a1) * r }, rc);
            }
        }
    }

    /* Rocas en arco balístico: pos = dir*speed*t, y = vy*t - 0.5*g*t^2 */
    const float g = 9.0f;
    for (int i = 0; i < rocks; i++) {
        int seed = i * 31 + 5;
        float life = EB_HashRange(seed, 0.5f, 1.0f);
        if (t >= life) continue;
        float ang = EB_HashRange(seed + 1, 0.0f, 2.0f * PI);
        float spd = EB_HashRange(seed + 2, 0.6f, 2.0f) * maxR;
        float vy  = EB_HashRange(seed + 3, 2.0f, 4.5f);
        float sz  = EB_HashRange(seed + 4, 0.03f, 0.09f);
        float x = cosf(ang) * spd * t, z = sinf(ang) * spd * t;
        float y = vy * t - 0.5f * g * t * t;
        if (y < 0.0f) y = 0.0f;
        float a = EB_FadeAlpha(t, life, 0.05f, 0.35f);
        if (a < 0.02f) continue;
        Color rc = (i % 3 == 0) ? core : mid; rc.a = (unsigned char)(a * 235.0f);
        DrawCube((Vector3){ x, y, z }, sz, sz, sz, rc);
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Fire Burst — columna central ascendente + brasas en arco ──────────────── */
static void EB_DrawFireBurstMesh(void) {
    float dur = fmaxf(EB_g_params.fireBurstDuration, 0.05f);
    float t = EB_g_loopAccum;
    int embers = EB_g_params.fireBurstEmberCount; if (embers > EB_FIRE_EMBER_MAX) embers = EB_FIRE_EMBER_MAX; if (embers < 0) embers = 0;
    float colH = fmaxf(EB_g_params.fireBurstColumnHeight, 0.1f);
    Color core = EB_g_fireBurstColorCore, mid = EB_g_fireBurstColorMid, outer = EB_g_fireBurstColorOuter;

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ADDITIVE);

    /* Columna central: crece rápido y se desvanece con la vida del burst */
    {
        float colT = EB_Clamp(t / (dur * 0.3f), 0.0f, 1.0f);
        float colFade = EB_FadeAlpha(t, dur, 0.03f, 0.65f);
        float h = colT * colH;
        if (colFade > 0.01f) {
            int cSegs = 14, bSegs = 10;
            for (int b = 0; b < bSegs; b++) {
                float fy0 = (float)b / bSegs * h, fy1 = (float)(b + 1) / bSegs * h;
                float taper0 = 0.10f * (1.0f - (float)b / bSegs * 0.55f);
                float taper1 = 0.10f * (1.0f - (float)(b + 1) / bSegs * 0.55f);
                float segA = colFade * (1.0f - (float)b / bSegs * 0.3f);
                for (int c = 0; c < cSegs; c++) {
                    float ca0 = (float)c / cSegs * 2.0f * PI + EB_g_time * 1.5f;
                    float ca1 = (float)(c + 1) / cSegs * 2.0f * PI + EB_g_time * 1.5f;
                    Color lc = mid; lc.a = (unsigned char)(segA * 200.0f);
                    DrawLine3D((Vector3){ cosf(ca0) * taper0, fy0, sinf(ca0) * taper0 },
                               (Vector3){ cosf(ca1) * taper1, fy1, sinf(ca1) * taper1 }, lc);
                }
            }
            Color topC = core; topC.a = (unsigned char)(colFade * 200.0f);
            DrawSphere((Vector3){ 0.0f, h, 0.0f }, 0.10f * colFade, topC);
        }
    }

    /* Brasas: arco balístico igual que earth_burst pero con vida más corta y color fuego */
    const float g = 6.0f;
    for (int i = 0; i < embers; i++) {
        int seed = i * 37 + 11;
        float life = EB_HashRange(seed, 0.35f, 0.9f);
        if (t >= life) continue;
        float ang = EB_HashRange(seed + 1, 0.0f, 2.0f * PI);
        float spd = EB_HashRange(seed + 2, 0.3f, 1.6f);
        float vy  = EB_HashRange(seed + 3, 2.0f, 5.5f);
        float sz  = EB_HashRange(seed + 4, 0.02f, 0.06f);
        float x = cosf(ang) * spd * t, z = sinf(ang) * spd * t;
        float y = vy * t - 0.5f * g * t * t;
        if (y < 0.0f) y = 0.0f;
        float a = EB_FadeAlpha(t, life, 0.03f, 0.5f);
        if (a < 0.02f) continue;
        Color ec = (i % 4 == 0) ? outer : mid; ec.a = (unsigned char)(a * 220.0f);
        DrawSphere((Vector3){ x, y, z }, sz, ec);
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Lightning Burst — rayos procedurales con zigzag + orbes orbitando ───────
   Cada rayo se regenera con una nueva forma cada 'lightningBoltLife'
   segundos: la "generación" (floor(loopAccum/life)) entra en el hash junto
   al índice de rayo/segmento, así el zigzag cambia en cada ciclo sin
   necesitar guardar los puntos entre frames. */
static void EB_DrawLightningBurstMesh(void) {
    float life = fmaxf(EB_g_params.lightningBoltLife, 0.05f);
    float jitter = EB_g_params.lightningJitter;
    float height = fmaxf(EB_g_params.lightningHeight, 0.2f);
    int bolts = EB_g_params.lightningBoltCount; if (bolts > EB_LIGHTNING_BOLT_MAX) bolts = EB_LIGHTNING_BOLT_MAX; if (bolts < 0) bolts = 0;
    Color core = EB_g_lightningColorCore, mid = EB_g_lightningColorMid, outer = EB_g_lightningColorOuter;

    int gen = (int)floorf(EB_g_time / life);
    float age = fmodf(EB_g_time, life);
    float alpha = EB_FadeAlpha(age, life, 0.06f, 0.6f);

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ADDITIVE);

    for (int b = 0; b < bolts; b++) {
        int baseSeed = b * 971 + gen * 131;
        float boltAngle = EB_HashRange(baseSeed, 0.0f, 2.0f * PI);
        float boltDist  = EB_HashRange(baseSeed + 1, 0.0f, 0.5f);
        Vector3 from = { cosf(boltAngle) * boltDist, height, sinf(boltAngle) * boltDist };
        Vector3 to   = { cosf(boltAngle + EB_HashRange(baseSeed + 2, -0.4f, 0.4f)) * boltDist * 0.6f, 0.0f,
                          sinf(boltAngle + EB_HashRange(baseSeed + 2, -0.4f, 0.4f)) * boltDist * 0.6f };
        Vector3 prev = from;
        for (int s = 1; s <= EB_LIGHTNING_SEGS; s++) {
            float u = (float)s / EB_LIGHTNING_SEGS;
            float env = sinf(u * PI);
            int ps = baseSeed * 17 + s * 3;
            Vector3 pt = {
                from.x + (to.x - from.x) * u + EB_HashRange(ps, -jitter, jitter) * env,
                from.y + (to.y - from.y) * u + EB_HashRange(ps + 1, -jitter * 0.4f, jitter * 0.4f) * env,
                from.z + (to.z - from.z) * u + EB_HashRange(ps + 2, -jitter, jitter) * env,
            };
            Color c1 = core; c1.a = (unsigned char)(alpha * 240.0f);
            Color c2 = mid;  c2.a = (unsigned char)(alpha * 160.0f);
            DrawLine3D(prev, pt, c1);
            DrawLine3D(prev, pt, c2);
            prev = pt;
        }
    }

    /* Orbes orbitando la base — continuos, no dependen del ciclo del rayo */
    {
        int orbs = 3;
        for (int i = 0; i < orbs; i++) {
            float a = EB_g_time * 4.0f + (float)i / orbs * 2.0f * PI;
            Vector3 pos = { cosf(a) * 0.6f, 0.6f + sinf(EB_g_time * 3.0f + i) * 0.1f, sinf(a) * 0.6f };
            Color oc = outer; oc.a = 200;
            DrawSphere(pos, 0.05f, oc);
        }
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Poison Burst — anillos "blob" orgánicos (multi-frecuencia) + burbujas ── */
static void EB_DrawPoisonBurstMesh(void) {
    float dur = fmaxf(EB_g_params.poisonDuration, 0.05f);
    float t = EB_g_loopAccum;
    int rings = EB_g_params.poisonRingCount; if (rings > EB_POISON_RING_MAX) rings = EB_POISON_RING_MAX; if (rings < 0) rings = 0;
    int bubbles = EB_g_params.poisonBubbleCount; if (bubbles > EB_POISON_BUBBLE_MAX) bubbles = EB_POISON_BUBBLE_MAX; if (bubbles < 0) bubbles = 0;
    float maxR = fmaxf(EB_g_params.radius, 0.05f);
    Color core = EB_g_poisonColorCore, mid = EB_g_poisonColorMid, outer = EB_g_poisonColorOuter;

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ALPHA);

    for (int ri = 0; ri < rings; ri++) {
        float delay = ri * 0.07f;
        float lt = t - delay;
        if (lt < 0.0f) continue;
        float rt = EB_Clamp(lt / dur, 0.0f, 1.0f);
        float r = maxR * (1.0f - (1.0f - rt) * (1.0f - rt));
        float aBase = EB_FadeAlpha(lt, dur, 0.05f, 0.4f);
        if (aBase < 0.01f) continue;
        int segs = 56;
        for (int s = 0; s < segs; s++) {
            float a0 = (float)s / segs * 2.0f * PI, a1 = (float)(s + 1) / segs * 2.0f * PI;
            float blob0 = 1.0f + 0.18f * sinf(a0 * 3.0f + EB_g_time * 1.8f + ri * 2.1f)
                                + 0.10f * sinf(a0 * 7.0f - EB_g_time * 2.7f + ri * 0.9f);
            float blob1 = 1.0f + 0.18f * sinf(a1 * 3.0f + EB_g_time * 1.8f + ri * 2.1f)
                                + 0.10f * sinf(a1 * 7.0f - EB_g_time * 2.7f + ri * 0.9f);
            Color oc = outer; oc.a = (unsigned char)(aBase * 210.0f);
            DrawLine3D((Vector3){ cosf(a0) * r * blob0, 0.02f, sinf(a0) * r * blob0 },
                       (Vector3){ cosf(a1) * r * blob1, 0.02f, sinf(a1) * r * blob1 }, oc);
            Color ic = mid; ic.a = (unsigned char)(aBase * 150.0f);
            DrawLine3D((Vector3){ cosf(a0) * r * blob0 * 0.78f, 0.015f, sinf(a0) * r * blob0 * 0.78f },
                       (Vector3){ cosf(a1) * r * blob1 * 0.78f, 0.015f, sinf(a1) * r * blob1 * 0.78f }, ic);
        }
    }

    /* Burbujas ascendiendo con wobble lateral, en bucle continuo */
    rlSetBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < bubbles; i++) {
        int seed = i * 53 + 3;
        float life = EB_HashRange(seed, 0.6f, 1.3f);
        float phase = EB_HashRange(seed + 1, 0.0f, life);
        float age = fmodf(EB_g_time + phase, life);
        float ang = EB_HashRange(seed + 2, 0.0f, 2.0f * PI);
        float dist = EB_HashRange(seed + 3, 0.0f, maxR * 0.6f);
        float wobbleFreq = EB_HashRange(seed + 4, 4.0f, 9.0f);
        float wobbleAmp = EB_HashRange(seed + 5, 0.02f, 0.05f);
        float x = cosf(ang) * dist + sinf(age * wobbleFreq) * wobbleAmp;
        float z = sinf(ang) * dist + cosf(age * wobbleFreq * 0.7f) * wobbleAmp;
        float y = age * 0.9f;
        float a = EB_FadeAlpha(age, life, 0.1f, 0.3f);
        Color bc = core; bc.a = (unsigned char)(a * 180.0f);
        DrawSphere((Vector3){ x, y, z }, EB_HashRange(seed + 6, 0.02f, 0.05f), bc);
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Heal Aura — cruces orbitales ascendentes + pilar de luz central ────────
   Persistente/continua (no ligada al stomp): usa EB_g_time con fase por
   índice para que cada cruz recorra su ciclo órbita→ascenso en bucle. */
static void EB_DrawHealAuraMesh(void) {
    int crosses = EB_g_params.healCrossCount; if (crosses > EB_HEAL_CROSS_MAX) crosses = EB_HEAL_CROSS_MAX; if (crosses < 0) crosses = 0;
    float cycle = fmaxf(EB_g_params.healCycleDuration, 0.1f);
    float pillarH = fmaxf(EB_g_params.healPillarHeight, 0.1f);
    Color core = EB_g_healColorCore, mid = EB_g_healColorMid, outer = EB_g_healColorOuter;

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ADDITIVE);

    /* Pilar de luz central pulsante */
    {
        float pulse = 0.55f + 0.45f * sinf(EB_g_time * 5.5f);
        int segs = 12;
        for (int s = 0; s < segs - 1; s++) {
            float y0 = (float)s / segs * pillarH, y1 = (float)(s + 1) / segs * pillarH;
            Color lc = core; lc.a = (unsigned char)(pulse * (1.0f - (float)s / segs * 0.7f) * 220.0f);
            DrawLine3D((Vector3){ 0.0f, y0, 0.0f }, (Vector3){ 0.0f, y1, 0.0f }, lc);
        }
    }

    for (int i = 0; i < crosses; i++) {
        int seed = i * 41 + 9;
        float phase = EB_HashRange(seed, 0.0f, cycle);
        float age = fmodf(EB_g_time + phase, cycle);
        float t01 = age / cycle;
        float orbitSpeed = EB_HashRange(seed + 1, 2.5f, 6.0f) * (((i % 2) == 0) ? 1.0f : -1.0f);
        float orbitR = EB_HashRange(seed + 2, 0.3f, 0.85f);
        float orbitAngle = EB_g_time * orbitSpeed + phase;
        float h = t01 * pillarH * 0.9f;
        Vector3 pos = { cosf(orbitAngle) * orbitR, h, sinf(orbitAngle) * orbitR };
        float a = EB_FadeAlpha(age, cycle, 0.1f, 0.4f);
        float size = 0.09f;
        Color cc = (i % 3 == 0) ? core : mid; cc.a = (unsigned char)(a * 235.0f);
        DrawLine3D((Vector3){ pos.x - size, pos.y, pos.z }, (Vector3){ pos.x + size, pos.y, pos.z }, cc);
        DrawLine3D((Vector3){ pos.x, pos.y - size, pos.z }, (Vector3){ pos.x, pos.y + size, pos.z }, cc);
        Color gc = outer; gc.a = (unsigned char)(a * 120.0f);
        DrawSphere(pos, size * 0.35f, gc);
    }

    /* Anillo dorado de suelo, pulsante */
    {
        float pulse = 0.6f + 0.4f * sinf(EB_g_time * 3.2f);
        int segs = 40;
        float r = 0.7f;
        Color rc = mid; rc.a = (unsigned char)(pulse * 160.0f);
        for (int s = 0; s < segs; s++) {
            float a0 = (float)s / segs * 2.0f * PI, a1 = (float)(s + 1) / segs * 2.0f * PI;
            DrawLine3D((Vector3){ cosf(a0) * r, 0.02f, sinf(a0) * r }, (Vector3){ cosf(a1) * r, 0.02f, sinf(a1) * r }, rc);
        }
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Dark Slash — tentáculos que emergen/retraen + orbes oscuros ────────────
   Ligado al stomp/cast: usa EB_g_loopAccum como reloj del ciclo (emerge en
   el primer 30%, se retrae en el resto), igual que en effects.h. */
static void EB_DrawDarkSlashMesh(void) {
    float t = EB_g_loopAccum;
    int tentacles = EB_g_params.darkTentacleCount; if (tentacles > EB_DARK_TENTACLE_MAX) tentacles = EB_DARK_TENTACLE_MAX; if (tentacles < 0) tentacles = 0;
    int orbs = EB_g_params.darkOrbCount; if (orbs > EB_DARK_ORB_MAX) orbs = EB_DARK_ORB_MAX; if (orbs < 0) orbs = 0;
    Color core = EB_g_darkSlashColorCore, mid = EB_g_darkSlashColorMid, outer = EB_g_darkSlashColorOuter;

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ADDITIVE);

    for (int i = 0; i < tentacles; i++) {
        int seed = i * 61 + 4;
        float life = EB_HashRange(seed, 0.25f, 0.55f) * (EB_g_params.darkDuration / 0.7f);
        if (life < 0.05f) life = 0.05f;
        if (t >= life) continue;
        float u = t / life;
        float grow = (u < 0.3f) ? (u / 0.3f) : (1.0f - (u - 0.3f) / 0.7f);
        float maxLen = EB_HashRange(seed + 1, 0.6f, 1.4f);
        float len = maxLen * grow * grow;
        float ang = ((float)i / tentacles) * 2.0f * PI + EB_HashRange(seed + 2, -0.3f, 0.3f) + sinf(t * 8.0f + i) * 0.15f;
        float a = EB_FadeAlpha(t, life, 0.05f, 0.3f);
        if (a < 0.02f) continue;
        Vector3 p0 = { 0.0f, 0.02f, 0.0f };
        Vector3 p1 = { cosf(ang) * len, 0.02f + len * 0.3f, sinf(ang) * len };
        Color cc = mid; cc.a = (unsigned char)(a * 235.0f);
        DrawLine3D(p0, p1, cc);
        Color bc = outer; bc.a = (unsigned char)(a * 100.0f);
        Vector3 pb = { p1.x + cosf(ang + 0.5f) * len * 0.3f, p1.y + 0.1f, p1.z + sinf(ang + 0.5f) * len * 0.3f };
        DrawLine3D(p1, pb, bc);
    }

    for (int i = 0; i < orbs; i++) {
        int seed = i * 83 + 6;
        float life = EB_HashRange(seed, 0.3f, 0.8f);
        float age = fmodf(t, life * 3.0f);
        if (age >= life) continue;
        float ang = EB_g_time * EB_HashRange(seed + 1, 3.0f, 8.0f) * (((i % 2) == 0) ? 1.0f : -1.0f);
        float r = EB_HashRange(seed + 2, 0.15f, 0.7f) * (1.0f + age * 0.8f);
        float a = EB_FadeAlpha(age, life, 0.08f, 0.35f);
        Color oc = core; oc.a = (unsigned char)(a * 220.0f);
        DrawSphere((Vector3){ cosf(ang) * r, 0.4f + age * 0.6f, sinf(ang) * r }, EB_HashRange(seed + 3, 0.02f, 0.05f), oc);
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Barrier — domo pulsante + 3 anillos orbitales inclinados + motas ───────
   Persistente/continua, variante enriquecida de "field": pulsa, y suma
   motas ascendiendo por la superficie de forma continua. */
static void EB_DrawBarrierMesh(void) {
    float r = fmaxf(EB_g_params.radius, 0.1f);
    float pulseSpeed = EB_g_params.barrierPulseSpeed;
    int motes = EB_g_params.barrierMoteCount; if (motes > EB_BARRIER_MOTE_MAX) motes = EB_BARRIER_MOTE_MAX; if (motes < 0) motes = 0;
    Color core = EB_g_barrierColorCore, mid = EB_g_barrierColorMid, outer = EB_g_barrierColorOuter;

    float pulse = 0.9f + 0.1f * sinf(EB_g_time * pulseSpeed);
    float rr = r * pulse;

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ALPHA);

    Color domeC = mid; domeC.a = 40;
    DrawSphere((Vector3){ 0.0f, 0.9f, 0.0f }, rr, domeC);
    Color wireC = outer; wireC.a = 130;
    DrawSphereWires((Vector3){ 0.0f, 0.9f, 0.0f }, rr, 10, 10, wireC);

    /* 3 anillos orbitales inclinados, girando a distintas velocidades */
    int segs = 36;
    float angles[3] = { EB_g_time * 0.8f, -EB_g_time * 1.1f, EB_g_time * 0.5f };
    float tilts[3]  = { 0.0f, 0.866f, 0.866f };
    for (int ring = 0; ring < 3; ring++) {
        Color rc = outer; rc.a = (unsigned char)(200 - ring * 40);
        float tilt = tilts[ring];
        for (int s = 0; s < segs; s++) {
            float a0 = angles[ring] + (float)s / segs * 2.0f * PI;
            float a1 = angles[ring] + (float)(s + 1) / segs * 2.0f * PI;
            Vector3 p0, p1;
            if (ring == 0) {
                p0 = (Vector3){ cosf(a0) * rr, 0.9f, sinf(a0) * rr };
                p1 = (Vector3){ cosf(a1) * rr, 0.9f, sinf(a1) * rr };
            } else if (ring == 1) {
                p0 = (Vector3){ cosf(a0) * rr, 0.9f + sinf(a0) * rr * tilt * 0.5f, sinf(a0) * rr * (1.0f - tilt * 0.4f) };
                p1 = (Vector3){ cosf(a1) * rr, 0.9f + sinf(a1) * rr * tilt * 0.5f, sinf(a1) * rr * (1.0f - tilt * 0.4f) };
            } else {
                p0 = (Vector3){ cosf(a0) * rr * (1.0f - tilt * 0.4f), 0.9f + sinf(a0) * rr * tilt * 0.5f, sinf(a0) * rr };
                p1 = (Vector3){ cosf(a1) * rr * (1.0f - tilt * 0.4f), 0.9f + sinf(a1) * rr * tilt * 0.5f, sinf(a1) * rr };
            }
            DrawLine3D(p0, p1, rc);
        }
    }

    /* Motas ascendiendo por la superficie, en bucle continuo por índice */
    rlSetBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < motes; i++) {
        int seed = i * 29 + 8;
        float life = EB_HashRange(seed, 1.2f, 2.2f);
        float phase = EB_HashRange(seed + 1, 0.0f, life);
        float age = fmodf(EB_g_time + phase, life);
        float ang = EB_HashRange(seed + 2, 0.0f, 2.0f * PI);
        float latitude = EB_Clamp(age / life, 0.0f, 1.0f);
        float ringR = rr * cosf(latitude * PI * 0.5f);
        Vector3 pos = { cosf(ang) * ringR, 0.9f + latitude * rr, sinf(ang) * ringR };
        float a = (latitude < 0.2f) ? latitude / 0.2f : (latitude > 0.7f ? (1.0f - latitude) / 0.3f : 1.0f);
        Color mc = core; mc.a = (unsigned char)(a * 200.0f);
        DrawSphere(pos, 0.02f, mc);
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ══════════════════════════════════════════════════════════════════════════
   PROYECTILES — fireball / wind_slash / rock_throw / lightning_bolt /
   water_jet / ice_shard / poison_orb / dark_orb / light_arrow.
   Todos viajan de `origin` a `target` (EB_ProjectileEndpoints, reutiliza
   directionYaw + radius) y usan EB_g_loopAccum como "tiempo desde el
   disparo" — se relanzan solos cada loopInterval. Estelas/desechos también
   son procedurales (hash por índice), sin arrays de estado propio.
   ══════════════════════════════════════════════════════════════════════════ */

/* ── Fireball — núcleo girando + cola ondulante + brasas cayendo ──────────── */
static void EB_DrawFireballMesh(void) {
    float dur = fmaxf(EB_g_params.fireballDuration, 0.05f);
    float t = EB_g_loopAccum;
    Vector3 origin, target;
    EB_ProjectileEndpoints(&origin, &target);
    Vector3 dirN = EB_V3Normalize(EB_V3Sub(target, origin));
    Vector3 perp = { -dirN.z, 0.0f, dirN.x };
    Color core = EB_g_fireballColorCore, mid = EB_g_fireballColorMid, outer = EB_g_fireballColorOuter;

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ADDITIVE);

    if (t < dur) {
        float t01 = t / dur;
        Vector3 pos = EB_V3Lerp(origin, target, t01);
        Color coreC = core; coreC.a = 235;
        DrawSphere(pos, 0.09f, coreC);
        Color midC = mid; midC.a = 140;
        DrawSphere(pos, 0.15f, midC);

        int trailSegs = EB_PROJ_TRAIL_SEGS;
        Vector3 prevP = pos;
        for (int s = 1; s <= trailSegs; s++) {
            float u = t01 - (float)s / trailSegs * 0.18f;
            if (u < 0.0f) break;
            Vector3 p = EB_V3Lerp(origin, target, u);
            float wig = sinf(u * 30.0f + EB_g_time * 10.0f) * 0.03f * (1.0f - u);
            p = EB_V3Add(p, EB_V3Scale(perp, wig));
            float segA = 1.0f - (float)s / trailSegs;
            Color lc = mid; lc.a = (unsigned char)(segA * 160.0f);
            DrawLine3D(prevP, p, lc);
            prevP = p;
        }
    }

    int embers = EB_g_params.fireballEmberCount; if (embers > EB_FIREBALL_EMBER_MAX) embers = EB_FIREBALL_EMBER_MAX; if (embers < 0) embers = 0;
    for (int i = 0; i < embers; i++) {
        int seed = i * 43 + 2;
        float spawnT = EB_HashRange(seed, 0.0f, dur);
        if (t < spawnT) continue;
        float age = t - spawnT;
        float life = EB_HashRange(seed + 1, 0.2f, 0.5f);
        if (age >= life) continue;
        float u = EB_Clamp(spawnT / dur, 0.0f, 1.0f);
        Vector3 spawnPos = EB_V3Lerp(origin, target, u);
        float fall = age * age * 3.0f;
        Vector3 p = { spawnPos.x, spawnPos.y - fall, spawnPos.z };
        float a = EB_FadeAlpha(age, life, 0.05f, 0.5f);
        Color ec = outer; ec.a = (unsigned char)(a * 200.0f);
        DrawSphere(p, 0.02f, ec);
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Wind Slash — cuchilla en media luna + estelas afterimage ─────────────── */
static void EB_DrawWindSlashMesh(void) {
    float dur = fmaxf(EB_g_params.windSlashDuration, 0.02f);
    float t = EB_g_loopAccum;
    if (t >= dur) return;
    Vector3 origin, target;
    EB_ProjectileEndpoints(&origin, &target);
    Vector3 dirN = EB_V3Normalize(EB_V3Sub(target, origin));
    Vector3 perp = { -dirN.z, 0.0f, dirN.x };
    Vector3 up = { 0.0f, 1.0f, 0.0f };
    Color core = EB_g_windSlashColorCore, mid = EB_g_windSlashColorMid;

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ADDITIVE);

    int trails = EB_g_params.windSlashTrailCount; if (trails > EB_WIND_SLASH_TRAIL_MAX) trails = EB_WIND_SLASH_TRAIL_MAX; if (trails < 0) trails = 0;
    for (int k = 0; k <= trails; k++) {
        float u = t / dur - (float)k * 0.05f;
        if (u < 0.0f) continue;
        u = EB_Clamp(u, 0.0f, 1.0f);
        Vector3 pos = EB_V3Lerp(origin, target, u);
        float kFade = 1.0f - (float)k / (float)(trails + 1);
        float fadeAlpha = EB_FadeAlpha(t, dur, 0.03f, 0.35f) * kFade;
        Color bc = (k == 0) ? core : mid; bc.a = (unsigned char)(fadeAlpha * 230.0f);

        int segs = 8;
        float bladeR = 0.18f, arcSpan = 1.6f;
        Vector3 prevP = { 0 };
        for (int s = 0; s <= segs; s++) {
            float ang = -arcSpan * 0.5f + arcSpan * ((float)s / segs);
            Vector3 off = EB_V3Add(EB_V3Scale(perp, sinf(ang) * bladeR), EB_V3Scale(up, cosf(ang) * bladeR * 0.6f));
            Vector3 p = EB_V3Add(pos, off);
            if (s > 0) DrawLine3D(prevP, p, bc);
            prevP = p;
        }
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Rock Throw — roca tumbling en arco balístico + polvo cayendo ─────────── */
static void EB_DrawRockThrowMesh(void) {
    float dur = fmaxf(EB_g_params.rockThrowDuration, 0.05f);
    float t = EB_g_loopAccum;
    Vector3 origin, target;
    EB_ProjectileEndpoints(&origin, &target);
    Color core = EB_g_rockThrowColorCore, mid = EB_g_rockThrowColorMid, outer = EB_g_rockThrowColorOuter;
    float arcH = 0.7f;

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ALPHA);

    if (t < dur) {
        float t01 = t / dur;
        Vector3 pos = EB_V3Lerp(origin, target, t01);
        pos.y += sinf(t01 * PI) * arcH;
        rlPushMatrix();
        rlTranslatef(pos.x, pos.y, pos.z);
        rlRotatef(t * 260.0f, 0.6f, 1.0f, 0.3f);
        DrawCube((Vector3){ 0, 0, 0 }, 0.1f, 0.1f, 0.1f, core);
        DrawCubeWires((Vector3){ 0, 0, 0 }, 0.11f, 0.11f, 0.11f, mid);
        rlPopMatrix();
    }

    int dust = EB_g_params.rockThrowDustCount; if (dust > EB_ROCK_DUST_MAX) dust = EB_ROCK_DUST_MAX; if (dust < 0) dust = 0;
    for (int i = 0; i < dust; i++) {
        int seed = i * 59 + 7;
        float spawnT = EB_HashRange(seed, 0.0f, dur);
        if (t < spawnT) continue;
        float age = t - spawnT;
        float life = EB_HashRange(seed + 1, 0.3f, 0.6f);
        if (age >= life) continue;
        float u = EB_Clamp(spawnT / dur, 0.0f, 1.0f);
        Vector3 spawnPos = EB_V3Lerp(origin, target, u);
        spawnPos.y += sinf(u * PI) * arcH;
        float fall = age * age * 3.5f;
        Vector3 p = { spawnPos.x + EB_HashRange(seed + 2, -0.05f, 0.05f), fmaxf(0.0f, spawnPos.y - fall), spawnPos.z + EB_HashRange(seed + 3, -0.05f, 0.05f) };
        float a = EB_FadeAlpha(age, life, 0.05f, 0.5f);
        Color dc = outer; dc.a = (unsigned char)(a * 180.0f);
        DrawSphere(p, 0.025f, dc);
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Lightning Bolt — zigzag instantáneo entre origen y objetivo ──────────── */
static void EB_DrawLightningBoltMesh(void) {
    float dur = fmaxf(EB_g_params.lightningBoltDuration, 0.02f);
    float t = EB_g_loopAccum;
    if (t >= dur) return;
    Vector3 origin, target;
    EB_ProjectileEndpoints(&origin, &target);
    Color core = EB_g_lightningBoltColorCore, mid = EB_g_lightningBoltColorMid, outer = EB_g_lightningBoltColorOuter;
    float alpha = EB_FadeAlpha(t, dur, 0.05f, 0.6f);
    int branches = EB_g_params.lightningBoltBranches; if (branches > EB_LIGHTNING_PROJ_BRANCH_MAX) branches = EB_LIGHTNING_PROJ_BRANCH_MAX; if (branches < 0) branches = 0;
    int gen = (int)floorf(EB_g_time / fmaxf(EB_g_params.loopInterval, 0.05f));

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ADDITIVE);

    Vector3 segStore[EB_LIGHTNING_PROJ_SEGS + 1];
    segStore[0] = origin;
    Vector3 prev = origin;
    for (int s = 1; s <= EB_LIGHTNING_PROJ_SEGS; s++) {
        float u = (float)s / EB_LIGHTNING_PROJ_SEGS;
        float env = sinf(u * PI);
        int ps = gen * 401 + s * 13;
        Vector3 pt = EB_V3Lerp(origin, target, u);
        pt.x += EB_HashRange(ps, -0.25f, 0.25f) * env;
        pt.y += EB_HashRange(ps + 1, -0.15f, 0.15f) * env;
        pt.z += EB_HashRange(ps + 2, -0.25f, 0.25f) * env;
        segStore[s] = pt;
        Color c1 = core; c1.a = (unsigned char)(alpha * 240.0f);
        Color c2 = mid;  c2.a = (unsigned char)(alpha * 140.0f);
        DrawLine3D(prev, pt, c1);
        DrawLine3D(prev, pt, c2);
        prev = pt;
    }

    for (int b = 0; b < branches; b++) {
        int seed = gen * 211 + b * 17 + 3;
        int segIdx = 2 + (int)EB_HashRange(seed, 0.0f, (float)(EB_LIGHTNING_PROJ_SEGS - 3));
        Vector3 from = segStore[segIdx];
        float ang = EB_HashRange(seed + 1, 0.0f, 2.0f * PI);
        float len = EB_HashRange(seed + 2, 0.15f, 0.4f);
        Vector3 to = { from.x + cosf(ang) * len, from.y + EB_HashRange(seed + 3, -0.1f, 0.2f), from.z + sinf(ang) * len };
        Color bc = mid; bc.a = (unsigned char)(alpha * 150.0f);
        DrawLine3D(from, to, bc);
    }

    if (t > dur * 0.6f) {
        float fa = (t - dur * 0.6f) / (dur * 0.4f);
        Color fc = outer; fc.a = (unsigned char)((1.0f - fa) * 200.0f);
        DrawSphere(target, 0.08f * (1.0f + fa), fc);
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Water Jet — chorro serpenteante + salpicadura de gotas ───────────────── */
static void EB_DrawWaterJetMesh(void) {
    float dur = fmaxf(EB_g_params.waterJetDuration, 0.05f);
    float t = EB_g_loopAccum;
    Vector3 origin, target;
    EB_ProjectileEndpoints(&origin, &target);
    Vector3 dirN = EB_V3Normalize(EB_V3Sub(target, origin));
    Vector3 perp = { -dirN.z, 0.0f, dirN.x };
    Color core = EB_g_waterJetColorCore, mid = EB_g_waterJetColorMid, outer = EB_g_waterJetColorOuter;

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ALPHA);

    if (t < dur) {
        float t01 = t / dur;
        int segs = EB_PROJ_TRAIL_SEGS;
        Vector3 prevP = origin;
        for (int s = 1; s <= segs; s++) {
            float u = t01 * (float)s / segs;
            Vector3 p = EB_V3Lerp(origin, target, u);
            float wig = sinf(u * 18.0f - t * 10.0f) * 0.10f;
            p = EB_V3Add(p, EB_V3Scale(perp, wig));
            Color lc = core; lc.a = 200;
            DrawLine3D(prevP, p, lc);
            prevP = p;
        }
        DrawSphere(prevP, 0.06f, mid);
    }

    int drops = EB_g_params.waterJetDropCount; if (drops > EB_WATER_JET_DROP_MAX) drops = EB_WATER_JET_DROP_MAX; if (drops < 0) drops = 0;
    for (int i = 0; i < drops; i++) {
        int seed = i * 67 + 11;
        float spawnT = EB_HashRange(seed, 0.0f, dur);
        if (t < spawnT) continue;
        float age = t - spawnT;
        float life = EB_HashRange(seed + 1, 0.2f, 0.45f);
        if (age >= life) continue;
        float u = EB_Clamp(spawnT / dur, 0.0f, 1.0f);
        Vector3 spawnPos = EB_V3Lerp(origin, target, u);
        float wig = sinf(u * 18.0f - spawnT * 10.0f) * 0.10f;
        spawnPos = EB_V3Add(spawnPos, EB_V3Scale(perp, wig));
        float fall = age * age * 3.0f;
        Vector3 p = { spawnPos.x + EB_HashRange(seed + 2, -0.05f, 0.05f), spawnPos.y - fall, spawnPos.z + EB_HashRange(seed + 3, -0.05f, 0.05f) };
        float a = EB_FadeAlpha(age, life, 0.05f, 0.5f);
        Color dc = outer; dc.a = (unsigned char)(a * 200.0f);
        DrawSphere(p, 0.018f, dc);
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Ice Shard — prisma facetado girando + chispas de escarcha ────────────── */
static void EB_DrawIceShardMesh(void) {
    float dur = fmaxf(EB_g_params.iceShardDuration, 0.05f);
    float t = EB_g_loopAccum;
    Vector3 origin, target;
    EB_ProjectileEndpoints(&origin, &target);
    Vector3 dirN = EB_V3Normalize(EB_V3Sub(target, origin));
    float yawDeg = atan2f(dirN.x, dirN.z) * RAD2DEG;
    Color core = EB_g_iceShardColorCore, mid = EB_g_iceShardColorMid, outer = EB_g_iceShardColorOuter;

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ALPHA);

    if (t < dur) {
        float t01 = t / dur;
        Vector3 pos = EB_V3Lerp(origin, target, t01);
        rlPushMatrix();
        rlTranslatef(pos.x, pos.y, pos.z);
        rlRotatef(yawDeg, 0.0f, 1.0f, 0.0f);
        rlRotatef(EB_g_time * 90.0f, 0.0f, 0.0f, 1.0f);
        DrawCube((Vector3){ 0, 0, 0 }, 0.05f, 0.05f, 0.22f, core);
        DrawCubeWires((Vector3){ 0, 0, 0 }, 0.06f, 0.06f, 0.23f, mid);
        rlPopMatrix();
    }

    int sparks = EB_g_params.iceShardSparkCount; if (sparks > EB_ICE_SHARD_SPARK_MAX) sparks = EB_ICE_SHARD_SPARK_MAX; if (sparks < 0) sparks = 0;
    for (int i = 0; i < sparks; i++) {
        int seed = i * 73 + 13;
        float spawnT = EB_HashRange(seed, 0.0f, dur);
        if (t < spawnT) continue;
        float age = t - spawnT;
        float life = EB_HashRange(seed + 1, 0.25f, 0.5f);
        if (age >= life) continue;
        float u = EB_Clamp(spawnT / dur, 0.0f, 1.0f);
        Vector3 spawnPos = EB_V3Lerp(origin, target, u);
        float fall = age * age * 2.0f;
        Vector3 p = { spawnPos.x + EB_HashRange(seed + 2, -0.06f, 0.06f), spawnPos.y - fall, spawnPos.z + EB_HashRange(seed + 3, -0.06f, 0.06f) };
        float a = EB_FadeAlpha(age, life, 0.05f, 0.5f);
        Color sc = outer; sc.a = (unsigned char)(a * 220.0f);
        DrawSphere(p, 0.014f, sc);
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Poison Orb — orbe pulsante + goteo de esporas ─────────────────────────── */
static void EB_DrawPoisonOrbMesh(void) {
    float dur = fmaxf(EB_g_params.poisonOrbDuration, 0.05f);
    float t = EB_g_loopAccum;
    Vector3 origin, target;
    EB_ProjectileEndpoints(&origin, &target);
    Color core = EB_g_poisonOrbColorCore, mid = EB_g_poisonOrbColorMid, outer = EB_g_poisonOrbColorOuter;

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ALPHA);

    if (t < dur) {
        float t01 = t / dur;
        Vector3 pos = EB_V3Lerp(origin, target, t01);
        float pulse = 0.85f + 0.15f * sinf(EB_g_time * 8.0f);
        DrawSphere(pos, 0.09f * pulse, core);
        Color glow = mid; glow.a = 110;
        DrawSphere(pos, 0.14f * pulse, glow);
    }

    int spores = EB_g_params.poisonOrbSporeCount; if (spores > EB_POISON_ORB_SPORE_MAX) spores = EB_POISON_ORB_SPORE_MAX; if (spores < 0) spores = 0;
    for (int i = 0; i < spores; i++) {
        int seed = i * 79 + 17;
        float spawnT = EB_HashRange(seed, 0.0f, dur);
        if (t < spawnT) continue;
        float age = t - spawnT;
        float life = EB_HashRange(seed + 1, 0.3f, 0.6f);
        if (age >= life) continue;
        float u = EB_Clamp(spawnT / dur, 0.0f, 1.0f);
        Vector3 spawnPos = EB_V3Lerp(origin, target, u);
        float fall = age * age * 2.2f;
        Vector3 p = { spawnPos.x + EB_HashRange(seed + 2, -0.05f, 0.05f), spawnPos.y - fall, spawnPos.z + EB_HashRange(seed + 3, -0.05f, 0.05f) };
        float a = EB_FadeAlpha(age, life, 0.05f, 0.5f);
        Color dc = outer; dc.a = (unsigned char)(a * 200.0f);
        DrawSphere(p, 0.02f, dc);
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Dark Orb — orbe void + tentáculos de sombra arrastrándose ────────────── */
static void EB_DrawDarkOrbMesh(void) {
    float dur = fmaxf(EB_g_params.darkOrbDuration, 0.05f);
    float t = EB_g_loopAccum;
    Vector3 origin, target;
    EB_ProjectileEndpoints(&origin, &target);
    Color core = EB_g_darkOrbColorCore, mid = EB_g_darkOrbColorMid, outer = EB_g_darkOrbColorOuter;

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ALPHA);

    if (t < dur) {
        float t01 = t / dur;
        Vector3 pos = EB_V3Lerp(origin, target, t01);
        DrawSphere(pos, 0.08f, core);
        Color glow = mid; glow.a = 90;
        DrawSphere(pos, 0.13f, glow);

        int tentacles = EB_g_params.darkOrbTentacleCount; if (tentacles > EB_DARK_ORB_TENTACLE_MAX) tentacles = EB_DARK_ORB_TENTACLE_MAX; if (tentacles < 0) tentacles = 0;
        for (int i = 0; i < tentacles; i++) {
            float back = 0.08f + 0.04f * i;
            float u2 = EB_Clamp(t01 - back, 0.0f, 1.0f);
            Vector3 tp = EB_V3Lerp(origin, target, u2);
            tp.y += sinf(EB_g_time * 6.0f + i * 2.0f) * 0.04f;
            Color tc = outer; tc.a = (unsigned char)(150 - i * 30);
            DrawLine3D(pos, tp, tc);
        }
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Bubble Burst — burbuja translucida pulsante + estela de mini-burbujas ── */
static void EB_DrawBubbleBurstMesh(void) {
    float dur = fmaxf(EB_g_params.bubbleBurstDuration, 0.05f);
    float t = EB_g_loopAccum;
    Vector3 origin, target;
    EB_ProjectileEndpoints(&origin, &target);
    Color core = EB_g_bubbleBurstColorCore, mid = EB_g_bubbleBurstColorMid, outer = EB_g_bubbleBurstColorOuter;

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ALPHA);

    if (t < dur) {
        float t01 = t / dur;
        Vector3 pos = EB_V3Lerp(origin, target, t01);
        float pulse = 0.82f + 0.18f * sinf(EB_g_time * 11.0f);
        Color shell = mid; shell.a = (unsigned char)(80.0f * (1.0f - t01 * 0.4f));
        Color shine = core; shine.a = (unsigned char)(200.0f * (1.0f - t01 * 0.4f));
        DrawSphere(pos, 0.13f * pulse, shell);
        DrawSphere(pos, 0.07f * pulse, shine);
        Color rim = outer; rim.a = 60;
        DrawSphereWires(pos, 0.14f * pulse, 8, 8, rim);

        float specX = pos.x + 0.04f, specY = pos.y + 0.05f, specZ = pos.z + 0.03f;
        Color spec = core; spec.a = (unsigned char)(160.0f * pulse);
        DrawSphere((Vector3){ specX, specY, specZ }, 0.022f, spec);
    }

    int trails = EB_g_params.bubbleBurstTrailCount;
    if (trails > EB_BUBBLE_BURST_TRAIL_MAX) trails = EB_BUBBLE_BURST_TRAIL_MAX;
    if (trails < 0) trails = 0;
    for (int i = 0; i < trails; i++) {
        int seed = i * 83 + 7;
        float spawnT = EB_HashRange(seed, 0.0f, dur);
        if (t < spawnT) continue;
        float age = t - spawnT;
        float life = EB_HashRange(seed + 1, 0.25f, 0.55f);
        if (age >= life) continue;
        float u = EB_Clamp(spawnT / dur, 0.0f, 1.0f);
        Vector3 spawnPos = EB_V3Lerp(origin, target, u);
        float rise = age * age * 1.8f;
        float sway = sinf(EB_g_time * 5.0f + (float)i * 1.3f) * 0.04f;
        Vector3 p = {
            spawnPos.x + EB_HashRange(seed + 2, -0.06f, 0.06f) + sway,
            spawnPos.y + rise,
            spawnPos.z + EB_HashRange(seed + 3, -0.06f, 0.06f)
        };
        float a = EB_FadeAlpha(age, life, 0.06f, 0.4f);
        float sz = EB_HashRange(seed + 4, 0.012f, 0.030f);
        Color bc = mid; bc.a = (unsigned char)(a * 140.0f);
        Color bc2 = core; bc2.a = (unsigned char)(a * 90.0f);
        DrawSphere(p, sz, bc);
        DrawSphere(p, sz * 0.55f, bc2);
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
}

/* ── Light Arrow — streak recto y veloz + destello al impactar ────────────── */
static void EB_DrawLightArrowMesh(void) {
    float dur = fmaxf(EB_g_params.lightArrowDuration, 0.02f);
    float t = EB_g_loopAccum;
    if (t >= dur) return;
    Vector3 origin, target;
    EB_ProjectileEndpoints(&origin, &target);
    Vector3 dirN = EB_V3Normalize(EB_V3Sub(target, origin));
    Color core = EB_g_lightArrowColorCore, mid = EB_g_lightArrowColorMid, outer = EB_g_lightArrowColorOuter;

    float t01 = t / dur;
    Vector3 pos = EB_V3Lerp(origin, target, t01);
    Vector3 tail = EB_V3Sub(pos, EB_V3Scale(dirN, 0.35f));
    float alpha = EB_FadeAlpha(t, dur, 0.02f, 0.3f);

    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ADDITIVE);

    Color c1 = core; c1.a = (unsigned char)(alpha * 245.0f);
    Color c2 = mid;  c2.a = (unsigned char)(alpha * 150.0f);
    DrawLine3D(tail, pos, c1);
    DrawLine3D(tail, pos, c2);
    DrawSphere(pos, 0.045f, c1);

    if (t01 >= 0.92f) {
        float fa = (t01 - 0.92f) / 0.08f;
        Color fc = outer; fc.a = (unsigned char)((1.0f - fa) * 220.0f);
        DrawSphere(target, 0.06f * (1.0f + fa * 1.5f), fc);
    }

    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
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
    if (strcmp(s, "water_ring") == 0) return EB_SHAPE_WATER_RING;
    if (strcmp(s, "earth_burst")     == 0) return EB_SHAPE_EARTH_BURST;
    if (strcmp(s, "fire_burst")      == 0) return EB_SHAPE_FIRE_BURST;
    if (strcmp(s, "lightning_burst") == 0) return EB_SHAPE_LIGHTNING_BURST;
    if (strcmp(s, "poison_burst")    == 0) return EB_SHAPE_POISON_BURST;
    if (strcmp(s, "heal_aura")       == 0) return EB_SHAPE_HEAL_AURA;
    if (strcmp(s, "dark_slash")      == 0) return EB_SHAPE_DARK_SLASH;
    if (strcmp(s, "barrier")         == 0) return EB_SHAPE_BARRIER;
    if (strcmp(s, "fireball")        == 0) return EB_SHAPE_FIREBALL;
    if (strcmp(s, "wind_slash")      == 0) return EB_SHAPE_WIND_SLASH;
    if (strcmp(s, "rock_throw")      == 0) return EB_SHAPE_ROCK_THROW;
    if (strcmp(s, "lightning_bolt")  == 0) return EB_SHAPE_LIGHTNING_BOLT;
    if (strcmp(s, "water_jet")       == 0) return EB_SHAPE_WATER_JET;
    if (strcmp(s, "ice_shard")       == 0) return EB_SHAPE_ICE_SHARD;
    if (strcmp(s, "poison_orb")      == 0) return EB_SHAPE_POISON_ORB;
    if (strcmp(s, "dark_orb")        == 0) return EB_SHAPE_DARK_ORB;
    if (strcmp(s, "light_arrow")     == 0) return EB_SHAPE_LIGHT_ARROW;
    if (strcmp(s, "bubble_burst")    == 0) return EB_SHAPE_BUBBLE_BURST;
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

    EB_g_params.waterRingCount        = (int)JsonAsNumber(JsonObjectGet(paramsObj, "waterRingCount"), EB_g_params.waterRingCount);
    EB_g_params.waterRingDuration     = (float)JsonAsNumber(JsonObjectGet(paramsObj, "waterRingDuration"), EB_g_params.waterRingDuration);
    EB_g_params.waterRingStagger      = (float)JsonAsNumber(JsonObjectGet(paramsObj, "waterRingStagger"), EB_g_params.waterRingStagger);
    EB_g_params.waterRingCrestHeight  = (float)JsonAsNumber(JsonObjectGet(paramsObj, "waterRingCrestHeight"), EB_g_params.waterRingCrestHeight);
    EB_g_params.waterRingThickness    = (float)JsonAsNumber(JsonObjectGet(paramsObj, "waterRingThickness"), EB_g_params.waterRingThickness);
    if (EB_g_params.waterRingCount > EB_WATER_RING_MAX) EB_g_params.waterRingCount = EB_WATER_RING_MAX;

    EB_g_params.earthBurstRockCount  = (int)JsonAsNumber(JsonObjectGet(paramsObj, "earthBurstRockCount"), EB_g_params.earthBurstRockCount);
    EB_g_params.earthBurstCrackCount = (int)JsonAsNumber(JsonObjectGet(paramsObj, "earthBurstCrackCount"), EB_g_params.earthBurstCrackCount);
    EB_g_params.earthBurstDuration   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "earthBurstDuration"), EB_g_params.earthBurstDuration);
    if (EB_g_params.earthBurstRockCount > EB_EARTH_ROCK_MAX) EB_g_params.earthBurstRockCount = EB_EARTH_ROCK_MAX;
    if (EB_g_params.earthBurstCrackCount > EB_EARTH_CRACK_MAX) EB_g_params.earthBurstCrackCount = EB_EARTH_CRACK_MAX;

    EB_g_params.fireBurstEmberCount  = (int)JsonAsNumber(JsonObjectGet(paramsObj, "fireBurstEmberCount"), EB_g_params.fireBurstEmberCount);
    EB_g_params.fireBurstDuration    = (float)JsonAsNumber(JsonObjectGet(paramsObj, "fireBurstDuration"), EB_g_params.fireBurstDuration);
    EB_g_params.fireBurstColumnHeight= (float)JsonAsNumber(JsonObjectGet(paramsObj, "fireBurstColumnHeight"), EB_g_params.fireBurstColumnHeight);
    if (EB_g_params.fireBurstEmberCount > EB_FIRE_EMBER_MAX) EB_g_params.fireBurstEmberCount = EB_FIRE_EMBER_MAX;

    EB_g_params.lightningBoltCount = (int)JsonAsNumber(JsonObjectGet(paramsObj, "lightningBoltCount"), EB_g_params.lightningBoltCount);
    EB_g_params.lightningBoltLife  = (float)JsonAsNumber(JsonObjectGet(paramsObj, "lightningBoltLife"), EB_g_params.lightningBoltLife);
    EB_g_params.lightningJitter    = (float)JsonAsNumber(JsonObjectGet(paramsObj, "lightningJitter"), EB_g_params.lightningJitter);
    EB_g_params.lightningHeight    = (float)JsonAsNumber(JsonObjectGet(paramsObj, "lightningHeight"), EB_g_params.lightningHeight);
    if (EB_g_params.lightningBoltCount > EB_LIGHTNING_BOLT_MAX) EB_g_params.lightningBoltCount = EB_LIGHTNING_BOLT_MAX;

    EB_g_params.poisonRingCount   = (int)JsonAsNumber(JsonObjectGet(paramsObj, "poisonRingCount"), EB_g_params.poisonRingCount);
    EB_g_params.poisonBubbleCount = (int)JsonAsNumber(JsonObjectGet(paramsObj, "poisonBubbleCount"), EB_g_params.poisonBubbleCount);
    EB_g_params.poisonDuration    = (float)JsonAsNumber(JsonObjectGet(paramsObj, "poisonDuration"), EB_g_params.poisonDuration);
    if (EB_g_params.poisonRingCount > EB_POISON_RING_MAX) EB_g_params.poisonRingCount = EB_POISON_RING_MAX;
    if (EB_g_params.poisonBubbleCount > EB_POISON_BUBBLE_MAX) EB_g_params.poisonBubbleCount = EB_POISON_BUBBLE_MAX;

    EB_g_params.healCrossCount   = (int)JsonAsNumber(JsonObjectGet(paramsObj, "healCrossCount"), EB_g_params.healCrossCount);
    EB_g_params.healCycleDuration= (float)JsonAsNumber(JsonObjectGet(paramsObj, "healCycleDuration"), EB_g_params.healCycleDuration);
    EB_g_params.healPillarHeight = (float)JsonAsNumber(JsonObjectGet(paramsObj, "healPillarHeight"), EB_g_params.healPillarHeight);
    if (EB_g_params.healCrossCount > EB_HEAL_CROSS_MAX) EB_g_params.healCrossCount = EB_HEAL_CROSS_MAX;

    EB_g_params.darkTentacleCount = (int)JsonAsNumber(JsonObjectGet(paramsObj, "darkTentacleCount"), EB_g_params.darkTentacleCount);
    EB_g_params.darkOrbCount      = (int)JsonAsNumber(JsonObjectGet(paramsObj, "darkOrbCount"), EB_g_params.darkOrbCount);
    EB_g_params.darkDuration      = (float)JsonAsNumber(JsonObjectGet(paramsObj, "darkDuration"), EB_g_params.darkDuration);
    if (EB_g_params.darkTentacleCount > EB_DARK_TENTACLE_MAX) EB_g_params.darkTentacleCount = EB_DARK_TENTACLE_MAX;
    if (EB_g_params.darkOrbCount > EB_DARK_ORB_MAX) EB_g_params.darkOrbCount = EB_DARK_ORB_MAX;

    EB_g_params.barrierMoteCount  = (int)JsonAsNumber(JsonObjectGet(paramsObj, "barrierMoteCount"), EB_g_params.barrierMoteCount);
    EB_g_params.barrierPulseSpeed = (float)JsonAsNumber(JsonObjectGet(paramsObj, "barrierPulseSpeed"), EB_g_params.barrierPulseSpeed);
    if (EB_g_params.barrierMoteCount > EB_BARRIER_MOTE_MAX) EB_g_params.barrierMoteCount = EB_BARRIER_MOTE_MAX;

    EB_g_params.fireballDuration  = (float)JsonAsNumber(JsonObjectGet(paramsObj, "fireballDuration"), EB_g_params.fireballDuration);
    EB_g_params.fireballEmberCount= (int)JsonAsNumber(JsonObjectGet(paramsObj, "fireballEmberCount"), EB_g_params.fireballEmberCount);
    if (EB_g_params.fireballEmberCount > EB_FIREBALL_EMBER_MAX) EB_g_params.fireballEmberCount = EB_FIREBALL_EMBER_MAX;

    EB_g_params.windSlashDuration   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "windSlashDuration"), EB_g_params.windSlashDuration);
    EB_g_params.windSlashTrailCount = (int)JsonAsNumber(JsonObjectGet(paramsObj, "windSlashTrailCount"), EB_g_params.windSlashTrailCount);
    if (EB_g_params.windSlashTrailCount > EB_WIND_SLASH_TRAIL_MAX) EB_g_params.windSlashTrailCount = EB_WIND_SLASH_TRAIL_MAX;

    EB_g_params.rockThrowDuration  = (float)JsonAsNumber(JsonObjectGet(paramsObj, "rockThrowDuration"), EB_g_params.rockThrowDuration);
    EB_g_params.rockThrowDustCount = (int)JsonAsNumber(JsonObjectGet(paramsObj, "rockThrowDustCount"), EB_g_params.rockThrowDustCount);
    if (EB_g_params.rockThrowDustCount > EB_ROCK_DUST_MAX) EB_g_params.rockThrowDustCount = EB_ROCK_DUST_MAX;

    EB_g_params.lightningBoltDuration = (float)JsonAsNumber(JsonObjectGet(paramsObj, "lightningBoltDuration"), EB_g_params.lightningBoltDuration);
    EB_g_params.lightningBoltBranches = (int)JsonAsNumber(JsonObjectGet(paramsObj, "lightningBoltBranches"), EB_g_params.lightningBoltBranches);
    if (EB_g_params.lightningBoltBranches > EB_LIGHTNING_PROJ_BRANCH_MAX) EB_g_params.lightningBoltBranches = EB_LIGHTNING_PROJ_BRANCH_MAX;

    EB_g_params.waterJetDuration  = (float)JsonAsNumber(JsonObjectGet(paramsObj, "waterJetDuration"), EB_g_params.waterJetDuration);
    EB_g_params.waterJetDropCount = (int)JsonAsNumber(JsonObjectGet(paramsObj, "waterJetDropCount"), EB_g_params.waterJetDropCount);
    if (EB_g_params.waterJetDropCount > EB_WATER_JET_DROP_MAX) EB_g_params.waterJetDropCount = EB_WATER_JET_DROP_MAX;

    EB_g_params.iceShardDuration   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "iceShardDuration"), EB_g_params.iceShardDuration);
    EB_g_params.iceShardSparkCount = (int)JsonAsNumber(JsonObjectGet(paramsObj, "iceShardSparkCount"), EB_g_params.iceShardSparkCount);
    if (EB_g_params.iceShardSparkCount > EB_ICE_SHARD_SPARK_MAX) EB_g_params.iceShardSparkCount = EB_ICE_SHARD_SPARK_MAX;

    EB_g_params.poisonOrbDuration   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "poisonOrbDuration"), EB_g_params.poisonOrbDuration);
    EB_g_params.poisonOrbSporeCount = (int)JsonAsNumber(JsonObjectGet(paramsObj, "poisonOrbSporeCount"), EB_g_params.poisonOrbSporeCount);
    if (EB_g_params.poisonOrbSporeCount > EB_POISON_ORB_SPORE_MAX) EB_g_params.poisonOrbSporeCount = EB_POISON_ORB_SPORE_MAX;

    EB_g_params.darkOrbDuration      = (float)JsonAsNumber(JsonObjectGet(paramsObj, "darkOrbDuration"), EB_g_params.darkOrbDuration);
    EB_g_params.darkOrbTentacleCount = (int)JsonAsNumber(JsonObjectGet(paramsObj, "darkOrbTentacleCount"), EB_g_params.darkOrbTentacleCount);
    if (EB_g_params.darkOrbTentacleCount > EB_DARK_ORB_TENTACLE_MAX) EB_g_params.darkOrbTentacleCount = EB_DARK_ORB_TENTACLE_MAX;

    EB_g_params.lightArrowDuration = (float)JsonAsNumber(JsonObjectGet(paramsObj, "lightArrowDuration"), EB_g_params.lightArrowDuration);

    EB_g_params.bubbleBurstDuration   = (float)JsonAsNumber(JsonObjectGet(paramsObj, "bubbleBurstDuration"), EB_g_params.bubbleBurstDuration);
    EB_g_params.bubbleBurstTrailCount = (int)JsonAsNumber(JsonObjectGet(paramsObj, "bubbleBurstTrailCount"), EB_g_params.bubbleBurstTrailCount);
    if (EB_g_params.bubbleBurstTrailCount > EB_BUBBLE_BURST_TRAIL_MAX) EB_g_params.bubbleBurstTrailCount = EB_BUBBLE_BURST_TRAIL_MAX;

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
        if (p->isRain && p->pos.y <= EB_RAIN_GROUND_Y) {
            EB_SpawnRainSplash((Vector3){ p->pos.x, EB_RAIN_GROUND_Y, p->pos.z }, p->colorMid);
            p->active = false;
            continue;
        }
        if (p->age >= p->life) { p->active = false; continue; }
        p->vel.y -= EB_g_params.gravity * dt;
        p->vel    = EB_V3Scale(p->vel, 1.0f - EB_Clamp(EB_g_params.drag * dt, 0.0f, 0.9f));
        p->pos    = EB_V3Add(p->pos, EB_V3Scale(p->vel, dt));
    }

    for (int i = 0; i < EB_RAIN_SPLASH_MAX; i++) {
        EB_RainSplash *sl = &EB_g_rainSplashes[i];
        if (!sl->active) continue;
        sl->age += dt;
        if (sl->age >= sl->life) { sl->active = false; continue; }
        float t = sl->age / sl->life;
        sl->radius = sl->maxRadius * (1.0f - (1.0f - t) * (1.0f - t));
    }
}

static void EB_DrawRainSplashes(void) {
    BeginMode3D(EB_g_camera);
    rlDisableDepthMask();
    rlSetBlendMode(BLEND_ALPHA);
    for (int i = 0; i < EB_RAIN_SPLASH_MAX; i++) {
        const EB_RainSplash *sl = &EB_g_rainSplashes[i];
        if (!sl->active) continue;
        float fa = 1.0f - sl->age / sl->life;
        Color c = sl->color; c.a = (unsigned char)(180.0f * fa);
        DrawCircle3D(sl->pos, sl->radius, (Vector3){1, 0, 0}, 90.0f, c);
    }
    rlSetBlendMode(BLEND_ALPHA);
    rlEnableDepthMask();
    EndMode3D();
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
    if (EB_g_waterRingActive)  EB_DrawWaterRingMesh();
    if (EB_g_earthBurstActive) EB_DrawEarthBurstMesh();
    if (EB_g_fireBurstActive)  EB_DrawFireBurstMesh();
    if (EB_g_lightningActive)  EB_DrawLightningBurstMesh();
    if (EB_g_poisonActive)     EB_DrawPoisonBurstMesh();
    if (EB_g_healActive)       EB_DrawHealAuraMesh();
    if (EB_g_darkSlashActive)  EB_DrawDarkSlashMesh();
    if (EB_g_barrierActive)    EB_DrawBarrierMesh();

    if (EB_g_fireballActive)      EB_DrawFireballMesh();
    if (EB_g_windSlashActive)     EB_DrawWindSlashMesh();
    if (EB_g_rockThrowActive)     EB_DrawRockThrowMesh();
    if (EB_g_lightningBoltActive) EB_DrawLightningBoltMesh();
    if (EB_g_waterJetActive)      EB_DrawWaterJetMesh();
    if (EB_g_iceShardActive)      EB_DrawIceShardMesh();
    if (EB_g_poisonOrbActive)     EB_DrawPoisonOrbMesh();
    if (EB_g_darkOrbActive)       EB_DrawDarkOrbMesh();
    if (EB_g_lightArrowActive)    EB_DrawLightArrowMesh();
    if (EB_g_bubbleBurstActive)   EB_DrawBubbleBurstMesh();

    if (EB_g_params.shape == EB_SHAPE_RAIN) EB_DrawRainSplashes();

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

/* Lanza el burst ya mismo (base + combo layers habilitadas), en vez de
   esperar al proximo loopInterval. Pensado para dispararse con un click,
   una tecla o cualquier evento de gameplay (golpe, hechizo, etc). */
void EffectAtelierEffect_Trigger(void) {
    EB_g_loopAccum = 0.0f;
    EB_SpawnBurst();
}

#endif /* EFFECT_ATELIER_H */
