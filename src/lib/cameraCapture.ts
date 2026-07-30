/**
 * cameraCapture.ts — thin getUserMedia lifecycle wrapper.
 *
 * Deliberately NOT a React hook: ViewportCanvas already owns a rAF loop for
 * the timeline, so camera frame pushing just piggybacks on that loop
 * (see pushCameraFrame in wasmBridge.ts) instead of running its own.
 * This module only owns the MediaStream + the hidden <video> element the
 * loop reads from.
 */

export class CameraCapture {
  private stream: MediaStream | null = null;
  private videoEl: HTMLVideoElement | null = null;

  get element(): HTMLVideoElement | null {
    return this.videoEl;
  }

  get active(): boolean {
    return this.stream !== null;
  }

  /** Requests camera access and starts a hidden <video> playing the feed.
   * Resolves once the video has enough data for reads (readyState >= 2) —
   * so the very first pushCameraFrame call after this resolves has real
   * pixels, not a blank frame. Throws with the browser's own message
   * (permission denied, no camera found, etc.) on failure — callers should
   * catch this and surface it via store.setCameraError. */
  async start(facingMode: "user" | "environment" = "user"): Promise<HTMLVideoElement> {
    this.stop(); // guard against a stray previous stream if start() is called twice

    const stream = await navigator.mediaDevices.getUserMedia({
      video: { facingMode, width: { ideal: 1280 }, height: { ideal: 720 } },
      audio: false,
    });

    const videoEl = document.createElement("video");
    videoEl.srcObject = stream;
    videoEl.muted = true;
    videoEl.playsInline = true;

    await videoEl.play();
    if (videoEl.readyState < 2) {
      await new Promise<void>((resolve) => {
        const onReady = () => {
          videoEl.removeEventListener("loadeddata", onReady);
          resolve();
        };
        videoEl.addEventListener("loadeddata", onReady);
      });
    }

    this.stream = stream;
    this.videoEl = videoEl;
    return videoEl;
  }

  /** Stops all tracks and releases the <video> element. Safe to call when
   * nothing is active. */
  stop() {
    this.stream?.getTracks().forEach((track) => track.stop());
    this.stream = null;
    if (this.videoEl) {
      this.videoEl.srcObject = null;
      this.videoEl = null;
    }
  }
}
