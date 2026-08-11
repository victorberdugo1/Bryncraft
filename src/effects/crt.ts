import {
  fmtFloat,
  escapeCString,
  extractCString,
  type EffectDefinition,
  type EffectCodegenModule,
  type EffectParams,
  type EffectModule,
  type ThumbnailDrawFn,
} from "./shared";

import headerRaw from "../../native/effects/crt/crt_effect.h?raw";
import mainRaw from "../../native/effects/crt/main002.c?raw";
import readmeRaw from "../../native/effects/crt/README.md?raw";

// --- 1. Definición de parámetros (Inspector) --------------------------------

const definition: EffectDefinition<"crt"> = {
  id: "crt",
  name: "CRT",
  description: "Scanlines, barrel distortion, noise and chromatic aberration shader stack.",
  params: [
    { key: "scanlineIntensity", label: "Scanlines", type: "float", default: 0.35, min: 0, max: 1, step: 0.01, group: "Scanlines" },
    { key: "scanlineCount", label: "Scanline Density", type: "int", default: 480, min: 60, max: 1080, step: 1, group: "Scanlines" },
    { key: "scanlineSpeed", label: "Scanline Scroll", type: "float", default: 0, min: -1, max: 1, step: 0.01, group: "Scanlines" },
    { key: "curvature", label: "Barrel Distortion", type: "float", default: 0.15, min: 0, max: 1, step: 0.01, group: "Geometry" },
    { key: "vignette", label: "Vignette", type: "float", default: 0.3, min: 0, max: 1, step: 0.01, group: "Geometry" },
    { key: "noise", label: "Noise", type: "float", default: 0.05, min: 0, max: 1, step: 0.01, group: "Signal" },
    { key: "chromaticAberration", label: "Chromatic Aberration", type: "float", default: 0.4, min: 0, max: 5, step: 0.05, group: "Signal" },
    { key: "flicker", label: "Flicker", type: "float", default: 0.1, min: 0, max: 1, step: 0.01, group: "Signal" },
    { key: "ghosting", label: "Ghosting", type: "float", default: 0, min: 0, max: 1, step: 0.01, group: "Signal" },
    { key: "verticalRoll", label: "Vertical Roll", type: "float", default: 0, min: -1, max: 1, step: 0.01, group: "Geometry" },

    { key: "trackingGlitch", label: "Tracking Glitch", type: "float", default: 0, min: 0, max: 1, step: 0.01, group: "VHS Glitch" },
    { key: "waveDistortion", label: "Wave Distortion", type: "float", default: 0, min: 0, max: 5, step: 0.05, group: "VHS Glitch" },
    { key: "waveSpeed", label: "Wave Speed", type: "float", default: 1.5, min: 0, max: 10, step: 0.1, group: "VHS Glitch" },
    { key: "dropoutLines", label: "Dropout Lines", type: "float", default: 0, min: 0, max: 1, step: 0.01, group: "VHS Glitch" },
    { key: "jitter", label: "Jitter", type: "float", default: 0, min: 0, max: 5, step: 0.05, group: "VHS Glitch" },

    { key: "vhsOverlay", label: "VHS Overlay", type: "bool", default: false, group: "VHS Overlay" },
    { key: "vhsIcon", label: "Icon", type: "select", default: "none", options: ["none", "play", "pause", "rew", "ff", "stop", "rec"], group: "VHS Overlay" },
    { key: "vhsTimestamp", label: "Timestamp", type: "string", default: "", group: "VHS Overlay" },
    { key: "vhsLabel", label: "Label", type: "string", default: "SP", group: "VHS Overlay" },
  ],
};

// --- 2. Codegen ---------------------------------------------------------------

const CRT_VHS_ICON_ENUM: Record<string, string> = {
  none: "CRT_VHS_ICON_NONE",
  play: "CRT_VHS_ICON_PLAY",
  pause: "CRT_VHS_ICON_PAUSE",
  rew: "CRT_VHS_ICON_REW",
  ff: "CRT_VHS_ICON_FF",
  stop: "CRT_VHS_ICON_STOP",
  rec: "CRT_VHS_ICON_REC",
};

function buildParamsBlock(params: EffectParams): string {
  return `static CRT_Params CRT_g_params = {
    .scanlineIntensity   = ${fmtFloat(params.scanlineIntensity)},
    .scanlineCount       = ${fmtFloat(params.scanlineCount)},
    .scanlineSpeed       = ${fmtFloat(params.scanlineSpeed)},
    .curvature           = ${fmtFloat(params.curvature)},
    .vignette            = ${fmtFloat(params.vignette)},
    .noise               = ${fmtFloat(params.noise)},
    .chromaticAberration = ${fmtFloat(params.chromaticAberration)},
    .flicker             = ${fmtFloat(params.flicker)},

    .trackingGlitch      = ${fmtFloat(params.trackingGlitch)},
    .waveDistortion      = ${fmtFloat(params.waveDistortion)},
    .waveSpeed           = ${fmtFloat(params.waveSpeed)},
    .dropoutLines        = ${fmtFloat(params.dropoutLines)},
    .jitter              = ${fmtFloat(params.jitter)},
    .verticalRoll        = ${fmtFloat(params.verticalRoll)},
    .ghosting            = ${fmtFloat(params.ghosting)},

    .vhsOverlay   = ${params.vhsOverlay ? "true" : "false"},
    .vhsIcon      = ${CRT_VHS_ICON_ENUM[String(params.vhsIcon)] ?? "CRT_VHS_ICON_NONE"},
    .vhsTimestamp = "${escapeCString(params.vhsTimestamp)}",
    .vhsLabel     = "${escapeCString(params.vhsLabel)}",
};
`;
}

const codegen: EffectCodegenModule = {
  headerRaw,
  mainRaw,
  mainFilename: "main002.c",
  readmeRaw,
  paramsRegex: /static CRT_Params CRT_g_params = \{[\s\S]*?\};\r?\n/,
  buildParamsBlock,
  extras: [
    {
      filename: "crt.fs",
      label: "Fragment shader (GLSL 100 / WebGL1-ES)",
      kind: "text",
      content: extractCString(headerRaw, "CRT_FS_SOURCE"),
    },
  ],
};

// --- 3. Thumbnail --------------------------------------------------------------

const thumbnail: ThumbnailDrawFn = (ctx, w, h, t) => {
  const grad = ctx.createLinearGradient(0, 0, w, h);
  grad.addColorStop(0, "#1b3a44");
  grad.addColorStop(1, "#0b0b0e");
  ctx.fillStyle = grad;
  ctx.fillRect(0, 0, w, h);
  ctx.fillStyle = "rgba(0,0,0,0.35)";
  for (let y = 0; y < h; y += 3) ctx.fillRect(0, y, w, 1);
  ctx.strokeStyle = "#44D4FF";
  ctx.beginPath();
  for (let x = 0; x < w; x++) {
    const y = h / 2 + Math.sin(x * 0.15 + t * 3) * (h * 0.18);
    if (x === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();
};

// --- Paquete final -----------------------------------------------------------

export const CRT_MODULE: EffectModule<"crt"> = { definition, codegen, thumbnail };
