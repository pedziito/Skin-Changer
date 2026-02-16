#pragma once

#include "ProcessManager.h"
#include "OffsetManager.h"
#include <memory>

class SkinChanger {
public:
    SkinChanger();
    ~SkinChanger();
    
    bool Initialize();
    void Shutdown();
    
    bool ApplySkin(int weaponId, int paintKit, float wear = 0.0001f, int seed = 0, int statTrak = -1);
    bool ResetSkins();
    
    bool IsInitialized() const { return m_initialized; }
    std::string GetLastError() const { return m_lastError; }

private:
    uintptr_t GetLocalPlayer();
    uintptr_t GetWeaponByIndex(int index);
    bool UpdateWeaponSkin(uintptr_t weaponAddress, int paintKit, float wear, int seed, int statTrak);
    
    std::unique_ptr<ProcessManager> m_processManager;
    uintptr_t m_clientBase;
    bool m_initialized;
    std::string m_lastError;
};
