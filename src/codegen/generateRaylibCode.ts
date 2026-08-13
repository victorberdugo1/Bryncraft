import type { EffectId, EffectParams } from "@/types/effects";
import { CODEGEN_MODULES, type ExtraAsset } from "@/effects";

export type { ExtraAsset } from "@/effects";

// Todo el contenido real vive en src/effects/<id>.ts (un archivo por
// efecto). Este archivo es solo la fachada que usa CodePanel.tsx — no hay
// nada que registrar acá; agregar un efecto nuevo se hace en
// src/effects/index.ts, no en este archivo.

/** native/effects/<efecto>/<efecto>_effect.h real, con su bloque
 * static X_Params X_g_params = { ... } sustituido por el estado actual del
 * Inspector. Todo lo demás es el archivo tal cual está en disco — no hay
 * una segunda copia mantenida a mano en el frontend. */
export function generateEffectHeader(effect: EffectId, params: EffectParams): string {
  const m = CODEGEN_MODULES[effect];
  return m.headerRaw.replace(m.paramsRegex, m.buildParamsBlock(params));
}

/** Nombre real del main mínimo en native/effects/ para este efecto
 * (main000.c / main001.c / main002.c / main003.c / ...). */
export function getMainFilename(effect: EffectId): string {
  return CODEGEN_MODULES[effect].mainFilename;
}

/** "Main" tab: el main00N.c real de native/effects/, tal cual está en
 * disco — no una copia embebida. */
export function generateMainTab(effect: EffectId): string {
  return CODEGEN_MODULES[effect].mainRaw;
}

/** Archivos extra de cada efecto — lo que sea que ese efecto necesite además
 * de su header/main, y que no es código C generado por el Inspector: fuente,
 * scripts de build, modelos... Array vacío si el efecto no tiene ninguno
 * (ej. particles). Algunos efectos (touchdesigner) devuelven un set distinto
 * según los params actuales — por eso siempre se le pasan acá. */
export function getExtras(effect: EffectId, params: EffectParams): ExtraAsset[] {
  const extras = CODEGEN_MODULES[effect].extras;
  return typeof extras === "function" ? extras(params) : extras;
}

/** README tab: el README.md real de native/effects/<efecto>/, tal cual está
 * en disco — no una plantilla generada en TS. Se muestra en un <pre> crudo,
 * sin renderizar Markdown. */
export function generateReadme(effect: EffectId): string {
  return CODEGEN_MODULES[effect].readmeRaw;
}
