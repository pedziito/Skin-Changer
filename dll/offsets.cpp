/*
 * AC Skin Changer - Offset Resolution
 * Downloads offsets from a2x/cs2-dumper GitHub repository.
 * Falls back to pattern scanning + hardcoded values.
 */

#include "core.h"
#include "../vendor/json/json.hpp"

using json = nlohmann::json;

namespace {
    // Download a file from URL using WinINet
    std::string DownloadString(const std::string& url) {
        std::string result;

        HINTERNET hInternet = InternetOpenA("AC_Changer/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
        if (!hInternet) return result;

        HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0,
                                           INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
        if (!hUrl) {
            InternetCloseHandle(hInternet);
            return result;
        }

        char buffer[4096];
        DWORD bytesRead;
        while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            result.append(buffer, bytesRead);
        }

        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return result;
    }

    // Pattern scanning in a module
    uintptr_t PatternScan(uintptr_t moduleBase, size_t moduleSize,
                           const std::vector<int>& pattern) {
        auto base = reinterpret_cast<const uint8_t*>(moduleBase);

        for (size_t i = 0; i < moduleSize - pattern.size(); i++) {
            bool found = true;
            for (size_t j = 0; j < pattern.size(); j++) {
                if (pattern[j] != -1 && base[i + j] != (uint8_t)pattern[j]) {
                    found = false;
                    break;
                }
            }
            if (found) return moduleBase + i;
        }
        return 0;
    }

    // Parse pattern string like "48 8B 05 ?? ?? ?? ??" into vector
    std::vector<int> ParsePattern(const std::string& pattern) {
        std::vector<int> bytes;
        std::istringstream stream(pattern);
        std::string token;
        while (stream >> token) {
            if (token == "??" || token == "?")
                bytes.push_back(-1);
            else
                bytes.push_back(std::stoi(token, nullptr, 16));
        }
        return bytes;
    }

    // Get module size
    size_t GetModuleSize(uintptr_t base) {
        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        return nt->OptionalHeader.SizeOfImage;
    }

    bool FetchFromGitHub() {
        LogMsg("Fetching offsets from a2x/cs2-dumper...");

        // Download offsets.json
        std::string offsetsJson = DownloadString(
            "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/offsets.json"
        );

        // Download client_dll.json (netvars)
        std::string clientJson = DownloadString(
            "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/client_dll.json"
        );

        if (offsetsJson.empty() || clientJson.empty()) {
            LogMsg("Failed to download offset files");
            return false;
        }

        try {
            // Parse offsets.json
            auto offsets = json::parse(offsetsJson);

            auto& clientDll = offsets["client.dll"];
            G::offsets.dwLocalPlayerController = clientDll.value("dwLocalPlayerController", (uintptr_t)0);
            G::offsets.dwLocalPlayerPawn = clientDll.value("dwLocalPlayerPawn", (uintptr_t)0);
            G::offsets.dwEntityList = clientDll.value("dwEntityList", (uintptr_t)0);
            G::offsets.dwViewMatrix = clientDll.value("dwViewMatrix", (uintptr_t)0);

            LogMsg("dwLocalPlayerController: 0x%llX", G::offsets.dwLocalPlayerController);
            LogMsg("dwLocalPlayerPawn: 0x%llX", G::offsets.dwLocalPlayerPawn);
            LogMsg("dwEntityList: 0x%llX", G::offsets.dwEntityList);

            // Parse client_dll.json for netvars
            auto netvars = json::parse(clientJson);

            // Navigate the JSON structure to find netvar offsets
            auto findNetvar = [&](const std::string& className, const std::string& memberName) -> uintptr_t {
                if (netvars.contains(className)) {
                    auto& cls = netvars[className];
                    if (cls.contains("data")) {
                        auto& data = cls["data"];
                        for (auto& [key, val] : data.items()) {
                            if (key == memberName && val.contains("value")) {
                                return val["value"].get<uintptr_t>();
                            }
                        }
                    }
                }
                return 0;
            };

            // CBasePlayerController
            auto hPawn = findNetvar("CBasePlayerController", "m_hPawn");
            if (hPawn) G::offsets.m_hPlayerPawn = hPawn;

            // CPlayer_WeaponServices
            auto weaponServices = findNetvar("CCSPlayerPawnBase", "m_pWeaponServices");
            if (weaponServices) G::offsets.m_pWeaponServices = weaponServices;

            // CPlayer_WeaponServices -> m_hMyWeapons
            auto myWeapons = findNetvar("CPlayer_WeaponServices", "m_hMyWeapons");
            if (myWeapons) G::offsets.m_hMyWeapons = myWeapons;

            // CEconItemView / CAttributeContainer members
            auto attrMgr = findNetvar("C_EconEntity", "m_AttributeManager");
            if (attrMgr) G::offsets.m_AttributeManager = attrMgr;

            auto item = findNetvar("C_AttributeContainer", "m_Item");
            if (item) G::offsets.m_Item = item;

            auto itemDefIdx = findNetvar("CEconItemView", "m_iItemDefinitionIndex");
            if (itemDefIdx) G::offsets.m_iItemDefinitionIndex = itemDefIdx;

            auto itemIdHigh = findNetvar("CEconItemView", "m_iItemIDHigh");
            if (itemIdHigh) G::offsets.m_iItemIDHigh = itemIdHigh;

            // Fallback paint fields
            auto fallbackPK = findNetvar("C_EconEntity", "m_nFallbackPaintKit");
            if (fallbackPK) G::offsets.m_nFallbackPaintKit = fallbackPK;

            auto fallbackSeed = findNetvar("C_EconEntity", "m_nFallbackSeed");
            if (fallbackSeed) G::offsets.m_nFallbackSeed = fallbackSeed;

            auto fallbackWear = findNetvar("C_EconEntity", "m_flFallbackWear");
            if (fallbackWear) G::offsets.m_flFallbackWear = fallbackWear;

            auto fallbackST = findNetvar("C_EconEntity", "m_nFallbackStatTrak");
            if (fallbackST) G::offsets.m_nFallbackStatTrak = fallbackST;

            auto quality = findNetvar("CEconItemView", "m_iEntityQuality");
            if (quality) G::offsets.m_iEntityQuality = quality;

            auto customName = findNetvar("CEconItemView", "m_szCustomName");
            if (customName) G::offsets.m_szCustomName = customName;

            auto ownerLow = findNetvar("C_EconEntity", "m_OriginalOwnerXuidLow");
            if (ownerLow) G::offsets.m_OriginalOwnerXuidLow = ownerLow;

            auto ownerHigh = findNetvar("C_EconEntity", "m_OriginalOwnerXuidHigh");
            if (ownerHigh) G::offsets.m_OriginalOwnerXuidHigh = ownerHigh;

            LogMsg("GitHub offsets loaded successfully");
            return true;

        } catch (const std::exception& e) {
            LogMsg("JSON parse error: %s", e.what());
            return false;
        }
    }

