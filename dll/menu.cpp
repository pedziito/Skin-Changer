/*
 * AC Skin Changer - Menu (Custom ACE Engine)
 * Clean dark UI matching AC subscription panel theme.
 * Uses ACE custom rendering engine — zero ImGui dependency.
 * Smooth rounded corners, no pixelated edges.
 */

#include "core.h"
#include "../engine/ace_engine.h"

namespace {
    // ========================================================================
    // PAGE DEFINITIONS
    // ========================================================================
    enum Page {
        PAGE_RIFLES, PAGE_PISTOLS, PAGE_SMGS, PAGE_HEAVY,
        PAGE_KNIVES, PAGE_GLOVES, PAGE_CONFIGS, PAGE_SETTINGS,
        PAGE_COUNT
    };

    struct PageInfo {
        const char* name;
        const char* icon;
        const char* category;   // key in G::weaponsByCategory, or nullptr
    };

    const PageInfo pages[] = {
        { "Rifles",   "R", "Rifles"  },
        { "Pistols",  "P", "Pistols" },
        { "SMGs",     "S", "SMGs"    },
        { "Heavy",    "H", "Heavy"   },
        { "Knives",   "K", "Knives"  },
        { "Gloves",   "G", "Gloves"  },
        { "Configs",  "C", nullptr   },
        { "Settings", "I", nullptr   },
    };

    // ========================================================================
    // MENU STATE
    // ========================================================================
    int   currentPage     = 0;
    int   selectedWeapon  = 0;
    int   selectedSkin    = -1;

    char  searchBuffer[128]     = "";
    char  presetNameBuffer[64]  = "";

    float editWear     = 0.0001f;
    int   editSeed     = 0;
    int   editStatTrak = -1;

    // Dimensions
    constexpr float MENU_W      = 860.0f;
    constexpr float MENU_H      = 580.0f;
    constexpr float SIDEBAR_W   = 180.0f;
    constexpr float TITLE_H     = 40.0f;
    constexpr float CONTENT_PAD = 12.0f;

    // Notification
    std::string notification;
    float       notificationTimer = 0.0f;

    // ========================================================================
    // HELPERS
    // ========================================================================
    bool MatchesSearch(const std::string& name, const char* query) {
        if (!query || query[0] == '\0') return true;
        std::string nl = name, ql = query;
        std::transform(nl.begin(), nl.end(), nl.begin(), ::tolower);
        std::transform(ql.begin(), ql.end(), ql.begin(), ::tolower);
        return nl.find(ql) != std::string::npos;
    }

    void ShowNotification(const char* msg) {
        notification      = msg;
        notificationTimer = 3.0f;
    }

