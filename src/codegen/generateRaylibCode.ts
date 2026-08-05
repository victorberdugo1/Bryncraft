import type { EffectId, EffectParams } from "@/types/effects";

// Los .h/.c/.md reales, cargados tal cual están en native/effects/<efecto>/
// (Vite ?raw). Nada se copia a mano aquí: si tocas esos archivos, esto se
// entera solo.
import asciiHeaderRaw from "../../native/effects/ascii/ascii_effect.h?raw";
import crtHeaderRaw from "../../native/effects/crt/crt_effect.h?raw";
import particlesHeaderRaw from "../../native/effects/particles/particles_effect.h?raw";
import opencvHeaderRaw from "../../native/effects/opencv/opencv_effect.h?raw";

import main000Raw from "../../native/effects/ascii/main000.c?raw";
import main001Raw from "../../native/effects/particles/main001.c?raw";
import main002Raw from "../../native/effects/crt/main002.c?raw";
import main003Raw from "../../native/effects/opencv/main003.c?raw";

// README.md real de cada carpeta de efecto — esto es lo que muestra el tab
// "README" del CodePanel. Antes ese tab generaba texto a mano en TS (y se
// desincronizaba de lo que había realmente en native/effects/); ahora es
// literalmente el archivo, ni una copia ni una plantilla.
import asciiReadmeRaw from "../../native/effects/ascii/README.md?raw";
import particlesReadmeRaw from "../../native/effects/particles/README.md?raw";
import crtReadmeRaw from "../../native/effects/crt/README.md?raw";
import opencvReadmeRaw from "../../native/effects/opencv/README.md?raw";

// Extras por efecto: archivos que ese efecto necesita además del header/main
// y que no son código C — fuente, scripts de build, etc. Los de texto se
// cargan con ?raw (mismo mecanismo que arriba); el .ttf es binario, así que
// se importa con ?url (Vite lo copia tal cual al bundle y devuelve su URL
// final — nunca pasa por un Blob de texto, que corrompería los bytes).
import opencvBuildShRaw from "../../native/effects/opencv/opencv_build_and_run.sh?raw";
import opencvBuildBatRaw from "../../native/effects/opencv/opencv_build_and_run.bat?raw";
import asciiFontUrl from "../../native/effects/ascii/NotoSansJP-Kana.ttf?url";

// raylib.h / libraylib.a viven en native/effects/ (compartidos por las 4
// demos standalone) — se ofrecen como extra en ascii (el primer efecto)
// para que alguien que solo se lleva esa carpeta tenga igual lo necesario
// para compilar main000.c. Ambos por ?url: raylib.h también, para no
// inflar el bundle JS con sus ~130KB como string — se sirve como asset
// estático igual que el .ttf.
import raylibHeaderUrl from "../../native/effects/raylib.h?url";
import libraylibUrl from "../../native/effects/libraylib.a?url";

const MAIN_BY_EFFECT: Record<EffectId, { filename: string; raw: string }> = {
  ascii: { filename: "main000.c", raw: main000Raw },
  particles: { filename: "main001.c", raw: main001Raw },
  crt: { filename: "main002.c", raw: main002Raw },
  opencv: { filename: "main003.c", raw: main003Raw },
};

