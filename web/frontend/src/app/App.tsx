import { useState } from "react";
import { Sidebar } from "./components/Sidebar";
import { ItemsPage } from "./components/ItemsPage";
import { ConfigsPage } from "./components/ConfigsPage";

export default function App() {
  const [activeTab, setActiveTab] = useState("items");

  return (
    <div className="size-full flex overflow-hidden bg-[#0b1121]">
      <Sidebar activeTab={activeTab} onTabChange={setActiveTab} />
      <div className="flex-1 flex overflow-hidden">
        {activeTab === "items" && <ItemsPage />}
        {activeTab === "configs" && <ConfigsPage />}
      </div>
    </div>
  );
}
