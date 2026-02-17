/*
 * AC Skin Changer - Core Header
 * Master header with CS2 SDK types, globals, and module declarations.
 * Architecture: Injected DLL → Hook DX11 Present → ImGui overlay + CS2 memory writing
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <TlHelp32.h>
#include <wininet.h>
#include <ShlObj.h>

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <mutex>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <functional>
#include <algorithm>
#include <chrono>
#include <thread>
#include <atomic>
#include <cmath>
#include <memory>

// ============================================================================
// CS2 ITEM DEFINITION INDEX ENUM
// ============================================================================
enum class EItemDefinitionIndex : int {
    WEAPON_DEAGLE = 1,
    WEAPON_ELITE = 2,
    WEAPON_FIVESEVEN = 3,
    WEAPON_GLOCK = 4,
    WEAPON_AK47 = 7,
    WEAPON_AUG = 8,
    WEAPON_AWP = 9,
    WEAPON_FAMAS = 10,
    WEAPON_G3SG1 = 11,
    WEAPON_GALILAR = 13,
    WEAPON_M249 = 14,
    WEAPON_M4A4 = 16,
    WEAPON_MAC10 = 17,
    WEAPON_P90 = 19,
    WEAPON_MP5SD = 23,
    WEAPON_UMP45 = 24,
    WEAPON_XM1014 = 25,
    WEAPON_BIZON = 26,
    WEAPON_MAG7 = 27,
    WEAPON_NEGEV = 28,
    WEAPON_SAWEDOFF = 29,
    WEAPON_TEC9 = 30,
    WEAPON_TASER = 31,
    WEAPON_HKP2000 = 32,
    WEAPON_MP7 = 33,
    WEAPON_MP9 = 34,
    WEAPON_NOVA = 35,
    WEAPON_P250 = 36,
    WEAPON_SCAR20 = 38,
    WEAPON_SG556 = 39,
    WEAPON_SSG08 = 40,
    WEAPON_KNIFE_CT = 42,
    WEAPON_KNIFE_T = 59,
    WEAPON_M4A1S = 60,
    WEAPON_USPS = 61,
    WEAPON_CZ75A = 63,
    WEAPON_REVOLVER = 64,

    // Knives
    KNIFE_BAYONET = 500,
    KNIFE_CSS = 503,
    KNIFE_FLIP = 505,
    KNIFE_GUT = 506,
    KNIFE_KARAMBIT = 507,
    KNIFE_M9_BAYONET = 508,
    KNIFE_HUNTSMAN = 509,
    KNIFE_FALCHION = 512,
    KNIFE_BOWIE = 514,
    KNIFE_BUTTERFLY = 515,
    KNIFE_SHADOW_DAGGERS = 516,
    KNIFE_PARACORD = 517,
    KNIFE_SURVIVAL = 518,
    KNIFE_URSUS = 519,
    KNIFE_NAVAJA = 520,
    KNIFE_NOMAD = 521,
    KNIFE_STILETTO = 522,
    KNIFE_TALON = 523,
    KNIFE_SKELETON = 525,
    KNIFE_KUKRI = 526,

    // Gloves
    GLOVE_STUDDED_BROKENFANG = 4725,
    GLOVE_STUDDED_BLOODHOUND = 5027,
    GLOVE_T = 5028,
    GLOVE_CT = 5029,
    GLOVE_SPORTY = 5030,
    GLOVE_SLICK = 5031,
    GLOVE_LEATHER_HANDWRAPS = 5032,
    GLOVE_MOTORCYCLE = 5033,
    GLOVE_SPECIALIST = 5034,
    GLOVE_STUDDED_HYDRA = 5035,
};

// ============================================================================
// SKIN CONFIGURATION STRUCTURES
// ============================================================================
struct SkinConfig {
    int weaponId = 0;
    int paintKit = 0;
    int seed = 0;
    float wear = 0.0001f;
    int statTrak = -1;
    std::string weaponName;
    std::string skinName;
    bool enabled = true;
};

struct SkinInfo {
    std::string name;
    int paintKit;
    int rarity; // 1=Consumer, 2=Industrial, 3=Mil-Spec, 4=Restricted, 5=Classified, 6=Covert, 7=Contraband
};

struct WeaponInfo {
    std::string name;
    int id;
    std::string category;
    std::vector<SkinInfo> skins;
};

// ============================================================================
// CS2 OFFSET STRUCTURE
// ============================================================================
struct CS2Offsets {
    // Client offsets
    uintptr_t dwLocalPlayerController = 0;
    uintptr_t dwLocalPlayerPawn = 0;
    uintptr_t dwEntityList = 0;
    uintptr_t dwViewMatrix = 0;

    // Controller netvars
    uintptr_t m_hPlayerPawn = 0;

    // Pawn netvars
    uintptr_t m_pWeaponServices = 0;
    uintptr_t m_hMyWeapons = 0;

    // Weapon netvars
    uintptr_t m_AttributeManager = 0;
    uintptr_t m_Item = 0;
    uintptr_t m_iItemDefinitionIndex = 0;
    uintptr_t m_iItemIDHigh = 0;
    uintptr_t m_nFallbackPaintKit = 0;
    uintptr_t m_nFallbackSeed = 0;
    uintptr_t m_flFallbackWear = 0;
    uintptr_t m_nFallbackStatTrak = 0;
    uintptr_t m_iEntityQuality = 0;
    uintptr_t m_szCustomName = 0;
    uintptr_t m_OriginalOwnerXuidLow = 0;
    uintptr_t m_OriginalOwnerXuidHigh = 0;
    uintptr_t m_nModelIndex = 0;

    // Active weapon
    uintptr_t m_hActiveWeapon = 0;
};

// ============================================================================
// GLOBAL NAMESPACE
// ============================================================================
namespace G {
    // Module bases
    inline uintptr_t clientBase = 0;
    inline uintptr_t engineBase = 0;
    inline HMODULE   hModule = nullptr;

    // Offsets
    inline CS2Offsets offsets{};

    // DX11 objects
    inline ID3D11Device*           pDevice = nullptr;
    inline ID3D11DeviceContext*    pContext = nullptr;
    inline IDXGISwapChain*         pSwapChain = nullptr;
    inline ID3D11RenderTargetView* pRenderTargetView = nullptr;

    // State
    inline bool menuOpen = true;
    inline bool initialized = false;
    inline bool shouldUnload = false;
    inline HWND gameWindow = nullptr;
    inline WNDPROC originalWndProc = nullptr;

    // Skin data
    inline std::unordered_map<int, SkinConfig> equippedSkins;  // weaponId → config
    inline std::mutex skinMutex;

    // Skin database
    inline std::vector<WeaponInfo> allWeapons;
    inline std::map<std::string, std::vector<WeaponInfo>> weaponsByCategory;

    // Logging
    inline std::string logPath;
    inline std::ofstream logFile;
}

// ============================================================================
// LOGGING
// ============================================================================
inline void LogMsg(const char* fmt, ...) {
    if (!G::logFile.is_open()) return;
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    char timeBuf[64];
    struct tm tmLocal;
    localtime_s(&tmLocal, &t);
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &tmLocal);

    G::logFile << "[" << timeBuf << "] " << buf << std::endl;
    G::logFile.flush();
}

// ============================================================================
// MODULE DECLARATIONS
// ============================================================================
namespace Offsets {
    bool Initialize();
}

namespace Netvars {
    bool Initialize();
}

namespace Hooks {
    bool Initialize();
    void Shutdown();
}

namespace Inventory {
    void ApplySkins();
}

namespace Render {
    void Initialize();
    void Shutdown();
}

namespace Menu {
    void Render();
    void SetupStyle();
}

namespace Config {
    void Save(const std::string& presetName);
    void Load(const std::string& presetName);
    void Delete(const std::string& presetName);
    std::vector<std::string> ListPresets();
    std::string GetConfigDir();
}

namespace SkinDB {
    void Initialize();
}
