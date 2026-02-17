/*
 * AC Skin Changer - NEVERLOSE-Style Menu
 * Full-featured skin changer UI using the custom rendering engine.
 * Layout: Sidebar + Content area with weapon browser, skin cards,
 *         float slider, pattern seed, StatTrak toggle, and config management.
 */

#include "core.h"
#include "render_engine.h"

#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_internal.h"

namespace {
    // Menu state
    int currentPage = 0;          // 0=Skins, 1=Knives, 2=Gloves, 3=Configs, 4=Settings
    int selectedCategory = 0;
    int selectedWeapon = -1;
    int selectedSkin = -1;
    char searchBuf[128] = "";
    char presetNameBuf[64] = "my_preset";

    // Notification
    std::string notifMessage;
    float notifAlpha = 0.0f;
    double notifTime = 0.0;

    // Skin detail edit state
    SkinConfig editConfig;
    bool showSkinDetail = false;

    void ShowNotification(const char* msg) {
        notifMessage = msg;
        notifAlpha = 1.0f;
        notifTime = ImGui::GetTime();
    }

    // Get categories for current page
    std::vector<std::string> GetCurrentCategories() {
        std::vector<std::string> cats;
        switch (currentPage) {
            case 0: // Skins (all weapons)
                cats = { "Pistols", "Rifles", "SMGs", "Shotguns", "Machine Guns", "Snipers" };
                break;
            case 1: // Knives
                cats = { "Knives" };
                break;
            case 2: // Gloves
                cats = { "Gloves" };
                break;
            default:
                break;
        }
        return cats;
    }

    // Get weapons for a category
    std::vector<WeaponInfo*> GetWeaponsForCategory(const std::string& category) {
        std::vector<WeaponInfo*> result;
        auto it = G::weaponsByCategory.find(category);
        if (it != G::weaponsByCategory.end()) {
            for (auto& w : it->second) {
                // Apply search filter
                if (searchBuf[0] != '\0') {
                    std::string search = searchBuf;
                    std::string name = w.name;
                    std::transform(search.begin(), search.end(), search.begin(), ::tolower);
                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                    if (name.find(search) == std::string::npos) continue;
                }
                // We need a mutable pointer, iterate from global list
                for (auto& gw : G::allWeapons) {
                    if (gw.id == w.id) {
                        result.push_back(&gw);
                        break;
                    }
                }
            }
        }
        return result;
    }

