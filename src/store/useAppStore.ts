import { create } from "zustand";
import {
  defaultParamsFor,
  type EffectId,
  type EffectParams,
  type ExportFormat,
  type ExportJobState,
  type ViewportOverlayStats,
} from "@/types/effects";
import { extractVideoFrames } from "@/lib/videoFrameExtractor";
import { deriveTrailColor } from "@/lib/color";

export type ZoomMode = "fit" | "50" | "100" | "200" | "custom";

const ZOOM_SCALE_MIN = 0.1;
const ZOOM_SCALE_MAX = 8;

function clampZoomScale(v: number) {
  return Math.min(ZOOM_SCALE_MAX, Math.max(ZOOM_SCALE_MIN, v));
}
export type MobileTab = "preview" | "parameters" | "code" | "export";
export type CodeTab = "code" | "main" | "shader" | "readme";

interface TimelineState {
  playing: boolean;
  currentFrame: number;
  durationSeconds: number;
  fps: number;
}

interface AppState {
  activeEffect: EffectId;
  paramsByEffect: Record<EffectId, EffectParams>;

  zoom: ZoomMode;
  /** Actual CSS scale factor applied to the viewport (1 = 100%). Presets
   * (fit/50/100/200) set this to a fixed value; the mouse wheel adjusts it
   * continuously and flips `zoom` to "custom". */
  zoomScale: number;
  pan: { x: number; y: number };
  stats: ViewportOverlayStats;

  timeline: TimelineState;

  codeTab: CodeTab;
  mobileTab: MobileTab;

  searchQuery: string;

  exportJob: ExportJobState;

  leftSidebarOpen: boolean;
  rightPanelOpen: boolean;
  bottomPanelOpen: boolean;

  video: {
    frames: ImageBitmap[] | null;
    loading: boolean;
    progress: number;
    error: string | null;
  };

  /** Live camera feed (getUserMedia), mutually exclusive with a loaded
   * video file — see setCameraActive/loadVideo, which clear one another.
   * The actual MediaStream/video element lifecycle lives in
   * src/lib/cameraCapture.ts + ViewportCanvas; the store only tracks
   * on/off + last error for the UI. */
  camera: { active: boolean; error: string | null };

  setActiveEffect: (effect: EffectId) => void;
  setParam: (effect: EffectId, key: string, value: EffectParams[string]) => void;
  resetParams: (effect: EffectId) => void;

  setZoom: (z: ZoomMode) => void;
  setZoomScale: (scale: number) => void;
  setPan: (p: { x: number; y: number }) => void;
  panBy: (dx: number, dy: number) => void;
  resetView: () => void;
  setStats: (s: ViewportOverlayStats) => void;

  togglePlay: () => void;
  setPlaying: (v: boolean) => void;
  setCurrentFrame: (f: number) => void;
  restartTimeline: () => void;

  setCodeTab: (t: CodeTab) => void;
  setMobileTab: (t: MobileTab) => void;
  setSearchQuery: (q: string) => void;

  startExport: (format: ExportFormat, totalFrames: number) => void;
  updateExportProgress: (currentFrame: number, etaSeconds: number) => void;
  updateExportStatus: (phase: NonNullable<ExportJobState["phase"]>, message: string) => void;
  updateExportPostCaptureProgress: (progress: number, etaSeconds?: number) => void;
  finishExport: () => void;
  cancelExport: () => void;

  toggleLeftSidebar: () => void;
  toggleRightPanel: () => void;
  toggleBottomPanel: () => void;

  loadVideo: (file: File) => Promise<void>;
  clearVideo: () => void;

  setCameraActive: (active: boolean) => void;
  setCameraError: (error: string | null) => void;
}

