// Registro central de efectos — el ÚNICO lugar del frontend que hay que
// tocar para dar de alta un efecto nuevo, además de crear su propio
// archivo en esta carpeta (copiá ascii.ts como plantilla).
//
// Para agregar un efecto:
//   1. Creá src/effects/<id>.ts exportando `<ID>_MODULE: EffectModule<"id">`
//   2. Importalo acá abajo y agregalo a EFFECT_MODULES
// Eso es todo. EffectId, EFFECT_DEFINITIONS, CODEGEN_MODULES, THUMBNAILS,
// y defaultParamsFor() salen solos de esa lista.

import type { EffectDefinition, EffectCodegenModule, EffectParams, ThumbnailDrawFn } from "./shared";
import { ASCII_MODULE } from "./ascii";
import { PARTICLES_MODULE } from "./particles";
import { CRT_MODULE } from "./crt";
import { OPENCV_MODULE } from "./opencv";
import { TOUCHDESIGNER_MODULE } from "./touchdesigner";

export * from "./shared";

export const EFFECT_MODULES = [ASCII_MODULE, PARTICLES_MODULE, CRT_MODULE, OPENCV_MODULE, TOUCHDESIGNER_MODULE] as const;

export type EffectId = (typeof EFFECT_MODULES)[number]["definition"]["id"];

// Sin anotar el tipo explícito (sin `: EffectDefinition[]`) para que TS
// infiera la unión de literales de "id" en vez de ensancharla a `string` —
// así ORDER en LeftSidebar.tsx y paramsByEffect en useAppStore.ts siguen
// tipados como EffectId[], no string[].
export const EFFECT_LIST = EFFECT_MODULES.map((m) => m.definition);
export const EFFECT_IDS: EffectId[] = EFFECT_MODULES.map((m) => m.definition.id);

export const EFFECT_DEFINITIONS: Record<EffectId, EffectDefinition> = Object.fromEntries(
  EFFECT_MODULES.map((m) => [m.definition.id, m.definition])
) as Record<EffectId, EffectDefinition>;

export const CODEGEN_MODULES: Record<EffectId, EffectCodegenModule> = Object.fromEntries(
  EFFECT_MODULES.map((m) => [m.definition.id, m.codegen])
) as Record<EffectId, EffectCodegenModule>;

export const THUMBNAILS: Record<EffectId, ThumbnailDrawFn> = Object.fromEntries(
  EFFECT_MODULES.map((m) => [m.definition.id, m.thumbnail])
) as Record<EffectId, ThumbnailDrawFn>;

export function defaultParamsFor(effect: EffectId): EffectParams {
  const out: EffectParams = {};
  for (const p of EFFECT_DEFINITIONS[effect].params) out[p.key] = p.default;
  return out;
}

export interface RenderMessage {
  effect: EffectId;
  params: EffectParams;
}

export interface ViewportOverlayStats {
  fps: number;
  resolutionW: number;
  resolutionH: number;
  frame: number;
  effect: EffectId;
  gpuFrameTimeMs: number;
}
