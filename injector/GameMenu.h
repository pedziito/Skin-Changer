#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

// ============================================================================
//  CS2 In-Game Overlay  –  NEVERLOSE Style
//  INSERT toggles (only when CS2 is foreground)
//  Sidebar: Inventory Changer + Configs
//  Grid skin browser → float slider → equip to CS2 inventory
//  Configs = save / load / delete inventory presets
// ============================================================================

// ── NEVERLOSE Exact Color Palette ──
#define NL_BG           RGB(13, 13, 17)
#define NL_BG_DARK      RGB(10, 10, 14)
#define NL_SIDEBAR      RGB(17, 17, 22)
#define NL_SIDEBAR_BR   RGB(30, 30, 38)
#define NL_HEADER       RGB(15, 15, 20)
#define NL_CARD         RGB(22, 22, 29)
#define NL_CARD_HOV     RGB(28, 28, 37)
#define NL_CARD_SEL     RGB(25, 35, 55)
#define NL_CARD_BR      RGB(35, 35, 45)
#define NL_ACCENT       RGB(59, 130, 246)
#define NL_ACCENT_L     RGB(96, 165, 250)
#define NL_ACCENT_D     RGB(37, 99, 235)
#define NL_TEXT          RGB(210, 210, 225)
#define NL_TEXT2         RGB(130, 130, 155)
#define NL_TEXT3         RGB(75, 75, 95)
#define NL_BORDER       RGB(35, 35, 45)
#define NL_WHITE        RGB(235, 235, 245)
#define NL_GREEN        RGB(34, 197, 94)
#define NL_RED          RGB(239, 68, 68)
#define NL_ORANGE       RGB(245, 158, 11)
#define NL_PURPLE       RGB(168, 85, 247)
#define NL_PINK         RGB(236, 72, 153)
#define NL_TOGGLE_ON    RGB(59, 130, 246)
#define NL_TOGGLE_OFF   RGB(55, 55, 65)

// Rarity tiers (for skin card top-bars)
#define NL_RARITY_COVERT     RGB(235, 75, 75)
#define NL_RARITY_CLASSIFIED RGB(211, 44, 230)
#define NL_RARITY_RESTRICTED RGB(136, 71, 255)
#define NL_RARITY_MILSPEC    RGB(75, 105, 255)
#define NL_RARITY_INDUSTRIAL RGB(94, 152, 217)
#define NL_RARITY_CONSUMER   RGB(176, 195, 217)

// ── Menu Layout ──
#define MENU_W          1100
#define MENU_H          700
#define SIDE_W          200
#define HDR_H           52
#define PAD             16
#define RAD             10

// ── Grid Cards ──
#define GCOLS           4
#define GCARDH          120
#define GGAP            10

// ============================================================================
class GameMenu {
public:
    enum Page    { PAGE_INVENTORY = 0, PAGE_CONFIGS, PAGE_COUNT };
    enum InvView { INV_EQUIPPED, INV_WEAPONS, INV_SKINS, INV_DETAIL };

    struct SkinEntry {
        std::string name;
        int         paintKit;
    };

    struct WeaponDef {
        std::string              name;
        int                      weaponId;
        std::string              category;
        std::vector<SkinEntry>   skins;
    };

    struct EquippedItem {
        std::string weapon;
        std::string skin;
        int         weaponId;
        int         paintKit;
        float       floatVal;
    };

    struct PresetInfo {
        std::string filename;
        std::string displayName;
        int         itemCount;
    };

    GameMenu();
    ~GameMenu();

    bool Initialize(HMODULE hModule);
    void Shutdown();

    void OnMouseMove(int x, int y);
    void OnMouseClick(int x, int y);
    void OnMouseUp();
    void OnMouseWheel(int delta);

    void Render(HDC hdc, int winW, int winH);
    void Toggle();
    bool IsVisible() const { return m_visible; }

    const std::vector<EquippedItem>& GetEquipped() const { return m_equipped; }

    // Legacy stubs
    void ProcessInput() {}
    void AddWeapon(const std::string&) {}
    void AddSkin(const std::string&, const std::string&) {}

private:
    struct Rect { int x, y, w, h; };
    bool Hit(const Rect& r, int mx, int my) const {
        return mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
    }

    void RRect(HDC,int,int,int,int,int,COLORREF,COLORREF);
    void Txt(HDC,const char*,int,int,int,int,COLORREF,HFONT,
             UINT fmt=DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    void Btn(HDC,int,int,int,int,const char*,bool,COLORREF,COLORREF);

    void DrawSidebar (HDC,int,int,int);
    void DrawHeader  (HDC,int,int,int);
    void DrawEquipped(HDC,int,int,int,int);
    void DrawWeapons (HDC,int,int,int,int);
    void DrawSkins   (HDC,int,int,int,int);
    void DrawDetail  (HDC,int,int,int,int);
    void DrawConfigs (HDC,int,int,int,int);

    void BuildDatabase();
    void ScanPresets();
    void SavePreset();
    void LoadPreset(int);
    void DeletePreset(int);

    bool    m_visible;
    Page    m_page;
    InvView m_view;
    int     m_scroll;
    int     m_mouseX, m_mouseY;
    int     m_menuX,  m_menuY;

    std::vector<WeaponDef>   m_weapons;
    int   m_selWeapon, m_selSkin;
    float m_detailFloat;

    std::vector<EquippedItem> m_equipped;
    std::vector<PresetInfo>   m_presets;

    static const int MG = 128;
    Rect m_gR[MG]; int m_gId[MG]; int m_gN;

    Rect m_sideR[PAGE_COUNT];
    Rect m_addBtn, m_backBtn, m_applyBtn;
    Rect m_sliderR; bool m_sliderDrag;

    static const int MI = 64;
    Rect m_remR[MI]; int m_remN;

    static const int MP = 32;
    Rect m_pLoadR[MP]; Rect m_pDelR[MP]; int m_pN;
    Rect m_saveBtn, m_applyGameBtn;

    HFONT m_fTitle, m_fBody, m_fSmall, m_fBold, m_fLogo, m_fGrid, m_fIcon;
};

// ============================================================================
class OverlayWindow {
public:
    OverlayWindow();
    ~OverlayWindow();

    bool Create(HMODULE hModule);
    bool Create(HMODULE hModule, DWORD cs2Pid);
    void Destroy();
    void RunFrame();
    void ShowMenu();

    GameMenu& GetMenu() { return m_menu; }
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

private:
    void UpdatePosition();

    HWND     m_hwnd;
    HWND     m_gameHwnd;
    DWORD    m_cs2Pid;
    GameMenu m_menu;
    HMODULE  m_hModule;
    bool     m_running;
    bool     m_autoShow;
};

// ============================================================================
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
