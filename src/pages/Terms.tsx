import { SiteLayout } from "@/pages/SiteLayout";

export default function Terms() {
  return (
    <SiteLayout>
      <h1 className="mb-2 text-3xl font-semibold tracking-tight">Terms of Service</h1>
      <p className="mb-8 text-[12px] text-muted-foreground">Last updated: August 2026</p>

      <div className="max-w-2xl space-y-6 text-[14px] leading-relaxed text-muted-foreground">
        <section>
          <h2 className="mb-2 text-[16px] font-semibold text-foreground">Acceptance of terms</h2>
          <p>
            By accessing or using Bryncraft, you agree to these Terms of Service. If you don't
            agree, please don't use the site.
          </p>
        </section>

        <section>
          <h2 className="mb-2 text-[16px] font-semibold text-foreground">Use of the service</h2>
          <p>
            Bryncraft is provided as a browser-based tool for creating and exporting visual
            effects. You agree to use it only for lawful purposes and not to attempt to disrupt,
            reverse engineer for malicious purposes, or overload the service.
          </p>
        </section>

        <section>
          <h2 className="mb-2 text-[16px] font-semibold text-foreground">Your content and exports</h2>
          <p>
            You retain ownership of the video clips and source code you export from Bryncraft.
            Bryncraft claims no rights over content you create with the tool.
          </p>
        </section>

        <section>
          <h2 className="mb-2 text-[16px] font-semibold text-foreground">No warranty</h2>
          <p>
            Bryncraft is provided "as is," without warranties of any kind, express or implied,
            including fitness for a particular purpose or non-infringement. Rendering behavior can
            vary across browsers, devices, and drivers.
          </p>
        </section>

        <section>
          <h2 className="mb-2 text-[16px] font-semibold text-foreground">Limitation of liability</h2>
          <p>
            To the extent permitted by law, Bryncraft and its operators are not liable for any
            indirect, incidental, or consequential damages arising from your use of the service.
          </p>
        </section>

        <section>
          <h2 className="mb-2 text-[16px] font-semibold text-foreground">Changes to these terms</h2>
          <p>
            These terms may be updated periodically. Continued use of Bryncraft after a change
            constitutes acceptance of the revised terms.
          </p>
        </section>

        <section>
          <h2 className="mb-2 text-[16px] font-semibold text-foreground">Contact</h2>
          <p>
            Questions about these terms can be sent to{" "}
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
