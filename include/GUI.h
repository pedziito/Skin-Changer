#pragma once

#include <Windows.h>
#include <string>

class GUI {
public:
    GUI();
    ~GUI();
    
    bool Initialize(HINSTANCE hInstance);
    int Run();
    void Shutdown();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void CreateControls(HWND hwnd);
    void OnApplySkin();
    void OnResetSkins();
    void OnRefresh();
    void UpdateStatus(const std::string& message);
    
    HWND m_hwnd;
    HWND m_categoryCombo;
    HWND m_weaponCombo;
    HWND m_skinCombo;
    HWND m_statusLabel;
    HWND m_applyButton;
    HWND m_resetButton;
    HWND m_refreshButton;
    
    std::string m_selectedCategory;
    std::string m_selectedWeapon;
    std::string m_selectedSkin;
};
