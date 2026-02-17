#include "GameMenu.h"
#include <iostream>
#include <windows.h>
#include <thread>

// Global menu instance
static GameMenu* g_pGameMenu = nullptr;
static bool g_bInsertPressed = false;
static bool g_bInsertWasDown = false;

// Keyboard hook callback
LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* pKeyInfo = (KBDLLHOOKSTRUCT*)lParam;
        
        // VK_INSERT = 0x2D
        if (pKeyInfo->vkCode == VK_INSERT) {
            if (wParam == WM_KEYDOWN && !g_bInsertWasDown) {
                g_bInsertPressed = true;
                g_bInsertWasDown = true;
            } else if (wParam == WM_KEYUP) {
                g_bInsertWasDown = false;
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

/**
 * GameMenu Implementation
 */

GameMenu::GameMenu()
    : m_showVACWarning(true) {
    m_state.isVisible = false;
    m_state.currentMenu = MENU_SKINS;
    m_state.selectedOption = 0;
}

GameMenu::~GameMenu() {
}

bool GameMenu::Initialize() {
    std::cout << "[+] Game Menu initialized\n";
    
    // Setup keyboard hook for INSERT key detection
    SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, GetModuleHandle(NULL), 0);

    // Add default weapons
    AddWeapon("AK-47");
    AddWeapon("AWP Dragon Lore");
    AddWeapon("M4A4");
    AddWeapon("M4A1-S");
    AddWeapon("USP-S");
    AddWeapon("Glock-18");

    // Add sample skins
    AddSkin("AK-47", "Dragon Lore");
    AddSkin("AK-47", "Howl");
    AddSkin("AK-47", "Point Disarray");
    AddSkin("AWP Dragon Lore", "Factory New");
    AddSkin("M4A4", "Asiimov");
    AddSkin("M4A4", "Howl");

    g_pGameMenu = this;
    return true;
}

void GameMenu::Shutdown() {
    std::cout << "[+] Game Menu shutdown\n";
}

void GameMenu::Update() {
    // Check if INSERT key was pressed
    if (g_bInsertPressed) {
        m_state.isVisible = !m_state.isVisible;
        g_bInsertPressed = false;
        
        if (m_state.isVisible) {
            std::cout << "[+] Menu opened\n";
            PlayNotificationSound();
        } else {
            std::cout << "[+] Menu closed\n";
        }
    }

    if (!m_state.isVisible) return;

    // Handle menu navigation
    if (GetAsyncKeyState(VK_UP) & 0x8000) {
        if (m_state.selectedOption > 0) m_state.selectedOption--;
    }
    if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
        if (m_state.selectedOption < 8) m_state.selectedOption++;
    }
    if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
        ApplySelectedSkin();
    }
}

void GameMenu::Render() {
    if (!m_state.isVisible) return;

    // Simple text-based menu for now
    // In real implementation, this would render with DirectX/ImGui

    system("cls");
    
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════╗\n";
    std::cout << "║        CS2 INVENTORY CHANGER MENU         ║\n";
    std::cout << "╚════════════════════════════════════════════╝\n\n";
    
    std::cout << "WEAPONS & SKINS:\n";
    for (size_t i = 0; i < m_weapons.size(); i++) {
        if (i == m_state.selectedOption) {
            std::cout << "  ► " << m_weapons[i] << "\n";
        } else {
            std::cout << "    " << m_weapons[i] << "\n";
        }
    }

    std::cout << "\n";
    std::cout << "  [↑↓] Navigate  [ENTER] Apply  [INS] Close\n";
    std::cout << "\n";
}

void GameMenu::PlayNotificationSound() {
    Beep(800, 100);
}

void GameMenu::ApplySelectedSkin() {
    if (m_state.selectedOption < m_weapons.size()) {
        std::cout << "[+] Applied: " << m_weapons[m_state.selectedOption] << "\n";
    }
}


void GameMenu::Render() {
    if (!m_state.isVisible) {
        return;
    }

    // TODO: Use DirectX/ImGui to render overlay
    switch (m_state.currentMenu) {
        case MENU_SKINS:
            RenderSkinsMenu();
            break;
        case MENU_SETTINGS:
            RenderSettingsMenu();
            break;
        case MENU_ABOUT:
            RenderAboutMenu();
            break;
        case MENU_EXIT:
            RenderMainMenu();
            break;
        default:
            RenderMainMenu();
    }
}

