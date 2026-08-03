import type { ReactNode } from "react";
import { ChevronUp, ChevronDown, ChevronLeft, ChevronRight, type LucideIcon } from "lucide-react";
import { usePanelInteraction } from "@/hooks/usePanelInteraction";
import { PanelTransparencyContext } from "@/hooks/usePanelTransparency";
import { cn } from "@/lib/utils";

export type DockEdge = "top" | "bottom" | "left" | "right";

const CHEVRON: Record<DockEdge, LucideIcon> = {
  top: ChevronDown,
  bottom: ChevronUp,
  left: ChevronRight,
  right: ChevronLeft,
};

const TAB_POSITION: Record<DockEdge, string> = {
  top: "top-0 left-1/2 -translate-x-1/2 rounded-b-xl flex-row w-14 h-6",
  bottom: "bottom-0 left-1 rounded-t-xl flex-row w-14 h-6",
  left: "left-0 top-1/2 -translate-y-1/2 rounded-r-xl flex-col w-6 h-14",
  right: "right-0 top-1/2 -translate-y-1/2 rounded-l-xl flex-col w-6 h-14",
};

const PANEL_POSITION: Record<DockEdge, string> = {
  top: "top-0 left-0 right-0 border-b",
  bottom: "bottom-0 left-0 right-0 border-t",
  left: "left-0 top-0 bottom-0 border-r",
  right: "right-0 top-0 bottom-0 border-l",
};

const PANEL_CLOSED_TRANSFORM: Record<DockEdge, string> = {
  top: "-translate-y-full",
  bottom: "translate-y-full",
  left: "-translate-x-full",
  right: "translate-x-full",
};

interface EdgeDockProps {
  edge: DockEdge;
  label: string;
  icon: LucideIcon;
  open: boolean;
  onToggle: () => void;
  size: string; // e.g. "min(80vw,320px)" applied as width or height depending on edge
  children: ReactNode;
  /** Overrides TAB_POSITION[edge] for the pull-tab button, e.g. to move it
   *  off-center so it doesn't sit on top of other toolbar controls. */
  tabPositionClassName?: string;
}

/**
 * A panel docked to one screen edge, tucked away behind a small tab. Tapping
 * the tab slides the panel out over the canvas (the canvas itself never
 * moves or resizes). While the user is dragging a control inside the open
 * panel it fades to near-transparent so the canvas stays visible underneath,
 * then eases back to its normal, solid look shortly after they let go.
 */
export function EdgeDock({ edge, label, icon: Icon, open, onToggle, size, children, tabPositionClassName }: EdgeDockProps) {
  const { active, handlers } = usePanelInteraction();
  const Chevron = CHEVRON[edge];
  const isHorizontalEdge = edge === "top" || edge === "bottom";

  return (
    <>
      <button
        type="button"
        onClick={onToggle}
        aria-label={open ? `Cerrar ${label}` : `Abrir ${label}`}
        aria-expanded={open}
        className={cn(
          "absolute z-50 flex items-center justify-center gap-0.5 border border-border bg-panel/95 text-muted-foreground shadow-floating backdrop-blur-md transition-colors active:bg-panel-raised",
          tabPositionClassName ?? TAB_POSITION[edge]
        )}
        style={{
          marginTop: edge === "top" ? "env(safe-area-inset-top)" : undefined,
          marginBottom: edge === "bottom" ? "env(safe-area-inset-bottom)" : undefined,
          marginLeft: edge === "left" ? "env(safe-area-inset-left)" : undefined,
          marginRight: edge === "right" ? "env(safe-area-inset-right)" : undefined,
        }}
      >
        <Icon className="h-3 w-3" />
        <Chevron className={cn("h-2.5 w-2.5 transition-transform", open && "rotate-180")} />
      </button>

      <div
        className={cn(
          "absolute z-30 flex overflow-hidden border-border shadow-floating transition-transform duration-300 ease-out",
          PANEL_POSITION[edge],
          open ? "translate-x-0 translate-y-0" : PANEL_CLOSED_TRANSFORM[edge]
        )}
        style={isHorizontalEdge ? { height: size } : { width: size }}
      >
        <div
          {...handlers}
          className={cn(
            "flex min-h-0 min-w-0 flex-1 flex-col transition-[background-color,backdrop-filter] duration-300 ease-out",
            active ? "bg-panel/10 backdrop-blur-[2px]" : "bg-panel/97 backdrop-blur-md"
          )}
          style={{
            paddingTop: edge === "top" ? "env(safe-area-inset-top)" : undefined,
            paddingBottom: edge === "bottom" ? "env(safe-area-inset-bottom)" : undefined,
            paddingLeft: edge === "left" ? "env(safe-area-inset-left)" : undefined,
            paddingRight: edge === "right" ? "env(safe-area-inset-right)" : undefined,
          }}
        >
          <PanelTransparencyContext.Provider value={active}>{children}</PanelTransparencyContext.Provider>
        </div>
      </div>
    </>
  );
}