    void RenderSidebar() {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 windowPos = ImGui::GetWindowPos();

        // Sidebar background
        float sidebarW = 200.0f;
        float windowH = ImGui::GetWindowSize().y;
        dl->AddRectFilled(windowPos, ImVec2(windowPos.x + sidebarW, windowPos.y + windowH),
                          ACRender::Colors::SidebarBg);

        // Separator line
        dl->AddLine(ImVec2(windowPos.x + sidebarW, windowPos.y),
                     ImVec2(windowPos.x + sidebarW, windowPos.y + windowH),
                     ACRender::Colors::Border);

        // Logo area
        ImGui::SetCursorPos(ImVec2(16, 16));
        ImGui::BeginGroup();

        // Logo circle with "AC" text
        ImVec2 logoCenter(windowPos.x + 30, windowPos.y + 32);
        dl->AddCircleFilled(logoCenter, 16.0f, ACRender::Colors::AccentBlue);
        dl->AddText(ImVec2(logoCenter.x - 8, logoCenter.y - 7), IM_COL32(255, 255, 255, 255), "AC");

        ImGui::SetCursorPos(ImVec2(54, 20));
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.94f, 1.0f), "Skin Changer");
        ImGui::SetCursorPos(ImVec2(54, 36));
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.63f, 1.0f), "v2.0.0");

        ImGui::EndGroup();

        ImGui::SetCursorPos(ImVec2(8, 70));

        // Navigation items
        ACRender::GradientSeparator();
        ImGui::SetCursorPosX(8);

        struct NavItem { const char* icon; const char* label; };
        NavItem items[] = {
            { ">", "Weapons" },
            { "*", "Knives" },
            { "#", "Gloves" },
            { "=", "Configs" },
            { "@", "Settings" },
        };

        for (int i = 0; i < 5; i++) {
            ImGui::SetCursorPosX(8);
            if (ACRender::SidebarItem(items[i].icon, items[i].label, currentPage == i, sidebarW - 16)) {
                currentPage = i;
                selectedWeapon = -1;
                selectedSkin = -1;
                showSkinDetail = false;
                searchBuf[0] = '\0';
            }
        }

        // Equipped skins counter at bottom
        ImGui::SetCursorPos(ImVec2(16, windowH - 60));
        {
            std::lock_guard<std::mutex> lock(G::skinMutex);
            char countBuf[64];
            snprintf(countBuf, sizeof(countBuf), "%zu skins equipped", G::equippedSkins.size());
            ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.63f, 1.0f), "%s", countBuf);
        }

        // Status indicator
        ImGui::SetCursorPos(ImVec2(16, windowH - 36));
        ImVec2 statusDotPos(windowPos.x + 22, windowPos.y + windowH - 28);
        dl->AddCircleFilled(statusDotPos, 4.0f, G::initialized ? ACRender::Colors::AccentGreen : ACRender::Colors::AccentRed);
        ImGui::SetCursorPos(ImVec2(32, windowH - 36));
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.63f, 1.0f), G::initialized ? "Connected" : "Waiting...");
    }

    void RenderWeaponBrowser() {
        auto categories = GetCurrentCategories();
        if (categories.empty()) return;

        // Category tabs
        ImGui::SetCursorPosX(220);
        ImGui::BeginGroup();

        for (size_t i = 0; i < categories.size(); i++) {
            if (i > 0) ImGui::SameLine();
            if (ACRender::TabButton(categories[i].c_str(), selectedCategory == (int)i)) {
                selectedCategory = (int)i;
                selectedWeapon = -1;
                selectedSkin = -1;
                showSkinDetail = false;
            }
        }

        ImGui::EndGroup();

        // Search bar
        ImGui::SetCursorPos(ImVec2(220, ImGui::GetCursorPosY() + 8));
        ACRender::SearchInput("##search", searchBuf, sizeof(searchBuf), 400);

        ImGui::SetCursorPos(ImVec2(220, ImGui::GetCursorPosY() + 12));

        // Get weapons for selected category
        std::string catName = (selectedCategory < (int)categories.size()) ? categories[selectedCategory] : "";
        auto weapons = GetWeaponsForCategory(catName);

        if (weapons.empty()) {
            ImGui::SetCursorPos(ImVec2(220, ImGui::GetCursorPosY() + 40));
            ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.63f, 1.0f), "No weapons found");
            return;
        }

        // Weapon cards grid
        float startX = 220.0f;
        float cardW = 160.0f;
        float cardH = 120.0f;
        float spacing = 10.0f;
        float contentWidth = ImGui::GetWindowSize().x - startX - 20;
        int cols = std::max(1, (int)(contentWidth / (cardW + spacing)));

        ImGui::BeginChild("##weapons_scroll", ImVec2(contentWidth, ImGui::GetContentRegionAvail().y - 10), false);

        if (!showSkinDetail) {
            // Show weapon list
            int col = 0;
            for (size_t i = 0; i < weapons.size(); i++) {
                auto* wp = weapons[i];

                // Check if this weapon has a skin equipped
                bool hasEquipped = false;
                {
                    std::lock_guard<std::mutex> lock(G::skinMutex);
                    hasEquipped = G::equippedSkins.count(wp->id) > 0;
                }

                if (col > 0) ImGui::SameLine(0, spacing);

                if (ACRender::SkinCard(wp->name.c_str(), hasEquipped ? "Equipped" : "Default",
                                        hasEquipped ? 6 : 1, hasEquipped, ImVec2(cardW, cardH))) {
                    selectedWeapon = (int)i;
                    selectedSkin = -1;
                }

                col++;
                if (col >= cols) col = 0;
            }

            // If a weapon is selected, show its skins below
            if (selectedWeapon >= 0 && selectedWeapon < (int)weapons.size()) {
                auto* wp = weapons[selectedWeapon];
                ImGui::Dummy(ImVec2(0, 20));
                ACRender::SectionHeader(wp->name.c_str());

                col = 0;
                for (size_t s = 0; s < wp->skins.size(); s++) {
                    auto& skin = wp->skins[s];

                    bool isEquipped = false;
                    {
                        std::lock_guard<std::mutex> lock(G::skinMutex);
                        auto it = G::equippedSkins.find(wp->id);
                        if (it != G::equippedSkins.end() && it->second.paintKit == skin.paintKit)
                            isEquipped = true;
                    }

                    if (col > 0) ImGui::SameLine(0, spacing);

                    // Apply search filter to skins too
                    if (searchBuf[0] != '\0') {
                        std::string search = searchBuf;
                        std::string name = skin.name;
                        std::transform(search.begin(), search.end(), search.begin(), ::tolower);
                        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                        if (name.find(search) == std::string::npos) continue;
                    }

                    if (ACRender::SkinCard(wp->name.c_str(), skin.name.c_str(),
                                            skin.rarity, isEquipped, ImVec2(cardW, cardH))) {
                        // Open detail view
                        selectedSkin = (int)s;
                        showSkinDetail = true;
                        editConfig.weaponId = wp->id;
                        editConfig.paintKit = skin.paintKit;
                        editConfig.weaponName = wp->name;
                        editConfig.skinName = skin.name;

                        // Load existing config if equipped
                        std::lock_guard<std::mutex> lock(G::skinMutex);
                        auto it = G::equippedSkins.find(wp->id);
                        if (it != G::equippedSkins.end() && it->second.paintKit == skin.paintKit) {
                            editConfig = it->second;
                        } else {
                            editConfig.seed = 0;
                            editConfig.wear = 0.0001f;
                            editConfig.statTrak = -1;
                            editConfig.enabled = true;
                        }
                    }

                    col++;
                    if (col >= cols) col = 0;
                }
            }
        } else {
            // Skin detail view
            RenderSkinDetail(weapons);
        }

        ImGui::EndChild();
    }

    void RenderSkinDetail(const std::vector<WeaponInfo*>& weapons) {
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Back button
        if (ACRender::StyledButton("< Back", ImVec2(80, 32), ACRender::Colors::CardBg)) {
            showSkinDetail = false;
            return;
        }

        ImGui::Dummy(ImVec2(0, 10));

        // Header
        char headerBuf[256];
        snprintf(headerBuf, sizeof(headerBuf), "%s | %s", editConfig.weaponName.c_str(), editConfig.skinName.c_str());
        ACRender::SectionHeader(headerBuf);

        // Detail card
        ImVec2 detailPos = ImGui::GetCursorScreenPos();
        float detailW = std::min(500.0f, ImGui::GetContentRegionAvail().x - 20);
        float detailH = 360.0f;

        dl->AddRectFilled(detailPos, ImVec2(detailPos.x + detailW, detailPos.y + detailH),
                          ACRender::Colors::CardBg, 12.0f);
        dl->AddRect(detailPos, ImVec2(detailPos.x + detailW, detailPos.y + detailH),
                     ACRender::Colors::Border, 12.0f);

        ImGui::SetCursorScreenPos(ImVec2(detailPos.x + 20, detailPos.y + 20));
        ImGui::BeginGroup();

        // Weapon + Skin name
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.94f, 1.0f), "%s", editConfig.weaponName.c_str());
        ImGui::TextColored(ImVec4(0.23f, 0.51f, 0.97f, 1.0f), "%s", editConfig.skinName.c_str());

        ImGui::Dummy(ImVec2(0, 4));
        char pkBuf[64];
        snprintf(pkBuf, sizeof(pkBuf), "Paint Kit: %d", editConfig.paintKit);
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.63f, 1.0f), "%s", pkBuf);

        ImGui::Dummy(ImVec2(0, 16));

        // Wear slider
        ImGui::PushItemWidth(detailW - 40);
        ACRender::WearSlider("Wear", &editConfig.wear);
        ImGui::Dummy(ImVec2(0, 16));

        // Pattern seed
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.63f, 1.0f), "Pattern Seed");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.11f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::InputInt("##seed", &editConfig.seed, 1, 100);
        if (editConfig.seed < 0) editConfig.seed = 0;
        if (editConfig.seed > 1000) editConfig.seed = 1000;
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 8));

        // StatTrak toggle
        bool hasStatTrak = (editConfig.statTrak >= 0);
        if (ACRender::ToggleSwitch("StatTrak", &hasStatTrak)) {
            editConfig.statTrak = hasStatTrak ? 0 : -1;
        }

        if (hasStatTrak) {
            ImGui::SameLine(0, 16);
            ImGui::PushItemWidth(100);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.11f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
            ImGui::InputInt("##stattrak", &editConfig.statTrak, 1, 100);
            if (editConfig.statTrak < 0) editConfig.statTrak = 0;
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            ImGui::PopItemWidth();
        }

        ImGui::PopItemWidth();
        ImGui::Dummy(ImVec2(0, 16));

        // Action buttons
        if (ACRender::StyledButton("Apply Skin", ImVec2(140, 36), ACRender::Colors::AccentBlue)) {
            std::lock_guard<std::mutex> lock(G::skinMutex);
            G::equippedSkins[editConfig.weaponId] = editConfig;
            ShowNotification("Skin applied! It will appear in your inventory.");
            LogMsg("Applied skin: %s | %s (PK: %d)", editConfig.weaponName.c_str(),
                   editConfig.skinName.c_str(), editConfig.paintKit);
        }

        ImGui::SameLine(0, 10);

        // Check if this skin is currently equipped
        bool isCurrentlyEquipped = false;
        {
            std::lock_guard<std::mutex> lock(G::skinMutex);
            auto it = G::equippedSkins.find(editConfig.weaponId);
            if (it != G::equippedSkins.end() && it->second.paintKit == editConfig.paintKit)
                isCurrentlyEquipped = true;
        }

        if (isCurrentlyEquipped) {
            if (ACRender::StyledButton("Remove", ImVec2(100, 36), ACRender::Colors::AccentRed)) {
                std::lock_guard<std::mutex> lock(G::skinMutex);
                G::equippedSkins.erase(editConfig.weaponId);
                ShowNotification("Skin removed from inventory.");
            }
        }

        ImGui::EndGroup();

        ImGui::SetCursorScreenPos(ImVec2(detailPos.x, detailPos.y + detailH + 10));
    }

    void RenderConfigPage() {
        float contentX = 220.0f;
        ImGui::SetCursorPos(ImVec2(contentX, 20));

        ACRender::SectionHeader("Configuration Presets");

        ImGui::SetCursorPosX(contentX);
        ImGui::BeginGroup();

        // Preset name input
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.63f, 1.0f), "Preset Name:");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.11f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("##preset_name", presetNameBuf, sizeof(presetNameBuf));
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 8));

        // Save button
        if (ACRender::StyledButton("Save Preset", ImVec2(140, 36), ACRender::Colors::AccentBlue)) {
            Config::Save(presetNameBuf);
            ShowNotification("Preset saved!");
        }

        ImGui::Dummy(ImVec2(0, 20));
        ACRender::GradientSeparator();

        // List existing presets
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.94f, 1.0f), "Saved Presets:");
        ImGui::Dummy(ImVec2(0, 8));

        auto presets = Config::ListPresets();
        if (presets.empty()) {
            ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.63f, 1.0f), "No presets saved yet");
        } else {
            for (auto& preset : presets) {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 pos = ImGui::GetCursorScreenPos();
                float cardW = 400.0f;
                float cardH = 50.0f;

                dl->AddRectFilled(pos, ImVec2(pos.x + cardW, pos.y + cardH),
                                  ACRender::Colors::CardBg, 8.0f);
                dl->AddRect(pos, ImVec2(pos.x + cardW, pos.y + cardH),
                             ACRender::Colors::Border, 8.0f);

                // Preset name
                dl->AddText(ImVec2(pos.x + 12, pos.y + 8), ACRender::Colors::TextPrimary, preset.c_str());

                // Load button
                ImGui::SetCursorScreenPos(ImVec2(pos.x + cardW - 170, pos.y + 10));
                char loadId[128];
                snprintf(loadId, sizeof(loadId), "Load##%s", preset.c_str());
                if (ACRender::StyledButton(loadId, ImVec2(70, 30), ACRender::Colors::AccentBlue)) {
                    Config::Load(preset);
                    ShowNotification("Preset loaded!");
                }

                ImGui::SameLine(0, 6);

                char delId[128];
                snprintf(delId, sizeof(delId), "Del##%s", preset.c_str());
                if (ACRender::StyledButton(delId, ImVec2(60, 30), ACRender::Colors::AccentRed)) {
                    Config::Delete(preset);
                    ShowNotification("Preset deleted!");
                }

                ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + cardH + 8));
            }
        }

        ImGui::EndGroup();
    }

    void RenderSettingsPage() {
        float contentX = 220.0f;
        ImGui::SetCursorPos(ImVec2(contentX, 20));

        ACRender::SectionHeader("Settings");

        ImGui::SetCursorPosX(contentX);
        ImGui::BeginGroup();

        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.94f, 1.0f), "AC Skin Changer v2.0.0");
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.63f, 1.0f), "Custom Rendering Engine + DX11 ImGui");
        ImGui::Dummy(ImVec2(0, 12));

        ACRender::GradientSeparator();

        // Equipped skins overview
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.94f, 1.0f), "Currently Equipped:");
        ImGui::Dummy(ImVec2(0, 8));

        {
            std::lock_guard<std::mutex> lock(G::skinMutex);
            if (G::equippedSkins.empty()) {
                ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.63f, 1.0f), "No skins equipped");
            } else {
                for (auto& [id, cfg] : G::equippedSkins) {
                    char buf[256];
                    snprintf(buf, sizeof(buf), "%s | %s (PK: %d, Wear: %.4f)",
                             cfg.weaponName.c_str(), cfg.skinName.c_str(),
                             cfg.paintKit, cfg.wear);

                    ACRender::StatDisplay("-", buf, ACRender::Colors::AccentBlue);
                }
            }
        }

        ImGui::Dummy(ImVec2(0, 20));
        ACRender::GradientSeparator();

        // Controls info
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.94f, 1.0f), "Controls:");
        ImGui::Dummy(ImVec2(0, 4));
        ACRender::StatDisplay("INSERT", "Toggle menu", ACRender::Colors::TextSecondary);
        ACRender::StatDisplay("END", "Unload cheat", ACRender::Colors::TextSecondary);

        ImGui::Dummy(ImVec2(0, 20));

        // Clear all button
        if (ACRender::StyledButton("Clear All Skins", ImVec2(160, 36), ACRender::Colors::AccentRed)) {
            std::lock_guard<std::mutex> lock(G::skinMutex);
            G::equippedSkins.clear();
            ShowNotification("All skins cleared!");
        }

        ImGui::EndGroup();
    }

    // Forward declaration needed for RenderSkinDetail
    void RenderSkinDetail(const std::vector<WeaponInfo*>& weapons);
}

