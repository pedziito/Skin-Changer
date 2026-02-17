#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

// ============================================================================
//  CS2 In-Game Overlay Menu
//  Opens/closes with O key
//  Dark, clean overlay rendered via transparent GDI window
// ============================================================================

// Menu colours (dark theme)
#define MENU_BG         RGB(12, 12, 14)
#define MENU_BG_HEADER  RGB(8, 8, 10)
#define MENU_ITEM_BG    RGB(20, 20, 22)
#define MENU_ITEM_SEL   RGB(38, 38, 44)
#define MENU_ACCENT     RGB(90, 180, 250)
#define MENU_TEXT        RGB(210, 210, 220)
#define MENU_TEXT_DIM    RGB(100, 100, 110)
#define MENU_BORDER     RGB(40, 40, 46)
#define MENU_GREEN       RGB(45, 200, 120)
#define MENU_RED         RGB(210, 60, 60)
#define MENU_WHITE       RGB(235, 235, 240)

// Layout
#define MENU_WIDTH   340
#define MENU_HEIGHT  460
#define MENU_HEADER  42
#define MENU_TAB_H   32
#define MENU_ROW_H   30
#define MENU_PAD     14
#define MENU_RAD     8

/**
 * In-Game Overlay Menu for CS2 Skin Changer
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

    // Called from main loop
    void ProcessInput();
    void Render(HDC hdc, int winW, int winH);

    // Toggle visibility (INSERT key)
    void Toggle();
    bool IsVisible() const { return m_visible; }

    // Weapon / skin data
    void AddWeapon(const std::string& weapon);
    void AddSkin(const std::string& weapon, const std::string& skin);

private:
    // Render helpers
    void DrawHeader(HDC hdc, int x, int y, int w);
    void DrawTabs(HDC hdc, int x, int y, int w);
    void DrawSkinsTab(HDC hdc, int x, int y, int w, int h);
    void DrawSettingsTab(HDC hdc, int x, int y, int w, int h);
    void DrawAboutTab(HDC hdc, int x, int y, int w, int h);
    void DrawRoundRect(HDC hdc, int x, int y, int w, int h, int r, COLORREF fill, COLORREF border);
    void DrawText_(HDC hdc, const char* text, int x, int y, int w, int h, COLORREF col, HFONT font, UINT fmt = DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    bool    m_visible;
    Tab     m_activeTab;
    int     m_selectedRow;
    int     m_scrollOffset;

    // Skin database
    std::vector<std::string> m_weapons;
    std::map<std::string, std::vector<std::string>> m_weaponSkins;
    std::map<std::string, std::string> m_activeSkins;  // weapon -> active skin

    // Settings
    bool m_skinChangerEnabled;
    bool m_knifeChangerEnabled;
    bool m_gloveChangerEnabled;

    // Fonts (created once)
    HFONT m_fontTitle;
    HFONT m_fontBody;
    HFONT m_fontSmall;
    HFONT m_fontBold;
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
    void Destroy();
    void RunFrame();
    void ShowMenu();   // Show menu and make window interactive

    GameMenu& GetMenu() { return m_menu; }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

private:
    void UpdatePosition();   // follow CS2 window

    HWND      m_hwnd;
    HWND      m_gameHwnd;    // CS2 window handle
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
