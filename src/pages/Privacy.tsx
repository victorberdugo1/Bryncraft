import { SiteLayout } from "@/pages/SiteLayout";

export default function Privacy() {
  return (
    <SiteLayout>
      <h1 className="mb-2 text-3xl font-semibold tracking-tight">Privacy Policy</h1>
      <p className="mb-8 text-[12px] text-muted-foreground">Last updated: August 2026</p>

      <div className="max-w-2xl space-y-6 text-[14px] leading-relaxed text-muted-foreground">
        <section>
          <h2 className="mb-2 text-[16px] font-semibold text-foreground">Camera and video data</h2>
          <p>
            Bryncraft asks for camera access, or lets you load a video file, so an effect has a
            source frame to render. That frame is decoded and processed locally in your browser by
            the WebAssembly renderer. Bryncraft does not upload, store, or transmit your camera or
            video frames to any server as part of normal use of the editor.
          </p>
        </section>

        <section>
          <h2 className="mb-2 text-[16px] font-semibold text-foreground">Exported files</h2>
          <p>
            Video clips and source-code files you export are generated and saved directly to your
            device through your browser's download mechanism. They are not copied to a server.
          </p>
        </section>

        <section>
          <h2 className="mb-2 text-[16px] font-semibold text-foreground">Cookies and advertising</h2>
          <p>
            Bryncraft may show advertising served through Google AdSense. Google and its partners
            may use cookies or similar technologies to serve ads based on your prior visits to this
            or other websites, and you can opt out of personalized advertising by visiting Google's
            Ads Settings. Third-party vendors, including Google, may also collect and use
            non-identifying data for analytics purposes related to how the site is used.
          </p>
        </section>

        <section>
          <h2 className="mb-2 text-[16px] font-semibold text-foreground">Data we don't collect</h2>
          <p>
            Bryncraft does not require an account, does not collect names or email addresses to
            use the studio, and does not sell personal data to third parties.
          </p>
        </section>

        <section>
          <h2 className="mb-2 text-[16px] font-semibold text-foreground">Changes to this policy</h2>
          <p>
            This policy may be updated from time to time as the product evolves. Continued use of
            Bryncraft after a change constitutes acceptance of the revised policy.
          </p>
        </section>

        <section>
          <h2 className="mb-2 text-[16px] font-semibold text-foreground">Contact</h2>
          <p>
            Questions about this policy can be sent to{" "}
            <a href="mailto:victorberdugo1@gmail.com" className="text-accent hover:underline">
              victorberdugo1@gmail.com
            </a>
            .
          </p>
        </section>
      </div>
    </SiteLayout>
  );
}
