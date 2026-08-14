import { useLayoutEffect, useEffect, useRef } from "react";
import { useAppStore } from "@/store/useAppStore";
import { wasmBridge } from "@/lib/wasmBridge";
import { MockRenderer } from "@/lib/mockRenderer";
import { CameraCapture } from "@/lib/cameraCapture";

function containerPixelSize(canvas: HTMLCanvasElement) {
  const dpr = Math.min(2, window.devicePixelRatio || 1);
  return { width: Math.round(canvas.clientWidth * dpr), height: Math.round(canvas.clientHeight * dpr) };
}

function watchDevicePixelRatio(onChange: () => void) {
  let mql = matchMedia(`(resolution: ${window.devicePixelRatio}dppx)`);
  const handler = () => {
    onChange();
    mql.removeEventListener("change", handler);
    mql = matchMedia(`(resolution: ${window.devicePixelRatio}dppx)`);
    mql.addEventListener("change", handler);
  };
  mql.addEventListener("change", handler);
  return () => mql.removeEventListener("change", handler);
}

export function ViewportCanvas() {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const rendererRef = useRef<MockRenderer | null>(null);
  // Native render resolution. Normally tracks the container's CSS size (see
  // the ResizeObserver-driven resync below); while a video is loaded it's
  // pinned to the video's own pixel dimensions instead, so the full frame
  // renders — and every effect, which samples g_sceneTarget at whatever
  // size it was drawn at — covers the whole thing instead of a
  // cropped/stretched slice.
  const videoDimsRef = useRef<{ width: number; height: number } | null>(null);

  const activeEffect = useAppStore((s) => s.activeEffect);
  const params = useAppStore((s) => s.paramsByEffect[s.activeEffect]);
  const setStats = useAppStore((s) => s.setStats);
  const videoFrames = useAppStore((s) => s.video.frames);
  const cameraActive = useAppStore((s) => s.camera.active);
  const cameraFacingMode = useAppStore((s) => s.camera.facingMode);
  const setCameraActive = useAppStore((s) => s.setCameraActive);
  const setCameraError = useAppStore((s) => s.setCameraError);

  // Sized synchronously before first paint (useLayoutEffect, not useEffect)
  // so the canvas never briefly renders at its default 300x150 box on
  // mount — every subsequent layout change (panel resize, window resize,
  // moving the window to a display with a different DPR, fonts finishing
  // load) is caught by the ResizeObserver/DPR watcher below.
  useLayoutEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    // Tracks the size we last told the container about, independently of
    // reading back canvas.width/height — the wasm module's own GL/canvas
    // bootstrapping can rewrite those attributes to match its internal
    // render target without going through js_set_canvas_size, which made
    // comparing against the live attribute a false "already in sync" and
    // left the native render target permanently narrower than the visible
    // canvas (content cut off, checker background showing through the
    // uncovered strip) until something else happened to nudge the values
    // out of agreement again.
    let lastSize: { width: number; height: number } | null = null;

    const resize = () => {
      if (videoDimsRef.current) return; // video drives resolution while active
      const { width, height } = containerPixelSize(canvas);
      if (width <= 0 || height <= 0) return; // parent not laid out yet — wait for a real measurement
      const unchanged = lastSize?.width === width && lastSize?.height === height;
      const nativeInSync = canvas.width === width && canvas.height === height;
      if (unchanged && nativeInSync) return;
      lastSize = { width, height };
      canvas.width = width;
      canvas.height = height;
      wasmBridge.setCanvasSize(width, height);
    };
    resize();

    const observer = new ResizeObserver(resize);
    observer.observe(canvas);
    const unwatchDpr = watchDevicePixelRatio(resize);

    let disposed = false;
    wasmBridge.attach(canvas).then((mode) => {
      if (disposed) return;
      lastSize = null; // force a fresh sync: the module's own boot may have rewritten canvas.width/height
      resize(); // re-check: layout may have settled further during the async attach()
      if (mode === "mock") {
        const renderer = new MockRenderer(canvas);
        renderer.setStatsListener(setStats);
        renderer.setEffect(useAppStore.getState().activeEffect, useAppStore.getState().paramsByEffect[useAppStore.getState().activeEffect]);
        renderer.start();
        rendererRef.current = renderer;
      } else {
        wasmBridge.onStats(setStats);
        // main()/InitWindow can run asynchronously just after attach()
        // resolves and silently rewrite canvas.width/height to its own
        // fixed startup resolution — resync a couple of frames out to
        // catch that without needing to poll indefinitely.
        requestAnimationFrame(() => {
          if (disposed) return;
          lastSize = null;
          resize();
        });
      }
    });

    return () => {
      disposed = true;
      observer.disconnect();
      unwatchDpr();
      rendererRef.current?.stop();
      rendererRef.current = null;
      wasmBridge.dispose();
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // push param/effect changes to whichever backend is active
  useEffect(() => {
    if (rendererRef.current) {
      rendererRef.current.setEffect(activeEffect, params);
    } else {
      wasmBridge.updateParams(activeEffect, params);
    }
  }, [activeEffect, params]);

  // when a video is loaded/cleared, size the canvas to the video's native
  // resolution (or back to the container's, once cleared) and hand the
  // frames to whichever backend is active so the effect samples the video
  // instead of the synthetic startup scene (mock reads ImageBitmaps
  // directly; wasm decodes them to RGBA8)
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    videoDimsRef.current =
      videoFrames && videoFrames.length > 0
        ? { width: videoFrames[0].width, height: videoFrames[0].height }
        : null;

    const dims = videoDimsRef.current ?? containerPixelSize(canvas);
    canvas.width = dims.width;
    canvas.height = dims.height;
    wasmBridge.setCanvasSize(dims.width, dims.height);

    rendererRef.current?.setSourceFrames(videoFrames);
    wasmBridge.setVideoFrames(videoFrames);
  }, [videoFrames]);

  // Camera lifecycle: mirrors the video-file effect above, but for a live
  // getUserMedia feed instead of pre-decoded frames. Runs its own rAF loop
  // (independent of the timeline-driven one below, since a camera has no
  // scrub position — it just always shows "now") that pushes the current
  // frame to whichever backend is active every tick.
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || !cameraActive) return;

    let cancelled = false;
    let rafId: number | null = null;
    const capture = new CameraCapture();

    capture
      .start(cameraFacingMode)
      .then((videoEl) => {
        if (cancelled) return;
        videoDimsRef.current = { width: videoEl.videoWidth, height: videoEl.videoHeight };
        canvas.width = videoEl.videoWidth;
        canvas.height = videoEl.videoHeight;
        wasmBridge.setCanvasSize(videoEl.videoWidth, videoEl.videoHeight);
        rendererRef.current?.setCameraSource(videoEl);

        const tick = () => {
          if (rendererRef.current) {
            rendererRef.current.pushCameraFrame();
          } else {
            wasmBridge.pushCameraFrame(videoEl);
          }

          // Nada más que empujar por rAF: la detección de manos del efecto
          // "touchdesigner" corre sola en C (OpenCV) leyendo el mismo frame
          // que pushCameraFrame acaba de mandar arriba — ver TD_DetectHands
          // en native/effects/touchdesigner/touchdesigner_effect.h.

          rafId = requestAnimationFrame(tick);
        };
        rafId = requestAnimationFrame(tick);
      })
      .catch((err) => {
        if (cancelled) return;
        console.error("[ViewportCanvas] camera start failed", err);
        setCameraError(err instanceof Error ? err.message : "No se pudo acceder a la cámara");
        setCameraActive(false);
      });

    return () => {
      cancelled = true;
      if (rafId !== null) cancelAnimationFrame(rafId);
      capture.stop();
      rendererRef.current?.setCameraSource(null);
      wasmBridge.clearCameraFrame();
      videoDimsRef.current = null;
      const dims = containerPixelSize(canvas);
      canvas.width = dims.width;
      canvas.height = dims.height;
      wasmBridge.setCanvasSize(dims.width, dims.height);
    };
  }, [cameraActive, cameraFacingMode, setCameraActive, setCameraError]);

  // once video frames are active, drive the sampled frame from the shared
  // timeline (play/pause/scrub) instead of the renderer's own free-running clock
  useEffect(() => {
    let lastFrame = -1;
    const unsubscribe = useAppStore.subscribe((state) => {
      const renderer = rendererRef.current;
      const cf = state.timeline.currentFrame;
      if (cf === lastFrame) return;

      if (renderer?.hasSourceFrames) {
        lastFrame = cf;
        renderer.setSourceFrameIndex(cf);
      } else if (wasmBridge.hasVideoFrames) {
        lastFrame = cf;
        wasmBridge.setVideoFrameIndex(cf);
      }
    });
    return unsubscribe;
  }, []);

  return <canvas ref={canvasRef} id="canvas" className="h-full w-full" />;
}
