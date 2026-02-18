import { useState } from "react";
import { Search, Plus } from "lucide-react";
import { WeaponCard, WeaponItem } from "./WeaponCard";
import { ItemDetailPanel } from "./ItemDetailPanel";

const initialItems: WeaponItem[] = [
  { id: "1", name: "AK-47", skin: "X-Ray", rarity: "classified", wear: "Factory New", stattrak: true },
  { id: "2", name: "AWP", skin: "Man-o'-war", rarity: "covert", wear: "Minimal Wear" },
  { id: "3", name: "Desert Eagle", skin: "Sunset Storm", rarity: "restricted", wear: "Field-Tested" },
  { id: "4", name: "Dual Berettas", skin: "Hemoglobin", rarity: "milspec", wear: "Minimal Wear", stattrak: true },
  { id: "5", name: "Nova", skin: "Bloomstick", rarity: "restricted", wear: "Factory New" },
  { id: "6", name: "SSG 08", skin: "Dragonfire", rarity: "covert", wear: "Well-Worn" },
  { id: "7", name: "USP-S", skin: "Kill Confirmed", rarity: "covert", wear: "Factory New", stattrak: true },
  { id: "8", name: "Specialist Gloves", skin: "Crimson Kimono", rarity: "extraordinary", wear: "Field-Tested" },
  { id: "9", name: "M4A4", skin: "Howl", rarity: "contraband", wear: "Factory New" },
  { id: "10", name: "Karambit", skin: "Fade", rarity: "covert", wear: "Factory New", stattrak: true },
  { id: "11", name: "Glock-18", skin: "Fade", rarity: "restricted", wear: "Minimal Wear" },
  { id: "12", name: "P250", skin: "Asiimov", rarity: "classified", wear: "Field-Tested" },
];

const categories = ["Pistols", "Rifles", "SMGs", "Heavy", "Knives", "Gloves"];

// simple demo DB mapping for weapons -> skins (minimal)
const weaponsByCategory: Record<string, { id: string; name: string; skins: string[] }[]> = {
  Pistols: [
    { id: "glock", name: "Glock-18", skins: ["Fade", "Night", "Twilight"] },
    { id: "deagle", name: "Desert Eagle", skins: ["Sunset Storm", "Golden Koi"] },
    { id: "usp", name: "USP-S", skins: ["Kill Confirmed", "Nitro"] },
  ],
  Rifles: [
    { id: "ak", name: "AK-47", skins: ["Redline", "X-Ray", "Emerald Web"] },
    { id: "m4", name: "M4A4", skins: ["Howl", "Desert Strike"] },
    { id: "awp", name: "AWP", skins: ["Asiimov", "Man-o'-war", "Dragon Lore"] },
  ],
  SMGs: [ { id: "mp9", name: "MP9", skins: ["Punch", "Bulldozer"] } ],
  Heavy: [],
  Knives: [ { id: "kar", name: "Karambit", skins: ["Fade", "Case Hardened"] } ],
  Gloves: [ { id: "specialist", name: "Specialist Gloves", skins: ["Crimson Kimono"] } ],
};

export function ItemsPage() {
  const [searchQuery, setSearchQuery] = useState("");
  const [items, setItems] = useState<WeaponItem[]>(initialItems);
  const [selectedItem, setSelectedItem] = useState<WeaponItem | null>(null);
  const [showAddModal, setShowAddModal] = useState(false);

  const filteredItems = items.filter((item) => {
    const q = searchQuery.toLowerCase();
    return item.name.toLowerCase().includes(q) || item.skin.toLowerCase().includes(q);
  });

  function addItem(newItem: WeaponItem) {
    setItems((s) => [newItem, ...s]);
  }

  return (
    <div className="flex-1 flex h-full overflow-hidden">
      <div className="flex-1 flex flex-col h-full overflow-hidden">
        <div className="px-5 pt-5 pb-3 flex items-center gap-3">
          <div className="relative flex-1 max-w-xs">
            <Search className="absolute left-2.5 top-1/2 -translate-y-1/2 w-3.5 h-3.5 text-white/20" />
            <input
              type="text"
              placeholder="Search..."
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="w-full pl-8 pr-3 py-1.5 bg-white/[0.03] border border-white/[0.06] rounded-md text-[12px] text-white/80 placeholder:text-white/20 focus:outline-none focus:border-white/[0.12] transition-colors"
            />
          </div>
          <span className="text-[11px] text-white/20">{filteredItems.length} items</span>
        </div>

        <div className="flex-1 overflow-y-auto px-5 pb-5">
          <div className="grid grid-cols-[repeat(auto-fill,minmax(120px,1fr))] gap-2">
            <button onClick={() => setShowAddModal(true)} className="aspect-square rounded-lg border border-dashed border-white/[0.08] bg-white/[0.02] hover:bg-white/[0.04] hover:border-white/[0.15] flex items-center justify-center transition-all cursor-pointer group">
              <Plus className="w-5 h-5 text-white/15 group-hover:text-white/30 transition-colors" />
            </button>

            {filteredItems.map((item) => (
              <WeaponCard
                key={item.id}
                item={item}
                selected={selectedItem?.id === item.id}
                onClick={(i) => setSelectedItem(i.id === selectedItem?.id ? null : i)}
              />
            ))}
          </div>
        </div>
      </div>

      {selectedItem && (
        <ItemDetailPanel
          item={selectedItem}
          onClose={() => setSelectedItem(null)}
        />
      )}

      {showAddModal && (
        <AddSkinModal onClose={() => setShowAddModal(false)} onAdd={(cfg) => { addItem(cfg); setShowAddModal(false); }} />
      )}
    </div>
  );
}

