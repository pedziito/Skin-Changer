/*
 * AC Skin Changer - DLL Entry Point
 * Injected into cs2.exe via loader. Initializes all modules in order.
 */

#include "core.h"

// Forward declare the main initialization thread
DWORD WINAPI MainThread(LPVOID lpParam);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        G::hModule = hModule;
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
    }
    return TRUE;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    // Setup logging
    char appData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, appData))) {
        G::logPath = std::string(appData) + "\\AC_Changer";
        std::filesystem::create_directories(G::logPath);
        G::logFile.open(G::logPath + "\\ac_changer.log", std::ios::out | std::ios::trunc);
    }

    LogMsg("=== AC Skin Changer Initializing ===");
    LogMsg("Build: %s %s", __DATE__, __TIME__);

    // Wait for client.dll to load (CS2 needs time to initialize)
    LogMsg("Waiting for client.dll...");
    HMODULE hClient = nullptr;
    for (int i = 0; i < 240; i++) { // 120 seconds max
        hClient = GetModuleHandleA("client.dll");
        if (hClient) break;
        Sleep(500);
    }

    if (!hClient) {
        LogMsg("FATAL: client.dll not found after 120s timeout");
        FreeLibraryAndExitThread(G::hModule, 1);
        return 1;
    }

    G::clientBase = (uintptr_t)hClient;
    LogMsg("client.dll base: 0x%p", (void*)G::clientBase);

    // Get engine2.dll
    HMODULE hEngine = GetModuleHandleA("engine2.dll");
    if (hEngine) G::engineBase = (uintptr_t)hEngine;
    LogMsg("engine2.dll base: 0x%p", (void*)G::engineBase);

    // Initialize modules in order
    LogMsg("Initializing Offsets...");
    if (!Offsets::Initialize()) {
        LogMsg("WARNING: Offset initialization had issues, using fallbacks");
    }

    LogMsg("Initializing Netvars...");
    if (!Netvars::Initialize()) {
        LogMsg("WARNING: Netvar initialization had issues, using fallbacks");
    }

    LogMsg("Initializing Skin Database...");
    SkinDB::Initialize();
    LogMsg("Loaded %zu weapons", G::allWeapons.size());

    LogMsg("Initializing DX11 Hooks...");
    if (!Hooks::Initialize()) {
        LogMsg("FATAL: Failed to hook DX11");
        FreeLibraryAndExitThread(G::hModule, 1);
        return 1;
    }

    LogMsg("=== Initialization Complete ===");
    G::initialized = true;

    // Keep-alive loop — watch for unload key (END)
    while (!G::shouldUnload) {
        if (GetAsyncKeyState(VK_END) & 1) {
            G::shouldUnload = true;
        }
        Sleep(100);
    }

    // Cleanup
    LogMsg("Unloading...");
    Hooks::Shutdown();
    Render::Shutdown();

    Sleep(200);
    LogMsg("Goodbye!");
    G::logFile.close();

    FreeLibraryAndExitThread(G::hModule, 0);
    return 0;
}
