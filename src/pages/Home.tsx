import { Link } from "react-router-dom";
import { SiteLayout } from "@/pages/SiteLayout";

const EFFECTS = [
  {
    name: "ASCII Renderer",
    text: "Turns the live camera feed into a grid of characters, redrawn every frame from the actual luminance of the video. Normal mode maps brightness to a configurable ramp of characters with adjustable brightness, contrast and gamma; Matrix mode replaces that ramp with falling katakana columns whose speed, density and trail length react to motion in the frame, so the code rain speeds up and thickens when you move.",
  },
  {
    name: "CRT / VHS",
    text: "A shader stack that reproduces the imperfections of analog displays and tape: scanlines with adjustable density and scroll speed, barrel distortion, vignette, static noise, chromatic aberration and flicker. A second VHS layer adds tracking glitches, wave distortion, dropout lines and an on-screen play/pause/rec overlay with a configurable timestamp and label, for a full tape-deck look.",
  },
  {
    name: "Particles",
    text: "A multi-mode particle emitter with three presets — a classic upward fountain, falling rain, and rising embers — all reactive to what the camera sees, so particle count, speed and color respond to motion and brightness in the source video instead of running on a fixed loop.",
  },
  {
    name: "Computer Vision (OpenCV/WASM)",
    text: "Real computer-vision pipelines — edge detection, contour tracing, optical flow, background subtraction and face detection — compiled natively to WebAssembly and running on the loaded video or a live camera feed, with no round trip to a server.",
  },
  {
    name: "Hand Tracker",
    text: "Live hand detection built entirely in C with OpenCV, no external JS models. Each detected hand is covered by a liquid-glass blob, and bringing both hands together stretches a slime bridge between them; the camera stays fully visible underneath.",
  },
  {
    name: "Effect Atelier",
    text: "A 3D, world-space particle system on an orbiting camera: over thirty shapes — hex-panel shields, full hex-dome fields, fire tornadoes, water-ring shockwaves, elemental bursts and auras, and traveling spell projectiles like fireballs, lightning bolts and ice shards — across ten elements, each with colors that follow the element automatically. Layer up to three combos at once, name the result, and export it as a ready-to-paste preset for a game project.",
  },
];

const STEPS = [
  {
    title: "Point a camera or load a clip",
    text: "Bryncraft reads from your webcam or an existing video file as its source frame. Nothing is uploaded — the frame stays on your machine.",
  },
  {
    title: "Pick an effect and tune it live",
    text: "Every effect exposes its parameters — colors, speeds, densities, thresholds — as sliders and switches in the inspector panel, and the canvas updates in real time as you drag them.",
  },
  {
    title: "Render on a WebAssembly engine, not the DOM",
    text: "React never touches a pixel. It only sends a small JSON message describing the chosen effect and its parameters; a Raylib renderer compiled to WebAssembly does the actual drawing inside a single canvas element, which is why the frame rate holds up even with several effects stacked.",
  },
  {
    title: "Export the result",
    text: "Save the frame as a transparent video clip for editing software, or export the effect as Raylib C source code that compiles and runs outside the browser, ready to drop into a native or WebAssembly project.",
  },
];

export default function Home() {
  return (
    <SiteLayout>
      <section className="mb-16">
        <h1 className="mb-4 text-3xl font-semibold tracking-tight sm:text-4xl">
          Bryncraft — a browser-based procedural VFX atelier
        </h1>
        <p className="max-w-2xl text-[15px] leading-relaxed text-muted-foreground">
          Bryncraft is a real-time visual effects editor that runs entirely in your browser.
          Point it at a webcam or a video file, layer effects like ASCII rendering, CRT/VHS
          shaders, reactive particles and computer-vision pipelines, tune every parameter live,
          and export the result as a video clip or as portable Raylib C source code.
        </p>
        <div className="mt-6 flex gap-3">
          <Link
            to="/studio"
            className="rounded-md bg-accent px-4 py-2 text-[13px] font-semibold text-background hover:bg-accent-glow"
          >
            Open the Studio
          </Link>
          <Link
            to="/about"
            className="rounded-md border border-border px-4 py-2 text-[13px] font-semibold hover:bg-panel-raised"
          >
            How it's built
          </Link>
        </div>
      </section>

      <section className="mb-16">
        <h2 className="mb-6 text-xl font-semibold tracking-tight">How it works</h2>
        <div className="grid gap-6 sm:grid-cols-2">
          {STEPS.map((step, i) => (
            <div key={step.title} className="rounded-lg border border-border bg-panel p-5">
              <div className="mb-2 text-[12px] font-semibold text-accent">Step {i + 1}</div>
              <h3 className="mb-2 text-[14px] font-semibold">{step.title}</h3>
              <p className="text-[13px] leading-relaxed text-muted-foreground">{step.text}</p>
            </div>
          ))}
        </div>
      </section>

      <section className="mb-16">
        <h2 className="mb-2 text-xl font-semibold tracking-tight">The effect library</h2>
        <p className="mb-6 max-w-2xl text-[13px] leading-relaxed text-muted-foreground">
          Six effect families ship with Bryncraft today, each with its own set of tunable
          parameters. All of them run on the same Raylib/WebAssembly renderer, so they can be
          combined and re-parametrized without leaving the canvas.
        </p>
        <div className="grid gap-5 sm:grid-cols-2">
          {EFFECTS.map((effect) => (
            <div key={effect.name} className="rounded-lg border border-border bg-panel p-5">
              <h3 className="mb-2 text-[14px] font-semibold text-accent">{effect.name}</h3>
              <p className="text-[13px] leading-relaxed text-muted-foreground">{effect.text}</p>
            </div>
          ))}
        </div>
      </section>

      <section>
        <h2 className="mb-4 text-xl font-semibold tracking-tight">Who it's for</h2>
        <p className="max-w-2xl text-[13px] leading-relaxed text-muted-foreground">
          Bryncraft is built for VJs and streamers who want a reactive live-camera look without
          installing native software, for game and tools developers who want a visual playground
          for tuning a particle or shader effect before exporting it as C source, and for anyone
          who wants to experiment with computer vision and generative visuals without setting up
          a local dev environment. Everything runs client-side in the browser tab you're reading
          this in.
        </p>
      </section>
    </SiteLayout>
  );
}
