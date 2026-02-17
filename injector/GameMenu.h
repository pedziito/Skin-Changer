#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

// ============================================================================
//  CS2 In-Game Overlay Menu  –  NEVERLOSE-style
//  INSERT toggles.  Sidebar: Inventory Changer + Configs.
//  Grid-based skin browser with float slider.
// ============================================================================

// Colours – NEVERLOSE dark theme
#define MENU_BG         RGB(18, 18, 22)
#define MENU_BG_HEADER  RGB(12, 12, 16)
#define MENU_SIDEBAR_C  RGB(14, 14, 18)
#define MENU_SIDEBAR_BR RGB(32, 32, 38)
#define MENU_CARD       RGB(28, 28, 34)
#define MENU_CARD_HOV   RGB(36, 36, 44)
#define MENU_CARD_BR    RGB(48, 48, 58)
#define MENU_ACCENT     RGB(50, 140, 220)
#define MENU_ACCENT_L   RGB(80, 170, 250)
#define MENU_TEXT        RGB(195, 195, 205)
#define MENU_TEXT_DIM    RGB(100, 100, 115)
#define MENU_BORDER     RGB(38, 38, 46)
#define MENU_GREEN       RGB(45, 200, 120)
#define MENU_RED         RGB(210, 60, 60)
#define MENU_WHITE       RGB(230, 230, 240)
#define MENU_ORANGE      RGB(220, 140, 40)
#define MENU_BTN_BG     RGB(50, 130, 210)
#define MENU_BTN_HOV    RGB(65, 150, 230)
#define MENU_TOGGLE_ON  RGB(50, 140, 220)
#define MENU_TOGGLE_OFF RGB(55, 55, 65)

// Layout
#define MENU_WIDTH      1100
#define MENU_HEIGHT     700
#define MENU_SIDEBAR_W  170
#define MENU_HEADER_H   50
#define MENU_PAD        16
#define MENU_RAD        8

// Grid
#define GRID_COLS       4
#define GRID_CARD_W     190
#define GRID_CARD_H     140
#define GRID_GAP        12

/**
 * In-Game Overlay Menu – NEVERLOSE style
 * Sidebar: Inventory Changer + Configs
 * Grid skin browser with float slider + equip
 */
class GameMenu {
public:
    // Sidebar pages
    enum Page {
        PAGE_INVENTORY = 0,
        PAGE_CONFIGS   = 1,
        PAGE_COUNT     = 2
    };

    // Sub-views within Inventory page
    enum InvView {
        INV_EQUIPPED = 0,   // shows equipped items
        INV_WEAPONS,        // weapon category grid
        INV_SKINS,          // skins for a weapon (grid)
        INV_DETAIL          // single skin detail + float slider
    };

    // An inventory item the user has equipped
    struct EquippedItem {
        std::string weapon;
        std::string skin;
        float       floatVal;
    };

    GameMenu();
    ~GameMenu();

    bool Initialize(HMODULE hModule);
    void Shutdown();

    // Mouse
    void OnMouseMove(int x, int y);
    void OnMouseClick(int x, int y);
    void OnMouseUp();
    void OnMouseWheel(int delta);

    // Render
    void Render(HDC hdc, int winW, int winH);

    void Toggle();
    bool IsVisible() const { return m_visible; }

    // Legacy compat
    void ProcessInput() {}
    void AddWeapon(const std::string&) {}
    void AddSkin(const std::string&, const std::string&) {}

private:
    // Hit-test
    struct HitRect { int x, y, w, h; };
    bool HitTest(const HitRect& r, int mx, int my) const {
        return mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
    }

    // Skin database types
    struct WeaponDef {
        std::string name;
        std::vector<std::string> skins;
    };

    // Drawing helpers
    void DrawRoundRect(HDC hdc, int x, int y, int w, int h, int r, COLORREF fill, COLORREF border);
    void DrawText_(HDC hdc, const char* text, int x, int y, int w, int h, COLORREF col, HFONT font, UINT fmt = DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    void DrawButton(HDC hdc, int x, int y, int w, int h, const char* text, bool hov, COLORREF bg, COLORREF bgHov);

    // Page renderers
    void DrawSidebar(HDC hdc, int x, int y, int h);
    void DrawHeader(HDC hdc, int x, int y, int w);
    void DrawEquipped(HDC hdc, int x, int y, int w, int h);
    void DrawWeaponGrid(HDC hdc, int x, int y, int w, int h);
    void DrawSkinGrid(HDC hdc, int x, int y, int w, int h);
    void DrawSkinDetail(HDC hdc, int x, int y, int w, int h);
    void DrawConfigs(HDC hdc, int x, int y, int w, int h);

    // State
    bool    m_visible;
    Page    m_activePage;
    InvView m_invView;
    int     m_scrollOffset;
    int     m_mouseX, m_mouseY;
    int     m_menuX, m_menuY;

    // Weapon/skin database
    std::vector<WeaponDef> m_weaponDB;
    void BuildDatabase();

    // Current selection
    int   m_selWeapon;          // index into m_weaponDB
    int   m_selSkin;            // index into weapon's skins
    float m_detailFloat;        // float slider value 0.0–1.0

    // Equipped items (user's inventory)
    std::vector<EquippedItem> m_equipped;

    // Configs
    bool m_skinChangerEnabled;
    bool m_knifeChangerEnabled;
    bool m_gloveChangerEnabled;

    // Hit rects (rebuilt each frame)
    static const int MAX_GRID = 64;
    HitRect m_gridRects[MAX_GRID];
    int     m_gridIds[MAX_GRID];
    int     m_gridCount;

    HitRect m_sidebarRects[PAGE_COUNT];
    HitRect m_addBtnRect;
    HitRect m_backBtnRect;
    HitRect m_applyBtnRect;
    HitRect m_sliderRect;
    bool    m_sliderDrag;

    static const int MAX_INV = 32;
    HitRect m_invRemoveRects[MAX_INV];
    int     m_invRemoveCount;

    HitRect m_toggleRects[8];
    int     m_toggleCount;

    // Fonts
    HFONT m_fontTitle;
    HFONT m_fontBody;
    HFONT m_fontSmall;
    HFONT m_fontBold;
    HFONT m_fontLogo;
    HFONT m_fontGrid;
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