namespace Menu {
    void SetupStyle() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // Rounding
        style.WindowRounding = 12.0f;
        style.FrameRounding = 6.0f;
        style.PopupRounding = 8.0f;
        style.ScrollbarRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 6.0f;
        style.ChildRounding = 8.0f;

        // Padding
        style.WindowPadding = ImVec2(0, 0);
        style.FramePadding = ImVec2(8, 5);
        style.ItemSpacing = ImVec2(8, 6);
        style.ItemInnerSpacing = ImVec2(6, 4);
        style.ScrollbarSize = 10.0f;
        style.GrabMinSize = 8.0f;

        // Border
        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;

        // Colors - NEVERLOSE dark theme
        ImVec4 bg      = ImVec4(0.051f, 0.051f, 0.067f, 1.0f);
        ImVec4 card     = ImVec4(0.086f, 0.086f, 0.118f, 1.0f);
        ImVec4 accent   = ImVec4(0.231f, 0.510f, 0.965f, 1.0f);
        ImVec4 text     = ImVec4(0.902f, 0.902f, 0.941f, 1.0f);
        ImVec4 textDim  = ImVec4(0.549f, 0.549f, 0.627f, 1.0f);
        ImVec4 border   = ImVec4(0.157f, 0.157f, 0.216f, 1.0f);

