#include "SkinChanger.h"
#include <thread>
#include <chrono>

SkinChanger::SkinChanger() 
    : m_processManager(std::make_unique<ProcessManager>()),
      m_clientBase(0),
      m_initialized(false) {
}

SkinChanger::~SkinChanger() {
    Shutdown();
}

bool SkinChanger::Initialize() {
    if (m_initialized) {
        return true;
    }
    
    // Try to attach to CS2 process
    if (!m_processManager->AttachToProcess(L"cs2.exe")) {
        m_lastError = "Failed to find CS2 process. Please make sure CS2 is running.";
        return false;
    }
    
    // Get client.dll base address
    m_clientBase = m_processManager->GetModuleBase(L"client.dll");
    if (m_clientBase == 0) {
        m_lastError = "Failed to find client.dll module.";
        return false;
    }
    
    m_initialized = true;
    m_lastError.clear();
    return true;
}

void SkinChanger::Shutdown() {
    if (m_processManager) {
        m_processManager->Detach();
    }
    m_initialized = false;
}

uintptr_t SkinChanger::GetLocalPlayer() {
    auto& offsetMgr = OffsetManager::Instance();
    uintptr_t localPlayerOffset = offsetMgr.GetOffset("dwLocalPlayerPawn");
    
    if (localPlayerOffset == 0) {
        return 0;
    }
    
    uintptr_t localPlayerPtrAddr = m_clientBase + localPlayerOffset;
    uintptr_t localPlayer = 0;
    
    if (!m_processManager->Read(localPlayerPtrAddr, localPlayer)) {
        return 0;
    }
    
    return localPlayer;
}

uintptr_t SkinChanger::GetWeaponByIndex(int index) {
    uintptr_t localPlayer = GetLocalPlayer();
    if (localPlayer == 0) {
        return 0;
    }
    
    auto& offsetMgr = OffsetManager::Instance();
    uintptr_t weaponOffset = offsetMgr.GetOffset("m_pClippingWeapon");
    
    if (weaponOffset == 0) {
        return 0;
    }
    
    uintptr_t weaponHandle = 0;
    if (!m_processManager->Read(localPlayer + weaponOffset, weaponHandle)) {
        return 0;
    }
    
    // Get entity from handle
    uintptr_t entityListOffset = offsetMgr.GetOffset("dwEntityList");
    if (entityListOffset == 0) {
        return 0;
    }
    
    uintptr_t entityList = m_clientBase + entityListOffset;
    uintptr_t entityListEntry = 0;
    
    if (!m_processManager->Read(entityList, entityListEntry)) {
        return 0;
    }
    
    // Calculate entity address from handle
    // CS2 entity handle format: lower 15 bits (0x7FFF) contain entity index
    // Entity index is shifted right by 9 to get the list entry
    int entryIndex = (weaponHandle & 0x7FFF) >> 9;
    uintptr_t entityAddress = 0;
    
    // Each entity list entry is 0x78 bytes
    if (!m_processManager->Read(entityListEntry + (entryIndex * 0x78), entityAddress)) {
        return 0;
    }
    
    return entityAddress;
}

bool SkinChanger::UpdateWeaponSkin(uintptr_t weaponAddress, int paintKit, float wear, int seed, int statTrak) {
    if (weaponAddress == 0) {
        return false;
    }
    
    auto& offsetMgr = OffsetManager::Instance();
    
    uintptr_t attributeManager = weaponAddress + offsetMgr.GetOffset("m_AttributeManager");
    uintptr_t item = 0;
    
    if (!m_processManager->Read(attributeManager + offsetMgr.GetOffset("m_Item"), item)) {
        return false;
    }
    
    // Write paint kit
    if (!m_processManager->Write(item + offsetMgr.GetOffset("m_nFallbackPaintKit"), paintKit)) {
        return false;
    }
    
    // Write wear
    if (!m_processManager->Write(item + offsetMgr.GetOffset("m_flFallbackWear"), wear)) {
        return false;
    }
    
    // Write seed
    if (!m_processManager->Write(item + offsetMgr.GetOffset("m_nFallbackSeed"), seed)) {
        return false;
    }
    
    // Write StatTrak (if enabled)
    if (statTrak >= 0) {
        if (!m_processManager->Write(item + offsetMgr.GetOffset("m_nFallbackStatTrak"), statTrak)) {
            return false;
        }
    }
    
    // Force item update
    int itemIDHigh = -1;
    m_processManager->Write(item + offsetMgr.GetOffset("m_iItemIDHigh"), itemIDHigh);
    
    return true;
}

bool SkinChanger::ApplySkin(int weaponId, int paintKit, float wear, int seed, int statTrak) {
    if (!m_initialized) {
        m_lastError = "SkinChanger not initialized.";
        return false;
    }
    
    uintptr_t weaponAddress = GetWeaponByIndex(0);
    if (weaponAddress == 0) {
        m_lastError = "Failed to get weapon address.";
        return false;
    }
    
    if (!UpdateWeaponSkin(weaponAddress, paintKit, wear, seed, statTrak)) {
        m_lastError = "Failed to update weapon skin.";
        return false;
    }
    
    m_lastError.clear();
    return true;
}

bool SkinChanger::ResetSkins() {
    if (!m_initialized) {
        m_lastError = "SkinChanger not initialized.";
        return false;
    }
    
    // Reset to default (paint kit 0)
    return ApplySkin(0, 0, 0.0f, 0, -1);
}