void GameMenu::HandleKeyInput(int key) {
    // TODO: Implement menu navigation with keyboard
    // UP/DOWN/LEFT/RIGHT arrows for navigation
    // ENTER to select
    // ESC to go back/close menu
}

void GameMenu::ToggleMenu() {
    m_state.isVisible = !m_state.isVisible;

    if (m_state.isVisible && m_showVACWarning) {
        RenderVACWarning();
    }
}

void GameMenu::AddWeapon(const std::string& weaponName) {
    if (m_weaponSkins.find(weaponName) == m_weaponSkins.end()) {
        m_weaponSkins[weaponName] = std::vector<std::string>();
    }
}

void GameMenu::AddSkin(const std::string& weaponName, const std::string& skinName) {
    if (m_weaponSkins.find(weaponName) != m_weaponSkins.end()) {
        m_weaponSkins[weaponName].push_back(skinName);
    } else {
        AddWeapon(weaponName);
        AddSkin(weaponName, skinName);
    }
}

void GameMenu::RenderMainMenu() {
    // Main menu structure (text-based for now, will be ImGui overlay)
    /*
    ┌─────────────────────────────────┐
    │  CS2 SKIN CHANGER - MAIN MENU   │
    ├─────────────────────────────────┤
    │  [ ] Change Skins               │
    │  [ ] Settings                   │
    │  [ ] About                      │
    │  [ ] Exit                       │
    └─────────────────────────────────┘
    */
}

void GameMenu::RenderSkinsMenu() {
    // Skins menu - show weapons and available skins
    /*
    ┌─────────────────────────────────┐
    │  SELECT WEAPON                  │
    ├─────────────────────────────────┤
    │  > AK-47                        │
    │    AWP Dragon Lore              │
    │    M4A4                         │
    │    M4A1-S                       │
    │    USP-S                        │
    │    Glock-18                     │
    └─────────────────────────────────┘
    */
}

void GameMenu::RenderSettingsMenu() {
    // Settings menu
    /*
    ┌─────────────────────────────────┐
    │  SETTINGS                       │
    ├─────────────────────────────────┤
    │  [ ] Enable Skins               │
    │  [ ] Show VAC Warning           │
    │  [ ] Hide Menu Key: INS         │
    │      Back                       │
    └─────────────────────────────────┘
    */
}

void GameMenu::RenderAboutMenu() {
    // About menu
    /*
    ┌─────────────────────────────────┐
    │  ABOUT                          │
    ├─────────────────────────────────┤
    │  CS2 Skin Changer v1.0          │
    │  Author: pedziito               │
    │  License Required: YES          │
    │                                 │
    │  Press INS to toggle menu       │
    │      Back                       │
    └─────────────────────────────────┘
    */
}

void GameMenu::RenderVACWarning() {
    // VAC warning display
    /*
    ╔═════════════════════════════════╗
    ║      ⚠️  VAC WARNING  ⚠️        ║
    ╠═════════════════════════════════╣
    ║ This tool modifies game.        ║
    ║ VAC or EAC may ban you!         ║
    ║ Use at your own risk.           ║
    ║                                 ║
    ║ Do NOT use main account.        ║
    ║                                 ║
    ║        [UNDERSTAND]              ║
    ╚═════════════════════════════════╝
    */
    m_showVACWarning = false;
}

/**
 * GameInjector Implementation
 */

GameInjector::GameInjector()
    : m_initialized(false) {
    m_menu = std::make_unique<GameMenu>();
}

GameInjector::~GameInjector() {
    if (m_initialized) {
        Shutdown();
    }
}

bool GameInjector::Initialize() {
    if (!m_menu->Initialize()) {
        return false;
    }

    m_initialized = true;
    return true;
}

void GameInjector::Shutdown() {
    if (m_menu) {
        m_menu->Shutdown();
    }
    m_initialized = false;
}

void GameInjector::Update() {
    if (!m_initialized) {
        return;
    }

    m_menu->Update();
    m_menu->Render();
}
