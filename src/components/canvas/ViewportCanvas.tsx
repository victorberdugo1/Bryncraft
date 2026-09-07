import { useLayoutEffect, useEffect, useRef } from "react";
import { useAppStore } from "@/store/useAppStore";
import { wasmBridge } from "@/lib/wasmBridge";
import { MockRenderer } from "@/lib/mockRenderer";
import { CameraCapture } from "@/lib/cameraCapture";
import { computeReframeOutputSize } from "@/lib/reframeOutputSize";

function getStage(el: HTMLElement): HTMLElement | null {
  return el.parentElement?.parentElement?.parentElement ?? null;
}

function stageCSSSize(el: HTMLElement): { width: number; height: number } {
  const stage = getStage(el);
  if (stage && stage.clientWidth > 0 && stage.clientHeight > 0) {
    return { width: stage.clientWidth, height: stage.clientHeight };
  }
  return { width: window.innerWidth, height: window.innerHeight };
}

function containerPixelSize(canvas: HTMLCanvasElement) {
  const dpr = Math.min(2, window.devicePixelRatio || 1);
  const css = stageCSSSize(canvas);
  return { width: Math.round(css.width * dpr), height: Math.round(css.height * dpr) };
}

function resolveCanvasSize(contentWidth: number, contentHeight: number): [number, number] {
  const state = useAppStore.getState();
  if (state.activeEffect !== "reframe") return [contentWidth, contentHeight];
  return computeReframeOutputSize(contentWidth, contentHeight, state.paramsByEffect.reframe?.aspectPreset);
}

function applyCanvasSize(
  canvas: HTMLCanvasElement,
  contentWidth: number,
  contentHeight: number,
) {
  if (contentWidth <= 0 || contentHeight <= 0) return;
  const [width, height] = resolveCanvasSize(contentWidth, contentHeight);

  // Set pixel dims and tell wasm
  if (canvas.width !== width || canvas.height !== height) {
    canvas.width = width;
    canvas.height = height;
    wasmBridge.setCanvasSize(width, height);
  }

  // CSS: fix the canvas to its exact pixel size in logical px.
  // The transformRef applies scale() on top, so "fit" = scale(1) means
  // the canvas renders at its natural size; we just need to stop it from
  // stretching to fill the flex container.
  const dpr = Math.min(2, window.devicePixelRatio || 1);
  const cssW = width / dpr;
  const cssH = height / dpr;

  // For "fit" zoom, scale the canvas so it fits within the stage.
  const { zoom } = useAppStore.getState();
  if (zoom === "fit") {
    const stage = stageCSSSize(canvas);
    const scale = Math.min(stage.width / cssW, stage.height / cssH, 1);
    canvas.style.width = `${Math.round(cssW * scale)}px`;
    canvas.style.height = `${Math.round(cssH * scale)}px`;
  } else {
    canvas.style.width = `${Math.round(cssW)}px`;
    canvas.style.height = `${Math.round(cssH)}px`;
  }
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
  const videoDimsRef = useRef<{ width: number; height: number } | null>(null);
  const contentDimsRef = useRef<{ width: number; height: number } | null>(null);

  const activeEffect = useAppStore((s) => s.activeEffect);
  const params = useAppStore((s) => s.paramsByEffect[s.activeEffect]);
  const zoom = useAppStore((s) => s.zoom);
  const setStats = useAppStore((s) => s.setStats);
  const videoFrames = useAppStore((s) => s.video.frames);
  const cameraActive = useAppStore((s) => s.camera.active);
  const cameraFacingMode = useAppStore((s) => s.camera.facingMode);
  const setCameraActive = useAppStore((s) => s.setCameraActive);
  const setCameraError = useAppStore((s) => s.setCameraError);

  useLayoutEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    let lastContent: { width: number; height: number } | null = null;

    const resize = () => {
      const srcDims = videoDimsRef.current;
      if (srcDims) {
        applyCanvasSize(canvas, srcDims.width, srcDims.height);
        return;
      }
      const { width, height } = containerPixelSize(canvas);
      if (width <= 0 || height <= 0) return;
      const [targetW, targetH] = resolveCanvasSize(width, height);
      const nativeInSync = canvas.width === targetW && canvas.height === targetH;
      const sameContent = lastContent?.width === width && lastContent?.height === height;
      if (sameContent && nativeInSync) return;
      lastContent = { width, height };
      contentDimsRef.current = { width, height };
      applyCanvasSize(canvas, width, height);
    };
    resize();

    const stage = getStage(canvas);
    const observer = new ResizeObserver(resize);
    observer.observe(stage ?? canvas);
    const unwatchDpr = watchDevicePixelRatio(resize);

    let disposed = false;
    wasmBridge.attach(canvas).then((mode) => {
      if (disposed) return;
      lastContent = null;
      resize();
      if (mode === "mock") {
        const renderer = new MockRenderer(canvas);
        renderer.setStatsListener(setStats);
        renderer.setEffect(useAppStore.getState().activeEffect, useAppStore.getState().paramsByEffect[useAppStore.getState().activeEffect]);
        renderer.start();
        rendererRef.current = renderer;
      } else {
        wasmBridge.onStats(setStats);
        requestAnimationFrame(() => {
          if (disposed) return;
          lastContent = null;
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

  useEffect(() => {
    if (rendererRef.current) {
      rendererRef.current.setEffect(activeEffect, params);
    } else {
      wasmBridge.updateParams(activeEffect, params);
    }
    const canvas = canvasRef.current;
    const contentDims = contentDimsRef.current;
    if (canvas && contentDims) {
      applyCanvasSize(canvas, contentDims.width, contentDims.height);
    }
  }, [activeEffect, params]);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const contentDims = contentDimsRef.current;
    if (!contentDims) return;
    const [w, h] = resolveCanvasSize(contentDims.width, contentDims.height);
    const dpr = Math.min(2, window.devicePixelRatio || 1);
    const cssW = w / dpr;
    const cssH = h / dpr;
    const { zoom: currentZoom } = useAppStore.getState();
    if (currentZoom === "fit") {
      const stage = stageCSSSize(canvas);
      const scale = Math.min(stage.width / cssW, stage.height / cssH, 1);
      canvas.style.width = `${Math.round(cssW * scale)}px`;
      canvas.style.height = `${Math.round(cssH * scale)}px`;
    } else {
      canvas.style.width = `${Math.round(cssW)}px`;
      canvas.style.height = `${Math.round(cssH)}px`;
    }
  }, [zoom]);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    videoDimsRef.current =
      videoFrames && videoFrames.length > 0
        ? { width: videoFrames[0].width, height: videoFrames[0].height }
        : null;

    const dims = videoDimsRef.current ?? containerPixelSize(canvas);
    contentDimsRef.current = dims;
    applyCanvasSize(canvas, dims.width, dims.height);

    rendererRef.current?.setSourceFrames(videoFrames);
    wasmBridge.setVideoFrames(videoFrames);
  }, [videoFrames]);

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
        contentDimsRef.current = videoDimsRef.current;
        applyCanvasSize(canvas, videoEl.videoWidth, videoEl.videoHeight);
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
      contentDimsRef.current = dims;
      applyCanvasSize(canvas, dims.width, dims.height);
    };
  }, [cameraActive, cameraFacingMode, setCameraActive, setCameraError]);

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

  return <canvas ref={canvasRef} id="canvas" className="block" />;
}
