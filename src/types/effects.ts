export type EffectId = "ascii" | "particles" | "crt" | "opencv";

export type ParamType = "float" | "int" | "bool" | "color" | "string" | "select";

export interface ParamSchema {
  key: string;
  label: string;
  type: ParamType;
  default: number | boolean | string;
  min?: number;
  max?: number;
  step?: number;
  options?: string[];
  group?: string;
  /** Only meaningful for type: "color" — when true, the value is an 8-digit
   * #RRGGBBAA hex string (with an alpha slider in the UI) instead of the
   * plain 6-digit #RRGGBB. */
  alpha?: boolean;
  /** If set, this control only shows in the Inspector while the effect's
   * own "mode" param is one of these values — purely visual (the
   * underlying param/value/behavior is unchanged either way, only whether
   * the control is rendered). Omit for controls that apply regardless of
   * mode. */
  modes?: string[];
}

export interface EffectDefinition {
  id: EffectId;
  name: string;
  description: string;
  params: ParamSchema[];
}

export type EffectParamValue = number | boolean | string;
export type EffectParams = Record<string, EffectParamValue>;

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

export type ExportFormat = "mp4" | "webm" | "png-sequence" | "mov-alpha" | "jpg" | "png" | "ascii-txt";

export interface ExportJobState {
  running: boolean;
  format: ExportFormat;
  progress: number; // 0..1 (frame capture progress)
  currentFrame: number;
  totalFrames: number;
  etaSeconds: number;
  error?: string;
  // Reported by video_export.js's status listener during the finishing
  // steps that happen after the last frame is captured (encode, virtual-FS
  // read-back, zip, download) — the part with no per-frame progress signal
  // of its own. "capturing" is the default while frames are being grabbed.
  phase?: "capturing" | "finalizing" | "encoding" | "reading" | "zipping" | "compressing" | "preparing" | "downloading" | "done" | "error";
  statusMessage?: string;
  // Post-capture progress (real percentage from compression/encoding operations)
  postCaptureProgress?: number; // 0..1 during post-processing phases
  // Estimated seconds remaining for the current post-capture phase, from
  // video_export.js's own timeElapsed/timeEstimated (ffmpeg's real
  // encoded-time-so-far, JSZip's real bytes-so-far, etc.) — same idea as
  // etaSeconds above, just for the phase after capture.
  postCaptureEtaSeconds?: number;
}

