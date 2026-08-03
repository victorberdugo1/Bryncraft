import { useState } from "react";
import { Layers, SlidersHorizontal, Code2, Clock, Download } from "lucide-react";
import { CenterViewport } from "@/components/layout/CenterViewport";
import { LeftSidebar } from "@/components/layout/LeftSidebar";
import { RightInspector } from "@/components/layout/RightInspector";
import { CodePanel } from "@/components/layout/CodePanel";
import { BottomTimeline } from "@/components/layout/BottomTimeline";
import { ExportPanel } from "@/components/layout/ExportPanel";
import { EdgeDock, type DockEdge } from "@/components/layout/EdgeDock";
import { Button } from "@/components/ui/button";

type EdgeState = Record<DockEdge, boolean>;

const INITIAL_STATE: EdgeState = { top: false, bottom: false, left: false, right: false };

/**
 * Full-screen canvas layout for phones/tablets: the viewport fills the
 * entire screen and every panel (effects, parameters, code, timeline) lives
 * tucked behind a small tab on one edge of the screen, so nothing but the
 * canvas is visible until the person pulls a panel out.
 */
export function MobileDockShell() {
  const [openEdge, setOpenEdge] = useState<EdgeState>(INITIAL_STATE);

  const toggle = (edge: DockEdge) => setOpenEdge((s) => ({ ...s, [edge]: !s[edge] }));

  return (
    <div className="relative min-h-0 flex-1 overflow-hidden">
      <div className="absolute inset-0">
        <CenterViewport />
      </div>

      <EdgeDock
        edge="left"
        label="Biblioteca de efectos"
        icon={Layers}
        open={openEdge.left}
        onToggle={() => toggle("left")}
        size="min(78vw, 300px)"
      >
        <LeftSidebar />
      </EdgeDock>

      <EdgeDock
        edge="right"
        label="Parámetros"
        icon={SlidersHorizontal}
        open={openEdge.right}
        onToggle={() => toggle("right")}
        size="min(82vw, 320px)"
      >
        <RightInspector />
      </EdgeDock>

      <EdgeDock
        edge="top"
        label="Código"
        icon={Code2}
        open={openEdge.top}
        onToggle={() => toggle("top")}
        size="min(55vh, 380px)"
        // Anchored at the far left, clear of the toolbar's buttons — the
        // toolbar itself now reserves space for this via pl-16 in
        // CenterViewport.
        tabPositionClassName="top-0 left-1 rounded-b-xl flex-row w-14 h-6"
      >
        <CodePanel />
      </EdgeDock>

      <EdgeDock
        edge="bottom"
        label="Línea de tiempo y exportación"
        icon={Clock}
        open={openEdge.bottom}
        onToggle={() => toggle("bottom")}
        size="auto"
      >
        <div className="flex flex-col">
          <BottomTimeline />
          <div className="flex justify-end border-t border-border/60 px-3 py-2">
            <ExportPanel
              trigger={
                <Button variant="accent" size="sm" className="gap-1.5">
                  <Download className="h-3.5 w-3.5" /> Exportar
                </Button>
              }
            />
          </div>
        </div>
      </EdgeDock>
    </div>
  );
}
