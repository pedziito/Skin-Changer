#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

// ============================================================================
//  CS2 In-Game Overlay Menu  –  Click-based, NL-style dark navy theme
//  Opens/closes with O key.  All interaction via mouse clicks.
// ============================================================================

// Colours – dark navy theme (same as the loader)
#define MENU_BG         RGB(15, 17, 26)
#define MENU_BG_HEADER  RGB(10, 12, 20)
#define MENU_CARD       RGB(24, 28, 42)
#define MENU_CARD_HOV   RGB(30, 35, 52)
#define MENU_CARD_BR    RGB(40, 46, 65)
#define MENU_ACCENT     RGB(70, 130, 220)
#define MENU_ACCENT_L   RGB(100, 160, 245)
#define MENU_TEXT        RGB(200, 205, 220)
#define MENU_TEXT_DIM    RGB(120, 128, 150)
#define MENU_BORDER     RGB(30, 34, 50)
#define MENU_GREEN       RGB(45, 200, 120)
#define MENU_RED         RGB(210, 60, 60)
#define MENU_WHITE       RGB(235, 238, 245)
#define MENU_SIDEBAR    RGB(15, 17, 26)
#define MENU_SIDEBAR_BR RGB(30, 34, 50)
#define MENU_FIELD      RGB(24, 28, 42)
#define MENU_BTN_BG     RGB(55, 110, 200)
#define MENU_BTN_HOV    RGB(70, 130, 220)
#define MENU_ICON_GOLD  RGB(220, 165, 40)

// Layout – 4× bigger than original (800×650 vs. 340×460)
#define MENU_WIDTH   800
#define MENU_HEIGHT  650
#define MENU_HEADER  60
#define MENU_TAB_H   44
#define MENU_ROW_H   42
#define MENU_PAD     20
#define MENU_RAD     10
#define MENU_SIDEBAR_W  180

/**
 * In-Game Overlay Menu for CS2 Skin Changer
 * Full mouse/click interaction – no keyboard navigation
 */
class GameMenu {
public:
    enum Tab {
        TAB_SKINS    = 0,
        TAB_SETTINGS = 1,
        TAB_ABOUT    = 2,
        TAB_COUNT    = 3
    };

    struct SkinEntry {
        std::string weapon;
        std::string skin;
        bool active;
    };

    GameMenu();
    ~GameMenu();

    // Lifecycle
    bool Initialize(HMODULE hModule);
    void Shutdown();

    // Mouse input – called from OverlayWindow
    void OnMouseMove(int x, int y);
    void OnMouseClick(int x, int y);
    void OnMouseWheel(int delta);

    // Render
    void Render(HDC hdc, int winW, int winH);

    // Toggle visibility
    void Toggle();
    bool IsVisible() const { return m_visible; }

    // Weapon / skin data
    void AddWeapon(const std::string& weapon);
    void AddSkin(const std::string& weapon, const std::string& skin);

    // Legacy (unused, kept for compat)
    void ProcessInput() {}

private:
    // Hit-test rectangle
    struct HitRect { int x, y, w, h; };
    bool HitTest(const HitRect& r, int mx, int my) const {
        return mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
    }

    // Render helpers
    void DrawSidebar(HDC hdc, int x, int y, int h);
    void DrawHeader(HDC hdc, int x, int y, int w);
    void DrawSkinsTab(HDC hdc, int x, int y, int w, int h);
    void DrawSettingsTab(HDC hdc, int x, int y, int w, int h);
    void DrawAboutTab(HDC hdc, int x, int y, int w, int h);
    void DrawRoundRect(HDC hdc, int x, int y, int w, int h, int r, COLORREF fill, COLORREF border);
    void DrawText_(HDC hdc, const char* text, int x, int y, int w, int h, COLORREF col, HFONT font, UINT fmt = DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    bool    m_visible;
    Tab     m_activeTab;
    int     m_scrollOffset;

    // Mouse state
    int  m_mouseX, m_mouseY;
    int  m_menuX, m_menuY;    // top-left of menu panel (computed in Render)

    // Hit rects built each frame during Render
    HitRect m_tabRects[TAB_COUNT];
    static const int MAX_ROWS = 128;
    HitRect m_rowRects[MAX_ROWS];
    int     m_rowWeaponIdx[MAX_ROWS];  // -1 = weapon header
    int     m_rowSkinIdx[MAX_ROWS];
    int     m_rowCount;
    HitRect m_toggleRects[8];
    int     m_toggleCount;

    // Skin database
    std::vector<std::string> m_weapons;
    std::map<std::string, std::vector<std::string>> m_weaponSkins;
    std::map<std::string, std::string> m_activeSkins;

    // Settings
    bool m_skinChangerEnabled;
    bool m_knifeChangerEnabled;
    bool m_gloveChangerEnabled;

    // Fonts
    HFONT m_fontTitle;
    HFONT m_fontBody;
    HFONT m_fontSmall;
    HFONT m_fontBold;
    HFONT m_fontLogo;
};

/**
 * Overlay Window Manager – creates a transparent always-on-top window
 * that sits over CS2 and renders the GameMenu
 */
class OverlayWindow {
public:
    OverlayWindow();
    ~OverlayWindow();

    bool Create(HMODULE hModule);
    bool Create(HMODULE hModule, DWORD cs2Pid);
    void Destroy();
    void RunFrame();
    void ShowMenu();   // Show menu and make window interactive

    GameMenu& GetMenu() { return m_menu; }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

private:
    void UpdatePosition();   // follow CS2 window

    HWND      m_hwnd;
    HWND      m_gameHwnd;    // CS2 window handle
    DWORD     m_cs2Pid;      // CS2 process ID for window search
    GameMenu  m_menu;
    HMODULE   m_hModule;
    bool      m_running;
    bool      m_autoShow;    // auto-show menu on first frame
};

/**
 * DLL Injection Handler
 */
class GameInjector {
public:
    GameInjector();
    ~GameInjector();

    bool Initialize(HMODULE hModule);
    void Shutdown();
    void Update();

    GameMenu& GetMenu() { return m_overlay->GetMenu(); }

private:
    std::unique_ptr<OverlayWindow> m_overlay;
    bool m_initialized;
};
