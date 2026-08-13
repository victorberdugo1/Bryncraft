import {
  fmtInt,
  fmtFloat,
  hexToRgbComment,
  type EffectDefinition,
  type EffectCodegenModule,
  type EffectParams,
  type EffectModule,
  type ThumbnailDrawFn,
} from "./shared";

import headerRaw from "../../native/effects/particles/particles_effect.h?raw";
import mainRaw from "../../native/effects/particles/main001.c?raw";
import readmeRaw from "../../native/effects/particles/README.md?raw";
import raylibHeaderUrl from "../../native/effects/raylib.h?url";
import libraylibWinUrl from "../../native/effects/win/libraylib.a?url";
import libraylibLnxUrl from "../../native/effects/lnx/libraylib.a?url";

// --- 1. Definición de parámetros (Inspector) --------------------------------

const definition: EffectDefinition<"particles"> = {
  id: "particles",
  name: "Particle System",
  description: "Multi-mode particle emitter: fountain (classic), rain (falling drops), or embers (rising glows) — all reactive to video content.",
  params: [
    { key: "mode", label: "Mode", type: "select", default: "rain", options: ["fountain", "rain", "embers"], group: "Type" },

    { key: "count", label: "Particle Count", type: "int", default: 2000, min: 10, max: 20000, step: 10, group: "Emission" },
    { key: "spawnRate", label: "Spawn Rate", type: "float", default: 120, min: 0, max: 2000, step: 1, group: "Emission" },
    { key: "spread", label: "Spread (deg)", type: "float", default: 45, min: 0, max: 360, step: 1, group: "Emission" },
    { key: "spawnX", label: "Spawn X (fountain only)", type: "float", default: 0.5, min: 0, max: 1, step: 0.01, group: "Emission", modes: ["fountain"] },
    { key: "spawnY", label: "Spawn Y (fountain only)", type: "float", default: 0.8, min: 0, max: 1, step: 0.01, group: "Emission", modes: ["fountain"] },

    { key: "gravity", label: "Gravity", type: "float", default: 9.8, min: -50, max: 50, step: 0.1, group: "Physics" },
    { key: "lifetime", label: "Lifetime", type: "float", default: 2.5, min: 0.1, max: 20, step: 0.1, group: "Physics" },
    { key: "wind", label: "Wind X", type: "float", default: 0, min: -50, max: 50, step: 0.5, group: "Physics", modes: ["rain", "embers"] },

    { key: "size", label: "Size", type: "float", default: 4, min: 0.5, max: 40, step: 0.1, group: "Appearance" },
    { key: "sizeFalloff", label: "Size Falloff", type: "float", default: 0.6, min: 0, max: 1, step: 0.01, group: "Appearance" },
    { key: "color", label: "Color", type: "color", default: "#44D4FF", group: "Appearance" },

    { key: "reactive", label: "React to Video", type: "bool", default: true, group: "Video Interaction" },
    { key: "reactiveStrength", label: "Interaction Strength", type: "float", default: 0.6, min: 0, max: 1, step: 0.01, group: "Video Interaction" },
    { key: "flowStrength", label: "Flow Field Strength", type: "float", default: 0.8, min: 0, max: 2, step: 0.01, group: "Video Interaction" },
  ],
};

// --- 2. Codegen ---------------------------------------------------------------

const PARTICLE_MODE_ENUM: Record<string, string> = {
  fountain: "PART_MODE_FOUNTAIN",
  rain: "PART_MODE_RAIN",
  embers: "PART_MODE_EMBERS",
};

function buildParamsBlock(params: EffectParams): string {
  return `static PART_ParticleParams PART_g_params = {
    .mode = ${PARTICLE_MODE_ENUM[String(params.mode)] ?? "PART_MODE_RAIN"},
    .count = ${fmtInt(params.count)},
    .spawnRate = ${fmtFloat(params.spawnRate)},
    .gravity = ${fmtFloat(params.gravity)},
    .lifetime = ${fmtFloat(params.lifetime)},
    .size = ${fmtFloat(params.size)},
    .sizeFalloff = ${fmtFloat(params.sizeFalloff)},
    .color = (Color){ ${hexToRgbComment(String(params.color))}, 255 },
    .spreadDeg = ${fmtFloat(params.spread)},
    .spawnX = ${fmtFloat(params.spawnX)},
    .spawnY = ${fmtFloat(params.spawnY)},
    .windX = ${fmtFloat(params.wind)},
    .reactive = ${params.reactive ? "1" : "0"},
    .reactiveStrength = ${fmtFloat(params.reactiveStrength)},
    .flowStrength = ${fmtFloat(params.flowStrength)},
};
`;
}

const codegen: EffectCodegenModule = {
  headerRaw,
  mainRaw,
  mainFilename: "main001.c",
  readmeRaw,
  paramsRegex: /static PART_ParticleParams PART_g_params = \{[\s\S]*?\};\r?\n/,
  buildParamsBlock,
  extras: [
    { filename: "raylib.h", label: "raylib.h (compartido por las 4 demos standalone)", kind: "binary-url", url: raylibHeaderUrl },
    { filename: "libraylib.a", label: "libraylib.a — Windows (MinGW)", kind: "binary-url", url: libraylibWinUrl },
    { filename: "libraylib.a", label: "libraylib.a — Linux", kind: "binary-url", url: libraylibLnxUrl },
  ],
};

// --- 3. Thumbnail --------------------------------------------------------------

const thumbnail: ThumbnailDrawFn = (ctx, w, h, t) => {
  for (let i = 0; i < 40; i++) {
    const a = i * 0.4 + t;
    const r = (i % 10) * (w / 22);
    const x = w / 2 + Math.cos(a) * r * 0.5;
    const y = h - ((t * 20 + i * 6) % h);
    ctx.fillStyle = `rgba(68,212,255,${1 - y / h})`;
    ctx.beginPath();
    ctx.arc(x, y, 1.6, 0, Math.PI * 2);
    ctx.fill();
  }
};

// --- Paquete final -----------------------------------------------------------

export const PARTICLES_MODULE: EffectModule<"particles"> = { definition, codegen, thumbnail };