export const ASCII_EFFECT: EffectDefinition = {
  id: "ascii",
  name: "ASCII Renderer",
  description: "Converts the rendered frame into a live ASCII character grid. Supports normal luminance-mapping and Matrix mode (code rain).",
  params: [
    // --- Modo ---
    { key: "mode", label: "Mode", type: "select", default: "normal", options: ["normal", "matrix"], group: "Mode" },

    // --- Normal mode (rampa de luminancia) ---
    { key: "characters", label: "Characters", type: "string", default: " .:-=+*#%@", group: "Ramp", modes: ["normal"] },
    { key: "fontSize", label: "Font", type: "int", default: 10, min: 4, max: 32, step: 1, group: "Ramp" },
    { key: "brightness", label: "Brightness", type: "float", default: 0.8, min: 0, max: 2, step: 0.01, group: "Image", modes: ["normal"] },
    { key: "contrast", label: "Contrast", type: "float", default: 1.2, min: 0, max: 3, step: 0.01, group: "Image", modes: ["normal"] },
    { key: "gamma", label: "Gamma", type: "float", default: 1.1, min: 0.2, max: 3, step: 0.01, group: "Image", modes: ["normal"] },
    { key: "foreground", label: "Foreground Color", type: "color", default: "#44D4FF", group: "Color" },
    { key: "background", label: "Background Color", type: "color", default: "#0B0B0E00", alpha: true, group: "Color" },
    { key: "invert", label: "Invert", type: "bool", default: false, group: "Color", modes: ["normal"] },

    // --- Matrix mode (code rain) ---
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

export const PARTICLES_EFFECT: EffectDefinition = {
  id: "particles",
  name: "Particle System",
  description: "Multi-mode particle emitter: fountain (classic), rain (falling drops), or embers (rising glows) — all reactive to video content.",
  params: [
    // --- Modo y tipo de partículas ---
    { key: "mode", label: "Mode", type: "select", default: "rain", options: ["fountain", "rain", "embers"], group: "Type" },
    
    // --- Emisión ---
    { key: "count", label: "Particle Count", type: "int", default: 2000, min: 10, max: 20000, step: 10, group: "Emission" },
    { key: "spawnRate", label: "Spawn Rate", type: "float", default: 120, min: 0, max: 2000, step: 1, group: "Emission" },
    { key: "spread", label: "Spread (deg)", type: "float", default: 45, min: 0, max: 360, step: 1, group: "Emission" },
    { key: "spawnX", label: "Spawn X (fountain only)", type: "float", default: 0.5, min: 0, max: 1, step: 0.01, group: "Emission", modes: ["fountain"] },
    { key: "spawnY", label: "Spawn Y (fountain only)", type: "float", default: 0.8, min: 0, max: 1, step: 0.01, group: "Emission", modes: ["fountain"] },
    
    // --- Física ---
    { key: "gravity", label: "Gravity", type: "float", default: 9.8, min: -50, max: 50, step: 0.1, group: "Physics" },
    { key: "lifetime", label: "Lifetime", type: "float", default: 2.5, min: 0.1, max: 20, step: 0.1, group: "Physics" },
    { key: "wind", label: "Wind X", type: "float", default: 0, min: -50, max: 50, step: 0.5, group: "Physics", modes: ["rain", "embers"] },
    
    // --- Apariencia ---
    { key: "size", label: "Size", type: "float", default: 4, min: 0.5, max: 40, step: 0.1, group: "Appearance" },
    { key: "sizeFalloff", label: "Size Falloff", type: "float", default: 0.6, min: 0, max: 1, step: 0.01, group: "Appearance" },
    { key: "color", label: "Color", type: "color", default: "#44D4FF", group: "Appearance" },
    
    // --- Interactividad con video (NUEVO: ahora visible y activado por defecto) ---
    { key: "reactive", label: "React to Video", type: "bool", default: true, group: "Video Interaction" },
    { key: "reactiveStrength", label: "Interaction Strength", type: "float", default: 0.6, min: 0, max: 1, step: 0.01, group: "Video Interaction" },
    { key: "flowStrength", label: "Flow Field Strength", type: "float", default: 0.8, min: 0, max: 2, step: 0.01, group: "Video Interaction" },
  ],
};

export const CRT_EFFECT: EffectDefinition = {
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
  ],
};

export const OPENCV_EFFECT: EffectDefinition = {
  id: "opencv",
  name: "OpenCV Vision",
  description: "Computer-vision pipelines (edges, contours, optical flow, background subtraction, face detection) running natively via OpenCV compiled to WASM. Works on the loaded video or a live camera feed.",
  params: [
    { key: "mode", label: "Mode", type: "select", default: "edges", options: ["edges", "contours", "optical_flow", "bg_subtract", "face_detect"], group: "Mode" },
    { key: "processScale", label: "Process Scale", type: "float", default: 0.5, min: 0.1, max: 1, step: 0.05, group: "Performance" },
    { key: "mirror", label: "Mirror (front camera)", type: "bool", default: false, group: "Performance" },

    // --- edges ---
    { key: "cannyLow", label: "Canny Low", type: "float", default: 60, min: 0, max: 255, step: 1, group: "Edges", modes: ["edges"] },
    { key: "cannyHigh", label: "Canny High", type: "float", default: 160, min: 0, max: 255, step: 1, group: "Edges", modes: ["edges"] },
    { key: "blur", label: "Blur", type: "int", default: 1, min: 0, max: 10, step: 1, group: "Edges", modes: ["edges"] },
    { key: "edgeOnSource", label: "Overlay on Source", type: "bool", default: false, group: "Edges", modes: ["edges"] },
    { key: "edgeColor", label: "Edge Color", type: "color", default: "#44D4FF", group: "Edges", modes: ["edges"] },

    // --- contours ---
    { key: "contourMinArea", label: "Min Area", type: "float", default: 80, min: 0, max: 5000, step: 10, group: "Contours", modes: ["contours"] },
    { key: "contourThickness", label: "Thickness", type: "int", default: 2, min: 1, max: 10, step: 1, group: "Contours", modes: ["contours"] },
    { key: "contourFill", label: "Fill", type: "bool", default: false, group: "Contours", modes: ["contours"] },
    { key: "contourColor", label: "Contour Color", type: "color", default: "#44D4FF", group: "Contours", modes: ["contours"] },

    // --- optical flow ---
    { key: "flowStrength", label: "Flow Strength", type: "float", default: 1, min: 0.1, max: 3, step: 0.05, group: "Optical Flow", modes: ["optical_flow"] },
    { key: "flowArrows", label: "Arrows (vs. color)", type: "bool", default: false, group: "Optical Flow", modes: ["optical_flow"] },
    { key: "flowArrowStep", label: "Arrow Spacing", type: "int", default: 16, min: 4, max: 40, step: 1, group: "Optical Flow", modes: ["optical_flow"] },

    // --- background subtraction ---
    { key: "bgHistory", label: "History", type: "int", default: 120, min: 10, max: 500, step: 10, group: "Background Subtraction", modes: ["bg_subtract"] },
    { key: "bgVarThreshold", label: "Var Threshold", type: "float", default: 16, min: 4, max: 64, step: 1, group: "Background Subtraction", modes: ["bg_subtract"] },
    { key: "bgShadows", label: "Detect Shadows", type: "bool", default: true, group: "Background Subtraction", modes: ["bg_subtract"] },
    { key: "bgMaskOnly", label: "Mask Only", type: "bool", default: false, group: "Background Subtraction", modes: ["bg_subtract"] },

    // --- face detection ---
    { key: "faceScaleFactor", label: "Scale Factor", type: "float", default: 1.1, min: 1.05, max: 1.4, step: 0.01, group: "Face Detection", modes: ["face_detect"] },
    { key: "faceMinNeighbors", label: "Min Neighbors", type: "int", default: 4, min: 1, max: 10, step: 1, group: "Face Detection", modes: ["face_detect"] },
    { key: "faceMinSizeFraction", label: "Min Size (% width)", type: "float", default: 0.08, min: 0.02, max: 0.5, step: 0.01, group: "Face Detection", modes: ["face_detect"] },
    { key: "faceBoxColor", label: "Box Color", type: "color", default: "#78FF78", group: "Face Detection", modes: ["face_detect"] },
    { key: "faceShowCount", label: "Show Count", type: "bool", default: true, group: "Face Detection", modes: ["face_detect"] },
  ],
};

export const EFFECT_DEFINITIONS: Record<EffectId, EffectDefinition> = {
  ascii: ASCII_EFFECT,
  particles: PARTICLES_EFFECT,
  crt: CRT_EFFECT,
  opencv: OPENCV_EFFECT,
};

export function defaultParamsFor(effect: EffectId): EffectParams {
  const out: EffectParams = {};
  for (const p of EFFECT_DEFINITIONS[effect].params) out[p.key] = p.default;
  return out;
}