    // ========================================================================
    // SIDEBAR — clean AC theme
    // ========================================================================
    void RenderSidebar(ACEUIContext& ctx) {
        float sx = ctx.wnd.x;
        float sy = ctx.wnd.y + TITLE_H;
        float sh = ctx.wnd.h - TITLE_H;

        // Background
        ctx.drawList.AddRectFilled(sx, sy, sx + SIDEBAR_W, sy + sh,
                                   ACE_COL32(16, 16, 22, 255), 0);
        ctx.drawList.AddLine(sx + SIDEBAR_W, sy, sx + SIDEBAR_W, sy + sh,
                             ACE_COL32(35, 35, 50, 100), 1.0f);

        // AC Brand Logo — rounded square
        float logoSize = 38.0f;
        float logoR = logoSize * 0.22f;
        float logoX = sx + (SIDEBAR_W - logoSize) * 0.5f;
        float logoY = sy + 12;

        ctx.drawList.AddRectFilled(logoX, logoY, logoX + logoSize, logoY + logoSize,
                                   ACE_COL32(22, 35, 70, 255), logoR);
        ctx.drawList.AddRect(logoX, logoY, logoX + logoSize, logoY + logoSize,
                             ACE_COL32(59, 130, 246, 60), logoR, 1.0f);

        const char* logoTxt = "AC";
        ACEVec2 lts = ctx.drawList.font->CalcTextSize(logoTxt);
        ctx.drawList.AddText(logoX + (logoSize - lts.x) * 0.5f,
                             logoY + (logoSize - lts.y) * 0.5f,
                             ACE_COL32(70, 150, 255, 255), logoTxt);

        // Version text below logo
        const char* verTxt = "v3.0";
        ACEVec2 vts = ctx.drawList.font->CalcTextSize(verTxt);
        ctx.drawList.AddText(sx + (SIDEBAR_W - vts.x) * 0.5f, logoY + logoSize + 4,
                             ACETheme::TextDim, verTxt);

        // Subtle gradient separator
        float sepY = logoY + logoSize + 22;
        ctx.drawList.AddRectFilledMultiColor(
            sx + 12, sepY, sx + SIDEBAR_W - 12, sepY + 1,
            ACE_COL32(35, 35, 50, 0), ACE_COL32(59, 130, 246, 60),
            ACE_COL32(59, 130, 246, 60), ACE_COL32(35, 35, 50, 0));

        // Page buttons
        float btnY = sepY + 10;
        for (int i = 0; i < PAGE_COUNT; i++) {
            // separator before Configs
            if (i == PAGE_CONFIGS) {
                ctx.drawList.AddRectFilledMultiColor(
                    sx + 12, btnY + 2, sx + SIDEBAR_W - 12, btnY + 3,
                    ACETheme::Border, ACETheme::Border,
                    ACE_COL32(40,40,55,0), ACE_COL32(40,40,55,0));
                btnY += 12;
            }

            float btnH   = 36.0f;
            uint32_t id  = ctx.GetID(pages[i].name);
            bool active  = (currentPage == i);
            bool hovered = ctx.input.IsMouseInRect(sx, btnY,
                                                    sx + SIDEBAR_W, btnY + btnH);
            bool clicked = hovered && ctx.input.mouseClicked[0];

            float target   = active ? 1.0f : (hovered ? 0.5f : 0.0f);
            float  anim    = ctx.SmoothAnim(id, target);

            // hover / active bg
            if (anim > 0.01f) {
                uint32_t bg = ACE_COL32(59, 130, 246, (int)(30 * anim));
                ctx.drawList.AddRectFilled(sx + 4, btnY,
                                            sx + SIDEBAR_W - 4, btnY + btnH,
                                            bg, 6.0f);
            }
            if (active)
                ctx.drawList.AddRectFilled(sx + 2, btnY + 6,
                                            sx + 5, btnY + btnH - 6,
                                            ACETheme::AccentBlue, 2.0f);

            uint32_t textCol = active  ? ACETheme::TextPrimary :
                               hovered ? ACETheme::TextPrimary :
                                         ACETheme::TextSecondary;

            ctx.drawList.AddText(sx + 20, btnY + 10, ACETheme::AccentBlue,
                                 pages[i].icon);
            ctx.drawList.AddText(sx + 40, btnY + 10, textCol, pages[i].name);

            if (clicked) {
                currentPage   = i;
                selectedWeapon = 0;
                selectedSkin   = -1;
                searchBuffer[0] = '\0';
            }

            btnY += btnH + 2;
        }

        // Bottom stats
        float bottomY = sy + sh - 70;
        ctx.drawList.AddRectFilledMultiColor(
            sx + 12, bottomY, sx + SIDEBAR_W - 12, bottomY + 1,
            ACE_COL32(35, 35, 50, 0), ACE_COL32(35, 35, 50, 100),
            ACE_COL32(35, 35, 50, 100), ACE_COL32(35, 35, 50, 0));

        size_t totalSkins = 0;
        for (auto& w : G::allWeapons) totalSkins += w.skins.size();

        char buf[64];
        snprintf(buf, sizeof(buf), "%zu skins loaded", totalSkins);
        ctx.drawList.AddText(sx + 16, bottomY + 8, ACETheme::TextDim, buf);

        snprintf(buf, sizeof(buf), "%zu equipped", G::equippedSkins.size());
        ctx.drawList.AddText(sx + 16, bottomY + 24, ACETheme::AccentGreen, buf);

        ctx.drawList.AddText(sx + 16, bottomY + 44, ACETheme::TextDim,
                             "ACE Engine v3.0");
    }

