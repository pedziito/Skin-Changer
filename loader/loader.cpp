/*
 * AC Skin Changer - DLL Injector (Loader)
 * Console application that injects the skin changer DLL into cs2.exe.
 * Uses CreateRemoteThread + LoadLibraryA injection method.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <string>

// ANSI color codes for terminal
#define RESET   "\033[0m"
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define CYAN    "\033[1;36m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"

void PrintBanner() {
    printf(BLUE);
    printf(R"(
     _    ____   ____  _    _       
    / \  / ___| / ___|| | _(_)_ __  
   / _ \| |     \___ \| |/ / | '_ \ 
  / ___ \ |___   ___) |   <| | | | |
 /_/   \_\____| |____/|_|\_\_|_| |_|
                                     
    )" RESET "\n");
    printf(CYAN "  CS2 Skin Changer v2.0.0" RESET "\n");
    printf(DIM "  Custom Rendering Engine + DX11 ImGui" RESET "\n");
    printf(DIM "  =====================================" RESET "\n\n");
}

// Check if running as administrator
bool IsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin != FALSE;
}

// Find process ID by name
DWORD FindProcess(const char* processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);

    if (Process32First(snapshot, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, processName) == 0) {
                CloseHandle(snapshot);
                return pe.th32ProcessID;
            }
        } while (Process32Next(snapshot, &pe));
    }

    CloseHandle(snapshot);
    return 0;
}

// Inject DLL into target process
bool InjectDLL(DWORD processId, const char* dllPath) {
    printf(YELLOW "[*] " RESET "Opening process (PID: %lu)...\n", processId);

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        printf(RED "[!] " RESET "Failed to open process. Error: %lu\n", GetLastError());
        printf(RED "[!] " RESET "Make sure you're running as Administrator.\n");
        return false;
    }

    // Allocate memory in target process for DLL path
    size_t pathLen = strlen(dllPath) + 1;
    LPVOID remoteMem = VirtualAllocEx(hProcess, nullptr, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        printf(RED "[!] " RESET "Failed to allocate memory in target process.\n");
        CloseHandle(hProcess);
        return false;
    }

    // Write DLL path to target process
    if (!WriteProcessMemory(hProcess, remoteMem, dllPath, pathLen, nullptr)) {
        printf(RED "[!] " RESET "Failed to write DLL path to target process.\n");
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Get LoadLibraryA address
    LPVOID loadLibAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!loadLibAddr) {
        printf(RED "[!] " RESET "Failed to get LoadLibraryA address.\n");
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Create remote thread
    printf(YELLOW "[*] " RESET "Creating remote thread...\n");
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                         (LPTHREAD_START_ROUTINE)loadLibAddr,
                                         remoteMem, 0, nullptr);
    if (!hThread) {
        printf(RED "[!] " RESET "Failed to create remote thread. Error: %lu\n", GetLastError());
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Wait for injection to complete
    printf(YELLOW "[*] " RESET "Waiting for injection...\n");
    WaitForSingleObject(hThread, 10000);

    // Check if DLL was loaded
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);

    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return exitCode != 0;
}

int main() {
    // Enable ANSI escape codes on Windows
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    PrintBanner();

    // Check admin
    if (!IsAdmin()) {
        printf(RED "[!] " RESET "Not running as Administrator!\n");
        printf(YELLOW "[*] " RESET "Please right-click and select 'Run as Administrator'.\n\n");
        printf("Press any key to exit...\n");
        getchar();
        return 1;
    }
    printf(GREEN "[+] " RESET "Running as Administrator\n");

    // Find CS2
    printf(YELLOW "[*] " RESET "Looking for cs2.exe...\n");

    DWORD pid = 0;
    for (int i = 0; i < 60; i++) {
        pid = FindProcess("cs2.exe");
        if (pid) break;

        if (i == 0) {
            printf(YELLOW "[*] " RESET "Waiting for CS2 to start...\n");
        }
        if (i % 10 == 0 && i > 0) {
            printf(DIM "     Still waiting... (%d seconds)" RESET "\n", i);
        }
        Sleep(1000);
    }

    if (!pid) {
        printf(RED "[!] " RESET "CS2 not found after 60 seconds.\n");
        printf(YELLOW "[*] " RESET "Please start CS2 first, then run this loader.\n\n");
        printf("Press any key to exit...\n");
        getchar();
        return 1;
    }

    printf(GREEN "[+] " RESET "Found cs2.exe (PID: %lu)\n", pid);

    // Wait a bit for CS2 to fully initialize
    printf(YELLOW "[*] " RESET "Waiting for CS2 to initialize...\n");
    Sleep(5000);

    // Get DLL path (same directory as loader)
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);

    // Replace loader exe name with DLL name
    std::string dllPath = exePath;
    size_t lastSlash = dllPath.find_last_of('\\');
    if (lastSlash != std::string::npos) {
        dllPath = dllPath.substr(0, lastSlash + 1);
    }
    dllPath += "skin_changer.dll";

    // Check if DLL exists
    if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        printf(RED "[!] " RESET "DLL not found: %s\n", dllPath.c_str());
        printf(YELLOW "[*] " RESET "Make sure skin_changer.dll is in the same folder as the loader.\n\n");
        printf("Press any key to exit...\n");
        getchar();
        return 1;
    }

    printf(GREEN "[+] " RESET "DLL found: %s\n", dllPath.c_str());

    // Inject
    printf(YELLOW "[*] " RESET "Injecting...\n\n");

    if (InjectDLL(pid, dllPath.c_str())) {
        printf(GREEN "  ========================================" RESET "\n");
        printf(GREEN "  [+] Injection successful!" RESET "\n");
        printf(GREEN "  ========================================" RESET "\n\n");
        printf(CYAN "  Controls:" RESET "\n");
        printf("    INSERT  -  Toggle menu\n");
        printf("    END     -  Unload cheat\n\n");
        printf(DIM "  Skins will appear in your CS2 inventory" RESET "\n");
        printf(DIM "  when you equip them through the menu." RESET "\n\n");
    } else {
        printf(RED "[!] " RESET "Injection failed!\n");
        printf(YELLOW "[*] " RESET "Common fixes:\n");
        printf("    - Run as Administrator\n");
        printf("    - Disable antivirus temporarily\n");
        printf("    - Make sure CS2 is fully loaded\n\n");
    }

    printf("Press any key to exit...\n");
    getchar();
    return 0;
}
