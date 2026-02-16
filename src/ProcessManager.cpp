#include "ProcessManager.h"
#include <stdexcept>

ProcessManager::ProcessManager() 
    : m_processHandle(nullptr), m_processId(0) {
}

ProcessManager::~ProcessManager() {
    Detach();
}

bool ProcessManager::AttachToProcess(const std::wstring& processName) {
    m_processId = FindProcessId(processName);
    if (m_processId == 0) {
        return false;
    }
    
    m_processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_processId);
    return m_processHandle != nullptr;
}

void ProcessManager::Detach() {
    if (m_processHandle) {
        CloseHandle(m_processHandle);
        m_processHandle = nullptr;
    }
    m_processId = 0;
}

DWORD ProcessManager::FindProcessId(const std::wstring& processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);
    
    DWORD processId = 0;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, processName.c_str()) == 0) {
                processId = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    
    CloseHandle(snapshot);
    return processId;
}

uintptr_t ProcessManager::GetModuleBase(const std::wstring& moduleName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_processId);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    MODULEENTRY32W entry;
    entry.dwSize = sizeof(MODULEENTRY32W);
    
    uintptr_t moduleBase = 0;
    if (Module32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szModule, moduleName.c_str()) == 0) {
                moduleBase = reinterpret_cast<uintptr_t>(entry.modBaseAddr);
                break;
            }
        } while (Module32NextW(snapshot, &entry));
    }
    
    CloseHandle(snapshot);
    return moduleBase;
}

bool ProcessManager::ReadBytes(uintptr_t address, void* buffer, size_t size) {
    if (!m_processHandle) {
        return false;
    }
    
    SIZE_T bytesRead;
    return ReadProcessMemory(m_processHandle, reinterpret_cast<LPCVOID>(address), 
                            buffer, size, &bytesRead) && bytesRead == size;
}

bool ProcessManager::WriteBytes(uintptr_t address, const void* buffer, size_t size) {
    if (!m_processHandle) {
        return false;
    }
    
    SIZE_T bytesWritten;
    return WriteProcessMemory(m_processHandle, reinterpret_cast<LPVOID>(address), 
                             buffer, size, &bytesWritten) && bytesWritten == size;
}
