#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <filesystem>

// Forward declarations
DWORD FindProcessByName(const std::string& processName);
bool InjectDLL(DWORD processId, const std::string& dllPath);
void ShowVACWarning();

int main() {
    // Display VAC Warning
    ShowVACWarning();

    // Check for license file
    if (!std::filesystem::exists("license.key")) {
        std::cerr << "[ERROR] License file not found. Please obtain a license.\n";
        std::cerr << "Contact admin for license generation.\n";
        system("pause");
        return 1;
    }

    std::cout << "[*] CS2 Skin Changer Loader v1.0\n";
    std::cout << "[*] Waiting for CS2 process...\n";

    // Wait for CS2 to appear
    DWORD cs2Pid = 0;
    int attempts = 0;
    while (cs2Pid == 0 && attempts < 300) {  // 5 minutes timeout
        cs2Pid = FindProcessByName("cs2.exe");
        if (cs2Pid == 0) {
            Sleep(1000);  // Wait 1 second between attempts
            attempts++;
        }
    }

    if (cs2Pid == 0) {
        std::cerr << "[ERROR] CS2 process not found. Please start the game first.\n";
        system("pause");
        return 1;
    }

    std::cout << "[+] CS2 found (PID: " << cs2Pid << ")\n";

    // Get DLL path (should be in same directory as loader)
    char dllPath[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, dllPath);
    strcat_s(dllPath, MAX_PATH, "\\CS2Changer.dll");

    if (!std::filesystem::exists(dllPath)) {
        std::cerr << "[ERROR] DLL not found: " << dllPath << "\n";
        system("pause");
        return 1;
    }

    std::cout << "[*] Injecting DLL...\n";

    // Inject DLL
    if (InjectDLL(cs2Pid, dllPath)) {
        std::cout << "[+] DLL injected successfully!\n";
        std::cout << "[+] In-game menu available (Press INS key to toggle)\n";
        std::cout << "[*] Press any key to exit loader (DLL will remain active)...\n";
        system("pause");
        return 0;
    }
    else {
        std::cerr << "[ERROR] Failed to inject DLL\n";
        system("pause");
        return 1;
    }
}

/**
 * Find process by executable name
 */
DWORD FindProcessByName(const std::string& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32 pe32 = { 0 };
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return 0;
    }

    DWORD result = 0;
    do {
        if (processName == pe32.szExeFile) {
            result = pe32.th32ProcessID;
            break;
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return result;
}

/**
 * Inject DLL into target process using CreateRemoteThread
 */
bool InjectDLL(DWORD processId, const std::string& dllPath) {
    // Open target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL) {
        std::cerr << "[ERROR] Failed to open process (might need admin rights)\n";
        return false;
    }

    // Allocate memory in target process
    SIZE_T dllPathSize = dllPath.size() + 1;
    LPVOID pDllPath = VirtualAllocEx(hProcess, NULL, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (pDllPath == NULL) {
        std::cerr << "[ERROR] Failed to allocate memory in target process\n";
        CloseHandle(hProcess);
        return false;
    }

    // Write DLL path to target process memory
    if (!WriteProcessMemory(hProcess, pDllPath, (LPVOID)dllPath.c_str(), dllPathSize, NULL)) {
        std::cerr << "[ERROR] Failed to write DLL path to process memory\n";
        VirtualFreeEx(hProcess, pDllPath, dllPathSize, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Get address of LoadLibraryA function
    LPVOID pLoadLibraryA = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (pLoadLibraryA == NULL) {
        std::cerr << "[ERROR] Failed to get LoadLibraryA address\n";
        VirtualFreeEx(hProcess, pDllPath, dllPathSize, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Create remote thread that calls LoadLibraryA
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryA, pDllPath, 0, NULL);
    if (hThread == NULL) {
        std::cerr << "[ERROR] Failed to create remote thread\n";
        VirtualFreeEx(hProcess, pDllPath, dllPathSize, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Wait for thread to complete
    WaitForSingleObject(hThread, INFINITE);

    // Cleanup
    VirtualFreeEx(hProcess, pDllPath, dllPathSize, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return true;
}

/**
 * Display VAC Warning
 */
void ShowVACWarning() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    ⚠️  VAC WARNING  ⚠️                          ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║                                                                ║\n";
    std::cout << "║  This tool modifies the game client and can trigger VAC.      ║\n";
    std::cout << "║  Use at your own risk. We are not responsible for bans.       ║\n";
    std::cout << "║                                                                ║\n";
    std::cout << "║  CONSEQUENCES OF USE:                                          ║\n";
    std::cout << "║  • Account suspension                                          ║\n";
    std::cout << "║  • Game/Application ban                                        ║\n";
    std::cout << "║  • IP block                                                    ║\n";
    std::cout << "║  • Loss of purchased items and games                           ║\n";
    std::cout << "║                                                                ║\n";
    std::cout << "║  Do NOT use on your main account. Use at your own risk.        ║\n";
    std::cout << "║                                                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
}
