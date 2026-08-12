import {
  fmtFloat,
  hexToColorLiteral,
  type EffectDefinition,
  type EffectCodegenModule,
  type EffectParams,
  type EffectModule,
  type ThumbnailDrawFn,
} from "./shared";

import headerRaw from "../../native/effects/touchdesigner/touchdesigner_effect.h?raw";
import mainRaw from "../../native/effects/touchdesigner/main.c?raw";
import readmeRaw from "../../native/effects/touchdesigner/README.md?raw";

import buildShRaw from "../../native/effects/touchdesigner/touchdesigner_build_and_run.sh?raw";
import buildBatRaw from "../../native/effects/touchdesigner/touchdesigner_build_and_run.bat?raw";
import palmModelUrl from "../../native/assets/cv/palm_detection_mediapipe_2023feb.onnx?url";

import asciiHeaderRaw from "../../native/effects/ascii/ascii_effect.h?raw";
import crtHeaderRaw from "../../native/effects/crt/crt_effect.h?raw";
import opencvHeaderRaw from "../../native/effects/opencv/opencv_effect.h?raw";

// --- 1. Definición de parámetros (Inspector) --------------------------------

const STYLE_OPTIONS = ["none", "ascii", "matrix", "crt", "edges"];

const definition: EffectDefinition<"touchdesigner"> = {
  id: "touchdesigner",
  name: "TouchDesigner Hand Tracker",
  description:
    "Detección de manos en vivo (OpenCV, todo en C — sin JS/modelos externos): la cámara se ve completa, sin negro, y cada mano detectada se cubre con un blob de vidrio líquido; si acercás las dos manos, un puente de slime las conecta.",
  params: [
    {
      key: "handStyle",
      label: "Hand Effect Style",
      type: "select",
      default: "none",
      options: STYLE_OPTIONS,
      group: "Hand Effect",
    },
    { key: "mirror", label: "Mirror (front camera)", type: "bool", default: true, group: "Detection" },
    {
      key: "handReanchorInterval",
      label: "Re-anchor Every N Frames",
      type: "int",
      default: 24,
      min: 8,
      max: 180,
      step: 1,
      group: "Detection",
    },
    {
      key: "handProcessScale",
      label: "Hand Detection Resolution",
      type: "float",
      default: 0.16,
      min: 0.05,
      max: 0.5,
      step: 0.01,
      group: "Detection",
    },
    {
      key: "handDetectSkip",
      label: "Detect Every N Frames",
      type: "int",
      default: 3,
      min: 1,
      max: 12,
      step: 1,
      group: "Detection",
    },

    { key: "glassColor", label: "Glass / Slime Color", type: "color", default: "#8CEBFF", group: "Liquid Glass" },
    { key: "glassEnabled", label: "Show Liquid Glass", type: "bool", default: true, group: "Liquid Glass" },
    { key: "glassSize", label: "Glass Blob Size", type: "float", default: 2.0, min: 1, max: 4, step: 0.05, group: "Liquid Glass" },
    { key: "slimeEnabled", label: "Show Slime Bridge", type: "bool", default: true, group: "Liquid Glass" },
    {
      key: "slimeDistance",
      label: "Slime Connect Distance",
      type: "float",
      default: 3.0,
      min: 1.2,
      max: 6,
      step: 0.1,
      group: "Liquid Glass",
    },
    { key: "showHandCount", label: "Show Hand Count", type: "bool", default: false, group: "Liquid Glass" },

    { key: "showCameraBg", label: "Show Camera Background", type: "bool", default: true, group: "Background" },
    {
      key: "bgStyle",
      label: "Background Style",
      type: "select",
      default: "none",
      options: STYLE_OPTIONS,
      group: "Background",
    },
    { key: "bgFallbackColor", label: "Background Color", type: "color", default: "#000000FF", group: "Background" },
  ],
};

// --- 2. Codegen ---------------------------------------------------------------

const TD_STYLE_ENUM: Record<string, string> = {
  none: "TD_STYLE_NONE",
  ascii: "TD_STYLE_ASCII",
  matrix: "TD_STYLE_MATRIX",
  crt: "TD_STYLE_CRT",
  edges: "TD_STYLE_EDGES",
};

function tdStyle(v: unknown): string {
  return TD_STYLE_ENUM[String(v)] ?? "TD_STYLE_NONE";
}

