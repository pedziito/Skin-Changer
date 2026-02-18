import { X } from "lucide-react";
import { WeaponItem, rarityColors } from "./WeaponCard";

interface ItemDetailPanelProps {
  item: WeaponItem;
  onClose: () => void;
}

export function ItemDetailPanel({ item, onClose }: ItemDetailPanelProps) {
  const color = rarityColors[item.rarity];

  return (
    <div className="w-72 h-full bg-[#0c1120] border-l border-white/[0.04] flex flex-col overflow-y-auto">
      <div className="flex items-center justify-between px-4 py-3 border-b border-white/[0.04]">
        <span className="text-white/60 text-[12px]">Details</span>
        <button onClick={onClose} className="p-1 rounded text-white/20 hover:text-white/50 hover:bg-white/5 transition-all cursor-pointer">
          <X className="w-3.5 h-3.5" />
        </button>
      </div>

      <div className="px-4 py-4">
        <div className="relative aspect-square rounded-lg overflow-hidden flex items-center justify-center border border-white/[0.04]" style={{ background: "#12182b" }}>
          <div className="absolute top-0 left-0 right-0 h-[2px]" style={{ background: color }} />
          <div className="w-3/4 h-3/4 rounded-md opacity-[0.06]" style={{ background: color }} />
        </div>
        <div className="mt-3">
          <p className="text-white/80 text-[13px]">{item.name}</p>
          <p className="text-[11px] mt-0.5" style={{ color }}>{item.skin}</p>
          <p className="text-[10px] text-white/20 mt-1">{item.rarity}</p>
        </div>
      </div>

      <div className="px-4 space-y-4 pb-5 flex-1">
        {/* Make this panel read-only: no float/wear editing here */}
        <div>
          <p className="text-[11px] text-white/30">Wear</p>
          <p className="text-[12px] text-white/70 mt-1">{item.wear ?? "—"}</p>
        </div>

        {item.stattrak && (
          <div>
            <p className="text-[11px] text-white/30">StatTrak</p>
            <p className="text-[12px] text-orange-400/70 mt-1">Yes</p>
          </div>
        )}
      </div>

      <div className="px-4 pb-4 flex gap-2">
        <button className="flex-1 flex items-center justify-center gap-1.5 px-3 py-2 bg-white/[0.03] border border-white/[0.06] rounded text-[11px] text-white/30 hover:text-white/50 hover:bg-white/[0.05] transition-all cursor-not-allowed" disabled>
          View Only
        </button>
      </div>
    </div>
  );
}