const README_BY_EFFECT: Record<EffectId, string> = {
  ascii: asciiReadmeRaw,
  particles: particlesReadmeRaw,
  crt: crtReadmeRaw,
  opencv: opencvReadmeRaw,
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

const OPENCV_MODE_ENUM: Record<string, string> = {
  edges: "OCV_MODE_EDGES",
  contours: "OCV_MODE_CONTOURS",
  optical_flow: "OCV_MODE_FLOW",
  bg_subtract: "OCV_MODE_BG",
  face_detect: "OCV_MODE_FACE",
};

/* ============================================================================
 * Cada *_g_params se reemplaza en el texto real del .h por una regex que
 * localiza únicamente ese bloque (static ... = { ... };). Todo lo demás del
 * archivo pasa intacto, tal cual está en disco.
 * ========================================================================== */

const ASCII_PARAMS_RE = /static ASCII_Params ASCII_g_params = \{[\s\S]*?\};\r?\n/;
const CRT_PARAMS_RE = /static CRT_Params CRT_g_params = \{[\s\S]*?\};\r?\n/;
const PART_PARAMS_RE = /static PART_ParticleParams PART_g_params = \{[\s\S]*?\};\r?\n/;
const OCV_PARAMS_RE = /static OcvParams g_params = \{[\s\S]*?\};\r?\n/;

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

function hexToColorLiteral(hex: string) {
  return `(Color){ ${hexToRgbComment(String(hex))}, 255 }`;
}

function buildOpencvParamsBlock(params: EffectParams): string {
  return `static OcvParams g_params = {
    .mode = ${OPENCV_MODE_ENUM[String(params.mode)] ?? "OCV_MODE_EDGES"},
    .processScale = ${fmtFloat(params.processScale)},
    .mirror = ${params.mirror ? "true" : "false"},

    .cannyLow = ${fmtFloat(params.cannyLow)},
    .cannyHigh = ${fmtFloat(params.cannyHigh)},
    .blur = ${fmtInt(params.blur)},
    .edgeOnSource = ${params.edgeOnSource ? "true" : "false"},
    .edgeColor = ${hexToColorLiteral(String(params.edgeColor))},

    .contourMinArea = ${fmtFloat(params.contourMinArea)},
    .contourThickness = ${fmtInt(params.contourThickness)},
    .contourFill = ${params.contourFill ? "true" : "false"},
    .contourColor = ${hexToColorLiteral(String(params.contourColor))},

    .flowStrength = ${fmtFloat(params.flowStrength)},
    .flowArrows = ${params.flowArrows ? "true" : "false"},
    .flowArrowStep = ${fmtInt(params.flowArrowStep)},

    .bgHistory = ${fmtInt(params.bgHistory)},
    .bgVarThreshold = ${fmtFloat(params.bgVarThreshold)},
    .bgShadows = ${params.bgShadows ? "true" : "false"},
    .bgMaskOnly = ${params.bgMaskOnly ? "true" : "false"},

    .faceScaleFactor = ${fmtFloat(params.faceScaleFactor)},
    .faceMinNeighbors = ${fmtInt(params.faceMinNeighbors)},
    .faceMinSizeFraction = ${fmtFloat(params.faceMinSizeFraction)},
    .faceBoxColor = ${hexToColorLiteral(String(params.faceBoxColor))},
    .faceShowCount = ${params.faceShowCount ? "true" : "false"},
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
      // effects/opencv/opencv_effect.h ahora es single-header (estilo stb_image.h):
      // arriba las declaraciones planas en C que main.c parsea, y debajo,
      // tras OPENCV_EFFECT_IMPLEMENTATION, el OcvParams/pipelines reales en
      // C++. Un único bloque `static OcvParams g_params = { ... };` (mismo
      // formato que ASCII_g_params/CRT_g_params/PART_g_params) permite
      // sustituirlo por regex igual que en los otros tres efectos.
      return opencvHeaderRaw.replace(OCV_PARAMS_RE, buildOpencvParamsBlock(params));
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

/** Un archivo descargable del tab "Extra" para el efecto activo. `text`
 * trae el contenido listo para mostrar/copiar/descargar como texto plano;
 * `binary-url` trae una URL (asset de Vite) para descargar tal cual, sin
 * pasar por texto — así un .ttf no se corrompe. */
export type ExtraAsset =
  | { filename: string; label: string; kind: "text"; content: string }
  | { filename: string; label: string; kind: "binary-url"; url: string };

/** Archivos extra de cada efecto — lo que sea que ese efecto necesite además
 * de su header/main, y que no es código C generado por el Inspector: fuente,
 * scripts de build, shader... No todos los efectos tienen alguno (particles
 * no necesita nada aparte). */
export function getExtras(effect: EffectId): ExtraAsset[] {
  switch (effect) {
    case "crt":
      return [
        {
          filename: "crt.fs",
          label: "Fragment shader (GLSL 100 / WebGL1-ES)",
          kind: "text",
          content: extractCString(crtHeaderRaw, "CRT_FS_SOURCE"),
        },
      ];
    case "ascii":
      return [
        {
          filename: "NotoSansJP-Kana.ttf",
          label: "Fuente del modo Matrix (kana)",
          kind: "binary-url",
          url: asciiFontUrl,
        },
        {
          filename: "raylib.h",
          label: "raylib.h (compartido por las 4 demos standalone)",
          kind: "binary-url",
          url: raylibHeaderUrl,
        },
        {
          filename: "libraylib.a",
          label: "libraylib.a (compartido por las 4 demos standalone)",
          kind: "binary-url",
          url: libraylibUrl,
        },
      ];
    case "opencv":
      return [
        {
          filename: "opencv_build_and_run.sh",
          label: "Build script (Linux/macOS)",
          kind: "text",
          content: opencvBuildShRaw,
        },
        {
          filename: "opencv_build_and_run.bat",
          label: "Build script (Windows / MinGW)",
          kind: "text",
          content: opencvBuildBatRaw,
        },
      ];
    case "particles":
      return [];
  }
}

/** README tab: el README.md real de native/effects/<efecto>/, tal cual está
 * en disco — no una plantilla generada en TS. Se muestra en un <pre> crudo,
 * sin renderizar Markdown. */
export function generateReadme(effect: EffectId): string {
  return README_BY_EFFECT[effect];
}
