#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

/**
 * In-Game Menu System for CS2 Skin Changer
 */
class GameMenu {
public:
    enum MenuItem {
        MENU_SKINS,
        MENU_SETTINGS,
        MENU_ABOUT,
        MENU_EXIT
    };

    struct MenuState {
        bool isVisible;
        MenuItem currentMenu;
        int selectedOption;
        std::string selectedWeapon;
        std::string selectedSkin;
    };

    GameMenu();
    ~GameMenu();

    // Menu lifecycle
    bool Initialize();
    void Shutdown();
    void Update();
    void Render();

    // Input handling
    void HandleKeyInput(int key);
    void ToggleMenu();

    // Menu state
    MenuState GetState() const { return m_state; }
    bool IsVisible() const { return m_state.isVisible; }

    // Add skins to menu
    void AddWeapon(const std::string& weaponName);
    void AddSkin(const std::string& weaponName, const std::string& skinName);

private:
    void RenderMainMenu();
    void RenderSkinsMenu();
    void RenderSettingsMenu();
    void RenderAboutMenu();
    void RenderVACWarning();

    MenuState m_state;
    std::map<std::string, std::vector<std::string>> m_weaponSkins;
    bool m_showVACWarning;
};

/**
 * DLL Injection Handler
 */
class GameInjector {
public:
    GameInjector();
    ~GameInjector();

    bool Initialize();
    void Shutdown();
    void Update();

    GameMenu& GetMenu() { return *m_menu; }

private:
    std::unique_ptr<GameMenu> m_menu;
    bool m_initialized;
};
