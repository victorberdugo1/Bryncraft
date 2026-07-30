import type { EffectId, EffectParams } from "@/types/effects";

// Los .h/.c reales, cargados tal cual están en native/effects/ (Vite ?raw).
// Nada se copia a mano aquí: si tocas esos archivos, esto se entera solo.
import asciiHeaderRaw from "../../native/effects/ascii_effect.h?raw";
import crtHeaderRaw from "../../native/effects/crt_effect.h?raw";
import particlesHeaderRaw from "../../native/effects/particles_effect.h?raw";
import opencvBridgeRaw from "../../native/opencv_bridge.cpp?raw";

import main000Raw from "../../native/effects/main000.c?raw";
import main001Raw from "../../native/effects/main001.c?raw";
import main002Raw from "../../native/effects/main002.c?raw";
import main003Raw from "../../native/effects/main003.c?raw";

const MAIN_BY_EFFECT: Record<EffectId, { filename: string; raw: string }> = {
  ascii: { filename: "main000.c", raw: main000Raw },
  particles: { filename: "main001.c", raw: main001Raw },
  crt: { filename: "main002.c", raw: main002Raw },
  opencv: { filename: "main003.c", raw: main003Raw },
};

// Campos int de C (fontSize, count): sin punto decimal.
function fmtInt(v: unknown): string {
  const n = typeof v === "number" ? v : Number(v);
  return String(Math.round(n));
}

// Campos float de C con sufijo `f`: SIEMPRE con punto decimal, aunque el
// valor sea un número entero (p.ej. 120 -> "120.000f", nunca "120f", que es
// un sufijo inválido en C para un literal sin punto decimal).
function fmtFloat(v: unknown): string {
  const n = typeof v === "number" ? v : Number(v);
  return `${n.toFixed(3)}f`;
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

const ASCII_MODE_ENUM: Record<string, string> = {
  normal: "ASCII_MODE_NORMAL",
  matrix: "ASCII_MODE_MATRIX",
};

const ASCII_MATRIX_DIRECTION_ENUM: Record<string, string> = {
  down: "ASCII_MATRIX_DIR_DOWN",
  up: "ASCII_MATRIX_DIR_UP",
  both: "ASCII_MATRIX_DIR_BOTH",
};

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
    .fontSize = ${fmtInt(params.fontSize)},
    .brightness = ${fmtFloat(params.brightness)},
    .contrast = ${fmtFloat(params.contrast)},
    .gamma = ${fmtFloat(params.gamma)},
    .foreground = (Color){ ${hexToRgbComment(String(params.foreground))}, 255 },
    .background = (Color){ ${hexToRgbaComment(String(params.background))} },
    .invert = ${params.invert ? "true" : "false"},
    .mode = ${ASCII_MODE_ENUM[String(params.mode)] ?? "ASCII_MODE_NORMAL"},
    .matrixChars = "${params.matrixChars}",
    .matrixDirection = ${ASCII_MATRIX_DIRECTION_ENUM[String(params.matrixDirection)] ?? "ASCII_MATRIX_DIR_DOWN"},
    .matrixSpeed = ${fmtFloat(params.matrixSpeed)},
    .matrixDensity = ${fmtFloat(params.matrixDensity)},
    .matrixTrailLength = ${fmtInt(params.matrixTrailLength)},
    .matrixHeadColor = (Color){ ${hexToRgbComment(String(params.matrixHeadColor))}, 255 },
    .matrixReactive = ${params.matrixReactive ? "true" : "false"},
    .matrixReactiveStrength = ${fmtFloat(params.matrixReactiveStrength)},
    .matrixImageStrength = ${fmtFloat(params.matrixImageStrength)},
};
`;
}

function buildCrtParamsBlock(params: EffectParams): string {
  return `static CRT_Params CRT_g_params = {
    .scanlineIntensity   = ${fmtFloat(params.scanlineIntensity)},
    .scanlineCount       = ${fmtFloat(params.scanlineCount)},
    .scanlineSpeed       = ${fmtFloat(params.scanlineSpeed)},
    .curvature           = ${fmtFloat(params.curvature)},
    .vignette            = ${fmtFloat(params.vignette)},
    .noise               = ${fmtFloat(params.noise)},
    .chromaticAberration = ${fmtFloat(params.chromaticAberration)},
    .flicker             = ${fmtFloat(params.flicker)},
};
`;
}

function buildParticlesParamsBlock(params: EffectParams): string {
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
    case "opencv":
      // opencv_bridge.cpp's OcvParams uses per-field default member
      // initializers (C++ struct syntax), not the single
      // `static X_Params X_g_params = { ... };` block the other three
      // effects use — there's no equivalent single block to regex-replace
      // with the Inspector's current values, so this just shows the real
      // file as-is rather than a partially-live substitution that would
      // only cover some fields and silently miss others.
      return opencvBridgeRaw;
  }
}

/** Nombre real del main mínimo en native/effects/ para este efecto
 * (main000.c / main001.c / main002.c). */
export function getMainFilename(effect: EffectId): string {
  return MAIN_BY_EFFECT[effect].filename;
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
  if (effect === "opencv") {
    return `// opencv runs entirely on the CPU via OpenCV (Canny/contours/optical flow/
// background subtraction/Haar cascades) — no fragment shader involved.
// See native/opencv_bridge.cpp (the actual pipeline) and
// native/effects/opencv_effect.h (the C-callable interface main.c uses).`;
  }
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
    opencv: "OpenCV Vision",
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
      ${effect === "opencv"
        ? "Interfaz C que expone el efecto a main.c (SetParams/Update/Draw/Unload).\n      La implementación real (OpenCV) vive en native/opencv_bridge.cpp — ver\n      la pestaña \"Code\" para ese archivo completo."
        : "Simulación + dibujado de este efecto (ver la pestaña \"Code\" para\n      el header exacto con los valores actuales del Inspector)."}

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
