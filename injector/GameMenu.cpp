// ============================================================================
//  CS2 In-Game Overlay Menu  –  Full Implementation
//  O key toggles the menu overlay on top of CS2
// ============================================================================
#include "GameMenu.h"
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cstdarg>
#include <ctime>

#pragma comment(lib, "gdi32.lib")

// ============================================================================
//  Debug logger – writes to ac_debug.log next to the exe
// ============================================================================
static void DbgLog(const char* fmt, ...) {
    FILE* f = nullptr;
    fopen_s(&f, "ac_debug.log", "a");
    if (!f) return;
    // Timestamp
    time_t now = time(nullptr);
    struct tm t; localtime_s(&t, &now);
    fprintf(f, "[%02d:%02d:%02d] ", t.tm_hour, t.tm_min, t.tm_sec);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fprintf(f, "\n");
    fclose(f);
}
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "msimg32.lib")

// ============================================================================
//  Global overlay pointer (for WndProc callback)
// ============================================================================
static OverlayWindow* g_pOverlay = nullptr;

// ============================================================================
//  GameMenu
// ============================================================================

GameMenu::GameMenu()
    : m_visible(false)
    , m_activeTab(TAB_SKINS)
    , m_scrollOffset(0)
    , m_mouseX(0), m_mouseY(0)
    , m_menuX(0), m_menuY(0)
    , m_rowCount(0)
    , m_toggleCount(0)
    , m_skinChangerEnabled(true)
    , m_knifeChangerEnabled(false)
    , m_gloveChangerEnabled(false)
    , m_fontTitle(nullptr)
    , m_fontBody(nullptr)
    , m_fontSmall(nullptr)
    , m_fontBold(nullptr)
    , m_fontLogo(nullptr) {
    memset(m_tabRects, 0, sizeof(m_tabRects));
    memset(m_rowRects, 0, sizeof(m_rowRects));
    memset(m_toggleRects, 0, sizeof(m_toggleRects));
}

GameMenu::~GameMenu() {
    if (m_fontTitle) DeleteObject(m_fontTitle);
    if (m_fontBody)  DeleteObject(m_fontBody);
    if (m_fontSmall) DeleteObject(m_fontSmall);
    if (m_fontBold)  DeleteObject(m_fontBold);
    if (m_fontLogo)  DeleteObject(m_fontLogo);
}

bool GameMenu::Initialize(HMODULE) {
    // Large fonts for the bigger menu
    m_fontTitle = CreateFontA(22, 0, 0, 0, FW_BOLD,   0, 0, 0, DEFAULT_CHARSET,
                              OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH, "Segoe UI");
    m_fontBody  = CreateFontA(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                              OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH, "Segoe UI");
    m_fontSmall = CreateFontA(13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                              OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH, "Segoe UI");
    m_fontBold  = CreateFontA(16, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
                              OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH, "Segoe UI");
    m_fontLogo  = CreateFontA(28, 0, 0, 0, FW_BOLD,   0, 0, 0, DEFAULT_CHARSET,
                              OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH, "Segoe UI");

    // Populate default weapons & skins
    AddWeapon("AK-47");
    AddSkin("AK-47", "Vulcan");
    AddSkin("AK-47", "Fire Serpent");
    AddSkin("AK-47", "Neon Rider");
    AddSkin("AK-47", "Redline");
    AddSkin("AK-47", "Asiimov");

    AddWeapon("M4A4");
    AddSkin("M4A4", "Howl");
    AddSkin("M4A4", "Asiimov");
    AddSkin("M4A4", "Neo-Noir");
    AddSkin("M4A4", "Desolate Space");

    AddWeapon("M4A1-S");
    AddSkin("M4A1-S", "Hyper Beast");
    AddSkin("M4A1-S", "Printstream");
    AddSkin("M4A1-S", "Chantico's Fire");

    AddWeapon("AWP");
    AddSkin("AWP", "Dragon Lore");
    AddSkin("AWP", "Asiimov");
    AddSkin("AWP", "Medusa");
    AddSkin("AWP", "Gungnir");
    AddSkin("AWP", "Fade");

    AddWeapon("USP-S");
    AddSkin("USP-S", "Kill Confirmed");
    AddSkin("USP-S", "Neo-Noir");
    AddSkin("USP-S", "Printstream");

    AddWeapon("Glock-18");
    AddSkin("Glock-18", "Fade");
    AddSkin("Glock-18", "Gamma Doppler");
    AddSkin("Glock-18", "Wasteland Rebel");

    AddWeapon("Desert Eagle");
    AddSkin("Desert Eagle", "Blaze");
    AddSkin("Desert Eagle", "Printstream");
    AddSkin("Desert Eagle", "Code Red");

    AddWeapon("Knife");
    AddSkin("Knife", "Karambit | Fade");
    AddSkin("Knife", "Butterfly | Doppler");
    AddSkin("Knife", "M9 Bayonet | Crimson Web");
    AddSkin("Knife", "Skeleton Knife | Slaughter");

    return true;
}

void GameMenu::Shutdown() {
    m_visible = false;
}

