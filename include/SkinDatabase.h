#pragma once

#include <string>
#include <vector>
#include <map>

struct SkinInfo {
    std::string name;
    int paintKit;
};

struct WeaponInfo {
    int id;
    std::string name;
    std::vector<SkinInfo> skins;
};

class SkinDatabase {
public:
    static SkinDatabase& Instance();
    
    bool LoadFromFile(const std::string& filepath);
    
    const std::map<std::string, std::vector<WeaponInfo>>& GetCategories() const {
        return m_categories;
    }
    
    const WeaponInfo* FindWeapon(const std::string& category, const std::string& name) const;
    const SkinInfo* FindSkin(const std::string& category, const std::string& weaponName, 
                              const std::string& skinName) const;

private:
    SkinDatabase() = default;
    SkinDatabase(const SkinDatabase&) = delete;
    SkinDatabase& operator=(const SkinDatabase&) = delete;
    
    std::map<std::string, std::vector<WeaponInfo>> m_categories;
};
