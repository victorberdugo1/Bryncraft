import { EFFECT_DEFINITIONS, EFFECT_IDS, type EffectId } from "@/types/effects";
import { EffectThumbnail } from "@/components/effects/EffectThumbnail";
import { useAppStore } from "@/store/useAppStore";
import { usePanelTransparentBg } from "@/hooks/usePanelTransparency";
import { cn } from "@/lib/utils";

// Orden de aparición en la barra lateral = orden de EFFECT_MODULES
// (src/effects/index.ts). Un efecto nuevo aparece acá solo.
const ORDER: EffectId[] = EFFECT_IDS;

export function LeftSidebar() {
  const activeEffect = useAppStore((s) => s.activeEffect);
  const setActiveEffect = useAppStore((s) => s.setActiveEffect);
  const searchQuery = useAppStore((s) => s.searchQuery);
  const transparent = usePanelTransparentBg();

  const query = searchQuery.trim().toLowerCase();
  const visible = query
    ? ORDER.filter((id) => {
        const def = EFFECT_DEFINITIONS[id];
        return def.name.toLowerCase().includes(query) || def.description.toLowerCase().includes(query);
      })
    : ORDER;

  return (
    <aside
      className={cn(
        "flex h-full w-full flex-col border-r border-border transition-colors duration-300",
        transparent ? "bg-transparent" : "bg-panel"
      )}
    >
      <div className="flex h-8 items-center px-3 text-[11px] font-semibold uppercase tracking-wide text-muted-foreground">
        Effect Library
      </div>
      <div className="flex-1 space-y-3 overflow-y-auto px-3 pb-3">
        {visible.length === 0 && (
          <p className="pt-3 text-center text-[11px] text-muted-foreground">No effects match "{searchQuery}"</p>
        )}
        {visible.map((id) => {
          const def = EFFECT_DEFINITIONS[id];
          const active = id === activeEffect;
          return (
            <button
              key={id}
              onClick={() => setActiveEffect(id)}
              className={cn(
                "block w-full rounded-md p-1.5 text-left transition-colors",
                active ? "bg-panel-raised" : "hover:bg-panel-raised/60"
              )}
            >
              <EffectThumbnail effect={id} active={active} />
              <div className="mt-1.5 flex items-center justify-between">
                <span className={cn("text-[12px] font-medium", active ? "text-accent" : "text-foreground")}>
                  {def.name}
                </span>
              </div>
              <p className="mt-0.5 text-[10.5px] leading-snug text-muted-foreground">{def.description}</p>
            </button>
          );
        })}
      </div>
    </aside>
  );
}
