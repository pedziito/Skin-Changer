/*
 * AC Skin Changer - CS2 Inventory Manipulation
 * Writes skin data directly to CS2 weapon entities in memory.
 * Uses the fallback skin system: setting m_iItemIDHigh = -1 forces CS2
 * to use m_nFallbackPaintKit, m_nFallbackSeed, m_flFallbackWear, m_nFallbackStatTrak.
 * This makes skins appear in the player's inventory as shown in-game.
 */

#include "core.h"

namespace {
    // Safe memory read
    template<typename T>
    T ReadMem(uintptr_t addr) {
        T value{};
        __try {
            value = *reinterpret_cast<T*>(addr);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            value = T{};
        }
        return value;
    }

    // Safe memory write
    template<typename T>
    bool WriteMem(uintptr_t addr, T value) {
        __try {
            *reinterpret_cast<T*>(addr) = value;
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    // Check if a weapon ID is a knife
    bool IsKnife(int weaponId) {
        return weaponId >= 500 && weaponId < 600;
    }

    // Check if a weapon ID is a glove
    bool IsGlove(int weaponId) {
        return weaponId >= 4725;
    }

    // Get entity from entity list by handle
    uintptr_t GetEntityFromHandle(uint32_t handle) {
        if (!G::offsets.dwEntityList || !G::clientBase) return 0;

        uintptr_t entityList = ReadMem<uintptr_t>(G::clientBase + G::offsets.dwEntityList);
        if (!entityList) return 0;

        // CS2 entity list: list -> entry[handle >> 9] -> entity[handle & 0x1FF]
        uint32_t listIdx = (handle >> 9) & 0x3F;
        uintptr_t listEntry = ReadMem<uintptr_t>(entityList + 0x8 * (listIdx + 1));
        if (!listEntry) return 0;

        uint32_t entIdx = handle & 0x1FF;
        return ReadMem<uintptr_t>(listEntry + 0x78 * entIdx);
    }
}

namespace Inventory {
    void ApplySkins() {
        if (!G::initialized || !G::clientBase) return;

        std::lock_guard<std::mutex> lock(G::skinMutex);
        if (G::equippedSkins.empty()) return;

        // Get local player controller
        uintptr_t localController = ReadMem<uintptr_t>(G::clientBase + G::offsets.dwLocalPlayerController);
        if (!localController) return;

        // Get local player pawn
        uint32_t pawnHandle = ReadMem<uint32_t>(localController + G::offsets.m_hPlayerPawn);
        uintptr_t localPawn = GetEntityFromHandle(pawnHandle);
        if (!localPawn) return;

        // Get weapon services
        uintptr_t weaponServices = ReadMem<uintptr_t>(localPawn + G::offsets.m_pWeaponServices);
        if (!weaponServices) return;

        // Get weapon list (CUtlVector<CHandle>)
        uintptr_t weaponListBase = weaponServices + G::offsets.m_hMyWeapons;
        uintptr_t weaponListData = ReadMem<uintptr_t>(weaponListBase);
        int weaponCount = ReadMem<int>(weaponListBase + 0x8);

        if (!weaponListData || weaponCount <= 0 || weaponCount > 64) return;

        // Iterate through all weapons
        for (int i = 0; i < weaponCount; i++) {
            uint32_t weaponHandle = ReadMem<uint32_t>(weaponListData + i * 0x4);
            if (!weaponHandle || weaponHandle == 0xFFFFFFFF) continue;

            uintptr_t weapon = GetEntityFromHandle(weaponHandle);
            if (!weapon) continue;

            // Read weapon definition index
            int weaponId = ReadMem<short>(
                weapon + G::offsets.m_AttributeManager
                       + G::offsets.m_Item
                       + G::offsets.m_iItemDefinitionIndex
            );

            // For default knives (42/59), check if we have a knife skin configured
            int lookupId = weaponId;
            if (weaponId == 42 || weaponId == 59) {
                // Look for any knife config
                for (auto& [id, cfg] : G::equippedSkins) {
                    if (IsKnife(id) && cfg.enabled) {
                        lookupId = id;
                        break;
                    }
                }
            }

            // Check if we have a skin config for this weapon
            auto it = G::equippedSkins.find(lookupId);
            if (it == G::equippedSkins.end() || !it->second.enabled) continue;

            const SkinConfig& config = it->second;

            // === APPLY SKIN ===

            // Step 1: Set item ID high to -1 (forces fallback mode)
            WriteMem<int>(
                weapon + G::offsets.m_AttributeManager
                       + G::offsets.m_Item
                       + G::offsets.m_iItemIDHigh,
                -1
            );

            // Step 2: Write fallback paint kit
            WriteMem<int>(weapon + G::offsets.m_nFallbackPaintKit, config.paintKit);

            // Step 3: Write fallback seed
            WriteMem<int>(weapon + G::offsets.m_nFallbackSeed, config.seed);

            // Step 4: Write fallback wear
            WriteMem<float>(weapon + G::offsets.m_flFallbackWear, config.wear);

            // Step 5: Write StatTrak value
            WriteMem<int>(weapon + G::offsets.m_nFallbackStatTrak, config.statTrak);

            // Step 6: For knives - change the definition index and set quality to ★(3)
            if (IsKnife(config.weaponId) && (weaponId == 42 || weaponId == 59)) {
                // Change definition index to the custom knife
                WriteMem<short>(
                    weapon + G::offsets.m_AttributeManager
                           + G::offsets.m_Item
                           + G::offsets.m_iItemDefinitionIndex,
                    (short)config.weaponId
                );

                // Set entity quality to 3 (★ star quality)
                WriteMem<int>(
                    weapon + G::offsets.m_AttributeManager
                           + G::offsets.m_Item
                           + G::offsets.m_iEntityQuality,
                    3
                );
            }

            // Step 7: For gloves - similar to knives
            if (IsGlove(config.weaponId)) {
                WriteMem<short>(
                    weapon + G::offsets.m_AttributeManager
                           + G::offsets.m_Item
                           + G::offsets.m_iItemDefinitionIndex,
                    (short)config.weaponId
                );

                // Gloves use quality 4
                WriteMem<int>(
                    weapon + G::offsets.m_AttributeManager
                           + G::offsets.m_Item
                           + G::offsets.m_iEntityQuality,
                    4
                );
            }
        }
    }
}