    // ========================================================================
    // WEAPON / SKIN PAGE
    // ========================================================================
    void RenderWeaponPage(ACEUIContext& ctx, const char* category) {
        float cx = ctx.wnd.x + SIDEBAR_W + CONTENT_PAD;
        float cy = ctx.wnd.y + TITLE_H  + CONTENT_PAD;
        float cw = ctx.wnd.w - SIDEBAR_W - CONTENT_PAD * 2;
        float ch = ctx.wnd.h - TITLE_H  - CONTENT_PAD * 2;

        auto it = G::weaponsByCategory.find(category);
        if (it == G::weaponsByCategory.end() || it->second.empty()) {
            ctx.drawList.AddText(cx + 20, cy + 20, ACETheme::TextSecondary,
                                 "No weapons in this category");
            return;
        }
        auto& weapons = it->second;
        if (selectedWeapon >= (int)weapons.size()) selectedWeapon = 0;

        // ---- Search bar ----
        ctx.SetCursorPos(cx - ctx.wnd.x, cy - ctx.wnd.y);
        ctx.InputText("Search", searchBuffer, sizeof(searchBuffer), cw - 10);

        // ---- Weapon tabs ----
        float tabY = cy + 40;
        float tabX = cx;
        for (int i = 0; i < (int)weapons.size(); i++) {
            const char* wname   = weapons[i].name.c_str();
            ACEVec2 tsize       = ctx.drawList.font->CalcTextSize(wname);
            float tabW          = tsize.x + 20;
            if (tabX + tabW > cx + cw) { tabX = cx; tabY += 28; }

            bool active  = (selectedWeapon == i);
            bool hovered = ctx.input.IsMouseInRect(tabX, tabY,
                                                    tabX + tabW, tabY + 24);
            if (hovered && ctx.input.mouseClicked[0]) {
                selectedWeapon = i;
                selectedSkin   = -1;
            }

            if (active) {
                ctx.drawList.AddRectFilled(tabX, tabY, tabX + tabW, tabY + 24,
                                            ACE_COL32(59,130,246,40), 4.0f);
                ctx.drawList.AddText(tabX + 10, tabY + 4, ACETheme::AccentBlue,
                                     wname);
                ctx.drawList.AddRectFilled(tabX + 4, tabY + 22, tabX + tabW - 4,
                                            tabY + 24, ACETheme::AccentBlue, 1.0f);
            } else {
                if (hovered)
                    ctx.drawList.AddRectFilled(tabX, tabY, tabX + tabW, tabY + 24,
                                                ACE_COL32(40,40,55,128), 4.0f);
                ctx.drawList.AddText(tabX + 10, tabY + 4,
                                     hovered ? ACETheme::TextPrimary
                                             : ACETheme::TextSecondary,
                                     wname);
            }
            tabX += tabW + 4;
        }

        // ---- Filter skins ----
        auto& selWpn = weapons[selectedWeapon];
        std::vector<int> filtered;
        for (int i = 0; i < (int)selWpn.skins.size(); i++)
            if (MatchesSearch(selWpn.skins[i].name, searchBuffer))
                filtered.push_back(i);

        // ---- Skin grid (scrollable) ----
        float gridY = tabY + 34;
        float gridH = ch - (gridY - cy) - 90;

        char childId[64];
        snprintf(childId, sizeof(childId), "skins_%s_%d", category, selectedWeapon);

        ctx.SetCursorPos(cx - ctx.wnd.x, gridY - ctx.wnd.y);
        ctx.BeginChild(childId, cw - 10, gridH);
        {
            constexpr float cardW = 150.0f;
            constexpr float cardH = 90.0f;
            constexpr float gap   = 8.0f;
            int cols = (int)((cw - 10) / (cardW + gap));
            if (cols < 1) cols = 1;

            float startX = ctx.wnd.cursorX;
            int col = 0;

            for (int fi = 0; fi < (int)filtered.size(); fi++) {
                int skinIdx  = filtered[fi];
                auto& skin   = selWpn.skins[skinIdx];

                auto eqIt    = G::equippedSkins.find(selWpn.id);
                bool equipped = eqIt != G::equippedSkins.end()
                                && eqIt->second.paintKit == skin.paintKit;
                bool selected = (selectedSkin == skinIdx);

                float cX = ctx.wnd.x + ctx.wnd.cursorX;
                float cY = ctx.wnd.y + ctx.wnd.cursorY;

                uint32_t cId   = ctx.GetID(skin.name.c_str());
                bool hovered   = ctx.input.IsMouseInRect(cX, cY,
                                                          cX + cardW, cY + cardH);
                if (hovered && ctx.input.mouseClicked[0]) {
                    selectedSkin = skinIdx;
                    if (equipped) {
                        editWear     = eqIt->second.wear;
                        editSeed     = eqIt->second.seed;
                        editStatTrak = eqIt->second.statTrak;
                    } else {
                        editWear = 0.0001f; editSeed = 0; editStatTrak = -1;
                    }
                }

                float target   = hovered ? 1.0f : (selected ? 0.7f : 0.0f);
                float  anim    = ctx.SmoothAnim(cId, target);

                // bg
                uint32_t bg = selected ? ACETheme::CardBgActive
                            : (anim > 0.01f ? ACETheme::CardBgHover
                                            : ACETheme::CardBg);
                float lift = anim * 2.0f;
                ctx.drawList.AddRectFilled(cX, cY - lift,
                                            cX + cardW, cY + cardH - lift,
                                            bg, 6.0f);

                // rarity bar
                uint32_t rc = ACETheme::GetRarityColor(skin.rarity);
                ctx.drawList.AddRectFilled(cX, cY - lift,
                                            cX + cardW, cY + 3 - lift,
                                            rc, 6.0f);

                // selected border
                if (selected)
                    ctx.drawList.AddRect(cX, cY - lift,
                                          cX + cardW, cY + cardH - lift,
                                          ACETheme::AccentBlue, 6.0f, 1.5f);

                // equipped badge
                if (equipped) {
                    ctx.drawList.AddRectFilled(cX + cardW - 24, cY + 6 - lift,
                                                cX + cardW - 4, cY + 22 - lift,
                                                ACETheme::AccentGreen, 4.0f);
                    ctx.drawList.AddText(cX + cardW - 20, cY + 7 - lift,
                                         ACE_COL32(255,255,255,255), "OK");
                }

                // name (truncate)
                std::string dn = skin.name;
                if (dn.length() > 18) dn = dn.substr(0, 16) + "..";
                ctx.drawList.AddText(cX + 8, cY + 14 - lift,
                                     ACETheme::TextPrimary, dn.c_str());

                // paint kit
                char pk[32]; snprintf(pk, sizeof(pk), "#%d", skin.paintKit);
                ctx.drawList.AddText(cX + 8, cY + 34 - lift, ACETheme::TextDim, pk);

                // rarity label
                ctx.drawList.AddText(cX + 8, cY + cardH - 22 - lift,
                                     rc, ACETheme::GetRarityName(skin.rarity));

                // cursor advance
                col++;
                if (col >= cols) {
                    col = 0;
                    ctx.wnd.cursorX  = startX;
                    ctx.wnd.cursorY += cardH + gap;
                } else {
                    ctx.wnd.cursorX += cardW + gap;
                }
            }
            if (col > 0) ctx.wnd.cursorY += cardH + gap;

            // count
            char countBuf[64];
            snprintf(countBuf, sizeof(countBuf), "%zu skins", filtered.size());
            ctx.drawList.AddText(ctx.wnd.x + ctx.wnd.cursorX,
                                 ctx.wnd.y + ctx.wnd.cursorY + 4,
                                 ACETheme::TextDim, countBuf);
            ctx.wnd.cursorY += 24;
        }
        ctx.EndChild();

        // ---- Detail panel ----
        float detailY = ctx.wnd.y + ctx.wnd.h - 80;
        float detailX = cx;
        float detailW = cw - 10;

        ctx.drawList.AddRectFilled(detailX, detailY,
                                    detailX + detailW, detailY + 70,
                                    ACETheme::CardBg, 6.0f);
        ctx.drawList.AddLine(detailX, detailY,
                              detailX + detailW, detailY,
                              ACETheme::Border, 1.0f);

        if (selectedSkin >= 0 && selectedSkin < (int)selWpn.skins.size()) {
            auto& skin   = selWpn.skins[selectedSkin];
            auto  eqIt   = G::equippedSkins.find(selWpn.id);
            bool equipped = eqIt != G::equippedSkins.end()
                            && eqIt->second.paintKit == skin.paintKit;

            // info
            ctx.drawList.AddTextShadow(detailX + 12, detailY + 8,
                                        ACETheme::TextPrimary, skin.name.c_str());
            uint32_t rc = ACETheme::GetRarityColor(skin.rarity);
            ctx.drawList.AddText(detailX + 12, detailY + 28, rc,
                                 ACETheme::GetRarityName(skin.rarity));

            // wear slider
            ctx.SetCursorPos(detailX - ctx.wnd.x + 200, detailY - ctx.wnd.y + 6);
            ctx.WearSlider("##wear", &editWear);

            // seed / stattrak
            ctx.SetCursorPos(detailX - ctx.wnd.x + 200, detailY - ctx.wnd.y + 38);
            ctx.InputInt("Seed", &editSeed, 1, 10, 80);
            ctx.SameLine(12);
            ctx.InputInt("ST", &editStatTrak, 1, 10, 80);

            // equip / unequip button
            float btnX = detailX + detailW - 130;
            float btnY = detailY + 14;
            float btnW = 120, btnH = 36;
            uint32_t bId = ctx.GetID("equip_btn");
            bool bHov    = ctx.input.IsMouseInRect(btnX, btnY,
                                                    btnX + btnW, btnY + btnH);
            bool bClk    = bHov && ctx.input.mouseClicked[0];
            float  bAnim = ctx.SmoothAnim(bId, bHov ? 1.0f : 0.0f);

            if (equipped) {
                uint32_t bg = ACE_COL32(248,113,113, (int)(180 + 75*bAnim));
                ctx.drawList.AddRectFilled(btnX, btnY, btnX+btnW, btnY+btnH,
                                            bg, 6.0f);
                ACEVec2 ts = ctx.drawList.font->CalcTextSize("UNEQUIP");
                ctx.drawList.AddText(btnX + (btnW-ts.x)/2,
                                     btnY + (btnH-ts.y)/2,
                                     ACE_COL32(255,255,255,255), "UNEQUIP");
                if (bClk) {
                    std::lock_guard<std::mutex> lock(G::skinMutex);
                    G::equippedSkins.erase(selWpn.id);
                    ShowNotification("Skin unequipped!");
                }
            } else {
                uint32_t bg = ACE_COL32(59,130,246, (int)(200 + 55*bAnim));
                ctx.drawList.AddRectFilled(btnX, btnY, btnX+btnW, btnY+btnH,
                                            bg, 6.0f);
                if (bAnim > 0.01f) {
                    uint32_t glow = ACE_COL32(59,130,246, (int)(40*bAnim));
                    ctx.drawList.AddRectFilled(btnX-3, btnY-3,
                                                btnX+btnW+3, btnY+btnH+3,
                                                glow, 8.0f);
                }
                ACEVec2 ts = ctx.drawList.font->CalcTextSize("EQUIP");
                ctx.drawList.AddText(btnX + (btnW-ts.x)/2,
                                     btnY + (btnH-ts.y)/2,
                                     ACE_COL32(255,255,255,255), "EQUIP");
                if (bClk) {
                    SkinConfig cfg;
                    cfg.weaponId  = selWpn.id;
                    cfg.paintKit  = skin.paintKit;
                    cfg.wear      = editWear;
                    cfg.seed      = editSeed;
                    cfg.statTrak  = editStatTrak;
                    cfg.weaponName = selWpn.name;
                    cfg.skinName  = skin.name;
                    cfg.enabled   = true;
                    {
                        std::lock_guard<std::mutex> lock(G::skinMutex);
                        G::equippedSkins[selWpn.id] = cfg;
                    }
                    ShowNotification("Skin equipped!");
                }
            }
        } else {
            ctx.drawList.AddText(detailX + 20, detailY + 25,
                                 ACETheme::TextDim,
                                 "Select a skin to configure");
        }
    }

