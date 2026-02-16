#pragma once

#include <string>
#include <map>
#include <cstdint>

class OffsetManager {
public:
    static OffsetManager& Instance();
    
    bool LoadFromFile(const std::string& filepath);
    bool SaveToFile(const std::string& filepath);
    
    uintptr_t GetOffset(const std::string& name) const;
    std::string GetPattern(const std::string& name) const;
    void SetOffset(const std::string& name, uintptr_t offset);
    
    const std::map<std::string, uintptr_t>& GetAllOffsets() const { return m_offsets; }
    const std::map<std::string, std::string>& GetAllPatterns() const { return m_patterns; }

private:
    OffsetManager() = default;
    OffsetManager(const OffsetManager&) = delete;
    OffsetManager& operator=(const OffsetManager&) = delete;
    
    std::map<std::string, uintptr_t> m_offsets;
    std::map<std::string, std::string> m_patterns;
};
