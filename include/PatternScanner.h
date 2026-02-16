#pragma once

#include <Windows.h>
#include <string>
#include <vector>

class PatternScanner {
public:
    PatternScanner(HANDLE processHandle, uintptr_t moduleBase, size_t moduleSize);
    
    uintptr_t FindPattern(const std::string& pattern);
    uintptr_t FindPattern(const std::vector<int>& pattern);
    
private:
    std::vector<int> ParsePattern(const std::string& pattern);
    bool ComparePattern(const BYTE* data, const std::vector<int>& pattern);
    
    HANDLE m_processHandle;
    uintptr_t m_moduleBase;
    size_t m_moduleSize;
    std::vector<BYTE> m_moduleData;
};
