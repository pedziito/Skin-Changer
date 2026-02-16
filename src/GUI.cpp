#include "GUI.h"
#include "SkinChanger.h"
#include "SkinDatabase.h"
#include "OffsetManager.h"
#include <CommCtrl.h>
#include <memory>

#pragma comment(lib, "comctl32.lib")

#define IDC_CATEGORY_COMBO 1001
#define IDC_WEAPON_COMBO 1002
#define IDC_SKIN_COMBO 1003
#define IDC_APPLY_BUTTON 1004
#define IDC_RESET_BUTTON 1005
#define IDC_REFRESH_BUTTON 1006
#define IDC_STATUS_LABEL 1007

static std::unique_ptr<SkinChanger> g_skinChanger;

GUI::GUI() 
    : m_hwnd(nullptr),
      m_categoryCombo(nullptr),
      m_weaponCombo(nullptr),
      m_skinCombo(nullptr),
      m_statusLabel(nullptr),
      m_applyButton(nullptr),
      m_resetButton(nullptr),
      m_refreshButton(nullptr) {
}

GUI::~GUI() {
    Shutdown();
}

bool GUI::Initialize(HINSTANCE hInstance) {
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"CS2SkinChangerClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    
    if (!RegisterClassExW(&wc)) {
        return false;
    }
    
    // Create window
    m_hwnd = CreateWindowExW(
        0,
        L"CS2SkinChangerClass",
        L"CS2 Skin Changer - Educational Purpose Only",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        600, 400,
        nullptr,
        nullptr,
        hInstance,
        this
    );
    
    if (!m_hwnd) {
        return false;
    }
    
    CreateControls(m_hwnd);
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    
    // Load configuration files
    if (!OffsetManager::Instance().LoadFromFile("config/offsets.json")) {
        UpdateStatus("Warning: Failed to load offsets.json");
    }
    
    if (!SkinDatabase::Instance().LoadFromFile("config/skins.json")) {
        UpdateStatus("Warning: Failed to load skins.json");
    }
    
    // Initialize skin changer
    g_skinChanger = std::make_unique<SkinChanger>();
    
    return true;
}

void GUI::CreateControls(HWND hwnd) {
    int y = 20;
    int labelWidth = 100;
    int controlWidth = 450;
    int controlHeight = 25;
    int spacing = 40;
    
    // Warning label
    HWND warningLabel = CreateWindowW(
        L"STATIC", L"⚠ WARNING: Using this tool violates Valve's ToS and may result in a permanent VAC ban!",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        20, y, 560, 40,
        hwnd, nullptr, nullptr, nullptr
    );
    
    HFONT hFont = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    SendMessage(warningLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    y += 60;
    
    // Category selection
    CreateWindowW(L"STATIC", L"Category:", WS_VISIBLE | WS_CHILD,
        20, y, labelWidth, controlHeight, hwnd, nullptr, nullptr, nullptr);
    
    m_categoryCombo = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        130, y, controlWidth, 200, hwnd, (HMENU)IDC_CATEGORY_COMBO, nullptr, nullptr);
    
    y += spacing;
    
    // Weapon selection
    CreateWindowW(L"STATIC", L"Weapon:", WS_VISIBLE | WS_CHILD,
        20, y, labelWidth, controlHeight, hwnd, nullptr, nullptr, nullptr);
    
    m_weaponCombo = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        130, y, controlWidth, 200, hwnd, (HMENU)IDC_WEAPON_COMBO, nullptr, nullptr);
    
    y += spacing;
    
    // Skin selection
    CreateWindowW(L"STATIC", L"Skin:", WS_VISIBLE | WS_CHILD,
        20, y, labelWidth, controlHeight, hwnd, nullptr, nullptr, nullptr);
    
    m_skinCombo = CreateWindowW(L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        130, y, controlWidth, 200, hwnd, (HMENU)IDC_SKIN_COMBO, nullptr, nullptr);
    
    y += spacing + 10;
    
    // Buttons
    m_refreshButton = CreateWindowW(L"BUTTON", L"Refresh Process",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        20, y, 150, 30, hwnd, (HMENU)IDC_REFRESH_BUTTON, nullptr, nullptr);
    
    m_applyButton = CreateWindowW(L"BUTTON", L"Apply Skin",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        180, y, 150, 30, hwnd, (HMENU)IDC_APPLY_BUTTON, nullptr, nullptr);
    
    m_resetButton = CreateWindowW(L"BUTTON", L"Reset Skins",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        340, y, 150, 30, hwnd, (HMENU)IDC_RESET_BUTTON, nullptr, nullptr);
    
    y += 50;
    
    // Status label
    m_statusLabel = CreateWindowW(L"STATIC", L"Status: Ready",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, y, 560, 30, hwnd, (HMENU)IDC_STATUS_LABEL, nullptr, nullptr);
    
    // Populate categories
    auto& db = SkinDatabase::Instance();
    for (const auto& [category, weapons] : db.GetCategories()) {
        std::wstring wCategory(category.begin(), category.end());
        SendMessageW(m_categoryCombo, CB_ADDSTRING, 0, (LPARAM)wCategory.c_str());
    }
}

