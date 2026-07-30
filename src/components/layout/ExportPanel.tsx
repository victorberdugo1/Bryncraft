import { useEffect, useRef, useState } from "react";
import {
  Dialog,
  DialogContent,
  DialogTitle,
  DialogDescription,
} from "@/components/ui/dialog";
import { Button } from "@/components/ui/button";
import { Progress } from "@/components/ui/progress";
import { useAppStore } from "@/store/useAppStore";
import type { ExportFormat } from "@/types/effects";
import { wasmBridge } from "@/lib/wasmBridge";
import { Download, Film, Image as ImageIcon, Layers, X } from "lucide-react";
import { cn } from "@/lib/utils";

const FORMATS: { id: ExportFormat; label: string; icon: typeof Film; implemented: boolean }[] = [
  { id: "mp4", label: "MP4", icon: Film, implemented: true },
  { id: "webm", label: "WebM", icon: Film, implemented: true },
  { id: "png-sequence", label: "PNG Sequence", icon: ImageIcon, implemented: true },
  { id: "mov-alpha", label: "MOV (Alpha)", icon: Layers, implemented: true },
];

interface ExportPanelProps {
  trigger: React.ReactNode;
}

// Gives the renderer a tick to actually push/paint the new frame before we
// grab the canvas. This used to wait on requestAnimationFrame, but browsers
// fully suspend rAF callbacks while the tab is hidden/backgrounded — which
// froze the export loop the moment the user switched tabs. setTimeout still
// fires (throttled, but never fully stopped) in background tabs, so use it
// instead — this keeps the export progressing while the user works on
// something else.
function nextFrame(): Promise<void> {
  return new Promise((resolve) => window.setTimeout(resolve, 16));
}

function sleep(ms: number): Promise<void> {
  return new Promise((resolve) => window.setTimeout(resolve, ms));
}

// Waits until raylib has actually drawn AND presented at least one new frame
// since `sinceCount` (see js_get_presented_frame_count in native/main.c) — a
// ground-truth counter from the renderer itself, instead of guessing from
// canvas pixels. The old pixel-fingerprint approach couldn't tell "stably
// rendered" apart from "stably cleared to blank by the browser", which is
// what let empty frames slip through; it also had no way to distinguish two
// genuinely-identical consecutive source frames from a render that hadn't
// happened yet. This can't be fooled either way: the count only advances
// when a real EndDrawing() completes.
// Falls back to a single fixed-tick wait when the counter isn't available
// (mock renderer / wasm not attached), since there's nothing to poll there.
async function waitForPresentedFrame(sinceCount: number | null): Promise<number | null> {
  if (sinceCount === null) {
    await nextFrame();
    return null;
  }
  const MAX_TRIES = 60; // generous cap so a genuinely stalled renderer can't hang the export forever
  for (let tries = 0; tries < MAX_TRIES; tries++) {
    await nextFrame();
    const current = wasmBridge.getPresentedFrameCount();
    if (current === null) return null;
    if (current > sinceCount) return current;
  }
  console.warn("[ExportPanel] timed out waiting for a new rendered frame");
  return wasmBridge.getPresentedFrameCount();
}

const MAX_FRAME_CAPTURE_ATTEMPTS = 3;

// Confirms a frame is actually captured before letting the export loop move
// on. wasmBridge.captureFrame() already retries internally on a blank read
// (see public/video_export.js); this is the outer confirmation layer — if it
// still comes back empty, wait for another real render tick and try the
// whole capture again, rather than silently accepting a missing frame.
// Throws only after every attempt is exhausted, so the export visibly fails
// instead of shipping a gap.
async function captureCurrentFrameOrThrow(frameIndex: number): Promise<void> {
  for (let attempt = 1; attempt <= MAX_FRAME_CAPTURE_ATTEMPTS; attempt++) {
    if (await wasmBridge.captureFrame()) return;
    console.warn(`[ExportPanel] frame ${frameIndex} capture attempt ${attempt}/${MAX_FRAME_CAPTURE_ATTEMPTS} failed`);
    if (attempt < MAX_FRAME_CAPTURE_ATTEMPTS) await nextFrame();
  }
  throw new Error(`Frame ${frameIndex} could not be captured after ${MAX_FRAME_CAPTURE_ATTEMPTS} attempts`);
}

