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

import headerRaw from "../../native/effects/reframe/reframe_effect.h?raw";
import mainRaw from "../../native/effects/reframe/main005.c?raw";
import readmeRaw from "../../native/effects/reframe/README.md?raw";

import buildShRaw from "../../native/effects/reframe/reframe_build_and_run.sh?raw";
import buildBatRaw from "../../native/effects/reframe/reframe_build_and_run.bat?raw";
import haarcascadeUrl from "../../native/assets/cv/haarcascade_frontalface_default.xml?url";
import raylibHeaderUrl from "../../native/effects/raylib.h?url";
import libraylibWinUrl from "../../native/effects/win/libraylib.a?url";
import libraylibLnxUrl from "../../native/effects/lnx/libraylib.a?url";

// --- 1. Definición de parámetros (Inspector) --------------------------------

const ASPECT_PRESETS: Record<string, [number, number]> = {
  "9:16": [9, 16],
  "4:5": [4, 5],
  "1:1": [1, 1],
};

const definition: EffectDefinition<"reframe"> = {
  id: "reframe",
  name: "Reframe (16:9 → 9:16)",
  description:
    "Automatic landscape-to-vertical converter. Detects and tracks faces natively via OpenCV, guesses the active speaker from mouth-region motion, and smoothly pans the crop from one person to another to keep what matters in frame — no manual keyframing.",
  params: [
    { key: "aspectPreset", label: "Output Aspect", type: "select", default: "9:16", options: ["9:16", "4:5", "1:1"], group: "Framing" },
    { key: "zoom", label: "Zoom", type: "float", default: 1.0, min: 1, max: 2.5, step: 0.01, group: "Framing" },
    { key: "headroom", label: "Headroom", type: "float", default: 0.12, min: 0, max: 0.4, step: 0.01, group: "Framing" },

    { key: "processScale", label: "Detection Resolution", type: "float", default: 0.75, min: 0.1, max: 1, step: 0.05, group: "Detection" },
    { key: "mirror", label: "Mirror (front camera)", type: "bool", default: false, group: "Detection" },
    { key: "faceScaleFactor", label: "Scale Factor", type: "float", default: 1.1, min: 1.05, max: 1.4, step: 0.01, group: "Detection" },
    { key: "faceMinNeighbors", label: "Min Neighbors", type: "int", default: 3, min: 1, max: 10, step: 1, group: "Detection" },
    { key: "faceMinSizeFraction", label: "Min Face Size (% width)", type: "float", default: 0.05, min: 0.02, max: 0.5, step: 0.01, group: "Detection" },

    { key: "activeSpeakerDetection", label: "Follow Active Speaker", type: "bool", default: true, group: "Camera" },
    { key: "panSmoothing", label: "Pan Smoothing", type: "float", default: 0.25, min: 0.05, max: 1, step: 0.01, group: "Camera" },
    { key: "maxPanSpeed", label: "Max Pan Speed", type: "float", default: 0.4, min: 0.1, max: 3, step: 0.05, group: "Camera" },
    { key: "switchCooldown", label: "Switch Cooldown (s)", type: "float", default: 1.2, min: 0.1, max: 3, step: 0.05, group: "Camera" },
    { key: "deadZone", label: "Dead Zone", type: "float", default: 0.02, min: 0, max: 0.1, step: 0.005, group: "Camera" },
    { key: "fallbackMode", label: "No Face Fallback", type: "select", default: "lastKnown", options: ["lastKnown", "center"], group: "Camera" },

    { key: "showDebugOverlay", label: "Show Debug Overlay", type: "bool", default: false, group: "Debug" },
    { key: "debugBoxColor", label: "Face Box Color", type: "color", default: "#78FF78", group: "Debug" },
    { key: "debugTargetColor", label: "Active Target Color", type: "color", default: "#FFC83C", group: "Debug" },
  ],
};

// --- 2. Codegen (arma el .h descargable con los params actuales) -----------

const REFRAME_FALLBACK_ENUM: Record<string, string> = {
  center: "REFRAME_FALLBACK_CENTER",
  lastKnown: "REFRAME_FALLBACK_LAST_KNOWN",
};

