#include "GameMenu.h"
#include <iostream>
#include <windows.h>

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
    // TODO: Initialize DirectX/ImGui for overlay rendering
    std::cout << "[+] Game Menu initialized\n";

    // Add default weapons
    AddWeapon("AK-47");
    AddWeapon("AWP Dragon Lore");
    AddWeapon("M4A4");
    AddWeapon("M4A1-S");
    AddWeapon("USP-S");
    AddWeapon("Glock-18");

    // Add sample skins
    AddSkin("AK-47", "Legion of Anubis");
    AddSkin("AK-47", "Phantom Disruptor");
    AddSkin("AK-47", "Point Disarray");
    AddSkin("AWP Dragon Lore", "Original");
    AddSkin("M4A4", "Asiimov");
    AddSkin("M4A4", "Howl");

    return true;
}

void GameMenu::Shutdown() {
    std::cout << "[+] Game Menu shutdown\n";
}

void GameMenu::Update() {
    // Called every frame to update menu state
    // TODO: Implement per-frame updates
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
