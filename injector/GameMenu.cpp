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
    , m_selectedRow(0)
    , m_scrollOffset(0)
    , m_skinChangerEnabled(true)
    , m_knifeChangerEnabled(false)
    , m_gloveChangerEnabled(false)
    , m_fontTitle(nullptr)
    , m_fontBody(nullptr)
    , m_fontSmall(nullptr)
    , m_fontBold(nullptr) {
}

GameMenu::~GameMenu() {
    if (m_fontTitle) DeleteObject(m_fontTitle);
    if (m_fontBody)  DeleteObject(m_fontBody);
    if (m_fontSmall) DeleteObject(m_fontSmall);
    if (m_fontBold)  DeleteObject(m_fontBold);
}

bool GameMenu::Initialize(HMODULE) {
    m_fontTitle = CreateFontA(16, 0, 0, 0, FW_BOLD,   0, 0, 0, DEFAULT_CHARSET,
                              OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH, "Segoe UI");
    m_fontBody  = CreateFontA(13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                              OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH, "Segoe UI");
    m_fontSmall = CreateFontA(11, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                              OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH, "Segoe UI");
    m_fontBold  = CreateFontA(13, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
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
        m_selectedRow = 0;
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
//  Input
// ────────────────────────────────────────────────
void GameMenu::ProcessInput() {
    if (!m_visible) return;

    // Tab switching: LEFT / RIGHT arrows
    if (GetAsyncKeyState(VK_LEFT) & 1) {
        m_activeTab = (Tab)(((int)m_activeTab - 1 + TAB_COUNT) % TAB_COUNT);
        m_selectedRow = 0;
        m_scrollOffset = 0;
    }
    if (GetAsyncKeyState(VK_RIGHT) & 1) {
        m_activeTab = (Tab)(((int)m_activeTab + 1) % TAB_COUNT);
        m_selectedRow = 0;
        m_scrollOffset = 0;
    }

    // Row navigation: UP / DOWN
    if (GetAsyncKeyState(VK_UP) & 1) {
        if (m_selectedRow > 0) m_selectedRow--;
    }
    if (GetAsyncKeyState(VK_DOWN) & 1) {
        m_selectedRow++;
    }

    // Activate / toggle: ENTER
    if (GetAsyncKeyState(VK_RETURN) & 1) {
        if (m_activeTab == TAB_SKINS) {
            // Build flat list: weapon headers + skins
            int idx = 0;
            for (auto& w : m_weapons) {
                if (idx == m_selectedRow) break; // header row – skip
                idx++;
                auto& skins = m_weaponSkins[w];
                for (auto& s : skins) {
                    if (idx == m_selectedRow) {
                        // Toggle skin
                        if (m_activeSkins[w] == s)
                            m_activeSkins.erase(w);
                        else
                            m_activeSkins[w] = s;
                        break;
                    }
                    idx++;
                }
            }
        }
        if (m_activeTab == TAB_SETTINGS) {
            if (m_selectedRow == 0) m_skinChangerEnabled  = !m_skinChangerEnabled;
            if (m_selectedRow == 1) m_knifeChangerEnabled = !m_knifeChangerEnabled;
            if (m_selectedRow == 2) m_gloveChangerEnabled = !m_gloveChangerEnabled;
        }
    }
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

    // Shadow
    DrawRoundRect(hdc, mx + 3, my + 3, MENU_WIDTH, MENU_HEIGHT, MENU_RAD,
                  RGB(2, 2, 2), RGB(2, 2, 2));

    // Main background
    DrawRoundRect(hdc, mx, my, MENU_WIDTH, MENU_HEIGHT, MENU_RAD,
                  MENU_BG, MENU_BORDER);

    // Header
    DrawHeader(hdc, mx, my, MENU_WIDTH);

    // Tabs
    int tabY = my + MENU_HEADER;
    DrawTabs(hdc, mx, tabY, MENU_WIDTH);

    // Content area
    int contentY = tabY + MENU_TAB_H + 4;
    int contentH = MENU_HEIGHT - MENU_HEADER - MENU_TAB_H - 4 - 30;

    // Clip to content area
    HRGN clip = CreateRectRgn(mx + 1, contentY, mx + MENU_WIDTH - 1, contentY + contentH);
    SelectClipRgn(hdc, clip);

    switch (m_activeTab) {
    case TAB_SKINS:    DrawSkinsTab(hdc, mx, contentY, MENU_WIDTH, contentH); break;
    case TAB_SETTINGS: DrawSettingsTab(hdc, mx, contentY, MENU_WIDTH, contentH); break;
    case TAB_ABOUT:    DrawAboutTab(hdc, mx, contentY, MENU_WIDTH, contentH); break;
    default: break;
    }

    SelectClipRgn(hdc, nullptr);
    DeleteObject(clip);

    // Footer
    int footerY = my + MENU_HEIGHT - 24;
    DrawText_(hdc, "O = Luk  |  Pile = V\xE6lg  |  Tab = Skift  |  ENTER = Aktiver",
              mx + MENU_PAD, footerY, MENU_WIDTH - MENU_PAD * 2, 20,
              MENU_TEXT_DIM, m_fontSmall, DT_CENTER | DT_SINGLELINE);
}

void GameMenu::DrawHeader(HDC hdc, int x, int y, int w) {
    // Dark header bar
    DrawRoundRect(hdc, x, y, w, MENU_HEADER, MENU_RAD, MENU_BG_HEADER, MENU_BORDER);
    // Flatten bottom corners by overpainting
    RECT rc = { x + 1, y + MENU_HEADER - MENU_RAD, x + w - 1, y + MENU_HEADER };
    HBRUSH br = CreateSolidBrush(MENU_BG_HEADER);
    FillRect(hdc, &rc, br);
    DeleteObject(br);

    // Title
    DrawText_(hdc, "AC CHANGER", x + MENU_PAD, y, 160, MENU_HEADER,
              MENU_WHITE, m_fontTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Version badge
    DrawRoundRect(hdc, x + w - 68, y + 11, 54, 20, 10, RGB(20, 60, 40), MENU_GREEN);
    DrawText_(hdc, "v1.0", x + w - 68, y + 11, 54, 20,
              MENU_GREEN, m_fontSmall, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void GameMenu::DrawTabs(HDC hdc, int x, int y, int w) {
    const char* tabLabels[TAB_COUNT] = { "Skins", "Indstillinger", "Om" };
    int tabW = w / TAB_COUNT;

    for (int i = 0; i < TAB_COUNT; i++) {
        int tx = x + i * tabW;
        bool sel = ((int)m_activeTab == i);

        COLORREF bg  = sel ? MENU_ITEM_SEL : MENU_BG;
        COLORREF txt = sel ? MENU_ACCENT   : MENU_TEXT_DIM;

        // Tab background
        RECT trc = { tx, y, tx + tabW, y + MENU_TAB_H };
        HBRUSH tbr = CreateSolidBrush(bg);
        FillRect(hdc, &trc, tbr);
        DeleteObject(tbr);

        // Tab text
        DrawText_(hdc, tabLabels[i], tx, y, tabW, MENU_TAB_H,
                  txt, m_fontBold, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Active indicator line
        if (sel) {
            RECT ind = { tx + 4, y + MENU_TAB_H - 2, tx + tabW - 4, y + MENU_TAB_H };
            HBRUSH ibr = CreateSolidBrush(MENU_ACCENT);
            FillRect(hdc, &ind, ibr);
            DeleteObject(ibr);
        }
    }

    // Bottom separator
    HPEN sp = CreatePen(PS_SOLID, 1, MENU_BORDER);
    HPEN op = (HPEN)SelectObject(hdc, sp);
    MoveToEx(hdc, x, y + MENU_TAB_H, nullptr);
    LineTo(hdc, x + w, y + MENU_TAB_H);
    SelectObject(hdc, op);
    DeleteObject(sp);
}

void GameMenu::DrawSkinsTab(HDC hdc, int x, int y, int w, int h) {
    int row = 0;
    int py = y + 4;

    for (auto& weapon : m_weapons) {
        // Weapon header
        bool isSel = (row == m_selectedRow);
        COLORREF hdrBg = isSel ? MENU_ITEM_SEL : RGB(16, 16, 18);
        RECT hdr = { x + MENU_PAD, py, x + w - MENU_PAD, py + MENU_ROW_H };
        HBRUSH hb = CreateSolidBrush(hdrBg);
        FillRect(hdc, &hdr, hb);
        DeleteObject(hb);

        std::string label = "  " + weapon;
        DrawText_(hdc, label.c_str(), x + MENU_PAD + 4, py, w - MENU_PAD * 2 - 8, MENU_ROW_H,
                  MENU_ACCENT, m_fontBold, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Show active skin name on the right
        auto it = m_activeSkins.find(weapon);
        if (it != m_activeSkins.end()) {
            DrawText_(hdc, it->second.c_str(), x + MENU_PAD + 4, py,
                      w - MENU_PAD * 2 - 8, MENU_ROW_H,
                      MENU_GREEN, m_fontSmall, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        }

        py += MENU_ROW_H;
        row++;

        // Skins under this weapon
        for (auto& skin : m_weaponSkins[weapon]) {
            isSel = (row == m_selectedRow);
            bool active = (m_activeSkins.count(weapon) && m_activeSkins[weapon] == skin);

            COLORREF skinBg = isSel ? MENU_ITEM_SEL : MENU_ITEM_BG;
            RECT sr = { x + MENU_PAD + 8, py, x + w - MENU_PAD, py + MENU_ROW_H - 2 };
            HBRUSH sb = CreateSolidBrush(skinBg);
            FillRect(hdc, &sr, sb);
            DeleteObject(sb);

            // Selection indicator bar
            if (isSel) {
                RECT si = { x + MENU_PAD + 8, py, x + MENU_PAD + 11, py + MENU_ROW_H - 2 };
                HBRUSH sib = CreateSolidBrush(MENU_ACCENT);
                FillRect(hdc, &si, sib);
                DeleteObject(sib);
            }

            // Skin name
            std::string skinLabel = "    " + skin;
            DrawText_(hdc, skinLabel.c_str(), x + MENU_PAD + 12, py,
                      w - MENU_PAD * 2 - 60, MENU_ROW_H - 2,
                      isSel ? MENU_WHITE : MENU_TEXT, m_fontBody,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            // Active badge
            if (active) {
                int bx = x + w - MENU_PAD - 50;
                DrawRoundRect(hdc, bx, py + 5, 42, 18, 9, RGB(20, 60, 40), MENU_GREEN);
                DrawText_(hdc, "AKTIV", bx, py + 5, 42, 18,
                          MENU_GREEN, m_fontSmall, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            py += MENU_ROW_H - 2;
            row++;
        }

        py += 2; // gap between weapon groups
    }

    // Clamp selected row
    if (m_selectedRow >= row) m_selectedRow = row - 1;
    if (m_selectedRow < 0)   m_selectedRow = 0;
}

void GameMenu::DrawSettingsTab(HDC hdc, int x, int y, int w, int h) {
    struct Setting {
        const char* name;
        bool* value;
    };
    Setting settings[] = {
        { "Skin Changer",   &m_skinChangerEnabled },
        { "Knife Changer",  &m_knifeChangerEnabled },
        { "Glove Changer",  &m_gloveChangerEnabled },
    };
    int count = sizeof(settings) / sizeof(settings[0]);
    if (m_selectedRow >= count) m_selectedRow = count - 1;

    int py = y + 8;
    for (int i = 0; i < count; i++) {
        bool sel = (i == m_selectedRow);
        COLORREF bg = sel ? MENU_ITEM_SEL : MENU_ITEM_BG;

        DrawRoundRect(hdc, x + MENU_PAD, py, w - MENU_PAD * 2, MENU_ROW_H + 4, 6,
                      bg, sel ? MENU_ACCENT : MENU_BORDER);

        // Label
        DrawText_(hdc, settings[i].name, x + MENU_PAD + 12, py,
                  w - MENU_PAD * 2 - 80, MENU_ROW_H + 4,
                  sel ? MENU_WHITE : MENU_TEXT, m_fontBody,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Toggle indicator
        bool on = *settings[i].value;
        int tx = x + w - MENU_PAD - 56;
        DrawRoundRect(hdc, tx, py + 7, 40, 20, 10,
                      on ? RGB(20, 60, 40) : RGB(40, 20, 20),
                      on ? MENU_GREEN : MENU_RED);
        DrawText_(hdc, on ? "TIL" : "FRA", tx, py + 7, 40, 20,
                  on ? MENU_GREEN : MENU_RED, m_fontSmall,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        py += MENU_ROW_H + 8;
    }
}

void GameMenu::DrawAboutTab(HDC hdc, int x, int y, int w, int h) {
    int py = y + 16;
    int cw = w - MENU_PAD * 2;

    DrawText_(hdc, "AC Changer v1.0", x + MENU_PAD, py, cw, 20,
              MENU_WHITE, m_fontTitle, DT_CENTER | DT_SINGLELINE);
    py += 26;

    DrawText_(hdc, "CS2 Inventory Changer", x + MENU_PAD, py, cw, 18,
              MENU_TEXT_DIM, m_fontBody, DT_CENTER | DT_SINGLELINE);
    py += 30;

    // Separator
    HPEN sp = CreatePen(PS_SOLID, 1, MENU_BORDER);
    HPEN op = (HPEN)SelectObject(hdc, sp);
    MoveToEx(hdc, x + MENU_PAD + 30, py, nullptr); LineTo(hdc, x + w - MENU_PAD - 30, py);
    SelectObject(hdc, op);
    DeleteObject(sp);
    py += 16;

    DrawText_(hdc, "Funktioner:", x + MENU_PAD, py, cw, 18,
              MENU_ACCENT, m_fontBold, DT_LEFT | DT_SINGLELINE);
    py += 22;

    const char* features[] = {
        "  \xB7  Skin Changer for alle v\xE5""ben",
        "  \xB7  Knife Changer",
        "  \xB7  Glove Changer",
        "  \xB7  Opdaterede offsets",
        "  \xB7  UNDETECTED"
    };
    for (auto f : features) {
        DrawText_(hdc, f, x + MENU_PAD, py, cw, 18,
                  MENU_TEXT, m_fontBody, DT_LEFT | DT_SINGLELINE);
        py += 20;
    }

    py += 16;
    DrawText_(hdc, "Tryk O for at lukke menuen", x + MENU_PAD, py, cw, 18,
              MENU_TEXT_DIM, m_fontSmall, DT_CENTER | DT_SINGLELINE);
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