function AddSkinModal({ onClose, onAdd }: { onClose: () => void; onAdd: (it: WeaponItem) => void }) {
  const [stage, setStage] = useState(0); // 0 categories, 1 weapons, 2 skins
  const [selCat, setSelCat] = useState<string | null>(null);
  const [selWpn, setSelWpn] = useState<{ id: string; name: string; skins: string[] } | null>(null);
  const [selSkin, setSelSkin] = useState<string | null>(null);
  const [wear, setWear] = useState(0.0);
  const [stattrak, setStattrak] = useState(false);

  return (
    <div className="fixed inset-0 flex items-center justify-center z-50">
      <div className="absolute inset-0 bg-black/60" onClick={onClose} />
      <div className="relative w-[820px] max-w-full bg-[#071024] border border-white/[0.04] rounded-lg p-4 flex gap-4">
        <div className="w-64">
          <h3 className="text-white/80 mb-2">Categories</h3>
          <div className="space-y-2">
            {categories.map((c) => (
              <button key={c} onClick={() => { setSelCat(c); setStage(1); }} className="w-full text-left px-3 py-2 rounded bg-white/[0.02] hover:bg-white/[0.03] transition-all">{c}</button>
            ))}
          </div>
        </div>

        <div className="flex-1">
          {stage === 1 && selCat && (
            <div>
              <h3 className="text-white/80 mb-2">Weapons — {selCat}</h3>
              <div className="grid grid-cols-3 gap-2">
                {(weaponsByCategory[selCat] || []).map((w) => (
                  <button key={w.id} onClick={() => { setSelWpn(w); setStage(2); }} className="text-left p-2 rounded bg-white/[0.02] hover:bg-white/[0.03]">{w.name}</button>
                ))}
              </div>
            </div>
          )}

          {stage === 2 && selWpn && (
            <div>
              <h3 className="text-white/80 mb-2">Skins — {selWpn.name}</h3>
              <div className="grid grid-cols-4 gap-2 mb-3">
                {selWpn.skins.map((s) => (
                  <button key={s} onClick={() => setSelSkin(s)} className={`p-2 rounded text-left ${selSkin === s ? 'bg-blue-500/10 border border-blue-500/20' : 'bg-white/[0.02] hover:bg-white/[0.03]'}`}>
                    {s}
                  </button>
                ))}
              </div>

              <div className="mb-3">
                <label className="text-[11px] text-white/40">Wear</label>
                <input type="range" min={0} max={100} value={Math.round(wear*100)} onChange={(e)=>setWear(Number(e.target.value)/100)} className="w-full" />
                <div className="text-[12px] text-white/60 mt-1">{(wear*100).toFixed(0)}%</div>
              </div>

              <div className="flex items-center gap-2 mb-3">
                <input id="stattrak" type="checkbox" checked={stattrak} onChange={(e)=>setStattrak(e.target.checked)} />
                <label htmlFor="stattrak" className="text-[12px] text-white/70">StatTrak</label>
              </div>

              <div className="flex gap-2">
                <button disabled={!selSkin} onClick={() => {
                  if (!selWpn || !selSkin) return;
                  const newItem: WeaponItem = {
                    id: Date.now().toString(),
                    name: selWpn.name,
                    skin: selSkin,
                    rarity: 'consumer',
                    wear: `${(wear*100).toFixed(0)}%`,
                    stattrak: stattrak,
                  };
                  onAdd(newItem);
                }} className="px-3 py-2 rounded bg-blue-500/10 border border-blue-500/20 text-blue-400/80 disabled:opacity-40">Add</button>
                <button onClick={onClose} className="px-3 py-2 rounded bg-white/[0.03] border border-white/[0.06]">Cancel</button>
              </div>
            </div>
          )}

          {stage === 0 && (
            <div className="text-white/60">Choose a category to start adding a skin.</div>
          )}
        </div>
      </div>
    </div>
  );
}
