/*
 * AC Skin Changer - Configuration System
 * Saves and loads skin presets to %APPDATA%\AC_Changer\presets\
 * Format: weaponName|skinName|weaponId|paintKit|seed|wear|statTrak per line
 */

#include "core.h"

namespace Config {
    std::string GetConfigDir() {
        char appData[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, appData))) {
            std::string dir = std::string(appData) + "\\AC_Changer\\presets";
            std::filesystem::create_directories(dir);
            return dir;
        }
        return ".";
    }

    void Save(const std::string& presetName) {
        std::string path = GetConfigDir() + "\\" + presetName + ".acpreset";
        std::ofstream file(path);
        if (!file.is_open()) {
            LogMsg("Failed to save preset: %s", path.c_str());
            return;
        }

        std::lock_guard<std::mutex> lock(G::skinMutex);
        for (auto& [id, cfg] : G::equippedSkins) {
            file << cfg.weaponName << "|"
                 << cfg.skinName << "|"
                 << cfg.weaponId << "|"
                 << cfg.paintKit << "|"
                 << cfg.seed << "|"
                 << cfg.wear << "|"
                 << cfg.statTrak << "\n";
        }

        file.close();
        LogMsg("Saved preset: %s (%zu skins)", presetName.c_str(), G::equippedSkins.size());
    }

    void Load(const std::string& presetName) {
        std::string path = GetConfigDir() + "\\" + presetName + ".acpreset";
        std::ifstream file(path);
        if (!file.is_open()) {
            LogMsg("Failed to load preset: %s", path.c_str());
            return;
        }

        std::lock_guard<std::mutex> lock(G::skinMutex);
        G::equippedSkins.clear();

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;

            SkinConfig cfg;
            std::istringstream ss(line);
            std::string token;

            // Parse: weaponName|skinName|weaponId|paintKit|seed|wear|statTrak
            if (!std::getline(ss, cfg.weaponName, '|')) continue;
            if (!std::getline(ss, cfg.skinName, '|')) continue;
            if (!std::getline(ss, token, '|')) continue;
            cfg.weaponId = std::stoi(token);
            if (!std::getline(ss, token, '|')) continue;
            cfg.paintKit = std::stoi(token);
            if (!std::getline(ss, token, '|')) continue;
            cfg.seed = std::stoi(token);
            if (!std::getline(ss, token, '|')) continue;
            cfg.wear = std::stof(token);
            if (!std::getline(ss, token, '|')) continue;
            cfg.statTrak = std::stoi(token);

            cfg.enabled = true;
            G::equippedSkins[cfg.weaponId] = cfg;
        }

        file.close();
        LogMsg("Loaded preset: %s (%zu skins)", presetName.c_str(), G::equippedSkins.size());
    }

    void Delete(const std::string& presetName) {
        std::string path = GetConfigDir() + "\\" + presetName + ".acpreset";
        std::filesystem::remove(path);
        LogMsg("Deleted preset: %s", presetName.c_str());
    }

    std::vector<std::string> ListPresets() {
        std::vector<std::string> presets;
        std::string dir = GetConfigDir();

        try {
            for (auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.path().extension() == ".acpreset") {
                    presets.push_back(entry.path().stem().string());
                }
            }
        } catch (...) {}

        std::sort(presets.begin(), presets.end());
        return presets;
    }
}