function buildParamsBlock(params: EffectParams): string {
  const [aspectW, aspectH] = ASPECT_PRESETS[String(params.aspectPreset)] ?? ASPECT_PRESETS["9:16"];

  return `static ReframeParams g_rfParams = {
    .targetAspectW = ${fmtFloat(aspectW)},
    .targetAspectH = ${fmtFloat(aspectH)},
    .processScale = ${fmtFloat(params.processScale)},
    .mirror = ${params.mirror ? "true" : "false"},

    .faceScaleFactor = ${fmtFloat(params.faceScaleFactor)},
    .faceMinNeighbors = ${fmtInt(params.faceMinNeighbors)},
    .faceMinSizeFraction = ${fmtFloat(params.faceMinSizeFraction)},

    .zoom = ${fmtFloat(params.zoom)},
    .headroom = ${fmtFloat(params.headroom)},
    .panSmoothing = ${fmtFloat(params.panSmoothing)},
    .maxPanSpeed = ${fmtFloat(params.maxPanSpeed)},
    .switchCooldown = ${fmtFloat(params.switchCooldown)},
    .deadZone = ${fmtFloat(params.deadZone)},
    .activeSpeakerDetection = ${params.activeSpeakerDetection ? "true" : "false"},
    .fallbackMode = ${REFRAME_FALLBACK_ENUM[String(params.fallbackMode)] ?? "REFRAME_FALLBACK_LAST_KNOWN"},

    .showDebugOverlay = ${params.showDebugOverlay ? "true" : "false"},
    .debugBoxColor = ${hexToColorLiteral(String(params.debugBoxColor))},
    .debugTargetColor = ${hexToColorLiteral(String(params.debugTargetColor))},
};
`;
}

const codegen: EffectCodegenModule = {
  headerRaw,
  mainRaw,
  mainFilename: "main005.c",
  readmeRaw,
  paramsRegex: /static ReframeParams g_rfParams = \{[\s\S]*?\};\r?\n/,
  buildParamsBlock,
  extras: [
    { filename: "reframe_build_and_run.sh", label: "Build script (Linux/macOS)", kind: "text", content: buildShRaw },
    { filename: "reframe_build_and_run.bat", label: "Build script (Windows / MinGW)", kind: "text", content: buildBatRaw },
    { filename: "haarcascade_frontalface_default.xml", label: "Modelo Haar Cascade (detección de rostros)", kind: "binary-url", url: haarcascadeUrl },
    { filename: "raylib.h", label: "raylib.h (compartido por las 4 demos standalone)", kind: "binary-url", url: raylibHeaderUrl },
    { filename: "libraylib.a", label: "libraylib.a — Windows (MinGW)", kind: "binary-url", url: libraylibWinUrl },
    { filename: "libraylib.a", label: "libraylib.a — Linux", kind: "binary-url", url: libraylibLnxUrl },
  ],
};

// --- 3. Thumbnail (miniatura animada, barra lateral) ------------------------

// Evoca el recorte vertical panning de un lado a otro: dos "personas"
// (círculos) sobre fondo oscuro, con un marco 9:16 barriendo entre ellas.
const thumbnail: ThumbnailDrawFn = (ctx, w, h, t) => {
  ctx.fillStyle = "#14161c";
  ctx.fillRect(0, 0, w, h);

  const cy = h * 0.5;
  const leftX = w * 0.28;
  const rightX = w * 0.72;
  ctx.fillStyle = "#3a4048";
  ctx.beginPath();
  ctx.arc(leftX, cy, h * 0.16, 0, Math.PI * 2);
  ctx.fill();
  ctx.beginPath();
  ctx.arc(rightX, cy, h * 0.16, 0, Math.PI * 2);
  ctx.fill();

  const swing = (Math.sin(t * 0.8) + 1) / 2;
  const frameCx = leftX + (rightX - leftX) * swing;
  const frameW = w * 0.34;
  const frameH = h * 0.92;

  ctx.strokeStyle = "#FFC83C";
  ctx.lineWidth = 1.5;
  ctx.strokeRect(frameCx - frameW / 2, (h - frameH) / 2, frameW, frameH);
};

// --- Paquete final -----------------------------------------------------------

export const REFRAME_MODULE: EffectModule<"reframe"> = { definition, codegen, thumbnail };
