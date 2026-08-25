import { SiteLayout } from "@/pages/SiteLayout";

export default function Contact() {
  return (
    <SiteLayout>
      <h1 className="mb-6 text-3xl font-semibold tracking-tight">Contact</h1>
      <div className="max-w-2xl space-y-4 text-[14px] leading-relaxed text-muted-foreground">
        <p>
          For bug reports, feature requests, or anything else about Bryncraft, reach out at{" "}
          <a href="mailto:victorberdugo1@gmail.com" className="text-accent hover:underline">
            victorberdugo1@gmail.com
          </a>
          .
        </p>
        <p>
          If you run into a rendering issue, it helps to include your browser and operating
          system, and whether the problem happens with the camera or with a loaded video file.
        </p>
      </div>
    </SiteLayout>
  );
}