    void ScanSignatures() {
        LogMsg("Scanning for signatures as fallback...");

        if (!G::clientBase) return;
        size_t clientSize = GetModuleSize(G::clientBase);

        // dwLocalPlayerController pattern
        auto pattern = ParsePattern("48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 8B 88");
        uintptr_t result = PatternScan(G::clientBase, clientSize, pattern);
        if (result) {
            int32_t offset = *reinterpret_cast<int32_t*>(result + 3);
            G::offsets.dwLocalPlayerController = (result + 7 + offset) - G::clientBase;
            LogMsg("Pattern scan: dwLocalPlayerController = 0x%llX", G::offsets.dwLocalPlayerController);
        }
    }

    void ApplyFallbackOffsets() {
        // These are fallback values that may need updating with CS2 patches
        // They serve as a last resort if both GitHub fetch and pattern scan fail

        if (!G::offsets.m_hPlayerPawn)           G::offsets.m_hPlayerPawn = 0x7E4;
        if (!G::offsets.m_pWeaponServices)       G::offsets.m_pWeaponServices = 0x12C0;
        if (!G::offsets.m_hMyWeapons)            G::offsets.m_hMyWeapons = 0x48;
        if (!G::offsets.m_AttributeManager)      G::offsets.m_AttributeManager = 0x1100;
        if (!G::offsets.m_Item)                  G::offsets.m_Item = 0x50;
        if (!G::offsets.m_iItemDefinitionIndex)  G::offsets.m_iItemDefinitionIndex = 0x1BA;
        if (!G::offsets.m_iItemIDHigh)           G::offsets.m_iItemIDHigh = 0x1E0;
        if (!G::offsets.m_nFallbackPaintKit)     G::offsets.m_nFallbackPaintKit = 0x31C8;
        if (!G::offsets.m_nFallbackSeed)         G::offsets.m_nFallbackSeed = 0x31CC;
        if (!G::offsets.m_flFallbackWear)        G::offsets.m_flFallbackWear = 0x31D0;
        if (!G::offsets.m_nFallbackStatTrak)     G::offsets.m_nFallbackStatTrak = 0x31D4;
        if (!G::offsets.m_iEntityQuality)        G::offsets.m_iEntityQuality = 0x1BC;
        if (!G::offsets.m_szCustomName)          G::offsets.m_szCustomName = 0x1E8;
        if (!G::offsets.m_hActiveWeapon)         G::offsets.m_hActiveWeapon = 0x60;

        LogMsg("Fallback offsets applied");
    }
}

namespace Offsets {
    bool Initialize() {
        bool success = FetchFromGitHub();

        if (!success) {
            LogMsg("GitHub fetch failed, trying pattern scan...");
            ScanSignatures();
        }

        // Always fill in any missing offsets with fallbacks
        ApplyFallbackOffsets();

        return success;
    }
}