void GameMenu::Toggle() {
    m_visible = !m_visible;
    if (m_visible) {
        m_scrollOffset = 0;
    }
}

void GameMenu::AddWeapon(const std::string& weapon) {
    if (m_weaponSkins.find(weapon) == m_weaponSkins.end()) {
        m_weapons.push_back(weapon);
        m_weaponSkins[weapon] = {};
    }
}

void GameMenu::AddSkin(const std::string& weapon, const std::string& skin) {
    if (m_weaponSkins.find(weapon) == m_weaponSkins.end()) {
        AddWeapon(weapon);
    }
    m_weaponSkins[weapon].push_back(skin);
}

// ────────────────────────────────────────────────
//  Mouse input
// ────────────────────────────────────────────────
void GameMenu::OnMouseMove(int x, int y) {
    m_mouseX = x;
    m_mouseY = y;
}

void GameMenu::OnMouseClick(int x, int y) {
    if (!m_visible) return;

    // Tab clicks
    for (int i = 0; i < TAB_COUNT; i++) {
        if (HitTest(m_tabRects[i], x, y)) {
            m_activeTab = (Tab)i;
            m_scrollOffset = 0;
            return;
        }
    }

    // Skins tab – click on skin rows
    if (m_activeTab == TAB_SKINS) {
        for (int r = 0; r < m_rowCount; r++) {
            if (HitTest(m_rowRects[r], x, y)) {
                int wi = m_rowWeaponIdx[r];
                int si = m_rowSkinIdx[r];
                if (si >= 0 && wi >= 0 && wi < (int)m_weapons.size()) {
                    auto& wname = m_weapons[wi];
                    auto& skins = m_weaponSkins[wname];
                    if (si < (int)skins.size()) {
                        if (m_activeSkins[wname] == skins[si])
                            m_activeSkins.erase(wname);
                        else
                            m_activeSkins[wname] = skins[si];
                    }
                }
                return;
            }
        }
    }

    // Settings tab – click toggles
    if (m_activeTab == TAB_SETTINGS) {
        for (int i = 0; i < m_toggleCount; i++) {
            if (HitTest(m_toggleRects[i], x, y)) {
                bool* vals[] = { &m_skinChangerEnabled, &m_knifeChangerEnabled, &m_gloveChangerEnabled };
                if (i < 3) *vals[i] = !*vals[i];
                return;
            }
        }
    }
}

void GameMenu::OnMouseWheel(int delta) {
    if (!m_visible) return;
    m_scrollOffset -= delta / 40;  // ~3 rows per scroll
    if (m_scrollOffset < 0) m_scrollOffset = 0;
}

// ────────────────────────────────────────────────
//  Rendering
// ────────────────────────────────────────────────
void GameMenu::DrawRoundRect(HDC hdc, int x, int y, int w, int h, int r,
                             COLORREF fill, COLORREF border) {
    HBRUSH br = CreateSolidBrush(fill);
    HPEN   pn = CreatePen(PS_SOLID, 1, border);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, br);
    HPEN   op = (HPEN)SelectObject(hdc, pn);
    RoundRect(hdc, x, y, x + w, y + h, r * 2, r * 2);
    SelectObject(hdc, ob);
    SelectObject(hdc, op);
    DeleteObject(br);
    DeleteObject(pn);
}

void GameMenu::DrawText_(HDC hdc, const char* text, int x, int y, int w, int h,
                         COLORREF col, HFONT font, UINT fmt) {
    HFONT old = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, col);
    SetBkMode(hdc, TRANSPARENT);
    RECT rc = { x, y, x + w, y + h };
    DrawTextA(hdc, text, -1, &rc, fmt);
    SelectObject(hdc, old);
}