function buildParamsBlock(params: EffectParams): string {
  return `static TD_Params TD_g_params = {
    .showCameraBg = ${params.showCameraBg ? "true" : "false"},
    .mirror = ${params.mirror === false ? "false" : "true"},
    .autoDetectHands = true,
    .handProcessScale = ${fmtFloat(params.handProcessScale)},
    .handReanchorInterval = ${Math.round(Number(params.handReanchorInterval))},
    .handDetectSkip = ${Math.round(Number(params.handDetectSkip))},
    .glassColor = ${hexToColorLiteral(String(params.glassColor))},
    .glassSize = ${fmtFloat(params.glassSize)},
    .glassEnabled = ${params.glassEnabled === false ? "false" : "true"},
    .slimeDistance = ${fmtFloat(params.slimeDistance)},
    .slimeEnabled = ${params.slimeEnabled === false ? "false" : "true"},
    .showHandCount = ${params.showHandCount ? "true" : "false"},
    .bgFallbackColor = ${hexToColorLiteral(String(params.bgFallbackColor ?? "#000000FF"))},
    .forceFallback = false,
    .handStyle = ${tdStyle(params.handStyle)},
    .bgStyle = ${tdStyle(params.bgStyle)},
};
`;
}

const codegen: EffectCodegenModule = {
  headerRaw,
  mainRaw,
  mainFilename: "main.c",
  readmeRaw,
  paramsRegex: /static TD_Params TD_g_params = \{[\s\S]*?\};\r?\n/,
  buildParamsBlock,
  extras: [
    { filename: "touchdesigner_build_and_run.sh", label: "Build script (Linux/macOS)", kind: "text", content: buildShRaw },
    { filename: "touchdesigner_build_and_run.bat", label: "Build script (Windows / MinGW)", kind: "text", content: buildBatRaw },
    {
      filename: "palm_detection_mediapipe_2023feb.onnx",
      label: "Modelo ONNX (detección de palma, MediaPipe)",
      kind: "binary-url",
      url: palmModelUrl,
    },
    { filename: "ascii_effect.h", label: "Estilo ASCII / Matrix (para Hand/Background Effect Style)", kind: "text", content: asciiHeaderRaw },
    { filename: "crt_effect.h", label: "Estilo CRT (para Hand/Background Effect Style)", kind: "text", content: crtHeaderRaw },
    { filename: "opencv_effect.h", label: "Estilo Edges (para Hand/Background Effect Style)", kind: "text", content: opencvHeaderRaw },
  ],
};

// --- 3. Thumbnail --------------------------------------------------------------

// Cámara de fondo + dos blobs de vidrio líquido unidos por un puente de
// slime, igual que el look real del efecto.
const thumbnail: ThumbnailDrawFn = (ctx, w, h, t) => {
  const grad = ctx.createLinearGradient(0, 0, w, h);
  grad.addColorStop(0, "#1b3a44");
  grad.addColorStop(1, "#0b0b0e");
  ctx.fillStyle = grad;
  ctx.fillRect(0, 0, w, h);

  const cx1 = w * (0.4 + 0.06 * Math.sin(t * 1.3));
  const cx2 = w * (0.6 + 0.06 * Math.sin(t * 1.3 + Math.PI));
  const cy = h * 0.5;
  const r = Math.min(w, h) * 0.15;

  ctx.strokeStyle = "rgba(140,235,255,0.55)";
  ctx.fillStyle = "rgba(140,235,255,0.22)";
  ctx.lineWidth = 3;
  const midY = cy;
  ctx.beginPath();
  ctx.moveTo(cx1, midY - r * 0.55);
  ctx.quadraticCurveTo((cx1 + cx2) / 2, midY - r * 0.3, cx2, midY - r * 0.55);
  ctx.lineTo(cx2, midY + r * 0.55);
  ctx.quadraticCurveTo((cx1 + cx2) / 2, midY + r * 0.3, cx1, midY + r * 0.55);
  ctx.closePath();
  ctx.fill();

  for (const cx of [cx1, cx2]) {
    const glassGrad = ctx.createRadialGradient(cx, cy, r * 0.1, cx, cy, r);
    glassGrad.addColorStop(0, "rgba(140,235,255,0.35)");
    glassGrad.addColorStop(1, "rgba(140,235,255,0.05)");
    ctx.fillStyle = glassGrad;
    ctx.beginPath();
    ctx.arc(cx, cy, r, 0, Math.PI * 2);
    ctx.fill();
    ctx.strokeStyle = "#8CEBFF";
    ctx.stroke();
    ctx.fillStyle = "rgba(255,255,255,0.5)";
    ctx.beginPath();
    ctx.ellipse(cx - r * 0.32, cy - r * 0.38, r * 0.24, r * 0.14, 0, 0, Math.PI * 2);
    ctx.fill();
  }
};

// --- Paquete final -----------------------------------------------------------

export const TOUCHDESIGNER_MODULE: EffectModule<"touchdesigner"> = { definition, codegen, thumbnail };