LRESULT CALLBACK GUI::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    GUI* gui = nullptr;
    
    if (uMsg == WM_CREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        gui = reinterpret_cast<GUI*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(gui));
    } else {
        gui = reinterpret_cast<GUI*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (gui) {
        switch (uMsg) {
            case WM_COMMAND: {
                int wmId = LOWORD(wParam);
                int wmEvent = HIWORD(wParam);
                
                switch (wmId) {
                    case IDC_CATEGORY_COMBO:
                        if (wmEvent == CBN_SELCHANGE) {
                            // Update weapon combo
                            int index = SendMessage(gui->m_categoryCombo, CB_GETCURSEL, 0, 0);
                            if (index != CB_ERR) {
                                wchar_t buffer[256];
                                SendMessageW(gui->m_categoryCombo, CB_GETLBTEXT, index, (LPARAM)buffer);
                                std::wstring wCategory(buffer);
                                gui->m_selectedCategory = std::string(wCategory.begin(), wCategory.end());
                                
                                // Clear and populate weapons
                                SendMessage(gui->m_weaponCombo, CB_RESETCONTENT, 0, 0);
                                auto& db = SkinDatabase::Instance();
                                auto& categories = db.GetCategories();
                                auto catIt = categories.find(gui->m_selectedCategory);
                                if (catIt != categories.end()) {
                                    for (const auto& weapon : catIt->second) {
                                        std::wstring wWeapon(weapon.name.begin(), weapon.name.end());
                                        SendMessageW(gui->m_weaponCombo, CB_ADDSTRING, 0, (LPARAM)wWeapon.c_str());
                                    }
                                }
                            }
                        }
                        break;
                        
                    case IDC_WEAPON_COMBO:
                        if (wmEvent == CBN_SELCHANGE) {
                            // Update skin combo
                            int index = SendMessage(gui->m_weaponCombo, CB_GETCURSEL, 0, 0);
                            if (index != CB_ERR) {
                                wchar_t buffer[256];
                                SendMessageW(gui->m_weaponCombo, CB_GETLBTEXT, index, (LPARAM)buffer);
                                std::wstring wWeapon(buffer);
                                gui->m_selectedWeapon = std::string(wWeapon.begin(), wWeapon.end());
                                
                                // Clear and populate skins
                                SendMessage(gui->m_skinCombo, CB_RESETCONTENT, 0, 0);
                                auto& db = SkinDatabase::Instance();
                                const WeaponInfo* weapon = db.FindWeapon(gui->m_selectedCategory, gui->m_selectedWeapon);
                                if (weapon) {
                                    for (const auto& skin : weapon->skins) {
                                        std::wstring wSkin(skin.name.begin(), skin.name.end());
                                        SendMessageW(gui->m_skinCombo, CB_ADDSTRING, 0, (LPARAM)wSkin.c_str());
                                    }
                                }
                            }
                        }
                        break;
                        
                    case IDC_APPLY_BUTTON:
                        gui->OnApplySkin();
                        break;
                        
                    case IDC_RESET_BUTTON:
                        gui->OnResetSkins();
                        break;
                        
                    case IDC_REFRESH_BUTTON:
                        gui->OnRefresh();
                        break;
                }
                break;
            }
            
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void GUI::OnApplySkin() {
    if (!g_skinChanger || !g_skinChanger->IsInitialized()) {
        UpdateStatus("Error: Not connected to CS2. Click 'Refresh Process'");
        return;
    }
    
    int skinIndex = SendMessage(m_skinCombo, CB_GETCURSEL, 0, 0);
    if (skinIndex == CB_ERR) {
        UpdateStatus("Error: Please select a skin");
        return;
    }
    
    wchar_t buffer[256];
    SendMessageW(m_skinCombo, CB_GETLBTEXT, skinIndex, (LPARAM)buffer);
    std::wstring wSkin(buffer);
    m_selectedSkin = std::string(wSkin.begin(), wSkin.end());
    
    auto& db = SkinDatabase::Instance();
    const WeaponInfo* weapon = db.FindWeapon(m_selectedCategory, m_selectedWeapon);
    const SkinInfo* skin = db.FindSkin(m_selectedCategory, m_selectedWeapon, m_selectedSkin);
    
    if (!weapon || !skin) {
        UpdateStatus("Error: Invalid weapon or skin selection");
        return;
    }
    
    if (g_skinChanger->ApplySkin(weapon->id, skin->paintKit, 0.0001f, 0, -1)) {
        UpdateStatus("Success: Skin applied! (May need to switch weapons to see)");
    } else {
        UpdateStatus("Error: " + g_skinChanger->GetLastError());
    }
}

void GUI::OnResetSkins() {
    if (!g_skinChanger || !g_skinChanger->IsInitialized()) {
        UpdateStatus("Error: Not connected to CS2");
        return;
    }
    
    if (g_skinChanger->ResetSkins()) {
        UpdateStatus("Success: Skins reset to default");
    } else {
        UpdateStatus("Error: Failed to reset skins");
    }
}

void GUI::OnRefresh() {
    UpdateStatus("Connecting to CS2...");
    
    if (g_skinChanger->Initialize()) {
        UpdateStatus("Success: Connected to CS2 process");
    } else {
        UpdateStatus("Error: " + g_skinChanger->GetLastError());
    }
}

void GUI::UpdateStatus(const std::string& message) {
    std::wstring wMessage(message.begin(), message.end());
    SetWindowTextW(m_statusLabel, wMessage.c_str());
}

int GUI::Run() {
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

void GUI::Shutdown() {
    if (g_skinChanger) {
        g_skinChanger->Shutdown();
        g_skinChanger.reset();
    }
}
