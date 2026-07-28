import type { EffectId, EffectParams } from "@/types/effects";

// Los .h/.c reales, cargados tal cual están en native/effects/ (Vite ?raw).
// Nada se copia a mano aquí: si tocas esos archivos, esto se entera solo.
import asciiHeaderRaw from "../../native/effects/ascii_effect.h?raw";
import crtHeaderRaw from "../../native/effects/crt_effect.h?raw";
import particlesHeaderRaw from "../../native/effects/particles_effect.h?raw";

import main000Raw from "../../native/effects/main000.c?raw";
import main001Raw from "../../native/effects/main001.c?raw";
import main002Raw from "../../native/effects/main002.c?raw";

const MAIN_BY_EFFECT: Record<EffectId, { filename: string; raw: string }> = {
  ascii: { filename: "main000.c", raw: main000Raw },
  particles: { filename: "main001.c", raw: main001Raw },
  crt: { filename: "main002.c", raw: main002Raw },
};

function fmtNum(v: unknown) {
  return typeof v === "number" ? (Number.isInteger(v) ? v.toFixed(0) : v.toFixed(3)) : v;
}

function hexToRgbComment(hex: string) {
  const h = hex.replace("#", "");
  const r = parseInt(h.slice(0, 2), 16);
  const g = parseInt(h.slice(2, 4), 16);
  const b = parseInt(h.slice(4, 6), 16);
  return `${r}, ${g}, ${b}`;
}

function hexToRgbaComment(hex: string) {
  const h = hex.replace("#", "");
  const a = h.length >= 8 ? parseInt(h.slice(6, 8), 16) : 255;
  return `${hexToRgbComment(hex)}, ${a}`;
}

const PARTICLE_MODE_ENUM: Record<string, string> = {
  fountain: "PART_MODE_FOUNTAIN",
  rain: "PART_MODE_RAIN",
  embers: "PART_MODE_EMBERS",
};

/* ============================================================================
 * Cada *_g_params se reemplaza en el texto real del .h por una regex que
 * localiza únicamente ese bloque (static ... = { ... };). Todo lo demás del
 * archivo pasa intacto, tal cual está en disco.
 * ========================================================================== */

const ASCII_PARAMS_RE = /static ASCII_Params ASCII_g_params = \{[\s\S]*?\};\r?\n/;
const CRT_PARAMS_RE = /static CRT_Params CRT_g_params = \{[\s\S]*?\};\r?\n/;
const PART_PARAMS_RE = /static PART_ParticleParams PART_g_params = \{[\s\S]*?\};\r?\n/;

function buildAsciiParamsBlock(params: EffectParams): string {
  return `static ASCII_Params ASCII_g_params = {
    .ramp = "${params.characters}",
    .fontSize = ${fmtNum(params.fontSize)},
    .brightness = ${fmtNum(params.brightness)}f,
    .contrast = ${fmtNum(params.contrast)}f,
    .gamma = ${fmtNum(params.gamma)}f,
    .foreground = (Color){ ${hexToRgbComment(String(params.foreground))}, 255 },
    .background = (Color){ ${hexToRgbaComment(String(params.background))} },
    .invert = ${params.invert ? "true" : "false"},
};
`;
}

function buildCrtParamsBlock(params: EffectParams): string {
  return `static CRT_Params CRT_g_params = {
    .scanlineIntensity   = ${fmtNum(params.scanlineIntensity)}f,
    .scanlineCount       = ${fmtNum(params.scanlineCount)}f,
    .scanlineSpeed       = ${fmtNum(params.scanlineSpeed)}f,
    .curvature           = ${fmtNum(params.curvature)}f,
    .vignette            = ${fmtNum(params.vignette)}f,
    .noise               = ${fmtNum(params.noise)}f,
    .chromaticAberration = ${fmtNum(params.chromaticAberration)}f,
    .flicker             = ${fmtNum(params.flicker)}f,
};
`;
}

function buildParticlesParamsBlock(params: EffectParams): string {
  return `static PART_ParticleParams PART_g_params = {
    .mode = ${PARTICLE_MODE_ENUM[String(params.mode)] ?? "PART_MODE_RAIN"},
    .count = ${fmtNum(params.count)},
    .spawnRate = ${fmtNum(params.spawnRate)}f,
    .gravity = ${fmtNum(params.gravity)}f,
    .lifetime = ${fmtNum(params.lifetime)}f,
    .size = ${fmtNum(params.size)}f,
    .sizeFalloff = ${fmtNum(params.sizeFalloff)}f,
    .color = (Color){ ${hexToRgbComment(String(params.color))}, 255 },
    .spreadDeg = ${fmtNum(params.spread)}f,
    .spawnX = ${fmtNum(params.spawnX)}f,
    .spawnY = ${fmtNum(params.spawnY)}f,
    .windX = ${fmtNum(params.wind)}f,
    .reactive = ${params.reactive ? "1" : "0"},
    .reactiveStrength = ${fmtNum(params.reactiveStrength)}f,
    .flowStrength = ${fmtNum(params.flowStrength)}f,
};
`;
}

