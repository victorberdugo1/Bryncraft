import type { EffectParams, RenderMessage, ViewportOverlayStats, ExportFormat } from "@/types/effects";

/**
 * Contract expected from the compiled native/ Raylib+Emscripten module
 * (see native/main.c). Built with:
 *   -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']
 *   -s EXPORTED_FUNCTIONS=['_js_set_effect_json','_js_get_stats_json','_main']
 */
interface EmscriptenModule {
  canvas: HTMLCanvasElement;
  onRuntimeInitialized?: () => void;
  ccall: (name: string, ret: string, args: string[], vals: unknown[]) => unknown;
  HEAPU8?: Uint8Array;
}

declare global {
  interface Window {
    Module?: Partial<EmscriptenModule>;
    VideoExportJS?: {
      startEncoder: (w: number, h: number, fps: number, format?: ExportFormat) => boolean | Promise<boolean>;
      captureFrame: () => Promise<boolean>;
      finishEncoder: (filename: string) => Promise<void>;
      cancelRecording: () => void;
      // Optional: registers (phase, message) notifications for the
      // post-capture finishing steps (encode / read-back / zip / download).
      // Purely informational — never influences what video_export.js does.
      setStatusListener?: (fn: (phase: string, message: string) => void) => void;
      // Optional: registers progress updates during ffmpeg encoding
      setProgressListener?: (fn: (data: { percent: number; timeElapsed: number; timeEstimated: number }) => void) => void;
    };
  }
}

type StatsListener = (stats: ViewportOverlayStats) => void;
type ExportStatusListener = (phase: string, message: string) => void;
type ExportProgressListener = (data: { percent: number; timeElapsed: number; timeEstimated: number }) => void;

const WASM_GLUE_PATH = "/wasm/index.js"; // output of native/Makefile, copied into /public/wasm
const VIDEO_EXPORT_PATHS = ["/video_export.js", "/wasm/video_export.js"]; // export helper loaded from public assets

class WasmBridge {
  private module: Partial<EmscriptenModule> | null = null;
  private ready = false;
  private mode: "wasm" | "mock" | "unloaded" = "unloaded";
  private statsListeners = new Set<StatsListener>();
  private pollHandle: number | null = null;
  // The Emscripten glue script bootstraps global FS/heap state once per page;
  // loading it a second time (e.g. React StrictMode's mount/unmount/remount,
  // or a second ViewportCanvas instance) throws ErrnoError(20) inside
  // FS.staticInit. Cache the in-flight/resolved attach so it only ever runs
  // once per page load, no matter how many times attach() is called.
  private attachPromise: Promise<"wasm" | "mock"> | null = null;
  // video_export.js only needs a <canvas> in the DOM — it works the same in
  // wasm and mock mode — so it's loaded independently of doAttach()'s
  // wasm/mock branching, and memoized the same way for the same reason.
  private videoExportPromise: Promise<boolean> | null = null;
  // Source-video frames (see ViewportCanvas's videoFrames effect). In wasm
  // mode these never reach MockRenderer, so this bridge owns its own copy
  // plus the scratch canvas used to decode each ImageBitmap into RGBA8
  // before handing it to js_set_video_frame.
  private videoFrames: ImageBitmap[] | null = null;
  private videoFrameCanvas: HTMLCanvasElement | null = null;
  private videoFrameCtx: CanvasRenderingContext2D | null = null;

  get isReady() {
    return this.ready;
  }

  get activeMode() {
    return this.mode;
  }

  async attach(canvas: HTMLCanvasElement): Promise<"wasm" | "mock"> {
    if (this.attachPromise) return this.attachPromise;

    this.attachPromise = this.doAttach(canvas);
    return this.attachPromise;
  }

  private async doAttach(canvas: HTMLCanvasElement): Promise<"wasm" | "mock"> {
    // Fire-and-forget in parallel with the wasm/mock probe below — recording
    // doesn't need to block first paint, but it does need to be requested.
    this.loadVideoExport();

    const glueAvailable = await this.probeGlueScript();
    if (!glueAvailable) {
      this.mode = "mock";
      this.ready = true;
      return "mock";
    }

    return new Promise((resolve) => {
      window.Module = {
        canvas,
        onRuntimeInitialized: () => {
          this.module = window.Module ?? null;
          this.ready = true;
          this.mode = "wasm";
          this.startStatsPolling();
          resolve("wasm");
        },
      };
      const script = document.createElement("script");
      script.src = WASM_GLUE_PATH;
      script.async = true;
      script.onerror = () => {
        this.mode = "mock";
        this.ready = true;
        resolve("mock");
      };
      document.body.appendChild(script);
    });
  }