        colors[ImGuiCol_WindowBg]           = bg;
        colors[ImGuiCol_ChildBg]            = ImVec4(0, 0, 0, 0);
        colors[ImGuiCol_PopupBg]            = ImVec4(0.071f, 0.071f, 0.094f, 0.94f);
        colors[ImGuiCol_Border]             = border;
        colors[ImGuiCol_BorderShadow]       = ImVec4(0, 0, 0, 0);
        colors[ImGuiCol_FrameBg]            = card;
        colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.110f, 0.110f, 0.149f, 1.0f);
        colors[ImGuiCol_FrameBgActive]      = ImVec4(0.137f, 0.137f, 0.188f, 1.0f);
        colors[ImGuiCol_TitleBg]            = bg;
        colors[ImGuiCol_TitleBgActive]      = bg;
        colors[ImGuiCol_TitleBgCollapsed]   = bg;
        colors[ImGuiCol_MenuBarBg]          = bg;
        colors[ImGuiCol_ScrollbarBg]        = ImVec4(0, 0, 0, 0);
        colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.2f, 0.2f, 0.27f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabHovered] = accent;
        colors[ImGuiCol_ScrollbarGrabActive]  = accent;
        colors[ImGuiCol_CheckMark]          = accent;
        colors[ImGuiCol_SliderGrab]         = accent;
        colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.31f, 0.59f, 1.0f, 1.0f);
        colors[ImGuiCol_Button]             = card;
        colors[ImGuiCol_ButtonHovered]      = ImVec4(0.110f, 0.110f, 0.149f, 1.0f);
        colors[ImGuiCol_ButtonActive]       = accent;
        colors[ImGuiCol_Header]             = card;
        colors[ImGuiCol_HeaderHovered]      = ImVec4(0.110f, 0.110f, 0.149f, 1.0f);
        colors[ImGuiCol_HeaderActive]       = accent;
        colors[ImGuiCol_Separator]          = border;
        colors[ImGuiCol_SeparatorHovered]   = accent;
        colors[ImGuiCol_SeparatorActive]    = accent;
        colors[ImGuiCol_ResizeGrip]         = ImVec4(0, 0, 0, 0);
        colors[ImGuiCol_ResizeGripHovered]  = accent;
        colors[ImGuiCol_ResizeGripActive]   = accent;
        colors[ImGuiCol_Tab]                = card;
        colors[ImGuiCol_TabHovered]         = accent;
        colors[ImGuiCol_Text]               = text;
        colors[ImGuiCol_TextDisabled]       = textDim;
        colors[ImGuiCol_PlotLines]          = accent;
        colors[ImGuiCol_PlotHistogram]      = accent;

        LogMsg("NEVERLOSE style applied");
    }

    void Render() {
        ImGuiIO& io = ImGui::GetIO();

        // Fade notification
        if (notifAlpha > 0.0f) {
            double elapsed = ImGui::GetTime() - notifTime;
            if (elapsed > 2.0) {
                notifAlpha -= io.DeltaTime * 2.0f;
                if (notifAlpha < 0) notifAlpha = 0;
            }
        }

        // Main window
        ImGui::SetNextWindowSize(ImVec2(960, 640), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - 480, io.DisplaySize.y * 0.5f - 320), ImGuiCond_FirstUseEver);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar
                                | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::Begin("AC Skin Changer", &G::menuOpen, flags);

        // Draw sidebar
        RenderSidebar();

        // Main content area
        switch (currentPage) {
            case 0: // Weapons
            case 1: // Knives
            case 2: // Gloves
                RenderWeaponBrowser();
                break;
            case 3: // Configs
                RenderConfigPage();
                break;
            case 4: // Settings
                RenderSettingsPage();
                break;
        }

        // Draw notification
        if (notifAlpha > 0.01f) {
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            ACRender::DrawNotification(dl, notifMessage.c_str(), notifAlpha, io.DisplaySize);
        }

        ImGui::End();
    }
}