export const useAppStore = create<AppState>((set) => ({
  activeEffect: "ascii",
  paramsByEffect: {
    ascii: defaultParamsFor("ascii"),
    particles: defaultParamsFor("particles"),
    crt: defaultParamsFor("crt"),
    opencv: defaultParamsFor("opencv"),
  },

  zoom: "fit",
  zoomScale: 1,
  pan: { x: 0, y: 0 },
  stats: { fps: 0, resolutionW: 0, resolutionH: 0, frame: 0, effect: "ascii", gpuFrameTimeMs: 0 },

  timeline: { playing: false, currentFrame: 0, durationSeconds: 10, fps: 60 },

  codeTab: "code",
  mobileTab: "preview",
  searchQuery: "",

  exportJob: { running: false, format: "mp4", progress: 0, currentFrame: 0, totalFrames: 0, etaSeconds: 0, phase: "capturing", statusMessage: undefined, postCaptureProgress: 0, postCaptureEtaSeconds: 0 },

  leftSidebarOpen: true,
  rightPanelOpen: true,
  bottomPanelOpen: true,

  video: { frames: null, loading: false, progress: 0, error: null },
  camera: { active: false, error: null },

  setActiveEffect: (effect) => set({ activeEffect: effect }),

  setParam: (effect, key, value) =>
    set((s) => {
      const current = s.paramsByEffect[effect];
      const next: EffectParams = { ...current, [key]: value };
      // Mover el color de la cabeza del efecto Matrix arrastra también el
      // color del resto de la estela (foreground), para que el conjunto
      // siga leyéndose como un solo efecto coherente. Al revés no: mover
      // Foreground Color a mano no toca el Head Color.
      if (effect === "ascii" && key === "matrixHeadColor" && typeof value === "string") {
        next.foreground = deriveTrailColor(value);
      }
      return {
        paramsByEffect: { ...s.paramsByEffect, [effect]: next },
      };
    }),

  resetParams: (effect) =>
    set((s) => ({
      paramsByEffect: { ...s.paramsByEffect, [effect]: defaultParamsFor(effect) },
    })),

  setZoom: (z) => {
    const presetScale: Partial<Record<ZoomMode, number>> = { fit: 1, "50": 0.5, "100": 1, "200": 2 };
    set({ zoom: z, zoomScale: presetScale[z] ?? 1, pan: { x: 0, y: 0 } });
  },
  setZoomScale: (scale) => set({ zoom: "custom", zoomScale: clampZoomScale(scale) }),
  setPan: (p) => set({ pan: p }),
  panBy: (dx, dy) => set((s) => ({ pan: { x: s.pan.x + dx, y: s.pan.y + dy } })),
  resetView: () => set({ zoom: "fit", zoomScale: 1, pan: { x: 0, y: 0 } }),
  setStats: (stats) => set({ stats }),

  togglePlay: () => set((s) => ({ timeline: { ...s.timeline, playing: !s.timeline.playing } })),
  setPlaying: (v) => set((s) => ({ timeline: { ...s.timeline, playing: v } })),
  setCurrentFrame: (f) => set((s) => ({ timeline: { ...s.timeline, currentFrame: f } })),
  restartTimeline: () => set((s) => ({ timeline: { ...s.timeline, currentFrame: 0 } })),

  setCodeTab: (t) => set({ codeTab: t }),
  setMobileTab: (t) => set({ mobileTab: t }),
  setSearchQuery: (q) => set({ searchQuery: q }),

  startExport: (format: ExportFormat, totalFrames: number) =>
    set({
      exportJob: {
        running: true,
        format,
        progress: 0,
        currentFrame: 0,
        totalFrames,
        etaSeconds: 0,
        phase: "capturing",
        statusMessage: undefined,
        postCaptureProgress: 0,
        postCaptureEtaSeconds: 0,
      },
    }),
  updateExportProgress: (currentFrame: number, etaSeconds: number) =>
    set((s) => ({
      exportJob: {
        ...s.exportJob,
        currentFrame,
        etaSeconds,
        progress: s.exportJob.totalFrames > 0 ? currentFrame / s.exportJob.totalFrames : 0,
      },
    })),
  // Driven by wasmBridge.onExportStatus (see ExportPanel), which forwards
  // video_export.js's own status-listener notifications. Only updates the
  // phase/message shown in the UI — never touches progress/currentFrame,
  // which stay owned by updateExportProgress during capture.
  updateExportStatus: (phase: NonNullable<ExportJobState["phase"]>, message: string) =>
    set((s) => ({ exportJob: { ...s.exportJob, phase, statusMessage: message } })),
  // Track post-capture progress (0..1) from compression/encoding operations
  updateExportPostCaptureProgress: (progress: number, etaSeconds?: number) =>
    set((s) => ({
      exportJob: {
        ...s.exportJob,
        postCaptureProgress: Math.min(1, Math.max(0, progress)),
        postCaptureEtaSeconds: etaSeconds ?? s.exportJob.postCaptureEtaSeconds,
      },
    })),
  finishExport: () => set((s) => ({ exportJob: { ...s.exportJob, running: false, progress: 1, phase: "done" } })),
  cancelExport: () =>
    set((s) => ({ exportJob: { ...s.exportJob, running: false, error: "Cancelled by user" } })),

  toggleLeftSidebar: () => set((s) => ({ leftSidebarOpen: !s.leftSidebarOpen })),
  toggleRightPanel: () => set((s) => ({ rightPanelOpen: !s.rightPanelOpen })),
  toggleBottomPanel: () => set((s) => ({ bottomPanelOpen: !s.bottomPanelOpen })),

  loadVideo: async (file: File) => {
    // Loading a video file and a live camera are mutually exclusive
    // sources — see setCameraActive for the other direction.
    set({ camera: { active: false, error: null }, video: { frames: null, loading: true, progress: 0, error: null } });
    try {
      const result = await extractVideoFrames(file, (done, total) => {
        set((s) => ({ video: { ...s.video, progress: total > 0 ? done / total : 0 } }));
      });
      set({
        video: { frames: result.frames, loading: false, progress: 1, error: null },
        timeline: {
          playing: false,
          currentFrame: 0,
          fps: result.fps,
          durationSeconds: result.frames.length / result.fps,
        },
      });
    } catch (err) {
      set({
        video: { frames: null, loading: false, progress: 0, error: err instanceof Error ? err.message : "Error al cargar el video" },
      });
    }
  },

  clearVideo: () =>
    set({
      video: { frames: null, loading: false, progress: 0, error: null },
      timeline: { playing: false, currentFrame: 0, durationSeconds: 10, fps: 60 },
    }),

  // Actual getUserMedia/MediaStream lifecycle lives in ViewportCanvas (via
  // src/lib/cameraCapture.ts) — this just toggles the on/off state it
  // reacts to, and clears a loaded video since the two sources conflict.
  setCameraActive: (active) =>
    set((s) => ({
      camera: { active, error: active ? null : s.camera.error },
      video: active ? { frames: null, loading: false, progress: 0, error: null } : s.video,
    })),
  setCameraError: (error) => set((s) => ({ camera: { ...s.camera, active: error ? false : s.camera.active, error } })),
}));

export function useActiveParams() {
  const activeEffect = useAppStore((s) => s.activeEffect);
  return useAppStore((s) => s.paramsByEffect[activeEffect]);
}
