export interface WeaponItem {
  id: string;
  name: string;
  skin: string;
  rarity: "consumer" | "industrial" | "milspec" | "restricted" | "classified" | "covert" | "extraordinary" | "contraband";
  wear?: string;
  stattrak?: boolean;
  image?: string; // optional path to asset image
}

export const rarityColors: Record<string, string> = {
  consumer: "#b0c3d9",
  industrial: "#5e98d9",
  milspec: "#4b69ff",
  restricted: "#8847ff",
  classified: "#d32ce6",
  covert: "#eb4b4b",
  extraordinary: "#e4ae39",
  contraband: "#e4ae39",
};

interface WeaponCardProps {
  item: WeaponItem;
  onClick?: (item: WeaponItem) => void;
  selected?: boolean;
}

export function WeaponCard({ item, onClick, selected }: WeaponCardProps) {
  const color = rarityColors[item.rarity];

  // try to resolve a default image path if not provided
  const src = item.image ?? `/assets/skins/${item.name.replace(/\s+/g, "_").toLowerCase()}.png`;

  return (
    <div
      onClick={() => onClick?.(item)}
      className={`
        group relative aspect-square rounded-lg overflow-hidden cursor-pointer transition-all duration-200
        border
        ${selected
          ? "border-blue-500/50 bg-[#171e36]"
          : "border-white/[0.04] bg-[#12182b] hover:border-white/[0.08] hover:bg-[#161d33]"
        }
      `}
    >
      <div className="absolute top-0 left-0 right-0 h-[2px]" style={{ background: color }} />

      {item.stattrak && (
        <div className="absolute top-2.5 left-2.5 z-10">
          <span className="text-[9px] tracking-wider px-1.5 py-0.5 rounded-[3px] bg-orange-500/15 text-orange-400/90 border border-orange-500/20">
            ST
          </span>
        </div>
      )}

      <div className="absolute inset-0 flex items-center justify-center p-1">
        {/** Use real image if available; fallback to colored panel */}
        <img src={src} alt={item.skin} className="object-cover w-full h-full rounded-md opacity-95" onError={(e)=>{(e.target as HTMLImageElement).style.display='none'}} />
        <div
          className="absolute inset-0 rounded-md opacity-[0.04] group-hover:opacity-[0.07] transition-opacity"
          style={{ background: color }}
        />
      </div>

      <div className="absolute bottom-0 left-0 right-0 px-2.5 pb-2.5">
        <p className="text-white/80 text-[11px] truncate">{item.name}</p>
        <p className="text-[10px] truncate mt-px" style={{ color }}>{item.skin}</p>
        {item.wear && (
          <p className="text-[9px] text-white/20 mt-px">{item.wear}</p>
        )}
      </div>
    </div>
  );
}
