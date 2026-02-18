import { Crosshair, Package, Settings } from "lucide-react";

interface SidebarProps {
  activeTab: string;
  onTabChange: (tab: string) => void;
}

const navItems = [
  { id: "items", label: "Items", icon: Package },
  { id: "configs", label: "Configs", icon: Settings },
];

export function Sidebar({ activeTab, onTabChange }: SidebarProps) {
  return (
    <div className="w-[60px] h-full flex flex-col items-center py-5 bg-[#090e1a] border-r border-white/[0.04]">
      <div className="mb-6">
        <div className="w-8 h-8 rounded-md bg-gradient-to-br from-blue-500 to-indigo-600 flex items-center justify-center">
          <Crosshair className="w-4 h-4 text-white" />
        </div>
      </div>
      <nav className="flex flex-col items-center gap-0.5 flex-1">
        {navItems.map((item) => {
          const Icon = item.icon;
          const isActive = activeTab === item.id;
          return (
            <button
              key={item.id}
              onClick={() => onTabChange(item.id)}
              className={`
                w-10 h-10 rounded-md flex items-center justify-center transition-all cursor-pointer
                ${isActive
                  ? "bg-white/[0.08] text-white/80"
                  : "text-white/20 hover:text-white/40 hover:bg-white/[0.03]"
                }
              `}
              title={item.label}
            >
              <Icon className="w-[18px] h-[18px]" />
            </button>
          );
        })}
      </nav>
    </div>
  );
}
