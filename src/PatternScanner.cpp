#include "PatternScanner.h"
#include <sstream>

PatternScanner::PatternScanner(HANDLE processHandle, uintptr_t moduleBase, size_t moduleSize)
    : m_processHandle(processHandle), m_moduleBase(moduleBase), m_moduleSize(moduleSize) {
    
    // Read module data into memory
    m_moduleData.resize(moduleSize);
    SIZE_T bytesRead = 0;
    
    if (!ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(moduleBase), 
                          m_moduleData.data(), moduleSize, &bytesRead) || 
        bytesRead != moduleSize) {
        // Failed to read module memory, clear the buffer
        m_moduleData.clear();
    }
}

uintptr_t PatternScanner::FindPattern(const std::string& pattern) {
    auto parsedPattern = ParsePattern(pattern);
    return FindPattern(parsedPattern);
}

uintptr_t PatternScanner::FindPattern(const std::vector<int>& pattern) {
    if (pattern.empty() || m_moduleData.empty()) {
        return 0;
    }
    
    size_t patternSize = pattern.size();
    for (size_t i = 0; i < m_moduleData.size() - patternSize; ++i) {
        if (ComparePattern(&m_moduleData[i], pattern)) {
            return m_moduleBase + i;
        }
    }
    
    return 0;
}

std::vector<int> PatternScanner::ParsePattern(const std::string& pattern) {
    std::vector<int> result;
    std::istringstream stream(pattern);
    std::string byte;
    
    while (stream >> byte) {
        if (byte == "?") {
            result.push_back(-1);
        } else {
            result.push_back(std::stoi(byte, nullptr, 16));
        }
    }
    
    return result;
}

bool PatternScanner::ComparePattern(const BYTE* data, const std::vector<int>& pattern) {
    for (size_t i = 0; i < pattern.size(); ++i) {
        if (pattern[i] != -1 && data[i] != static_cast<BYTE>(pattern[i])) {
            return false;
        }
    }
    return true;
}