    // ========================================================================
    // CONFIGS PAGE
    // ========================================================================
    void RenderConfigsPage(ACEUIContext& ctx) {
        float cx = ctx.wnd.x + SIDEBAR_W + CONTENT_PAD;
        float cy = ctx.wnd.y + TITLE_H  + CONTENT_PAD;
        float cw = ctx.wnd.w - SIDEBAR_W - CONTENT_PAD * 2;
        float ch = ctx.wnd.h - TITLE_H  - CONTENT_PAD * 2;

        ctx.drawList.AddTextShadow(cx + 8, cy + 4, ACETheme::TextPrimary,
                                    "Configuration Presets");
        ctx.drawList.AddRectFilledMultiColor(
            cx, cy + 24, cx + cw - 10, cy + 25,
            ACE_COL32(59, 130, 246, 180), ACE_COL32(139, 92, 246, 180),
            ACE_COL32(139, 92, 246, 60), ACE_COL32(59, 130, 246, 60));

        // preset name input
        ctx.SetCursorPos(cx - ctx.wnd.x, cy + 40 - ctx.wnd.y);
        ctx.InputText("Preset Name", presetNameBuffer, sizeof(presetNameBuffer), 240);

        // save button
        float savX = cx + 260, savY = cy + 40, savW = 80, savH = 28;
        uint32_t savId = ctx.GetID("save_preset");
        bool savHov = ctx.input.IsMouseInRect(savX, savY, savX+savW, savY+savH);
        ctx.drawList.AddRectFilled(savX, savY, savX+savW, savY+savH,
            savHov ? ACETheme::AccentGreen : ACE_COL32(74,222,128,160), 4.0f);
        ACEVec2 sts = ctx.drawList.font->CalcTextSize("Save");
        ctx.drawList.AddText(savX+(savW-sts.x)/2, savY+(savH-sts.y)/2,
                             ACE_COL32(255,255,255,255), "Save");
        if (savHov && ctx.input.mouseClicked[0] && presetNameBuffer[0]) {
            Config::Save(presetNameBuffer);
            ShowNotification("Preset saved!");
        }

        // preset list (scrollable)
        auto presets = Config::ListPresets();
        ctx.SetCursorPos(cx - ctx.wnd.x, cy + 84 - ctx.wnd.y);
        ctx.BeginChild("preset_list", cw - 10, ch - 100);
        {
            for (auto& preset : presets) {
                float iX = ctx.wnd.x + ctx.wnd.cursorX;
                float iY = ctx.wnd.y + ctx.wnd.cursorY;
                float iW = cw - 30, iH = 44;

                uint32_t iId = ctx.GetID(preset.c_str());
                bool hov = ctx.input.IsMouseInRect(iX, iY, iX+iW, iY+iH);
                float  anim = ctx.SmoothAnim(iId, hov ? 1.0f : 0.0f);

                uint32_t bg = ACE_COL32(22,22,30, (int)(180 + 75*anim));
        ctx.drawList.AddRectFilled(iX, iY, iX+iW, iY+iH, bg, 8.0f);

                ctx.drawList.AddText(iX + 12, iY + 6, ACETheme::TextPrimary,
                                     preset.c_str());
                ctx.drawList.AddText(iX + 12, iY + 24, ACETheme::TextDim,
                                     ".acpreset");

                // load
                float bX = iX + iW - 160;
                float bH = 28, bY2 = iY + (iH - bH)/2;
                uint32_t lId = ctx.GetID((preset + "_load").c_str());
                bool lHov = ctx.input.IsMouseInRect(bX, bY2, bX+70, bY2+bH);
                ctx.drawList.AddRectFilled(bX, bY2, bX+70, bY2+bH,
                    lHov ? ACETheme::AccentBlue : ACETheme::AccentBlueDim, 4.0f);
                ACEVec2 lt = ctx.drawList.font->CalcTextSize("Load");
                ctx.drawList.AddText(bX+(70-lt.x)/2, bY2+(bH-lt.y)/2,
                                     ACE_COL32(255,255,255,255), "Load");
                if (lHov && ctx.input.mouseClicked[0]) {
                    Config::Load(preset);
                    ShowNotification("Preset loaded!");
                }

                // delete
                float dX = bX + 78;
                uint32_t dId = ctx.GetID((preset + "_del").c_str());
                bool dHov = ctx.input.IsMouseInRect(dX, bY2, dX+70, bY2+bH);
                ctx.drawList.AddRectFilled(dX, bY2, dX+70, bY2+bH,
                    dHov ? ACETheme::AccentRed : ACE_COL32(248,113,113,120),
                    4.0f);
                ACEVec2 dt = ctx.drawList.font->CalcTextSize("Delete");
                ctx.drawList.AddText(dX+(70-dt.x)/2, bY2+(bH-dt.y)/2,
                                     ACE_COL32(255,255,255,255), "Delete");
                if (dHov && ctx.input.mouseClicked[0]) {
                    Config::Delete(preset);
                    ShowNotification("Preset deleted!");
                }

                ctx.wnd.cursorY += iH + 6;
            }
            if (presets.empty()) {
                ctx.drawList.AddText(ctx.wnd.x + ctx.wnd.cursorX + 20,
                                     ctx.wnd.y + ctx.wnd.cursorY + 20,
                                     ACETheme::TextDim, "No presets saved yet");
                ctx.wnd.cursorY += 60;
            }
        }
        ctx.EndChild();
    }

