#include "GameMenu.h"
#include <iostream>
#include <windows.h>
#include <vector>
#include <string>

/**
 * GameMenu Implementation - Simple Menu System
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
    AddSkin("M4A4", "Asiimov");

    return true;
}

void GameMenu::Shutdown() {
    std::cout << "[+] Game Menu shutdown\n";
}

void GameMenu::Update() {
    // Menu update logic (called each frame)
    // Check for INSERT key to toggle menu visibility
}

void GameMenu::Render() {
    if (!m_state.isVisible) {
        return;
    }

    // Simple text-based menu rendering
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║    CS2 INVENTORY CHANGER MENU          ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
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

void GameMenu::HandleKeyInput(int key) {
    // Handle menu navigation
}

void GameMenu::ToggleMenu() {
    m_state.isVisible = !m_state.isVisible;
}

void GameMenu::RenderMainMenu() {
    // Main menu rendering
}

void GameMenu::RenderSkinsMenu() {
    // Skins menu rendering
}

void GameMenu::RenderSettingsMenu() {
    // Settings menu rendering
}

void GameMenu::RenderAboutMenu() {
    // About menu rendering
}

void GameMenu::RenderVACWarning() {
    // VAC warning display
    m_showVACWarning = false;
}

/**
 * GameInjector Implementation
 */

GameInjector::GameInjector()
    : m_initialized(false), m_menu(std::make_unique<GameMenu>()) {
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
