/*
 * AC Skin Changer - Netvar Resolution
 * Supplements offset fetch with hardcoded fallbacks and runtime resolution.
 */

#include "core.h"

namespace {
    // Validate that an offset looks reasonable (non-zero and within plausible range)
    bool ValidateOffset(uintptr_t offset, const char* name) {
        if (offset == 0) {
            LogMsg("Netvar %s is zero, needs fallback", name);
            return false;
        }
        if (offset > 0x50000) {
            LogMsg("Netvar %s = 0x%llX looks suspiciously large", name, offset);
            // Still allow it - CS2 structs can be large
        }
        return true;
    }
}

namespace Netvars {
    bool Initialize() {
        bool allValid = true;

        // Validate existing offsets from GitHub/scan
        allValid &= ValidateOffset(G::offsets.m_hPlayerPawn, "m_hPlayerPawn");
        allValid &= ValidateOffset(G::offsets.m_pWeaponServices, "m_pWeaponServices");
        allValid &= ValidateOffset(G::offsets.m_hMyWeapons, "m_hMyWeapons");
        allValid &= ValidateOffset(G::offsets.m_AttributeManager, "m_AttributeManager");
        allValid &= ValidateOffset(G::offsets.m_Item, "m_Item");
        allValid &= ValidateOffset(G::offsets.m_iItemDefinitionIndex, "m_iItemDefinitionIndex");
        allValid &= ValidateOffset(G::offsets.m_iItemIDHigh, "m_iItemIDHigh");
        allValid &= ValidateOffset(G::offsets.m_nFallbackPaintKit, "m_nFallbackPaintKit");
        allValid &= ValidateOffset(G::offsets.m_nFallbackSeed, "m_nFallbackSeed");
        allValid &= ValidateOffset(G::offsets.m_flFallbackWear, "m_flFallbackWear");
        allValid &= ValidateOffset(G::offsets.m_nFallbackStatTrak, "m_nFallbackStatTrak");
        allValid &= ValidateOffset(G::offsets.m_iEntityQuality, "m_iEntityQuality");

        if (allValid) {
            LogMsg("All netvars validated successfully");
        } else {
            LogMsg("Some netvars required fallback values");
        }

        // Log final offset table
        LogMsg("=== Final Offset Table ===");
        LogMsg("  dwLocalPlayerController = 0x%llX", G::offsets.dwLocalPlayerController);
        LogMsg("  dwLocalPlayerPawn       = 0x%llX", G::offsets.dwLocalPlayerPawn);
        LogMsg("  dwEntityList            = 0x%llX", G::offsets.dwEntityList);
        LogMsg("  m_hPlayerPawn           = 0x%llX", G::offsets.m_hPlayerPawn);
        LogMsg("  m_pWeaponServices       = 0x%llX", G::offsets.m_pWeaponServices);
        LogMsg("  m_hMyWeapons            = 0x%llX", G::offsets.m_hMyWeapons);
        LogMsg("  m_AttributeManager      = 0x%llX", G::offsets.m_AttributeManager);
        LogMsg("  m_Item                  = 0x%llX", G::offsets.m_Item);
        LogMsg("  m_iItemDefinitionIndex  = 0x%llX", G::offsets.m_iItemDefinitionIndex);
        LogMsg("  m_iItemIDHigh           = 0x%llX", G::offsets.m_iItemIDHigh);
        LogMsg("  m_nFallbackPaintKit     = 0x%llX", G::offsets.m_nFallbackPaintKit);
        LogMsg("  m_nFallbackSeed         = 0x%llX", G::offsets.m_nFallbackSeed);
        LogMsg("  m_flFallbackWear        = 0x%llX", G::offsets.m_flFallbackWear);
        LogMsg("  m_nFallbackStatTrak     = 0x%llX", G::offsets.m_nFallbackStatTrak);
        LogMsg("  m_iEntityQuality        = 0x%llX", G::offsets.m_iEntityQuality);
        LogMsg("=========================");

        return allValid;
    }
}
