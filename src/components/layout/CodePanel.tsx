import { Tabs, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { useAppStore } from "@/store/useAppStore";
import {
  generateEffectHeader,
  generateMainTab,
  generateReadme,
  getExtras,
  getMainFilename,
  type ExtraAsset,
} from "@/codegen/generateRaylibCode";
import { Button } from "@/components/ui/button";
import { Copy, Download } from "lucide-react";
import { useMemo } from "react";
import { marked } from "marked";
import { usePanelTransparentBg } from "@/hooks/usePanelTransparency";
import { cn } from "@/lib/utils";

function downloadTextFile(filename: string, content: string) {
  const blob = new Blob([content], { type: "text/plain" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  a.click();
  URL.revokeObjectURL(url);
}

// Para binarios (p.ej. el .ttf del modo Matrix) el contenido nunca pasa por
// un Blob de texto — eso corrompería los bytes. `url` ya es el asset final
// resuelto por Vite (?url), así que basta con un <a download> normal.
function downloadFromUrl(filename: string, url: string) {
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  a.click();
}

function CodeBlock({ code, filename }: { code: string; filename: string }) {
  return (
    <div className="relative flex min-h-0 flex-1 overflow-hidden">
      <div className="absolute right-2 top-2 z-10 flex gap-1">
        <Button size="icon" variant="ghost" onClick={() => navigator.clipboard.writeText(code)} title="Copy">
          <Copy className="h-3 w-3" />
        </Button>
        <Button size="icon" variant="ghost" onClick={() => downloadTextFile(filename, code)} title={`Download ${filename}`}>
          <Download className="h-3 w-3" />
        </Button>
      </div>
      <pre className="h-full w-full overflow-auto whitespace-pre p-3 font-mono text-[11.5px] leading-5 text-foreground/90">
        <code className="block min-h-full">{code}</code>
      </pre>
    </div>
  );
}

// Tab "Extra": 0, 1 o varios archivos según el efecto (ninguno para
// particles; el shader para crt; la fuente para ascii; los dos scripts de
// build para opencv). Cada uno se descarga por su cuenta — texto con
// copy+download como el resto de tabs, binario solo con download.
function ExtraTab({ extras }: { extras: ExtraAsset[] }) {
  if (extras.length === 0) {
    return (
      <div className="flex h-full w-full items-center justify-center p-6 text-center text-xs text-muted-foreground">
        Este efecto no necesita archivos extra.
      </div>
    );
  }

  return (
    <div className="flex h-full min-h-0 flex-1 flex-col divide-y divide-border overflow-auto">
      {extras.map((extra) => (
        <div key={extra.filename} className="flex min-h-0 flex-1 flex-col">
          <div className="flex shrink-0 items-center justify-between gap-2 px-3 py-2">
            <div className="min-w-0">
              <div className="truncate font-mono text-[11.5px]">{extra.filename}</div>
              <div className="truncate text-[10.5px] text-muted-foreground">{extra.label}</div>
            </div>
            <div className="flex shrink-0 gap-1">
              {extra.kind === "text" && (
                <Button size="icon" variant="ghost" onClick={() => navigator.clipboard.writeText(extra.content)} title="Copy">
                  <Copy className="h-3 w-3" />
                </Button>
              )}
              <Button
                size="icon"
                variant="ghost"
                onClick={() =>
                  extra.kind === "text"
                    ? downloadTextFile(extra.filename, extra.content)
                    : downloadFromUrl(extra.filename, extra.url)
                }
                title={`Download ${extra.filename}`}
              >
                <Download className="h-3 w-3" />
              </Button>
            </div>
          </div>
          {extra.kind === "text" && (
            <pre className="min-h-0 flex-1 overflow-auto whitespace-pre px-3 pb-3 font-mono text-[11.5px] leading-5 text-foreground/90">
              <code className="block min-h-full">{extra.content}</code>
            </pre>
          )}
        </div>
      ))}
    </div>
  );
}

// Tab "README": renderizado real del .md (marked.parse — sync porque no
// usamos ninguna extensión async), estilado por .md-body en index.css. Sin
// sanitizer aparte (DOMPurify, etc.): el contenido sale de los README.md
// del propio repo, no de nada que escriba el usuario, así que no hay
// superficie de XSS que sanitizar. Copy/Download siguen operando sobre el
// markdown crudo, no sobre el HTML renderizado.
function ReadmeTab({ raw, filename }: { raw: string; filename: string }) {
  const html = useMemo(() => marked.parse(raw, { async: false }) as string, [raw]);
  return (
    <div className="relative flex min-h-0 flex-1 overflow-hidden">
      <div className="absolute right-2 top-2 z-10 flex gap-1">
        <Button size="icon" variant="ghost" onClick={() => navigator.clipboard.writeText(raw)} title="Copy">
          <Copy className="h-3 w-3" />
        </Button>
        <Button size="icon" variant="ghost" onClick={() => downloadTextFile(filename, raw)} title={`Download ${filename}`}>
          <Download className="h-3 w-3" />
        </Button>
      </div>
      <div className="md-body h-full w-full overflow-auto p-3" dangerouslySetInnerHTML={{ __html: html }} />
    </div>
  );
}

export function CodePanel() {
  const codeTab = useAppStore((s) => s.codeTab);
  const setCodeTab = useAppStore((s) => s.setCodeTab);
  const activeEffect = useAppStore((s) => s.activeEffect);
  const params = useAppStore((s) => s.paramsByEffect[s.activeEffect]);
  const transparent = usePanelTransparentBg();

  const code = useMemo(() => generateEffectHeader(activeEffect, params), [activeEffect, params]);
  const main = useMemo(() => generateMainTab(activeEffect), [activeEffect]);
  const readme = useMemo(() => generateReadme(activeEffect), [activeEffect]);
  const extras = useMemo(() => getExtras(activeEffect), [activeEffect]);

  const activeCode = codeTab === "main" ? main : code;

  const filename = useMemo(() => {
    switch (codeTab) {
      case "main":
        return getMainFilename(activeEffect);
      case "readme":
        return `${activeEffect}-README.md`;
      default:
        return `${activeEffect}_effect.h`;
    }
  }, [codeTab, activeEffect]);

  return (
    <div className={cn("flex h-full flex-col transition-colors duration-300", transparent ? "bg-transparent" : "bg-panel")}>
      <Tabs value={codeTab} onValueChange={(v) => setCodeTab(v as typeof codeTab)} className="flex h-full min-h-0 flex-col">
        <TabsList className="shrink-0">
          <TabsTrigger value="code">Code</TabsTrigger>
          <TabsTrigger value="main">Main</TabsTrigger>
          <TabsTrigger value="extra">Extra</TabsTrigger>
          <TabsTrigger value="readme">README</TabsTrigger>
        </TabsList>
        <div className="mt-0 flex min-h-0 flex-1 flex-col">
          {codeTab === "extra" ? (
            <ExtraTab extras={extras} />
          ) : codeTab === "readme" ? (
            <ReadmeTab raw={readme} filename={filename} />
          ) : (
            <CodeBlock code={activeCode} filename={filename} />
          )}
        </div>
      </Tabs>
    </div>
  );
}
