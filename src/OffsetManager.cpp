#include "OffsetManager.h"
#include <fstream>
#include <sstream>

// Simple JSON parser for our specific use case
namespace {
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r\"");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\n\r\",");
        return str.substr(first, (last - first + 1));
    }
    
    uintptr_t parseHex(const std::string& str) {
        std::string cleaned = trim(str);
        if (cleaned.substr(0, 2) == "0x" || cleaned.substr(0, 2) == "0X") {
            cleaned = cleaned.substr(2);
        }
        return std::stoull(cleaned, nullptr, 16);
    }
}

OffsetManager& OffsetManager::Instance() {
    static OffsetManager instance;
    return instance;
}

bool OffsetManager::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    std::string currentSection;
    
    while (std::getline(file, line)) {
        line = trim(line);
        
        if (line.empty() || line[0] == '{' || line[0] == '}' || 
            line.find("version") != std::string::npos || 
            line.find("lastUpdated") != std::string::npos ||
            line.find("clientDll") != std::string::npos ||
            line.find("notes") != std::string::npos) {
            continue;
        }
        
        if (line.find("\"offsets\"") != std::string::npos) {
            currentSection = "offsets";
            continue;
        }
        
        if (line.find("\"patterns\"") != std::string::npos) {
            currentSection = "patterns";
            continue;
        }
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = trim(line.substr(0, colonPos));
            std::string value = trim(line.substr(colonPos + 1));
            
            if (currentSection == "offsets") {
                m_offsets[key] = parseHex(value);
            } else if (currentSection == "patterns") {
                m_patterns[key] = value;
            }
        }
    }
    
    return !m_offsets.empty();
}

bool OffsetManager::SaveToFile(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    file << "{\n";
    file << "  \"version\": \"1.0.0\",\n";
    file << "  \"offsets\": {\n";
    
    size_t count = 0;
    for (const auto& [name, offset] : m_offsets) {
        file << "    \"" << name << "\": \"0x" << std::hex << offset << "\"";
        if (++count < m_offsets.size()) {
            file << ",";
        }
        file << "\n";
    }
    
    file << "  },\n";
    file << "  \"patterns\": {\n";
    
    count = 0;
    for (const auto& [name, pattern] : m_patterns) {
        file << "    \"" << name << "\": \"" << pattern << "\"";
        if (++count < m_patterns.size()) {
            file << ",";
        }
        file << "\n";
    }
    
    file << "  }\n";
    file << "}\n";
    
    return true;
}

uintptr_t OffsetManager::GetOffset(const std::string& name) const {
    auto it = m_offsets.find(name);
    return it != m_offsets.end() ? it->second : 0;
}

std::string OffsetManager::GetPattern(const std::string& name) const {
    auto it = m_patterns.find(name);
    return it != m_patterns.end() ? it->second : "";
}

void OffsetManager::SetOffset(const std::string& name, uintptr_t offset) {
    m_offsets[name] = offset;
}
