import {
  fmtInt,
  fmtFloat,
  hexToColorLiteral,
  type EffectDefinition,
  type EffectCodegenModule,
  type EffectParams,
  type EffectModule,
  type ThumbnailDrawFn,
} from "./shared";

import headerRaw from "../../native/effects/opencv/opencv_effect.h?raw";
import mainRaw from "../../native/effects/opencv/main003.c?raw";
import readmeRaw from "../../native/effects/opencv/README.md?raw";

import buildShRaw from "../../native/effects/opencv/opencv_build_and_run.sh?raw";
import buildBatRaw from "../../native/effects/opencv/opencv_build_and_run.bat?raw";
import haarcascadeUrl from "../../native/assets/cv/haarcascade_frontalface_default.xml?url";

// --- 1. Definición de parámetros (Inspector) --------------------------------

const definition: EffectDefinition<"opencv"> = {
  id: "opencv",
  name: "OpenCV Vision",
  description: "Computer-vision pipelines (edges, contours, optical flow, background subtraction, face detection) running natively via OpenCV compiled to WASM. Works on the loaded video or a live camera feed.",
  params: [
    { key: "mode", label: "Mode", type: "select", default: "edges", options: ["edges", "contours", "optical_flow", "bg_subtract", "face_detect"], group: "Mode" },
    { key: "processScale", label: "Process Scale", type: "float", default: 0.5, min: 0.1, max: 1, step: 0.05, group: "Performance" },
    { key: "mirror", label: "Mirror (front camera)", type: "bool", default: false, group: "Performance" },

    { key: "cannyLow", label: "Canny Low", type: "float", default: 60, min: 0, max: 255, step: 1, group: "Edges", modes: ["edges"] },
    { key: "cannyHigh", label: "Canny High", type: "float", default: 160, min: 0, max: 255, step: 1, group: "Edges", modes: ["edges"] },
    { key: "blur", label: "Blur", type: "int", default: 1, min: 0, max: 10, step: 1, group: "Edges", modes: ["edges"] },
    { key: "edgeOnSource", label: "Overlay on Source", type: "bool", default: false, group: "Edges", modes: ["edges"] },
    { key: "edgeColor", label: "Edge Color", type: "color", default: "#44D4FF", group: "Edges", modes: ["edges"] },

    { key: "contourMinArea", label: "Min Area", type: "float", default: 80, min: 0, max: 5000, step: 10, group: "Contours", modes: ["contours"] },
    { key: "contourThickness", label: "Thickness", type: "int", default: 2, min: 1, max: 10, step: 1, group: "Contours", modes: ["contours"] },
    { key: "contourFill", label: "Fill", type: "bool", default: false, group: "Contours", modes: ["contours"] },
    { key: "contourColor", label: "Contour Color", type: "color", default: "#44D4FF", group: "Contours", modes: ["contours"] },

    { key: "flowStrength", label: "Flow Strength", type: "float", default: 1, min: 0.1, max: 3, step: 0.05, group: "Optical Flow", modes: ["optical_flow"] },
    { key: "flowArrows", label: "Arrows (vs. color)", type: "bool", default: false, group: "Optical Flow", modes: ["optical_flow"] },
    { key: "flowArrowStep", label: "Arrow Spacing", type: "int", default: 16, min: 4, max: 40, step: 1, group: "Optical Flow", modes: ["optical_flow"] },

    { key: "bgHistory", label: "History", type: "int", default: 120, min: 10, max: 500, step: 10, group: "Background Subtraction", modes: ["bg_subtract"] },
    { key: "bgVarThreshold", label: "Var Threshold", type: "float", default: 16, min: 4, max: 64, step: 1, group: "Background Subtraction", modes: ["bg_subtract"] },
    { key: "bgShadows", label: "Detect Shadows", type: "bool", default: true, group: "Background Subtraction", modes: ["bg_subtract"] },
    { key: "bgMaskOnly", label: "Mask Only", type: "bool", default: false, group: "Background Subtraction", modes: ["bg_subtract"] },

    { key: "faceScaleFactor", label: "Scale Factor", type: "float", default: 1.1, min: 1.05, max: 1.4, step: 0.01, group: "Face Detection", modes: ["face_detect"] },
    { key: "faceMinNeighbors", label: "Min Neighbors", type: "int", default: 4, min: 1, max: 10, step: 1, group: "Face Detection", modes: ["face_detect"] },
    { key: "faceMinSizeFraction", label: "Min Size (% width)", type: "float", default: 0.08, min: 0.02, max: 0.5, step: 0.01, group: "Face Detection", modes: ["face_detect"] },
    { key: "faceBoxColor", label: "Box Color", type: "color", default: "#78FF78", group: "Face Detection", modes: ["face_detect"] },
    { key: "faceShowCount", label: "Show Count", type: "bool", default: true, group: "Face Detection", modes: ["face_detect"] },
  ],
};

// --- 2. Codegen ---------------------------------------------------------------

const OPENCV_MODE_ENUM: Record<string, string> = {
  edges: "OCV_MODE_EDGES",
  contours: "OCV_MODE_CONTOURS",
  optical_flow: "OCV_MODE_FLOW",
  bg_subtract: "OCV_MODE_BG",
  face_detect: "OCV_MODE_FACE",
};

function buildParamsBlock(params: EffectParams): string {
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

const codegen: EffectCodegenModule = {
  headerRaw,
  mainRaw,
  mainFilename: "main003.c",
  readmeRaw,
  paramsRegex: /static OcvParams g_params = \{[\s\S]*?\};\r?\n/,
  buildParamsBlock,
  extras: [
    { filename: "opencv_build_and_run.sh", label: "Build script (Linux/macOS)", kind: "text", content: buildShRaw },
    { filename: "opencv_build_and_run.bat", label: "Build script (Windows / MinGW)", kind: "text", content: buildBatRaw },
    { filename: "haarcascade_frontalface_default.xml", label: "Modelo Haar Cascade (detección de rostros)", kind: "binary-url", url: haarcascadeUrl },
  ],
};

// --- 3. Thumbnail --------------------------------------------------------------

// Evoca detección de bordes: un wireframe rotando sobre negro, en línea con
// lo que Canny/contours realmente produce.
const thumbnail: ThumbnailDrawFn = (ctx, w, h, t) => {
  ctx.strokeStyle = "#78FF78";
  ctx.lineWidth = 1.2;
  const cx = w / 2;
  const cy = h / 2;
  const sides = 6;
  const radius = Math.min(w, h) * 0.32;
  ctx.beginPath();
  for (let i = 0; i <= sides; i++) {
    const a = (i / sides) * Math.PI * 2 + t;
    const x = cx + Math.cos(a) * radius;
    const y = cy + Math.sin(a) * radius * 0.8;
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();
  ctx.strokeStyle = "#44D4FF";
  ctx.beginPath();
  ctx.rect(cx - radius * 0.55, cy - radius * 0.55, radius * 1.1, radius * 1.1);
  ctx.stroke();
};

// --- Paquete final -----------------------------------------------------------

export const OPENCV_MODULE: EffectModule<"opencv"> = { definition, codegen, thumbnail };
