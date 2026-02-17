#include <windows.h>
#include <iostream>
#include <fstream>
#include "GameMenu.h"

// Global injector instance
GameInjector* g_Injector = nullptr;
HMODULE       g_hModule  = nullptr;

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

    return !key.empty();
}

/**
 * Main overlay thread – runs the menu loop
 */
DWORD WINAPI OverlayThread(LPVOID) {
    // Wait a bit for the game to finish loading
    Sleep(3000);

    g_Injector = new GameInjector();
    if (!g_Injector->Initialize(g_hModule)) {
        delete g_Injector;
        g_Injector = nullptr;
        return 1;
    }

    // Main loop – runs until the DLL is unloaded
    while (true) {
        g_Injector->Update();
        Sleep(16); // ~60 fps
    }

    return 0;
}

/**
 * DLL Entry Point
 */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            {
                DisableThreadLibraryCalls(hModule);
                g_hModule = hModule;

                // Validate license (optional – skip if file is missing)
                ValidateLicense("license.key");

                // Start overlay on a separate thread so we don't block DllMain
                HANDLE hThread = CreateThread(nullptr, 0, OverlayThread, nullptr, 0, nullptr);
                if (hThread) CloseHandle(hThread);
            }
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            {
                if (g_Injector) {
                    g_Injector->Shutdown();
                    delete g_Injector;
                    g_Injector = nullptr;
                }
            }
            break;
    }

    return TRUE;
}

/**
 * Export for external menu update calls
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
        g_Injector->GetMenu().Toggle();
    }
}
