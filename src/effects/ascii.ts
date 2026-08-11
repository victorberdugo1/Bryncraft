import {
  fmtInt,
  fmtFloat,
  hexToRgbComment,
  hexToRgbaComment,
  type EffectDefinition,
  type EffectCodegenModule,
  type EffectParams,
  type EffectModule,
  type ThumbnailDrawFn,
} from "./shared";

import headerRaw from "../../native/effects/ascii/ascii_effect.h?raw";
import mainRaw from "../../native/effects/ascii/main000.c?raw";
import readmeRaw from "../../native/effects/ascii/README.md?raw";

// Extras: fuente del modo Matrix, más raylib.h/libraylib.a (compartidos por
// las 4 demos standalone — se ofrecen acá, el primer efecto, para que
// alguien que solo se lleva esta carpeta tenga igual lo necesario para
// compilar main000.c).
import fontUrl from "../../native/effects/ascii/NotoSansJP-Kana.ttf?url";
import raylibHeaderUrl from "../../native/effects/raylib.h?url";
import libraylibWinUrl from "../../native/effects/win/libraylib.a?url";
import libraylibLnxUrl from "../../native/effects/lnx/libraylib.a?url";

// --- 1. Definición de parámetros (Inspector) --------------------------------

const definition: EffectDefinition<"ascii"> = {
  id: "ascii",
  name: "ASCII Renderer",
  description: "Converts the rendered frame into a live ASCII character grid. Supports normal luminance-mapping and Matrix mode (code rain).",
  params: [
    { key: "mode", label: "Mode", type: "select", default: "normal", options: ["normal", "matrix"], group: "Mode" },

    { key: "characters", label: "Characters", type: "string", default: " .:-=+*#%@", group: "Ramp", modes: ["normal"] },
    { key: "fontSize", label: "Font", type: "int", default: 10, min: 4, max: 32, step: 1, group: "Ramp" },
    { key: "brightness", label: "Brightness", type: "float", default: 0.8, min: 0, max: 2, step: 0.01, group: "Image", modes: ["normal"] },
    { key: "contrast", label: "Contrast", type: "float", default: 1.2, min: 0, max: 3, step: 0.01, group: "Image", modes: ["normal"] },
    { key: "gamma", label: "Gamma", type: "float", default: 1.1, min: 0.2, max: 3, step: 0.01, group: "Image", modes: ["normal"] },
    { key: "foreground", label: "Foreground Color", type: "color", default: "#44D4FF", group: "Color" },
    { key: "background", label: "Background Color", type: "color", default: "#0B0B0E00", alpha: true, group: "Color" },
    { key: "invert", label: "Invert", type: "bool", default: false, group: "Color", modes: ["normal"] },

    { key: "matrixChars", label: "Characters", type: "string", default: "0123456789アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲン", group: "Matrix", modes: ["matrix"] },
    { key: "matrixDirection", label: "Direction", type: "select", default: "down", options: ["down", "up", "both"], group: "Matrix", modes: ["matrix"] },
    { key: "matrixSpeed", label: "Speed", type: "float", default: 14, min: 1, max: 50, step: 1, group: "Matrix", modes: ["matrix"] },
    { key: "matrixDensity", label: "Density", type: "float", default: 0.97, min: 0, max: 1, step: 0.01, group: "Matrix", modes: ["matrix"] },
    { key: "matrixTrailLength", label: "Trail Length", type: "int", default: 24, min: 2, max: 64, step: 1, group: "Matrix", modes: ["matrix"] },
    { key: "matrixHeadColor", label: "Head Color", type: "color", default: "#CFFFE0", group: "Matrix", modes: ["matrix"] },
    { key: "matrixImageStrength", label: "Image Strength", type: "float", default: 1.3, min: 0, max: 2.5, step: 0.05, group: "Matrix", modes: ["matrix"] },
    { key: "matrixReactive", label: "React to Video", type: "bool", default: true, group: "Matrix", modes: ["matrix"] },
    { key: "matrixReactivityMode", label: "Video Interaction", type: "select", default: "all", options: ["off", "speed", "density", "colors", "all"], group: "Matrix", modes: ["matrix"] },
    { key: "matrixReactiveStrength", label: "Interaction Intensity", type: "float", default: 1.2, min: 0, max: 2, step: 0.01, group: "Matrix", modes: ["matrix"] },
  ],
};

// --- 2. Codegen (arma el .h descargable con los params actuales) -----------

const ASCII_MODE_ENUM: Record<string, string> = {
  normal: "ASCII_MODE_NORMAL",
  matrix: "ASCII_MODE_MATRIX",
};

const ASCII_MATRIX_DIRECTION_ENUM: Record<string, string> = {
  down: "ASCII_MATRIX_DIR_DOWN",
  up: "ASCII_MATRIX_DIR_UP",
  both: "ASCII_MATRIX_DIR_BOTH",
};

function buildParamsBlock(params: EffectParams): string {
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

const codegen: EffectCodegenModule = {
  headerRaw,
  mainRaw,
  mainFilename: "main000.c",
  readmeRaw,
  paramsRegex: /static ASCII_Params ASCII_g_params = \{[\s\S]*?\};\r?\n/,
  buildParamsBlock,
  extras: [
    { filename: "NotoSansJP-Kana.ttf", label: "Fuente del modo Matrix (kana)", kind: "binary-url", url: fontUrl },
    { filename: "raylib.h", label: "raylib.h (compartido por las 4 demos standalone)", kind: "binary-url", url: raylibHeaderUrl },
    { filename: "libraylib.a", label: "libraylib.a — Windows (MinGW)", kind: "binary-url", url: libraylibWinUrl },
    { filename: "libraylib.a", label: "libraylib.a — Linux", kind: "binary-url", url: libraylibLnxUrl },
  ],
};

// --- 3. Thumbnail (miniatura animada, barra lateral) ------------------------

const thumbnail: ThumbnailDrawFn = (ctx, w, h, t) => {
  ctx.font = "7px monospace";
  ctx.fillStyle = "#44D4FF";
  const ramp = " .:-=+*#%@";
  for (let y = 0; y < h; y += 8) {
    let line = "";
    for (let x = 0; x < w; x += 5) {
      const v = (Math.sin(x * 0.2 + t) + Math.cos(y * 0.2 + t)) * 0.5 + 0.5;
      line += ramp[Math.floor(v * (ramp.length - 1))] ?? " ";
    }
    ctx.fillText(line, 0, y + 6);
  }
};

// --- Paquete final -----------------------------------------------------------

export const ASCII_MODULE: EffectModule<"ascii"> = { definition, codegen, thumbnail };