// Best-effort: keeps the display from sleeping mid-export. Has no effect on
// background-tab timer throttling (that's handled by switching off rAF
// above and in the wasm main loop), it only stops the screen itself from
// locking. Silently no-ops if unsupported or if the request is rejected
// (e.g. the tab was already hidden when requested).
async function requestWakeLock(): Promise<WakeLockSentinel | null> {
  try {
    return (await navigator.wakeLock?.request("screen")) ?? null;
  } catch {
    return null;
  }
}

export function ExportPanel({ trigger }: ExportPanelProps) {
  const [open, setOpen] = useState(false);
  const [format, setFormat] = useState<ExportFormat>("mp4");
  const exportJob = useAppStore((s) => s.exportJob);
  const startExport = useAppStore((s) => s.startExport);
  const updateExportProgress = useAppStore((s) => s.updateExportProgress);
  const updateExportStatus = useAppStore((s) => s.updateExportStatus);
  const finishExport = useAppStore((s) => s.finishExport);
  const cancelExport = useAppStore((s) => s.cancelExport);
  const timeline = useAppStore((s) => s.timeline);
  const setPlaying = useAppStore((s) => s.setPlaying);
  const setCurrentFrame = useAppStore((s) => s.setCurrentFrame);
  const videoFrameCount = useAppStore((s) => s.video.frames?.length ?? 0);

  const runIdRef = useRef(0);

  // stop any in-flight recording if the component unmounts mid-export
  useEffect(() => {
    return () => {
      runIdRef.current += 1; // invalidate any in-flight export loop
      if (useAppStore.getState().exportJob.running) {
        wasmBridge.cancelRecording();
      }
    };
  }, []);

  // Reflects video_export.js's post-capture phase in the UI
  useEffect(() => {
    return wasmBridge.onExportStatus((phase, message) => {
      updateExportStatus(phase as Parameters<typeof updateExportStatus>[0], message);
    });
  }, [updateExportStatus]);

  // Listen to progress during capture and post-capture phases
  const updateExportPostCaptureProgress = useAppStore((s) => s.updateExportPostCaptureProgress);
  
  useEffect(() => {
    const handleProgress = (data: { percent: number; timeElapsed: number; timeEstimated: number }) => {
      const state = useAppStore.getState();
      const isPostCapture = state.exportJob.phase && state.exportJob.phase !== "capturing";
      
      if (isPostCapture) {
        // During post-processing (encoding/compression): show real progress
        // AND a real ETA, from video_export.js's own timeElapsed/timeEstimated
        // (ffmpeg's encoded-time-so-far vs. total source duration, JSZip's
        // bytes-compressed-so-far vs. total, etc.) — same idea as the ETA
        // shown during capture, just sourced from the post-capture phase.
        updateExportPostCaptureProgress(data.percent / 100, Math.max(0, data.timeEstimated - data.timeElapsed));
      } else {
        // During frame capture: use for frame progress
        updateExportProgress(data.percent, Math.max(0, data.timeEstimated - data.timeElapsed));
      }
    };

    return wasmBridge.onExportProgress(handleProgress);
  }, [updateExportProgress, updateExportPostCaptureProgress]);

  async function handleStart() {
    const spec = FORMATS.find((f) => f.id === format);
    if (!spec?.implemented) return;

    const canvas = document.getElementById("canvas") as HTMLCanvasElement | null;
    if (!canvas) {
      console.error("[ExportPanel] No canvas found to record");
      return;
    }

    const runId = ++runIdRef.current;
    const hasVideo = videoFrameCount > 0;
    const totalFrames = hasVideo
      ? videoFrameCount
      : Math.max(1, Math.round(timeline.durationSeconds * timeline.fps));
    const isSnapshotFormat = format === "png-sequence" || format === "mov-alpha";
    const frameIntervalMs = Math.max(1, 1000 / Math.max(1, timeline.fps));

    startExport(format, totalFrames);
    const started = await wasmBridge.startRecording(canvas.width, canvas.height, timeline.fps, format);
    if (!started) {
      cancelExport();
      return;
    }

    // Freeze the timeline's own playback loop while we drive currentFrame
    // ourselves — otherwise both would fight over it and frames would skip
    // or repeat unpredictably.
    const wasPlaying = timeline.playing;
    if (hasVideo) setPlaying(false);

    // Best-effort: keep the screen from locking mid-export (see
    // requestWakeLock's comment above for what this does and doesn't cover).
    const wakeLock = await requestWakeLock();

    const stillCurrent = () => runIdRef.current === runId && useAppStore.getState().exportJob.running;

    try {
      if (hasVideo) {
        // Deterministic export: step through every imported video frame one
        // by one (independent of real time), so ALL frames get rendered
        // with the effect and captured — not just whatever frame happened
        // to be showing when Export was clicked.
        let lastPresentedCount = wasmBridge.getPresentedFrameCount();
        for (let i = 0; i < totalFrames; i++) {
          if (!stillCurrent()) return;
          setCurrentFrame(i);
          // Confirm raylib actually drew a new frame for this timeline
          // position before grabbing it — ground truth from the renderer,
          // not a guess from canvas pixels (see waitForPresentedFrame above).
          lastPresentedCount = await waitForPresentedFrame(lastPresentedCount);

          if (isSnapshotFormat) {
            // Captures, verifies the result isn't blank, and retries until
            // it's confirmed — see captureCurrentFrameOrThrow above.
            await captureCurrentFrameOrThrow(i);
          } else {
            // mp4/webm: MediaRecorder samples the live canvas stream, so the
            // frame needs to stay on screen long enough to actually get
            // grabbed at the target fps.
            await sleep(frameIntervalMs);
          }
          updateExportProgress(i + 1, ((totalFrames - i - 1) * frameIntervalMs) / 1000);
        }
      } else {
        // No source video: the effect animates on its own real-time clock
        // (particles/CRT/etc. inside the wasm render loop), so just let it
        // run for the timeline's configured duration, snapshotting along
        // the way for the frame-by-frame formats.
        const start = performance.now();
        const durationMs = timeline.durationSeconds * 1000;
        let lastPresentedCount = wasmBridge.getPresentedFrameCount();
        let snapshotIndex = 0;
        while (performance.now() - start < durationMs) {
          if (!stillCurrent()) return;
          if (isSnapshotFormat) {
            lastPresentedCount = await waitForPresentedFrame(lastPresentedCount);
            await captureCurrentFrameOrThrow(snapshotIndex);
            snapshotIndex++;
          }
          const elapsed = performance.now() - start;
          const frame = Math.min(totalFrames, Math.round((elapsed / durationMs) * totalFrames));
          updateExportProgress(frame, Math.max(0, (durationMs - elapsed) / 1000));
          await sleep(frameIntervalMs);
        }
      }

      if (!stillCurrent()) return;
      updateExportProgress(totalFrames, 0);
      const filename = format === "mp4" ? "export.mp4" : format === "webm" ? "export.webm" : format === "mov-alpha" ? "export.mov" : "export.png";
      await wasmBridge.stopRecording(filename);
      finishExport();
    } catch (err) {
      console.error("[ExportPanel] export failed:", err);
      if (runIdRef.current === runId) {
        // Make sure any partially-written encoder buffers (ffmpeg FS frames,
        // in-flight MediaRecorder chunks, etc.) are cleared even when the
        // failure happened outside our own try block above, so the next
        // export starts from a clean slate instead of inheriting leftovers.
        wasmBridge.cancelRecording();
        cancelExport();
      }
    } finally {
      if (runIdRef.current === runId) {
        if (hasVideo) setPlaying(wasPlaying);
        // Always rewind the timeline slider to the start once an export
        // ends — whether it finished, failed, or was cancelled — so a new
        // export begins at frame 0 instead of resuming from wherever this
        // one stopped.
        setCurrentFrame(0);
      }
      wakeLock?.release().catch(() => {});
    }
  }

  function handleCancel() {
    runIdRef.current += 1;
    wasmBridge.cancelRecording();
    cancelExport();
    setCurrentFrame(0);
  }

  return (
    <Dialog open={open} onOpenChange={setOpen}>
      <div onClick={() => setOpen(true)}>{trigger}</div>
      <DialogContent>
        <DialogTitle className="text-sm font-semibold">Export</DialogTitle>
        <DialogDescription className="text-[11px] text-muted-foreground">
          Render the current timeline out to a file.
        </DialogDescription>

        <div className="mt-4 grid grid-cols-2 gap-2">
          {FORMATS.map((f) => (
            <button
              key={f.id}
              onClick={() => f.implemented && setFormat(f.id)}
              disabled={exportJob.running || !f.implemented}
              title={f.implemented ? undefined : "Not implemented yet"}
              className={cn(
                "flex items-center gap-2 rounded-md border px-3 py-2 text-xs transition-colors",
                !f.implemented && "cursor-not-allowed opacity-40",
                format === f.id ? "border-accent bg-accent/10 text-accent" : "border-border hover:bg-panel-raised"
              )}
            >
              <f.icon className="h-3.5 w-3.5" />
              {f.label}
              {!f.implemented && <span className="ml-auto text-[9px] uppercase text-muted-foreground">Soon</span>}
            </button>
          ))}
        </div>

        {exportJob.running ? (
          <div className="mt-4 space-y-2">
            {exportJob.phase && exportJob.phase !== "capturing" ? (
              // Frame capture is over. Only ONE bar is shown from here on —
              // the capture bar (frozen at 100%) is intentionally not
              // rendered anymore, since showing it next to a 0%-starting
              // post-processing bar is exactly what made this look like it
              // "starts at 100%". This bar reflects video_export.js's real
              // per-phase progress (frame= from ffmpeg, JSZip's onUpdate,
              // etc.), not a guess.
              <div className="space-y-1">
                <Progress value={exportJob.postCaptureProgress ?? 0} className="transition-all" />
                <div className="flex justify-between font-mono text-[10.5px] text-muted-foreground">
                  <span>{exportJob.statusMessage || "Finishing export…"}</span>
                  <span>
                    {Math.round((exportJob.postCaptureProgress ?? 0) * 100)}%
                    {exportJob.postCaptureEtaSeconds ? ` · ETA ${Math.max(0, Math.round(exportJob.postCaptureEtaSeconds))}s` : ""}
                  </span>
                </div>
              </div>
            ) : (
              <>
                <Progress value={exportJob.progress} />
                <div className="flex justify-between font-mono text-[10.5px] text-muted-foreground">
                  <span>Frame {exportJob.currentFrame}/{exportJob.totalFrames}</span>
                  <span>ETA {Math.max(0, Math.round(exportJob.etaSeconds))}s</span>
                </div>
              </>
            )}
            <Button variant="destructive" size="sm" className="w-full gap-1.5" onClick={handleCancel}>
              <X className="h-3 w-3" /> Cancel
            </Button>
          </div>
        ) : (
          <Button
            variant="accent"
            size="default"
            className="mt-4 w-full gap-1.5"
            onClick={handleStart}
            disabled={!FORMATS.find((f) => f.id === format)?.implemented}
          >
            <Download className="h-3.5 w-3.5" /> Start Export
          </Button>
        )}
      </DialogContent>
    </Dialog>
  );
}