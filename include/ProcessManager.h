#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>

class ProcessManager {
public:
    ProcessManager();
    ~ProcessManager();

    bool AttachToProcess(const std::wstring& processName);
    void Detach();
    bool IsAttached() const { return m_processHandle != nullptr; }
    
    DWORD GetProcessId() const { return m_processId; }
    HANDLE GetProcessHandle() const { return m_processHandle; }
    uintptr_t GetModuleBase(const std::wstring& moduleName);
    
    template<typename T>
    bool Read(uintptr_t address, T& value);
    
    template<typename T>
    bool Write(uintptr_t address, const T& value);
    
    bool ReadBytes(uintptr_t address, void* buffer, size_t size);
    bool WriteBytes(uintptr_t address, const void* buffer, size_t size);

private:
    DWORD FindProcessId(const std::wstring& processName);
    
    HANDLE m_processHandle;
    DWORD m_processId;
};

// Template implementations
template<typename T>
bool ProcessManager::Read(uintptr_t address, T& value) {
    return ReadBytes(address, &value, sizeof(T));
}

template<typename T>
bool ProcessManager::Write(uintptr_t address, const T& value) {
    return WriteBytes(address, &value, sizeof(T));
}
