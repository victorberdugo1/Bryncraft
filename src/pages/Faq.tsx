import { SiteLayout } from "@/pages/SiteLayout";

const QA = [
  {
    q: "Is Bryncraft free to use?",
    a: "Yes. The studio runs entirely in your browser and there is no account or paywall to open it.",
  },
  {
    q: "Does Bryncraft need a webcam?",
    a: "Most effects work best with a live camera feed, but you can also load a video file as the source frame if you'd rather not grant camera access.",
  },
  {
    q: "Where does my video go?",
    a: "Nowhere. Camera and video frames are decoded and rendered on your own device by the WebAssembly renderer; Bryncraft doesn't upload frames to a server as part of normal use.",
  },
  {
    q: "Can I export what I make?",
    a: "Yes, in two forms: as a transparent video clip you can drop into editing software, or as the underlying Raylib C source code for the effect, which compiles and runs outside the browser.",
  },
  {
    q: "Which browsers are supported?",
    a: "Any recent Chromium, Firefox, or Safari build with WebAssembly and getUserMedia support works. Very old browsers, or ones with WebAssembly disabled, won't be able to load the renderer.",
  },
  {
    q: "Why does the first load take a moment?",
    a: "The Raylib/OpenCV renderer is a compiled WebAssembly module a few megabytes in size. It's fetched and initialized once when you open the studio; after that, effect switching is instant.",
  },
];

export default function Faq() {
  return (
    <SiteLayout>
      <h1 className="mb-6 text-3xl font-semibold tracking-tight">Frequently asked questions</h1>
      <div className="max-w-2xl space-y-8">
        {QA.map((item) => (
          <div key={item.q}>
            <h2 className="mb-2 text-[15px] font-semibold text-foreground">{item.q}</h2>
            <p className="text-[13px] leading-relaxed text-muted-foreground">{item.a}</p>
          </div>
        ))}
      </div>
    </SiteLayout>
  );
}