  private async probeGlueScript(): Promise<boolean> {
    try {
      const res = await fetch(WASM_GLUE_PATH, { method: "HEAD" });
      return res.ok;
    } catch {
      return false;
    }
  }

  /** Loads video_export.js once per page. Resolves true once window.VideoExportJS is set. */
  private loadVideoExport(): Promise<boolean> {
    if (this.videoExportPromise) return this.videoExportPromise;
    this.videoExportPromise = new Promise((resolve) => {
      if (window.VideoExportJS) {
        resolve(true);
        return;
      }

      const tryLoad = (index: number) => {
        if (index >= VIDEO_EXPORT_PATHS.length) {
          console.error("[wasmBridge] failed to load video export helper — recording is unavailable");
          resolve(false);
          return;
        }
        const script = document.createElement("script");
        script.src = VIDEO_EXPORT_PATHS[index];
        script.onload = () => resolve(!!window.VideoExportJS);
        script.onerror = () => tryLoad(index + 1);
        document.body.appendChild(script);
      };

      tryLoad(0);
    });
    return this.videoExportPromise;
  }

  sendEffect(message: RenderMessage) {
    if (this.mode !== "wasm" || !this.module) return;
    const json = JSON.stringify(message);
    this.module.ccall?.("js_set_effect_json", "void", ["string"], [json]);
  }

  updateParams(effect: RenderMessage["effect"], params: EffectParams) {
    this.sendEffect({ effect, params });
  }

  get hasVideoFrames() {
    return !!this.videoFrames?.length;
  }

  /** Resizes the wasm module's internal render resolution (RenderTexture +
   * GL viewport + backing canvas element) — a no-op outside wasm mode or
   * before the module is ready. Call this any time the <canvas> element's
   * width/height attributes change, so the two stay in sync. */
  setCanvasSize(width: number, height: number) {
    if (this.mode !== "wasm" || !this.module?.ccall || width <= 0 || height <= 0) return;
    this.module.ccall("js_set_canvas_size", "void", ["number", "number"], [width, height]);
  }

  /** Wasm-mode counterpart of MockRenderer.setSourceFrames — called from
   * ViewportCanvas whenever a video is loaded/cleared. */
  setVideoFrames(frames: ImageBitmap[] | null) {
    this.videoFrames = frames && frames.length ? frames : null;
    if (!this.videoFrames && this.mode === "wasm" && this.module) {
      this.module.ccall?.("js_clear_video_frame", "void", [], []);
    }
  }

  /** Wasm-mode counterpart of MockRenderer.setSourceFrameIndex — decodes one
   * ImageBitmap to RGBA8 and pushes it into the native texture via
   * js_set_video_frame. Called once per timeline tick from ViewportCanvas. */
  setVideoFrameIndex(index: number) {
    if (this.mode !== "wasm" || !this.module || !this.videoFrames?.length) return;
    const n = this.videoFrames.length;
    const clamped = ((index % n) + n) % n;
    const bitmap = this.videoFrames[clamped];
    if (!bitmap) return;

    if (!this.videoFrameCanvas) {
      this.videoFrameCanvas = document.createElement("canvas");
      this.videoFrameCtx = this.videoFrameCanvas.getContext("2d", { willReadFrequently: true });
    }
    const canvas = this.videoFrameCanvas;
    const ctx = this.videoFrameCtx;
    if (!ctx) return;

    if (canvas.width !== bitmap.width || canvas.height !== bitmap.height) {
      canvas.width = bitmap.width;
      canvas.height = bitmap.height;
    }
    ctx.drawImage(bitmap, 0, 0);
    const { data } = ctx.getImageData(0, 0, bitmap.width, bitmap.height);
    this.uploadRgbaToNative(data, bitmap.width, bitmap.height);
  }

