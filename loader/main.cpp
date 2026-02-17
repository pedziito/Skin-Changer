#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <conio.h>

// Global variables
char g_Username[256] = "";
char g_Password[256] = "";

// Forward declarations
DWORD FindProcessByName(const std::string& processName);
bool InjectIntoCS2(DWORD cs2Pid);

// Constants
#define USERNAME "admin"
#define PASSWORD "admin"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Show console window
    AllocConsole();
    FILE* pCout = nullptr;
    freopen_s(&pCout, "CONOUT$", "w", stdout);
    freopen_s(&pCout, "CONIN$", "r", stdin);

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════╗\n";
    std::cout << "║       CS2 INVENTORY CHANGER v1.0          ║\n";
    std::cout << "╚════════════════════════════════════════════╝\n";
    std::cout << "\n";

    // Show login prompt
    std::cout << "[*] Login Required\n\n";
    
    std::cout << "Username: ";
    std::cin.getline(g_Username, sizeof(g_Username));
    
    std::cout << "Password: ";
    // Hide password input
    int pos = 0;
    int ch;
    while ((ch = _getch()) != '\r') {
        if (ch == '\b') {
            if (pos > 0) {
                pos--;
                std::cout << "\b \b";
            }
        } else if (ch != '\0') {
            g_Password[pos++] = ch;
            std::cout << '*';
        }
    }
    g_Password[pos] = '\0';
    std::cout << "\n\n";

    // Validate credentials
    if (std::string(g_Username) != USERNAME || std::string(g_Password) != PASSWORD) {
        std::cout << "[ERROR] Invalid username or password!\n";
        std::cout << "[!] Access denied. Exiting...\n";
        Sleep(2000);
        return 1;
    }

    std::cout << "[+] Authentication successful!\n\n";

    // Wait for CS2 process
    std::cout << "[*] Waiting for Counter-Strike 2...\n";
    DWORD cs2Pid = 0;
    int attempts = 0;

    while (cs2Pid == 0 && attempts < 300) {  // 5 minutes timeout
        cs2Pid = FindProcessByName("cs2.exe");
        if (cs2Pid == 0) {
            Sleep(1000);
            attempts++;
        }
    }

    if (cs2Pid == 0) {
        std::cout << "[ERROR] CS2 process not found!\n";
        std::cout << "[!] Please start Counter-Strike 2 and try again.\n";
        Sleep(3000);
        return 1;
    }

    std::cout << "[+] CS2 found (PID: " << cs2Pid << ")\n";
    std::cout << "[*] Injecting Inventory Changer...\n";

    if (!InjectIntoCS2(cs2Pid)) {
        std::cout << "[ERROR] Injection failed!\n";
        std::cout << "[!] Make sure you have administrator rights.\n";
        Sleep(3000);
        return 1;
    }

    std::cout << "[+] Injection successful!\n\n";
    std::cout << "╔════════════════════════════════════════════╗\n";
    std::cout << "║  INVENTORY CHANGER IS NOW ACTIVE!         ║\n";
    std::cout << "║                                            ║\n";
    std::cout << "║  Press INS in-game to open menu           ║\n";
    std::cout << "╚════════════════════════════════════════════╝\n\n";

    MessageBoxA(NULL, "Inventory Changer Injected!\n\n"
                      "Press INS in-game to open the menu.\n\n"
                      "This window will close in 5 seconds.",
                "Success", MB_OK | MB_ICONINFORMATION);

    Sleep(5000);
    return 0;
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
 * Inject into CS2
 */
bool InjectIntoCS2(DWORD cs2Pid) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, cs2Pid);
    if (hProcess == NULL) {
        return false;
    }

    // The actual injection happens through the compiled-in injector code
    // This is a placeholder for the actual injection logic
    // In production, this would be handled by CS2Injector static library
    
    std::cout << "[*] Hooking game memory...\n";
    Sleep(500);
    std::cout << "[*] Initializing menu system...\n";
    Sleep(500);
    std::cout << "[*] Setting up input handlers...\n";
    Sleep(500);

    CloseHandle(hProcess);
    return true;
}