void GameMenu::Render(HDC hdc, int winW, int winH) {
    if (!m_visible) return;

    // Centre the menu panel
    int mx = (winW - MENU_WIDTH) / 2;
    int my = (winH - MENU_HEIGHT) / 2;
    m_menuX = mx;
    m_menuY = my;

    // Reset hit-test data
    m_rowCount = 0;
    m_toggleCount = 0;

    // Shadow
    DrawRoundRect(hdc, mx + 4, my + 4, MENU_WIDTH, MENU_HEIGHT, MENU_RAD,
                  RGB(2, 3, 6), RGB(2, 3, 6));

    // Main background
    DrawRoundRect(hdc, mx, my, MENU_WIDTH, MENU_HEIGHT, MENU_RAD,
                  MENU_BG, MENU_BORDER);

    // Left sidebar
    DrawSidebar(hdc, mx, my, MENU_HEIGHT);

    // Content area
    int cx = mx + MENU_SIDEBAR_W;
    int cw = MENU_WIDTH - MENU_SIDEBAR_W;

    // Header area in content
    DrawHeader(hdc, cx, my, cw);

    // Tab bar below header
    int tabY = my + MENU_HEADER;
    const char* tabLabels[TAB_COUNT] = { "Skins", "Indstillinger", "Om" };
    int tabW = cw / TAB_COUNT;

    for (int i = 0; i < TAB_COUNT; i++) {
        int tx = cx + i * tabW;
        bool sel = ((int)m_activeTab == i);
        bool hov = HitTest({tx, tabY, tabW, MENU_TAB_H}, m_mouseX, m_mouseY);

        COLORREF bg  = sel ? MENU_CARD : (hov ? RGB(20, 23, 35) : MENU_BG);
        COLORREF txt = sel ? MENU_ACCENT : (hov ? MENU_ACCENT_L : MENU_TEXT_DIM);

        RECT trc = { tx, tabY, tx + tabW, tabY + MENU_TAB_H };
        HBRUSH tbr = CreateSolidBrush(bg);
        FillRect(hdc, &trc, tbr);
        DeleteObject(tbr);

        DrawText_(hdc, tabLabels[i], tx, tabY, tabW, MENU_TAB_H,
                  txt, m_fontBold, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        if (sel) {
            RECT ind = { tx + 6, tabY + MENU_TAB_H - 3, tx + tabW - 6, tabY + MENU_TAB_H };
            HBRUSH ibr = CreateSolidBrush(MENU_ACCENT);
            FillRect(hdc, &ind, ibr);
            DeleteObject(ibr);
        }

        m_tabRects[i] = { tx, tabY, tabW, MENU_TAB_H };
    }

    // Separator
    HPEN sp = CreatePen(PS_SOLID, 1, MENU_BORDER);
    HPEN op = (HPEN)SelectObject(hdc, sp);
    MoveToEx(hdc, cx, tabY + MENU_TAB_H, nullptr);
    LineTo(hdc, cx + cw, tabY + MENU_TAB_H);
    SelectObject(hdc, op);
    DeleteObject(sp);

    // Content body
    int contentY = tabY + MENU_TAB_H + 2;
    int contentH = MENU_HEIGHT - MENU_HEADER - MENU_TAB_H - 2 - (my + MENU_HEIGHT - (my + MENU_HEIGHT));
    contentH = (my + MENU_HEIGHT) - contentY - 36;  // leave room for footer

    // Clip
    HRGN clip = CreateRectRgn(cx + 1, contentY, cx + cw - 1, contentY + contentH);
    SelectClipRgn(hdc, clip);

    switch (m_activeTab) {
    case TAB_SKINS:    DrawSkinsTab(hdc, cx, contentY, cw, contentH); break;
    case TAB_SETTINGS: DrawSettingsTab(hdc, cx, contentY, cw, contentH); break;
    case TAB_ABOUT:    DrawAboutTab(hdc, cx, contentY, cw, contentH); break;
    default: break;
    }

    SelectClipRgn(hdc, nullptr);
    DeleteObject(clip);

    // Footer
    int footerY = my + MENU_HEIGHT - 30;
    DrawText_(hdc, "Tryk O for at lukke menuen", cx, footerY, cw, 24,
              MENU_TEXT_DIM, m_fontSmall, DT_CENTER | DT_SINGLELINE);
}

void GameMenu::DrawSidebar(HDC hdc, int x, int y, int h) {
    // Sidebar background
    RECT sbr = { x, y, x + MENU_SIDEBAR_W, y + h };
    HBRUSH sbb = CreateSolidBrush(MENU_SIDEBAR);
    FillRect(hdc, &sbr, sbb);
    DeleteObject(sbb);

    // Right border
    HPEN bp = CreatePen(PS_SOLID, 1, MENU_SIDEBAR_BR);
    HPEN obp = (HPEN)SelectObject(hdc, bp);
    MoveToEx(hdc, x + MENU_SIDEBAR_W - 1, y, nullptr);
    LineTo(hdc, x + MENU_SIDEBAR_W - 1, y + h);
    SelectObject(hdc, obp);
    DeleteObject(bp);

    // Logo
    int logoY = y + 20;
    DrawRoundRect(hdc, x + 20, logoY, 50, 50, 12, MENU_ACCENT, MENU_ACCENT);
    DrawText_(hdc, "AC", x + 20, logoY, 50, 50,
              MENU_WHITE, m_fontLogo, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    DrawText_(hdc, "AC Changer", x + 78, logoY + 6, 90, 20,
              MENU_WHITE, m_fontBold, DT_LEFT | DT_SINGLELINE);
    DrawText_(hdc, "v1.0", x + 78, logoY + 28, 90, 16,
              MENU_TEXT_DIM, m_fontSmall, DT_LEFT | DT_SINGLELINE);

    // Navigation items
    int navY = logoY + 70;
    const char* navItems[] = { "Dashboard", "Support", "Info" };
    for (int i = 0; i < 3; i++) {
        bool sel = ((int)m_activeTab == i);
        bool hov = HitTest({x + 10, navY, MENU_SIDEBAR_W - 20, 36}, m_mouseX, m_mouseY);

        if (sel) {
            DrawRoundRect(hdc, x + 10, navY, MENU_SIDEBAR_W - 20, 36, 8,
                          RGB(25, 40, 70), MENU_ACCENT);
        } else if (hov) {
            DrawRoundRect(hdc, x + 10, navY, MENU_SIDEBAR_W - 20, 36, 8,
                          RGB(22, 26, 38), MENU_SIDEBAR_BR);
        }

        COLORREF nc = sel ? MENU_ACCENT_L : (hov ? MENU_TEXT : MENU_TEXT_DIM);
        DrawText_(hdc, navItems[i], x + 24, navY, MENU_SIDEBAR_W - 34, 36,
                  nc, m_fontBody, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        navY += 42;
    }

    // Status indicator at bottom
    int statusY = y + h - 60;
    DrawRoundRect(hdc, x + 15, statusY, MENU_SIDEBAR_W - 30, 40, 8,
                  RGB(20, 50, 35), MENU_GREEN);
    DrawText_(hdc, "\xB7  Online", x + 15, statusY, MENU_SIDEBAR_W - 30, 40,
              MENU_GREEN, m_fontBody, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void GameMenu::DrawHeader(HDC hdc, int x, int y, int w) {
    // Dark header bar
    RECT hr = { x, y, x + w, y + MENU_HEADER };
    HBRUSH hb = CreateSolidBrush(MENU_BG_HEADER);
    FillRect(hdc, &hr, hb);
    DeleteObject(hb);

    // Title
    DrawText_(hdc, "SKIN CHANGER", x + MENU_PAD, y, 200, MENU_HEADER,
              MENU_WHITE, m_fontTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Version badge
    DrawRoundRect(hdc, x + w - 90, y + 17, 70, 26, 13, RGB(15, 45, 35), MENU_GREEN);
    DrawText_(hdc, "v1.0", x + w - 90, y + 17, 70, 26,
              MENU_GREEN, m_fontBody, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Separator
    HPEN sp = CreatePen(PS_SOLID, 1, MENU_BORDER);
    HPEN op = (HPEN)SelectObject(hdc, sp);
    MoveToEx(hdc, x, y + MENU_HEADER - 1, nullptr);
    LineTo(hdc, x + w, y + MENU_HEADER - 1);
    SelectObject(hdc, op);
    DeleteObject(sp);
}

void GameMenu::DrawSkinsTab(HDC hdc, int x, int y, int w, int h) {
    int py = y + 8 - m_scrollOffset;
    int weaponIndex = -1;

    for (auto& weapon : m_weapons) {
        weaponIndex++;

        // Weapon header card
        int headerH = MENU_ROW_H + 4;
        if (py + headerH > y && py < y + h) {
            DrawRoundRect(hdc, x + MENU_PAD, py, w - MENU_PAD * 2, headerH, 6,
                          RGB(18, 22, 34), MENU_BORDER);

            // Weapon icon (gold circle)
            DrawRoundRect(hdc, x + MENU_PAD + 10, py + 8, 30, 30, 15,
                          RGB(40, 35, 15), MENU_ICON_GOLD);
            DrawText_(hdc, weapon.substr(0, 1).c_str(), x + MENU_PAD + 10, py + 8, 30, 30,
                      MENU_ICON_GOLD, m_fontBold, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            DrawText_(hdc, weapon.c_str(), x + MENU_PAD + 50, py, w - MENU_PAD * 2 - 60, headerH,
                      MENU_ACCENT_L, m_fontBold, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            // Active skin on right
            auto it = m_activeSkins.find(weapon);
            if (it != m_activeSkins.end()) {
                DrawText_(hdc, it->second.c_str(), x + MENU_PAD + 50, py,
                          w - MENU_PAD * 2 - 60, headerH,
                          MENU_GREEN, m_fontBody, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
            }
        }
        py += headerH + 4;

        // Skin rows
        auto& skins = m_weaponSkins[weapon];
        for (int si = 0; si < (int)skins.size(); si++) {
            int rowH = MENU_ROW_H;
            if (py + rowH > y && py < y + h) {
                bool hov = HitTest({x + MENU_PAD + 12, py, w - MENU_PAD * 2 - 12, rowH}, m_mouseX, m_mouseY);
                bool active = (m_activeSkins.count(weapon) && m_activeSkins[weapon] == skins[si]);

                COLORREF rowBg = active ? RGB(20, 45, 35) : (hov ? MENU_CARD_HOV : MENU_CARD);
                COLORREF rowBr = active ? MENU_GREEN : (hov ? MENU_CARD_BR : RGB(30, 34, 48));

                DrawRoundRect(hdc, x + MENU_PAD + 12, py, w - MENU_PAD * 2 - 12, rowH, 6,
                              rowBg, rowBr);

                // Left accent bar on hover
                if (hov) {
                    RECT bar = { x + MENU_PAD + 12, py + 4, x + MENU_PAD + 15, py + rowH - 4 };
                    HBRUSH ab = CreateSolidBrush(MENU_ACCENT);
                    FillRect(hdc, &bar, ab);
                    DeleteObject(ab);
                }

                // Skin name
                DrawText_(hdc, skins[si].c_str(), x + MENU_PAD + 28, py,
                          w - MENU_PAD * 2 - 100, rowH,
                          hov ? MENU_WHITE : MENU_TEXT, m_fontBody,
                          DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                // Active badge
                if (active) {
                    int bx = x + w - MENU_PAD - 70;
                    DrawRoundRect(hdc, bx, py + 8, 55, 24, 12, RGB(15, 50, 30), MENU_GREEN);
                    DrawText_(hdc, "AKTIV", bx, py + 8, 55, 24,
                              MENU_GREEN, m_fontSmall, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                }

                // Store hit rect
                if (m_rowCount < MAX_ROWS) {
                    m_rowRects[m_rowCount] = { x + MENU_PAD + 12, py, w - MENU_PAD * 2 - 12, rowH };
                    m_rowWeaponIdx[m_rowCount] = weaponIndex;
                    m_rowSkinIdx[m_rowCount] = si;
                    m_rowCount++;
                }
            }
            py += rowH + 3;
        }

        py += 8; // gap between weapon groups
    }

    // Clamp scroll
    int totalH = py + m_scrollOffset - y;
    if (totalH < h) m_scrollOffset = 0;
    else if (m_scrollOffset > totalH - h) m_scrollOffset = totalH - h;
}

void GameMenu::DrawSettingsTab(HDC hdc, int x, int y, int w, int h) {
    struct Setting {
        const char* name;
        const char* desc;
        bool* value;
    };
    Setting settings[] = {
        { "Skin Changer",   "Skift skins p\xE5 alle v\xE5""ben",        &m_skinChangerEnabled },
        { "Knife Changer",  "Skift din kniv model",                       &m_knifeChangerEnabled },
        { "Glove Changer",  "Skift handske model",                        &m_gloveChangerEnabled },
    };
    int count = sizeof(settings) / sizeof(settings[0]);
    m_toggleCount = 0;

    int py = y + 16;
    for (int i = 0; i < count; i++) {
        int cardH = 64;
        bool hov = HitTest({x + MENU_PAD, py, w - MENU_PAD * 2, cardH}, m_mouseX, m_mouseY);

        COLORREF bg = hov ? MENU_CARD_HOV : MENU_CARD;
        COLORREF br = hov ? MENU_CARD_BR : MENU_BORDER;

        DrawRoundRect(hdc, x + MENU_PAD, py, w - MENU_PAD * 2, cardH, 8, bg, br);

        // Label
        DrawText_(hdc, settings[i].name, x + MENU_PAD + 18, py + 8,
                  w - MENU_PAD * 2 - 120, 24,
                  MENU_WHITE, m_fontBold, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Description
        DrawText_(hdc, settings[i].desc, x + MENU_PAD + 18, py + 32,
                  w - MENU_PAD * 2 - 120, 22,
                  MENU_TEXT_DIM, m_fontSmall, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Toggle switch
        bool on = *settings[i].value;
        int sw = 56, sh = 28;
        int tx = x + w - MENU_PAD - sw - 12;
        int ty = py + (cardH - sh) / 2;

        // Track background
        DrawRoundRect(hdc, tx, ty, sw, sh, sh / 2,
                      on ? RGB(25, 80, 55) : RGB(50, 25, 25),
                      on ? MENU_GREEN : MENU_RED);

        // Knob
        int knobR = sh - 6;
        int knobX = on ? (tx + sw - knobR - 3) : (tx + 3);
        DrawRoundRect(hdc, knobX, ty + 3, knobR, knobR, knobR / 2,
                      on ? MENU_GREEN : MENU_RED,
                      on ? RGB(60, 220, 140) : RGB(230, 80, 80));

        // Status text
        DrawText_(hdc, on ? "TIL" : "FRA", tx, ty, sw, sh,
                  on ? MENU_GREEN : MENU_RED, m_fontSmall,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Store hit rect for the entire card
        if (m_toggleCount < 8) {
            m_toggleRects[m_toggleCount++] = { x + MENU_PAD, py, w - MENU_PAD * 2, cardH };
        }

        py += cardH + 12;
    }
}

void GameMenu::DrawAboutTab(HDC hdc, int x, int y, int w, int h) {
    int py = y + 24;
    int cw = w - MENU_PAD * 2;

    DrawText_(hdc, "AC Changer v1.0", x + MENU_PAD, py, cw, 28,
              MENU_WHITE, m_fontTitle, DT_CENTER | DT_SINGLELINE);
    py += 36;

    DrawText_(hdc, "CS2 Inventory Changer", x + MENU_PAD, py, cw, 22,
              MENU_TEXT_DIM, m_fontBody, DT_CENTER | DT_SINGLELINE);
    py += 40;

    // Separator
    HPEN sp = CreatePen(PS_SOLID, 1, MENU_BORDER);
    HPEN op = (HPEN)SelectObject(hdc, sp);
    MoveToEx(hdc, x + MENU_PAD + 40, py, nullptr);
    LineTo(hdc, x + w - MENU_PAD - 40, py);
    SelectObject(hdc, op);
    DeleteObject(sp);
    py += 24;

    // Features card
    DrawRoundRect(hdc, x + MENU_PAD, py, cw, 220, 8, MENU_CARD, MENU_BORDER);
    py += 16;

    DrawText_(hdc, "Funktioner:", x + MENU_PAD + 16, py, cw - 32, 22,
              MENU_ACCENT, m_fontBold, DT_LEFT | DT_SINGLELINE);
    py += 32;

    const char* features[] = {
        "  \xB7  Skin Changer for alle v\xE5""ben",
        "  \xB7  Knife Changer",
        "  \xB7  Glove Changer",
        "  \xB7  Opdaterede offsets",
        "  \xB7  UNDETECTED"
    };
    for (auto f : features) {
        DrawText_(hdc, f, x + MENU_PAD + 16, py, cw - 32, 24,
                  MENU_TEXT, m_fontBody, DT_LEFT | DT_SINGLELINE);
        py += 28;
    }

    py += 30;
    DrawText_(hdc, "Tryk O for at lukke menuen", x + MENU_PAD, py, cw, 22,
              MENU_TEXT_DIM, m_fontBody, DT_CENTER | DT_SINGLELINE);
}


// ============================================================================
//  OverlayWindow
// ============================================================================

OverlayWindow::OverlayWindow()
    : m_hwnd(nullptr)
    , m_gameHwnd(nullptr)
    , m_cs2Pid(0)
    , m_hModule(nullptr)
    , m_running(false)
    , m_autoShow(true) {
}

OverlayWindow::~OverlayWindow() {
    Destroy();
}

// Helper struct for EnumWindows callback
struct FindCS2WndData {
    DWORD  pid;
    HWND   bestHwnd;
    int    bestArea;
};

static BOOL CALLBACK FindCS2WndCallback(HWND hwnd, LPARAM lp) {
    auto* data = (FindCS2WndData*)lp;
    DWORD wndPid = 0;
    GetWindowThreadProcessId(hwnd, &wndPid);
    if (wndPid != data->pid) return TRUE;
    // Skip invisible windows
    if (!IsWindowVisible(hwnd)) return TRUE;
    RECT wr;
    GetWindowRect(hwnd, &wr);
    int w = wr.right - wr.left;
    int h = wr.bottom - wr.top;
    // Skip tiny or off-screen windows
    if (w < 200 || h < 200) return TRUE;
    if (wr.left > 10000 || wr.top > 10000) return TRUE;
    if (wr.left < -10000 || wr.top < -10000) return TRUE;
    int area = w * h;
    char cls[128] = {}, ttl[256] = {};
    GetClassNameA(hwnd, cls, 128);
    GetWindowTextA(hwnd, ttl, 256);
    DbgLog("  PID-enum: hwnd=%p class='%s' title='%s' rect=(%d,%d,%d,%d) area=%d",
           (void*)hwnd, cls, ttl, wr.left, wr.top, wr.right, wr.bottom, area);
    if (area > data->bestArea) {
        data->bestArea = area;
        data->bestHwnd = hwnd;
    }
    return TRUE;
}

bool OverlayWindow::Create(HMODULE hModule) {
    return Create(hModule, 0);
}

bool OverlayWindow::Create(HMODULE hModule, DWORD cs2Pid) {
    m_hModule = hModule;
    m_cs2Pid = cs2Pid;
    g_pOverlay = this;
    DbgLog("=== OverlayWindow::Create START (PID=%d) ===", (int)cs2Pid);

    // Strategy 1: Find by PID (most reliable)
    if (cs2Pid != 0) {
        DbgLog("Searching windows by PID %d...", (int)cs2Pid);
        FindCS2WndData data = { cs2Pid, nullptr, 0 };
        EnumWindows(FindCS2WndCallback, (LPARAM)&data);
        m_gameHwnd = data.bestHwnd;
        DbgLog("PID search result => %p (area=%d)", (void*)m_gameHwnd, data.bestArea);
    }

    // Strategy 2: FindWindow by title
    if (!m_gameHwnd) {
        m_gameHwnd = FindWindowA(nullptr, "Counter-Strike 2");
        DbgLog("FindWindowA(title='Counter-Strike 2') => %p", (void*)m_gameHwnd);
    }

    // Strategy 3: FindWindow by class, but validate position
    if (!m_gameHwnd) {
        HWND sdlHwnd = FindWindowA("SDL_app", nullptr);
        if (sdlHwnd) {
            RECT wr; GetWindowRect(sdlHwnd, &wr);
            DbgLog("FindWindowA(class='SDL_app') => %p rect=(%d,%d,%d,%d)",
                   (void*)sdlHwnd, wr.left, wr.top, wr.right, wr.bottom);
            // Only use if on-screen
            if (wr.left < 10000 && wr.top < 10000 && wr.left > -10000 && wr.top > -10000) {
                int w = wr.right - wr.left;
                int h = wr.bottom - wr.top;
                if (w >= 200 && h >= 200) {
                    m_gameHwnd = sdlHwnd;
                    DbgLog("SDL_app window accepted (on-screen, %dx%d)", w, h);
                } else {
                    DbgLog("SDL_app window REJECTED (too small: %dx%d)", w, h);
                }
            } else {
                DbgLog("SDL_app window REJECTED (off-screen)");
            }
        }
    }

    if (m_gameHwnd) {
        char cls[128] = {}, ttl[256] = {};
        GetClassNameA(m_gameHwnd, cls, 128);
        GetWindowTextA(m_gameHwnd, ttl, 256);
        RECT wr; GetWindowRect(m_gameHwnd, &wr);
        DbgLog("Selected CS2 window: hwnd=%p class='%s' title='%s' rect=(%d,%d,%d,%d)",
               (void*)m_gameHwnd, cls, ttl, wr.left, wr.top, wr.right, wr.bottom);
        LONG style = GetWindowLong(m_gameHwnd, GWL_STYLE);
        LONG exstyle = GetWindowLong(m_gameHwnd, GWL_EXSTYLE);
        DbgLog("CS2 style=0x%08X exstyle=0x%08X", style, exstyle);
    } else {
        DbgLog("WARNING: No suitable CS2 window found! Using primary monitor.");
    }

    // Register overlay window class
    WNDCLASSEXA wc = { sizeof(wc) };
    wc.lpfnWndProc   = OverlayWindow::WndProc;
    wc.hInstance      = hModule;
    wc.lpszClassName  = "AC_Overlay";
    wc.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
    ATOM regResult = RegisterClassExA(&wc);
    DbgLog("RegisterClassExA => %d (LastError=%d)", regResult, (int)GetLastError());

    // Get game window position / size (use primary monitor as fallback)
    RECT gameRect = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    if (m_gameHwnd) {
        RECT wr;
        GetWindowRect(m_gameHwnd, &wr);
        // Validate the rect is sane
        if (wr.left < 10000 && wr.top < 10000 && wr.left > -10000 && wr.top > -10000) {
            int w = wr.right - wr.left;
            int h = wr.bottom - wr.top;
            if (w >= 200 && h >= 200) {
                gameRect = wr;
            } else {
                DbgLog("Game window rect too small (%dx%d), using monitor", w, h);
                m_gameHwnd = nullptr;
            }
        } else {
            DbgLog("Game window rect off-screen (%d,%d), using monitor", wr.left, wr.top);
            m_gameHwnd = nullptr;
        }
    }
    int gw = gameRect.right  - gameRect.left;
    int gh = gameRect.bottom - gameRect.top;
    DbgLog("Overlay size: %dx%d at (%d,%d)", gw, gh, gameRect.left, gameRect.top);

    // Create transparent layered window
    m_hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        "AC_Overlay", "AC Overlay",
        WS_POPUP,
        gameRect.left, gameRect.top, gw, gh,
        nullptr, nullptr, hModule, nullptr);

    if (!m_hwnd) {
        DbgLog("ERROR: CreateWindowExA FAILED! LastError=%d", (int)GetLastError());
        return false;
    }
    DbgLog("Overlay window created: hwnd=%p", (void*)m_hwnd);

    // Make window transparent (magenta is the transparent colour key)
    BOOL layerOk = SetLayeredWindowAttributes(m_hwnd, RGB(255, 0, 255), 0, LWA_COLORKEY);
    DbgLog("SetLayeredWindowAttributes => %d", (int)layerOk);

    // Initialise menu
    if (!m_menu.Initialize(hModule)) {
        DbgLog("ERROR: GameMenu::Initialize failed!");
        return false;
    }
    DbgLog("GameMenu initialized OK");

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    SetForegroundWindow(m_hwnd);
    BringWindowToTop(m_hwnd);
    DbgLog("Overlay window shown and brought to front");

    m_running = true;
    DbgLog("=== OverlayWindow::Create SUCCESS ===");
    return true;
}

void OverlayWindow::Destroy() {
    m_running = false;
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    m_menu.Shutdown();
    g_pOverlay = nullptr;
}

void OverlayWindow::ShowMenu() {
    DbgLog("ShowMenu() called, m_visible was %d", (int)m_menu.IsVisible());
    if (!m_menu.IsVisible()) {
        m_menu.Toggle();
    }
    // Make overlay interactive (remove click-through)
    LONG ex = GetWindowLong(m_hwnd, GWL_EXSTYLE);
    ex &= ~WS_EX_TRANSPARENT;
    SetWindowLong(m_hwnd, GWL_EXSTYLE, ex);
    DbgLog("ShowMenu() done, m_visible=%d exstyle=0x%08X", (int)m_menu.IsVisible(), (int)GetWindowLong(m_hwnd, GWL_EXSTYLE));
}

void OverlayWindow::UpdatePosition() {
    // Try to find the real CS2 window if we don't have one or it's invalid
    if (!m_gameHwnd || !IsWindow(m_gameHwnd)) {
        m_gameHwnd = nullptr;
        // Search by PID first
        if (m_cs2Pid != 0) {
            FindCS2WndData data = { m_cs2Pid, nullptr, 0 };
            EnumWindows(FindCS2WndCallback, (LPARAM)&data);
            m_gameHwnd = data.bestHwnd;
        }
        // Fallback: by title
        if (!m_gameHwnd) {
            m_gameHwnd = FindWindowA(nullptr, "Counter-Strike 2");
        }
        if (!m_gameHwnd) return;

        // Log when we find a new window
        static bool loggedNewWindow = false;
        if (!loggedNewWindow) {
            char cls[128] = {}, ttl[256] = {};
            GetClassNameA(m_gameHwnd, cls, 128);
            GetWindowTextA(m_gameHwnd, ttl, 256);
            RECT wr; GetWindowRect(m_gameHwnd, &wr);
            DbgLog("UpdatePosition: Found new CS2 window hwnd=%p class='%s' title='%s' rect=(%d,%d,%d,%d)",
                   (void*)m_gameHwnd, cls, ttl, wr.left, wr.top, wr.right, wr.bottom);
            loggedNewWindow = true;
        }
    }

    RECT gr;
    GetWindowRect(m_gameHwnd, &gr);

    // Validate position is sane
    if (gr.left > 10000 || gr.top > 10000 || gr.left < -10000 || gr.top < -10000) {
        // Window is off-screen, use monitor instead
        gr.left = 0; gr.top = 0;
        gr.right = GetSystemMetrics(SM_CXSCREEN);
        gr.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    int gw = gr.right  - gr.left;
    int gh = gr.bottom - gr.top;
    if (gw < 200 || gh < 200) return;  // skip invalid sizes

    SetWindowPos(m_hwnd, HWND_TOPMOST, gr.left, gr.top, gw, gh,
                 SWP_NOACTIVATE);
}

void OverlayWindow::RunFrame() {
    if (!m_running) return;

    // Follow game window position
    UpdatePosition();

    // Auto-show menu on first frame
    if (m_autoShow) {
        m_autoShow = false;
        DbgLog("Auto-show: showing menu on first frame");
        ShowMenu();
        DbgLog("Auto-show complete. m_hwnd=%p visible=%d", (void*)m_hwnd, (int)IsWindowVisible(m_hwnd));
    }

    // Periodic debug (every ~5 seconds = 300 frames)
    static int frameCount = 0;
    frameCount++;
    if (frameCount % 300 == 1) {
        DbgLog("Frame %d: m_hwnd=%p gameHwnd=%p menuVisible=%d hwndVisible=%d",
               frameCount, (void*)m_hwnd, (void*)m_gameHwnd,
               (int)m_menu.IsVisible(), (int)IsWindowVisible(m_hwnd));
        RECT wr; GetWindowRect(m_hwnd, &wr);
        DbgLog("  overlay rect=(%d,%d,%d,%d) size=%dx%d",
               wr.left, wr.top, wr.right, wr.bottom,
               wr.right-wr.left, wr.bottom-wr.top);
    }

    // Check O key to toggle menu
    static bool keyWasDown = false;
    bool keyDown = (GetAsyncKeyState('O') & 0x8000) != 0;
    if (keyDown && !keyWasDown) {
        if (m_menu.IsVisible()) {
            // Hide menu, make window click-through
            m_menu.Toggle();
            LONG ex = GetWindowLong(m_hwnd, GWL_EXSTYLE);
            ex |= WS_EX_TRANSPARENT;
            SetWindowLong(m_hwnd, GWL_EXSTYLE, ex);
        } else {
            ShowMenu();
        }
    }
    keyWasDown = keyDown;

    // Process menu input
    m_menu.ProcessInput();

    // Repaint
    InvalidateRect(m_hwnd, nullptr, TRUE);
    UpdateWindow(m_hwnd);

    // Process pending messages
    MSG msg;
    while (PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK OverlayWindow::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right, h = rc.bottom;

        // Double buffer
        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
        SelectObject(mem, bmp);

        // Clear to colour key (magenta = transparent)
        HBRUSH bg = CreateSolidBrush(RGB(255, 0, 255));
        FillRect(mem, &rc, bg);
        DeleteObject(bg);

        // Draw menu
        if (g_pOverlay) {
            g_pOverlay->m_menu.Render(mem, w, h);
        }

        BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
        DeleteObject(bmp);
        DeleteDC(mem);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE:
        if (g_pOverlay) {
            int mx = (short)LOWORD(lp);
            int my = (short)HIWORD(lp);
            g_pOverlay->m_menu.OnMouseMove(mx, my);
            InvalidateRect(hwnd, nullptr, FALSE);  // repaint for hover
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (g_pOverlay) {
            int mx = (short)LOWORD(lp);
            int my = (short)HIWORD(lp);
            g_pOverlay->m_menu.OnMouseClick(mx, my);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_MOUSEWHEEL:
        if (g_pOverlay) {
            int delta = GET_WHEEL_DELTA_WPARAM(wp);
            g_pOverlay->m_menu.OnMouseWheel(delta);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}


// ============================================================================
//  GameInjector
// ============================================================================

GameInjector::GameInjector()
    : m_initialized(false)
    , m_overlay(std::make_unique<OverlayWindow>()) {
}

GameInjector::~GameInjector() {
    if (m_initialized) Shutdown();
}

bool GameInjector::Initialize(HMODULE hModule) {
    if (!m_overlay->Create(hModule)) {
        return false;
    }
    m_initialized = true;
    return true;
}

void GameInjector::Shutdown() {
    if (m_overlay) m_overlay->Destroy();
    m_initialized = false;
}

void GameInjector::Update() {
    if (!m_initialized) return;
    m_overlay->RunFrame();
}