  /** Shared by setVideoFrameIndex and pushCameraFrame — both end up with a
   * decoded RGBA8 buffer + dimensions that need to reach js_set_video_frame
   * the same way. See setVideoFrameIndex's comment for why this heap-
   * allocates (via malloc) instead of using ccall's "array" param type. */
  private uploadRgbaToNative(data: Uint8ClampedArray, width: number, height: number) {
    if (!this.module?.HEAPU8) return;
    const ccall = this.module.ccall;
    if (!ccall) return;
    const ptr = ccall("malloc", "number", ["number"], [data.byteLength]) as number;
    if (!ptr) return;
    try {
      this.module.HEAPU8.set(data, ptr);
      ccall("js_set_video_frame", "void", ["number", "number", "number"], [ptr, width, height]);
    } finally {
      ccall("free", "void", ["number"], [ptr]);
    }
  }

  /** Draws a live <video> element's current frame (fed by getUserMedia —
   * see src/lib/cameraCapture.ts) and pushes it into the native texture via
   * the exact same js_set_video_frame bridge used for decoded video-file
   * frames. main.c has no idea, and no need to know, whether a frame came
   * from a camera or a loaded file. Called once per rAF tick from
   * ViewportCanvas while the camera is active; a no-op outside wasm mode. */
  pushCameraFrame(videoEl: HTMLVideoElement) {
    if (this.mode !== "wasm" || !this.module) return;
    if (videoEl.readyState < 2 || !videoEl.videoWidth || !videoEl.videoHeight) return; // not enough data yet

    if (!this.videoFrameCanvas) {
      this.videoFrameCanvas = document.createElement("canvas");
      this.videoFrameCtx = this.videoFrameCanvas.getContext("2d", { willReadFrequently: true });
    }
    const canvas = this.videoFrameCanvas;
    const ctx = this.videoFrameCtx;
    if (!ctx) return;

    if (canvas.width !== videoEl.videoWidth || canvas.height !== videoEl.videoHeight) {
      canvas.width = videoEl.videoWidth;
      canvas.height = videoEl.videoHeight;
    }
    ctx.drawImage(videoEl, 0, 0);
    const { data } = ctx.getImageData(0, 0, canvas.width, canvas.height);
    this.uploadRgbaToNative(data, canvas.width, canvas.height);
  }

  /** Counterpart to pushCameraFrame — clears the native video texture when
   * the camera stops. Deliberately separate from setVideoFrames(null): the
   * camera never populates this.videoFrames, so reusing that setter would
   * be misleading about what's actually being cleared. */
  clearCameraFrame() {
    if (this.mode !== "wasm" || !this.module) return;
    this.module.ccall?.("js_clear_video_frame", "void", [], []);
  }

  async startRecording(width: number, height: number, fps: number, format: ExportFormat): Promise<boolean> {
    const loaded = await this.loadVideoExport();
    if (!loaded || !window.VideoExportJS) {
      console.error("[wasmBridge] startRecording: VideoExportJS is not available");
      return false;
    }
    try {
      window.Module?.ccall?.("js_start_export", "void", ["number", "number", "number"], [width, height, fps]);
    } catch (error) {
      console.warn("[wasmBridge] js_start_export bridge unavailable, continuing with JS-only recording", error);
    }

    const started = await window.VideoExportJS.startEncoder(width, height, fps, format);
    if (!started) {
      console.error("[wasmBridge] startRecording: VideoExportJS could not start the recorder");
      return false;
    }

    return true;
  }

  /** Captures the current frame and confirms it's genuinely non-blank
   * before resolving (see public/video_export.js) — returns false if it
   * never could get a valid frame, so the export loop can retry or abort
   * instead of silently moving on with a missing/empty frame. */
  async captureFrame(): Promise<boolean> {
    const loaded = await this.loadVideoExport();
    if (!loaded || !window.VideoExportJS) {
      return false;
    }
    const result = await window.VideoExportJS.captureFrame?.();
    return result !== false;
  }

