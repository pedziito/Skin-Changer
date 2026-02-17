#include <windows.h>
#include <iostream>
#include <fstream>
#include "GameMenu.h"

// Global injector instance
GameInjector* g_Injector = nullptr;

/**
 * License validation
 */
bool ValidateLicense(const std::string& licenseFile) {
    std::ifstream file(licenseFile);
    if (!file.is_open()) {
        return false;
    }

    std::string key;
    if (!std::getline(file, key)) {
        return false;
    }

    // TODO: Implement proper license validation
    // For now, just check if the file exists and has content
    return !key.empty();
}

/**
 * Initialize the injector and hook into game
 */
bool InitializeInjection() {
    try {
        // Validate license
        if (!ValidateLicense("license.key")) {
            MessageBoxA(NULL, "Invalid or missing license key!", "CS2 Skin Changer - License Error", MB_ICONERROR | MB_OK);
            return false;
        }

        // Create injector
        g_Injector = new GameInjector();
        if (!g_Injector->Initialize()) {
            MessageBoxA(NULL, "Failed to initialize menu system!", "CS2 Skin Changer - Error", MB_ICONERROR | MB_OK);
            delete g_Injector;
            g_Injector = nullptr;
            return false;
        }

        // TODO: Hook into CS2 game loop for per-frame updates
        // This would typically be done using a hooking library like detours or a custom hook

        MessageBoxA(NULL, "CS2 Skin Changer loaded successfully!\nPress INS to open menu", "CS2 Skin Changer", MB_ICONINFORMATION | MB_OK);

        return true;
    }
    catch (const std::exception& e) {
        std::string error = std::string("Error: ") + e.what();
        MessageBoxA(NULL, error.c_str(), "CS2 Skin Changer - Exception", MB_ICONERROR | MB_OK);
        return false;
    }
}

/**
 * Cleanup and shutdown
 */
void ShutdownInjection() {
    if (g_Injector) {
        g_Injector->Shutdown();
        delete g_Injector;
        g_Injector = nullptr;
    }
}

/**
 * DLL Entry Point
 */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            {
                // Disable thread notifications for better performance
                DisableThreadLibraryCalls(hModule);

                // Initialize the injection system
                if (!InitializeInjection()) {
                    return FALSE;
                }
            }
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            {
                // Cleanup on process detach
                ShutdownInjection();
            }
            break;
    }

    return TRUE;
}

/**
 * Export this function for the loader to call if needed
 */
extern "C" __declspec(dllexport) void UpdateMenu() {
    if (g_Injector) {
        g_Injector->Update();
    }
}

/**
 * Export for toggling menu visibility
 */
extern "C" __declspec(dllexport) void ToggleMenu() {
    if (g_Injector) {
        g_Injector->GetMenu().ToggleMenu();
    }
}
