import { Tabs, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { useAppStore } from "@/store/useAppStore";
import {
  generateEffectHeader,
  generateMainTab,
  generateReadme,
  generateShaderSnippet,
  getMainFilename,
} from "@/codegen/generateRaylibCode";
import { Button } from "@/components/ui/button";
import { Copy, Download } from "lucide-react";
import { useMemo } from "react";
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

export function CodePanel() {
  const codeTab = useAppStore((s) => s.codeTab);
  const setCodeTab = useAppStore((s) => s.setCodeTab);
  const activeEffect = useAppStore((s) => s.activeEffect);
  const params = useAppStore((s) => s.paramsByEffect[s.activeEffect]);
  const transparent = usePanelTransparentBg();

  const code = useMemo(() => generateEffectHeader(activeEffect, params), [activeEffect, params]);
  const main = useMemo(() => generateMainTab(activeEffect), [activeEffect]);
  const shader = useMemo(() => generateShaderSnippet(activeEffect), [activeEffect]);
  const readme = useMemo(() => generateReadme(activeEffect), [activeEffect]);

  const activeCode = codeTab === "main" ? main : codeTab === "shader" ? shader : codeTab === "readme" ? readme : code;

  const filename = useMemo(() => {
    switch (codeTab) {
      case "main":
        return getMainFilename(activeEffect);
      case "shader":
        return activeEffect === "crt" ? "crt.fs" : `${activeEffect}.fs`;
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
          <TabsTrigger value="shader">Shader</TabsTrigger>
          <TabsTrigger value="readme">README</TabsTrigger>
        </TabsList>
        <div className="mt-0 flex min-h-0 flex-1 flex-col">
          <CodeBlock code={activeCode} filename={filename} />
        </div>
      </Tabs>
    </div>
  );
}