/** native/effects/${effect}_effect.h real, con ASCII_g_params / CRT_g_params /
 * PART_g_params sustituido por el estado actual del Inspector. Todo lo demás
 * es el archivo tal cual está en disco — no hay una segunda copia mantenida
 * a mano en el frontend. */
export function generateEffectHeader(effect: EffectId, params: EffectParams): string {
  switch (effect) {
    case "ascii":
      return asciiHeaderRaw.replace(ASCII_PARAMS_RE, buildAsciiParamsBlock(params));
    case "particles":
      return particlesHeaderRaw.replace(PART_PARAMS_RE, buildParticlesParamsBlock(params));
    case "crt":
      return crtHeaderRaw.replace(CRT_PARAMS_RE, buildCrtParamsBlock(params));
  }
}

/** "Main" tab: el main000/001/002.c real de native/effects/, tal cual está
 * en disco — no una copia embebida. */
export function generateMainTab(effect: EffectId): string {
  return MAIN_BY_EFFECT[effect].raw;
}

/** Extrae y desescapa un `static const char *NOMBRE = "..." "..." ...;` de C,
 * tal como está en el .h real — así el shader que se muestra es siempre
 * exactamente el que se compila, nunca una copia aparte. */
function extractCString(headerRaw: string, varName: string): string {
  const declRe = new RegExp(`static const char \\*${varName} =([\\s\\S]*?);\\r?\\n`);
  const m = headerRaw.match(declRe);
  if (!m) return "";
  const body = m[1];
  const strRe = /"(?:[^"\\]|\\.)*"/g;
  const parts = body.match(strRe) ?? [];
  return parts.map((p) => JSON.parse(p)).join("");
}

export function generateShaderSnippet(effect: EffectId): string {
  if (effect !== "crt") {
    return `// ${effect} runs on the CPU/immediate-mode raylib API — no fragment shader needed.
// See native/effects/${effect}_effect.h`;
  }
  const glsl = extractCString(crtHeaderRaw, "CRT_FS_SOURCE");
  return `// Extraído de CRT_FS_SOURCE en native/effects/crt_effect.h (GLSL 100, WebGL1/ES)
// Esto es exactamente la cadena que se compila en CrtEffect_Init(), no una copia aparte.

${glsl}`;
}

/** README tab: texto plano, sin sintaxis Markdown — se muestra en un <pre>
 * crudo, no en un visor de markdown. */
export function generateReadme(effect: EffectId): string {
  const titles: Record<EffectId, string> = {
    ascii: "ASCII Renderer",
    particles: "Particle System",
    crt: "CRT",
  };

  const title = titles[effect];
  const bar = "=".repeat(Math.max(60, title.length + 8));

  return `${bar}
  ${title}
${bar}

Este efecto corre entero dentro del canvas Raylib/WebAssembly. React nunca
dibuja en el canvas — solo envía actualizaciones de parámetros en JSON por
el bridge definido en src/lib/wasmBridge.ts, que llama a js_set_effect_json()
en native/main.c.

  CONTRATO DEL MENSAJE
  --------------------
  { "effect": "${effect}", "params": { ... } }

  DÓNDE VIVE LA LÓGICA
  --------------------
  native/main.c
      Reparte el mensaje decodificado al módulo del efecto activo.

  native/effects/${effect}_effect.h
      Simulación + dibujado de este efecto (ver la pestaña "Code" para
      el header exacto con los valores actuales del Inspector).

  native/effects/${MAIN_BY_EFFECT[effect].filename}
      Ejemplo mínimo standalone — el programa raylib más pequeño que
      hace andar este efecto por sí solo (pestaña "Main").

  js_get_stats_json()
      Reporta FPS / frame / GPU time de vuelta a React.

  RECOMPILAR
  ----------
  cd native
  make raylib   (solo la primera vez — compila libraylib.a para PLATFORM_WEB)
  make          (genera index.html/js/wasm vía emcc)
  make run      (sirve en http://localhost:8000)

  Copia la salida del build a public/wasm/ (como index.js + index.wasm) para
  que el servidor de Vite lo encuentre — ver WASM_GLUE_PATH en wasmBridge.ts.
  Hasta que ese build exista, la app corre sobre el preview Canvas2D de
  src/lib/mockRenderer.ts, que refleja el mismo contrato de parámetros.
`;
}
