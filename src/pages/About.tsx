import { SiteLayout } from "@/pages/SiteLayout";

export default function About() {
  return (
    <SiteLayout>
      <h1 className="mb-6 text-3xl font-semibold tracking-tight">About Bryncraft</h1>

      <div className="max-w-2xl space-y-6 text-[14px] leading-relaxed text-muted-foreground">
        <p>
          Bryncraft started from a simple frustration: most real-time visual-effects tools are
          either heavyweight native applications with steep learning curves, or browser demos
          that look impressive once and can't be tuned, combined, or taken anywhere else. Bryncraft
          is an attempt to split the difference — a docking editor, in the spirit of a small DAW or
          node-based compositor, that runs from a single browser tab and produces artifacts you can
          actually reuse.
        </p>

        <h2 className="text-[16px] font-semibold text-foreground">Two layers, one contract</h2>
        <p>
          The interface — effect library, inspector, timeline, code panel — is a plain React
          application. It never draws a single pixel. Every parameter change is packaged into a
          small, flat JSON message and handed to a Raylib renderer compiled to WebAssembly, which
          owns the camera feed, the simulation, and everything that ends up on the canvas. That
          split means the UI can be redesigned freely without touching the rendering code, and the
          renderer can be compiled and run completely outside the browser, unchanged.
        </p>

        <h2 className="text-[16px] font-semibold text-foreground">Why export as C source</h2>
        <p>
          Tuning an effect visually is only useful if the result can leave the browser. Every
          effect in Bryncraft can be exported as the same Raylib C source that runs inside the
          WebAssembly build, so a particle system or shader tuned live in the browser can be
          dropped straight into a native game or tool without being reimplemented from scratch.
        </p>

        <h2 className="text-[16px] font-semibold text-foreground">Camera and video stay local</h2>
        <p>
          Bryncraft reads camera and video frames directly in the browser and renders them on
          your own device. No frame is uploaded to a server as part of using the editor. See the{" "}
          <a href="/privacy" className="text-accent hover:underline">Privacy Policy</a> for details.
        </p>
      </div>
    </SiteLayout>
  );
}
