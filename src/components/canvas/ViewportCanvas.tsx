import { useEffect, useRef } from "react";
import { useAppStore } from "@/store/useAppStore";
import { wasmBridge } from "@/lib/wasmBridge";
import { MockRenderer } from "@/lib/mockRenderer";
import { CameraCapture } from "@/lib/cameraCapture";

function containerPixelSize(canvas: HTMLCanvasElement) {
  // clientWidth/clientHeight reflect the untransformed CSS layout box.
  // getBoundingClientRect(), by contrast, returns the box AFTER the
  // ancestor `transformRef` wrapper's `translate()/scale()` is applied —
  // since that's a live pan/zoom transform, reading it here bakes the
  // current zoom level into the raster buffer size instead of the stage's
  // true, untransformed pixel size — which is what left the mock render
  // off-center until a manual panel resize forced a fresh, correct read.
  const dpr = Math.min(2, window.devicePixelRatio || 1);
  return { width: Math.round(canvas.clientWidth * dpr), height: Math.round(canvas.clientHeight * dpr) };
}

export function ViewportCanvas() {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const rendererRef = useRef<MockRenderer | null>(null);
  // Native render resolution. Normally tracks the container's CSS size (see
  // the ResizeObserver below); while a video is loaded it's pinned to the
  // video's own pixel dimensions instead, so the full frame renders — and
  // every effect, which samples g_sceneTarget at whatever size it was
  // drawn at — covers the whole thing instead of a cropped/stretched slice.
  const videoDimsRef = useRef<{ width: number; height: number } | null>(null);

  const activeEffect = useAppStore((s) => s.activeEffect);
  const params = useAppStore((s) => s.paramsByEffect[s.activeEffect]);
  const setStats = useAppStore((s) => s.setStats);
  const videoFrames = useAppStore((s) => s.video.frames);
  const cameraActive = useAppStore((s) => s.camera.active);
  const setCameraActive = useAppStore((s) => s.setCameraActive);
  const setCameraError = useAppStore((s) => s.setCameraError);

  // attach once
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const resize = () => {
      if (videoDimsRef.current) return; // video drives resolution while active
      const { width, height } = containerPixelSize(canvas);
      if (width <= 0 || height <= 0) return; // parent not laid out yet — wait for a real measurement
      if (canvas.width === width && canvas.height === height) return;
      canvas.width = width;
      canvas.height = height;
      wasmBridge.setCanvasSize(width, height);
    };
    resize();
    // Observing the canvas itself is unreliable here: it's a replaced element
    // sized via h-full/w-full (percentage), and its box is a *consequence* of
    // the flex layout resolving in the resizable shell (AppShell's
    // left/right/code panels). That resolution can land a frame after this
    // effect runs, and browsers don't always re-fire ResizeObserver for a
    // canvas whose own box only changed because an ancestor's flex-basis
    // changed in the same layout pass. Observing the canvas's parent
    // container picks up every real resize — including the very first
    // layout — instead of only reacting once the user manually drags a
    // ResizeHandle and forces a fresh reflow.
    const resizeTarget = canvas.parentElement ?? canvas;
    const ro = new ResizeObserver(resize);
    ro.observe(resizeTarget);
    // Layout can still settle a frame or two late (webfonts, first paint of
    // sibling panels). Re-check for the next few frames after mount so the
    // canvas locks to its final size without needing a manual resize.
    let rafId = requestAnimationFrame(function settle(framesLeft = 3) {
      resize();
      if (framesLeft > 0) rafId = requestAnimationFrame(() => settle(framesLeft - 1));
    });

    let disposed = false;
    wasmBridge.attach(canvas).then((mode) => {
      if (disposed) return;
      if (mode === "mock") {
        resize(); // re-check: layout may have settled further during the async attach()
        const renderer = new MockRenderer(canvas);
        renderer.setStatsListener(setStats);
        renderer.setEffect(useAppStore.getState().activeEffect, useAppStore.getState().paramsByEffect[useAppStore.getState().activeEffect]);
        renderer.start();
        rendererRef.current = renderer;
      } else {
        wasmBridge.onStats(setStats);
        // The module just booted at its fixed startup resolution — sync it
        // to whatever the canvas is already sized to (container, or video
        // if one finished loading before attach resolved).
        wasmBridge.setCanvasSize(canvas.width, canvas.height);
      }
    });

    return () => {
      disposed = true;
      cancelAnimationFrame(rafId);
      ro.disconnect();
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
      .start()
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
  }, [cameraActive, setCameraActive, setCameraError]);

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