  /** Ground-truth count of frames raylib has actually drawn AND presented
   * (see js_get_presented_frame_count in native/main.c). The export loop
   * polls this to confirm a real new frame landed, instead of guessing from
   * canvas pixels — pixel sampling can't tell "stably rendered" apart from
   * "stably cleared to blank", which is what caused empty PNG exports.
   * Returns null outside wasm mode (mock renderer has no such signal). */
  getPresentedFrameCount(): number | null {
    if (this.mode !== "wasm" || !this.module?.ccall) return null;
    try {
      return this.module.ccall("js_get_presented_frame_count", "number", [], []) as number;
    } catch {
      return null;
    }
  }

  /** Subscribes to (phase, message) notifications for everything that
   * happens after the last frame is captured — encode, virtual-FS
   * read-back, zipping, handing the blob to the browser. This is a thin
   * pass-through to video_export.js's own listener registration; it does
   * not wrap, delay, or otherwise touch the encode calls themselves.
   * Returns an unsubscribe function. Safe to call before VideoExportJS has
   * loaded — the listener is attached as soon as it becomes available, and
   * a no-op unsubscribe is returned if loading never completes. */
  onExportStatus(listener: ExportStatusListener): () => void {
    let cancelled = false;
    this.loadVideoExport().then((loaded) => {
      if (cancelled || !loaded || !window.VideoExportJS?.setStatusListener) return;
      window.VideoExportJS.setStatusListener(listener);
    });
    return () => {
      cancelled = true;
      if (window.VideoExportJS?.setStatusListener) {
        window.VideoExportJS.setStatusListener(() => {});
      }
    };
  }

  /** Subscribes to { percent, timeElapsed, timeEstimated } updates from
   * video_export.js — covers both frame-capture progress and post-capture
   * (encode/zip) progress. Same safe-registration pattern as
   * onExportStatus: video_export.js is loaded lazily (only once
   * startRecording/captureFrame/etc. is first called), so a caller that
   * registers this on mount — well before any of those run — needs the
   * listener attached the instant loading finishes, not attempted once
   * against a VideoExportJS that doesn't exist yet. */
  onExportProgress(listener: ExportProgressListener): () => void {
    let cancelled = false;
    this.loadVideoExport().then((loaded) => {
      if (cancelled || !loaded || !window.VideoExportJS?.setProgressListener) return;
      window.VideoExportJS.setProgressListener(listener);
    });
    return () => {
      cancelled = true;
      if (window.VideoExportJS?.setProgressListener) {
        window.VideoExportJS.setProgressListener(() => {});
      }
    };
  }

  async stopRecording(filename: string) {
    const loaded = await this.loadVideoExport();
    if (!loaded || !window.VideoExportJS) {
      console.error("[wasmBridge] stopRecording: VideoExportJS is not available");
      return;
    }
    try {
      window.Module?.ccall?.("js_stop_export", "void", [], []);
    } catch (error) {
      console.warn("[wasmBridge] js_stop_export bridge unavailable", error);
    }
    await window.VideoExportJS.finishEncoder(filename);
  }

  cancelRecording() {
    try {
      window.Module?.ccall?.("js_stop_export", "void", [], []);
    } catch (error) {
      console.warn("[wasmBridge] js_stop_export bridge unavailable during cancel", error);
    }
    window.VideoExportJS?.cancelRecording();
  }

  onStats(listener: StatsListener): () => void {
    this.statsListeners.add(listener);
    return () => this.statsListeners.delete(listener);
  }

  emitStats(stats: ViewportOverlayStats) {
    this.statsListeners.forEach((l) => l(stats));
  }

  private startStatsPolling() {
    if (this.pollHandle !== null) return;
    const poll = () => {
      if (this.module?.ccall) {
        try {
          const raw = this.module.ccall("js_get_stats_json", "string", [], []) as string;
          if (raw) this.emitStats(JSON.parse(raw));
        } catch {
          /* module not ready yet for this call */
        }
      }
      this.pollHandle = requestAnimationFrame(poll);
    };
    this.pollHandle = requestAnimationFrame(poll);
  }

  dispose() {
    if (this.pollHandle !== null) cancelAnimationFrame(this.pollHandle);
    this.pollHandle = null;
    this.statsListeners.clear();
    this.videoFrames = null;
  }
}

export const wasmBridge = new WasmBridge();