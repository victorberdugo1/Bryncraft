#ifndef EFFECT_COMMON_H
#define EFFECT_COMMON_H

#include <string.h>
#include "raylib.h"
#include "../json_mini.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * EFFECT_LIST — la ÚNICA lista de efectos que existe. Todo lo demás en este
 * header (el enum, el string<->enum, y el dispatch en native/main.c) se
 * genera a partir de esta lista con macros. Agregar un efecto nuevo es
 * agregar UNA línea acá (más el #include del header en native/main.c, que no
 * se puede automatizar porque cada efecto vive en su propia carpeta).
 *
 * X(ENUM, id, FnPrefix, needsClear)
 *   ENUM       -> sufijo del enum: EffectKind será EFFECT_##ENUM
 *   id         -> string que usa React en el JSON ("ascii", "crt", ...) y
 *                 que además es el nombre de la carpeta en native/effects/
 *   FnPrefix   -> prefijo de las funciones C del efecto, ej. Crt ->
 *                 CrtEffect_Init/_SetParams/_Update/_Draw/_Unload (las 5
 *                 tienen que existir con ESE nombre exacto — es el contrato)
 *   needsClear -> si native/main.c debe hacer ClearBackground(BLANK) antes
 *                 de dibujar este efecto (true para efectos que pueden dejar
 *                 zonas transparentes — shaders con alpha, overlays CV—,
 *                 false para los que siempre cubren el frame entero)
 * ========================================================================== */
#define EFFECT_LIST(X) \
    X(ASCII,         ascii,         Ascii,         false) \
    X(PARTICLES,     particles,     Particles,     false) \
    X(CRT,           crt,           Crt,           true)  \
    X(OPENCV,        opencv,        Opencv,        true)  \
    X(TOUCHDESIGNER, touchdesigner, Touchdesigner, true)

typedef enum {
#define X(ENUM, id, FnPrefix, needsClear) EFFECT_##ENUM,
    EFFECT_LIST(X)
#undef X
    EFFECT_COUNT
} EffectKind;

static inline EffectKind EffectKindFromString(const char *name) {
    if (!name) return EFFECT_ASCII;
#define X(ENUM, id, FnPrefix, needsClear) if (strcmp(name, #id) == 0) return EFFECT_##ENUM;
    EFFECT_LIST(X)
#undef X
    return EFFECT_ASCII;
}

static inline const char *EffectKindToString(EffectKind kind) {
    switch (kind) {
#define X(ENUM, id, FnPrefix, needsClear) case EFFECT_##ENUM: return #id;
        EFFECT_LIST(X)
#undef X
        default: return "ascii";
    }
}

// Todo efecto (ascii_effect.h, particles_effect.h, crt_effect.h,
// opencv_effect.h, y cualquiera nuevo) implementa estas 5 funciones con este
// nombre exacto (sustituyendo <Name> por su FnPrefix en EFFECT_LIST arriba)
// para que native/main.c pueda generar el dispatch completo con macros, sin
// un switch escrito a mano por cada efecto nuevo:
//
//   void <Name>Effect_Init(void);        // una vez al arrancar (puede ser no-op)
//   void <Name>Effect_SetParams(const JsonValue *paramsObj);
//   void <Name>Effect_Update(float dt);
//   void <Name>Effect_Draw(RenderTexture2D scene, int screenW, int screenH);
//   void <Name>Effect_Unload(void);      // una vez al cerrar (puede ser no-op)

#ifdef __cplusplus
}
#endif

#endif // EFFECT_COMMON_H