    // ========================================================================
    // SETTINGS / ABOUT PAGE
    // ========================================================================
    void RenderSettingsPage(ACEUIContext& ctx) {
        float cx = ctx.wnd.x + SIDEBAR_W + CONTENT_PAD;
        float cy = ctx.wnd.y + TITLE_H  + CONTENT_PAD;
        float cw = ctx.wnd.w - SIDEBAR_W - CONTENT_PAD * 2;

        ctx.drawList.AddTextShadow(cx + 8, cy + 4, ACETheme::TextPrimary,
                                    "Settings & About");
        ctx.drawList.AddRectFilledMultiColor(
            cx, cy + 24, cx + cw - 10, cy + 25,
            ACE_COL32(59, 130, 246, 180), ACE_COL32(139, 92, 246, 180),
            ACE_COL32(139, 92, 246, 60), ACE_COL32(59, 130, 246, 60));

        float y = cy + 40;

        // About card — clean rounded corners
        ctx.drawList.AddRectFilled(cx, y, cx + cw - 10, y + 200,
                                    ACETheme::CardBg, 10.0f);
        ctx.drawList.AddText(cx + 16, y + 12, ACETheme::AccentBlue,
                             "AC Skin Changer");
        ctx.drawList.AddText(cx + 16, y + 32, ACETheme::TextSecondary,
                             "Version 3.0 — Custom Rendering Engine");
        ctx.drawList.AddText(cx + 16, y + 52, ACETheme::TextDim,
                             "Zero ImGui dependency. Fully custom DX11 pipeline.");

        ctx.drawList.AddRectFilledMultiColor(
            cx + 16, y + 74, cx + cw - 26, y + 75,
            ACE_COL32(35, 35, 50, 0), ACE_COL32(35, 35, 50, 100),
            ACE_COL32(35, 35, 50, 100), ACE_COL32(35, 35, 50, 0));

        ctx.drawList.AddText(cx + 16, y + 84, ACETheme::TextSecondary,
                             "Engine Components:");

        const char* comps[] = {
            "DX11 Renderer",
            "Font System (stb_truetype)",
            "Input System (WndProc)",
            "Animation System",
            "UI Component Tree",
            "Layout Engine",
            "Shader Pipeline (HLSL)",
        };
        for (int i = 0; i < 7; i++) {
            ctx.drawList.AddText(cx + 32, y + 104 + i * 16,
                                 ACETheme::AccentGreen, "+");
            ctx.drawList.AddText(cx + 48, y + 104 + i * 16,
                                 ACETheme::TextPrimary, comps[i]);
        }

        // Stats card
        y += 216;
        ctx.drawList.AddRectFilled(cx, y, cx + cw - 10, y + 130,
                                    ACETheme::CardBg, 10.0f);
        ctx.drawList.AddText(cx + 16, y + 12, ACETheme::AccentBlue,
                             "Statistics");

        size_t totalSkins = 0, totalW = G::allWeapons.size();
        for (auto& w : G::allWeapons) totalSkins += w.skins.size();

        char sb[128];
        snprintf(sb, sizeof(sb), "Total Weapons: %zu", totalW);
        ctx.drawList.AddText(cx + 16, y + 36, ACETheme::TextPrimary, sb);
        snprintf(sb, sizeof(sb), "Total Skins: %zu", totalSkins);
        ctx.drawList.AddText(cx + 16, y + 56, ACETheme::TextPrimary, sb);
        snprintf(sb, sizeof(sb), "Equipped: %zu", G::equippedSkins.size());
        ctx.drawList.AddText(cx + 16, y + 76, ACETheme::AccentGreen, sb);
        snprintf(sb, sizeof(sb), "Categories: %zu", G::weaponsByCategory.size());
        ctx.drawList.AddText(cx + 16, y + 96, ACETheme::TextPrimary, sb);

        // Hotkeys card
        y += 146;
        ctx.drawList.AddRectFilled(cx, y, cx + cw - 10, y + 60,
                                    ACETheme::CardBg, 10.0f);
        ctx.drawList.AddText(cx + 16, y + 12, ACETheme::AccentBlue,
                             "Hotkeys");
        ctx.drawList.AddText(cx + 16, y + 34, ACETheme::TextPrimary,
                             "INSERT — Toggle menu    |    END — Unload DLL");
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================
namespace Menu {
    void Render() {
        auto& ctx = ACE::gUI;

        float mx = (ctx.displayW - MENU_W) / 2;
        float my = (ctx.displayH - MENU_H) / 2;

        ctx.BeginWindow("AC Skin Changer", mx, my, MENU_W, MENU_H, true);

        // Background with clean rounded corners
        ctx.drawList.AddRectFilled(ctx.wnd.x, ctx.wnd.y,
                                    ctx.wnd.x + ctx.wnd.w,
                                    ctx.wnd.y + ctx.wnd.h,
                                    ACE_COL32(13, 13, 18, 255), 12.0f);

        // Title bar
        ctx.drawList.AddRectFilledMultiColor(
            ctx.wnd.x, ctx.wnd.y,
            ctx.wnd.x + ctx.wnd.w, ctx.wnd.y + TITLE_H,
            ACE_COL32(18, 18, 26, 255), ACE_COL32(18, 18, 26, 255),
            ACE_COL32(14, 14, 20, 255), ACE_COL32(14, 14, 20, 255));

        ctx.drawList.AddTextShadow(ctx.wnd.x + SIDEBAR_W + 16,
                                    ctx.wnd.y + 12,
                                    ACETheme::TextPrimary,
                                    "AC SKIN CHANGER");

        // Clean accent line under title
        ctx.drawList.AddRectFilledMultiColor(
            ctx.wnd.x, ctx.wnd.y + TITLE_H - 2,
            ctx.wnd.x + ctx.wnd.w, ctx.wnd.y + TITLE_H,
            ACE_COL32(59, 130, 246, 200), ACE_COL32(139, 92, 246, 200),
            ACE_COL32(139, 92, 246, 100), ACE_COL32(59, 130, 246, 100));

        // Outer border - smooth rounding
        ctx.drawList.AddRect(ctx.wnd.x, ctx.wnd.y,
                              ctx.wnd.x + ctx.wnd.w, ctx.wnd.y + ctx.wnd.h,
                              ACE_COL32(35, 35, 50, 200), 12.0f, 1.0f);

        RenderSidebar(ctx);

        if (currentPage < PAGE_CONFIGS && pages[currentPage].category)
            RenderWeaponPage(ctx, pages[currentPage].category);
        else if (currentPage == PAGE_CONFIGS)
            RenderConfigsPage(ctx);
        else if (currentPage == PAGE_SETTINGS)
            RenderSettingsPage(ctx);

        // notification toast
        if (notificationTimer > 0) {
            notificationTimer -= ctx.deltaTime;
            float a = notificationTimer > 0.5f ? 1.0f
                                                : notificationTimer / 0.5f;
            if (a > 0.01f) {
                ctx.overlayDrawList.font = ctx.drawList.font;
                float nW = 250, nH = 36;
                float nX = ctx.wnd.x + ctx.wnd.w - nW - 12;
                float nY = ctx.wnd.y + ctx.wnd.h - nH - 12;

                ctx.overlayDrawList.AddRectFilled(nX, nY, nX+nW, nY+nH,
                    ACE_COL32(74,222,128, (int)(200*a)), 6.0f);
                uint32_t tc = ACE_COL32(255,255,255, (int)(255*a));
                ACEVec2 ts = ctx.overlayDrawList.font->CalcTextSize(
                    notification.c_str());
                ctx.overlayDrawList.AddText(nX + (nW-ts.x)/2,
                                             nY + (nH-ts.y)/2,
                                             tc, notification.c_str());
            }
        }

        ctx.EndWindow();
    }

    void SetupStyle() {
        // Custom engine manages theme via ACETheme namespace — no-op
    }
}
