#include "SkinDatabase.h"
#include <fstream>
#include <sstream>

namespace {
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r\"");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\n\r\",");
        return str.substr(first, (last - first + 1));
    }
}

SkinDatabase& SkinDatabase::Instance() {
    static SkinDatabase instance;
    return instance;
}

bool SkinDatabase::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    std::string currentCategory;
    std::string currentWeapon;
    WeaponInfo weaponInfo;
    
    while (std::getline(file, line)) {
        line = trim(line);
        
        if (line.empty() || line[0] == '{' || line[0] == '}' || 
            line.find("version") != std::string::npos ||
            line.find("categories") != std::string::npos) {
            continue;
        }
        
        // Check for category
        if (line.find("\"Rifles\"") != std::string::npos || 
            line.find("\"Pistols\"") != std::string::npos ||
            line.find("\"Knives\"") != std::string::npos ||
            line.find("\"SMGs\"") != std::string::npos) {
            currentCategory = line;
            continue;
        }
        
        // Check for weapon name
        if (line.find("\"AK-47\"") != std::string::npos || 
            line.find("\"M4A4\"") != std::string::npos ||
            line.find("\"M4A1-S\"") != std::string::npos ||
            line.find("\"AWP\"") != std::string::npos ||
            line.find("\"Desert Eagle\"") != std::string::npos ||
            line.find("\"Glock-18\"") != std::string::npos ||
            line.find("\"USP-S\"") != std::string::npos ||
            line.find("\"Karambit\"") != std::string::npos ||
            line.find("\"Butterfly Knife\"") != std::string::npos ||
            line.find("\"Bayonet\"") != std::string::npos ||
            line.find("\"MP9\"") != std::string::npos ||
            line.find("\"MP7\"") != std::string::npos) {
            
            if (!currentWeapon.empty() && weaponInfo.id != 0) {
                // Save previous weapon
                if (!currentCategory.empty()) {
                    m_categories[currentCategory].push_back(weaponInfo);
                }
            }
            
            currentWeapon = line;
            weaponInfo = WeaponInfo();
            weaponInfo.name = currentWeapon;
            continue;
        }
        
        // Parse weapon id
        if (line.find("\"id\"") != std::string::npos) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string value = trim(line.substr(colonPos + 1));
                weaponInfo.id = std::stoi(value);
            }
        }
        
        // Parse skin name and paintKit
        if (line.find("\"name\"") != std::string::npos) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string skinName = trim(line.substr(colonPos + 1));
                
                // Read next line for paintKit
                std::getline(file, line);
                line = trim(line);
                if (line.find("\"paintKit\"") != std::string::npos) {
                    colonPos = line.find(':');
                    if (colonPos != std::string::npos) {
                        std::string paintKitStr = trim(line.substr(colonPos + 1));
                        int paintKit = std::stoi(paintKitStr);
                        weaponInfo.skins.push_back({skinName, paintKit});
                    }
                }
            }
        }
    }
    
    // Save last weapon
    if (!currentWeapon.empty() && weaponInfo.id != 0 && !currentCategory.empty()) {
        m_categories[currentCategory].push_back(weaponInfo);
    }
    
    return !m_categories.empty();
}

const WeaponInfo* SkinDatabase::FindWeapon(const std::string& category, const std::string& name) const {
    auto catIt = m_categories.find(category);
    if (catIt == m_categories.end()) {
        return nullptr;
    }
    
    for (const auto& weapon : catIt->second) {
        if (weapon.name == name) {
            return &weapon;
        }
    }
    
    return nullptr;
}

const SkinInfo* SkinDatabase::FindSkin(const std::string& category, const std::string& weaponName, 
                                        const std::string& skinName) const {
    const WeaponInfo* weapon = FindWeapon(category, weaponName);
    if (!weapon) {
        return nullptr;
    }
    
    for (const auto& skin : weapon->skins) {
        if (skin.name == skinName) {
            return &skin;
        }
    }
    
    return nullptr;
}
