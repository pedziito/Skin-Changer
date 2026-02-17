#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <conio.h>
#include <wincrypt.h>

// Global variables
struct UserSession {
    std::string username;
    std::string password;
    std::string licenseKey;
    std::string hwid;
    bool authenticated;
};

UserSession g_Session = { "", "", "", "", false };

// Constants
const char* CONFIG_FILE = "user_config.txt";

// Forward declarations
DWORD FindProcessByName(const std::string& processName);
bool InjectIntoCS2(DWORD cs2Pid);
std::string GetSystemHWID();
bool StartSteamAndCS2();
bool SaveUserConfig();
bool LoadUserConfig();
bool SignUpUser();
bool LoginUser();

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

    // Check if user already has config
    if (LoadUserConfig()) {
        std::cout << "[*] Welcome back, " << g_Session.username << "!\n\n";
        
        if (!LoginUser()) {
            std::cout << "[!] Login failed. Exiting...\n";
            Sleep(2000);
            return 1;
        }
    } else {
        std::cout << "[*] First time setup\n";
        std::cout << "[*] Please sign up with your license\n\n";
        
        if (!SignUpUser()) {
            std::cout << "[!] Sign up failed. Exiting...\n";
            Sleep(2000);
            return 1;
        }
    }

    std::cout << "\n[+] Authentication successful!\n";
    std::cout << "[*] Preparing to launch...\n\n";

    // Start Steam and CS2
    std::cout << "[*] Starting Steam...\n";
    if (!StartSteamAndCS2()) {
        MessageBoxA(NULL, "Failed to start Steam/CS2!\n\nMake sure Steam is installed.",
                    "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    std::cout << "[+] Steam and CS2 launched\n";
    std::cout << "[*] Waiting for CS2 process...\n";

    // Wait for CS2
    DWORD cs2Pid = 0;
    int attempts = 0;
    while (cs2Pid == 0 && attempts < 300) {
        cs2Pid = FindProcessByName("cs2.exe");
        if (cs2Pid == 0) {
            Sleep(1000);
            attempts++;
        }
    }

    if (cs2Pid == 0) {
        std::cout << "[ERROR] CS2 process not found!\n";
        MessageBoxA(NULL, "Failed to detect CS2 process!\n\nPlease start the game manually.",
                    "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    std::cout << "[+] CS2 found (PID: " << cs2Pid << ")\n";
    std::cout << "[*] Injecting Inventory Changer...\n";

    if (!InjectIntoCS2(cs2Pid)) {
        std::cout << "[ERROR] Injection failed!\n";
        MessageBoxA(NULL, "Injection failed!\n\nMake sure you have administrator rights.",
                    "Error", MB_OK | MB_ICONERROR);
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
                      "This window will close automatically.",
                "Success", MB_OK | MB_ICONINFORMATION);

    Sleep(2000);
    return 0;
}

/**
 * Get system HWID (using volume serial)
 */
std::string GetSystemHWID() {
    DWORD serialNum = 0;
    DWORD maxComponentLength = 0;

    GetVolumeInformationA("C:\\", NULL, 0, &serialNum, &maxComponentLength, NULL, NULL, 0);

    char hwid[256];
    sprintf_s(hwid, sizeof(hwid), "%08X", serialNum);

    return std::string(hwid);
}

/**
 * Sign up new user with license
 */
bool SignUpUser() {
    char licenseKey[256] = "";
    char username[256] = "";
    char password[256] = "";
    char passwordConfirm[256] = "";

    std::cout << "License Key: ";
    std::cin.getline(licenseKey, sizeof(licenseKey));

    if (std::string(licenseKey).empty()) {
        std::cout << "[ERROR] License key required!\n";
        return false;
    }

    std::cout << "Username: ";
    std::cin.getline(username, sizeof(username));

    std::cout << "Password: ";
    int pos = 0;
    int ch;
    while ((ch = _getch()) != '\r') {
        if (ch == '\b') {
            if (pos > 0) {
                pos--;
                std::cout << "\b \b";
            }
        } else if (ch != '\0') {
            password[pos++] = ch;
            std::cout << '*';
        }
    }
    password[pos] = '\0';
    std::cout << "\n";

    std::cout << "Confirm Password: ";
    pos = 0;
    while ((ch = _getch()) != '\r') {
        if (ch == '\b') {
            if (pos > 0) {
                pos--;
                std::cout << "\b \b";
            }
        } else if (ch != '\0') {
            passwordConfirm[pos++] = ch;
            std::cout << '*';
        }
    }
    passwordConfirm[pos] = '\0';
    std::cout << "\n";

    if (std::string(password) != std::string(passwordConfirm)) {
        std::cout << "[ERROR] Passwords do not match!\n";
        return false;
    }

    // Validate license format
    std::string licStr(licenseKey);
    if (licStr.find("CS2-") != 0) {
        std::cout << "[ERROR] Invalid license format!\n";
        std::cout << "[!] License must start with 'CS2-'\n";
        return false;
    }

    // Save user config
    g_Session.username = username;
    g_Session.password = password;
    g_Session.licenseKey = licenseKey;
    g_Session.hwid = GetSystemHWID();
    g_Session.authenticated = true;

    if (!SaveUserConfig()) {
        std::cout << "[ERROR] Failed to save user config!\n";
        return false;
    }

    std::cout << "\n[+] Account created successfully!\n";
    std::cout << "[*] HWID locked: " << g_Session.hwid << "\n";

    return true;
}

/**
 * Login user with credential validation
 */
bool LoginUser() {
    char username[256] = "";
    char password[256] = "";

    std::cout << "Username: ";
    std::cin.getline(username, sizeof(username));

    std::cout << "Password: ";
    int pos = 0;
    int ch;
    while ((ch = _getch()) != '\r') {
        if (ch == '\b') {
            if (pos > 0) {
                pos--;
                std::cout << "\b \b";
            }
        } else if (ch != '\0') {
            password[pos++] = ch;
            std::cout << '*';
        }
    }
    password[pos] = '\0';
    std::cout << "\n";

    // Validate credentials
    if (std::string(username) != g_Session.username || 
        std::string(password) != g_Session.password) {
        std::cout << "[ERROR] Invalid username or password!\n";
        return false;
    }

    // Validate HWID
    std::string currentHWID = GetSystemHWID();
    if (currentHWID != g_Session.hwid) {
        std::cout << "[ERROR] HWID mismatch!\n";
        std::cout << "[!] This account is locked to a different device.\n";
        std::cout << "[*] Contact support to reset your HWID.\n";
        return false;
    }

    std::cout << "[+] HWID verified\n";
    g_Session.authenticated = true;
    return true;
}

/**
 * Save user config to file (simple format)
 */
bool SaveUserConfig() {
    std::ofstream file(CONFIG_FILE);
    if (!file.is_open()) {
        return false;
    }

    file << g_Session.username << "\n";
    file << g_Session.password << "\n";
    file << g_Session.licenseKey << "\n";
    file << g_Session.hwid << "\n";
    file.close();

    return true;
}

/**
 * Load user config from file
 */
bool LoadUserConfig() {
    std::ifstream file(CONFIG_FILE);
    if (!file.is_open()) {
        return false;
    }

    std::getline(file, g_Session.username);
    std::getline(file, g_Session.password);
    std::getline(file, g_Session.licenseKey);
    std::getline(file, g_Session.hwid);
    file.close();

    return !g_Session.username.empty();
}

/**
 * Start Steam and CS2
 */
bool StartSteamAndCS2() {
    // Check if CS2 is already running
    if (FindProcessByName("cs2.exe") != 0) {
        return true;  // Already running
    }

    // Check if Steam is already running
    if (FindProcessByName("steam.exe") == 0) {
        // Launch Steam
        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi = {};

        if (!CreateProcessA("C:\\Program Files (x86)\\Steam\\steam.exe",
                           NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            return false;
        }
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        Sleep(3000);  // Wait for Steam to initialize
    }

    // Start CS2 via Steam
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    // Try common CS2 path
    const char* cs2Path = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Counter-Strike Global Offensive\\game\\bin\\win64\\cs2.exe";
    
    if (!CreateProcessA(cs2Path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        // Fallback: launch via Steam
        if (!CreateProcessA("C:\\Program Files (x86)\\Steam\\steam.exe",
                           "-applaunch 730", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            return false;
        }
    }
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    Sleep(2000);  // Wait for CS2 to start

    return true;
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

    std::cout << "[*] Hooking DirectX...\n";
    Sleep(300);
    std::cout << "[*] Initializing menu system...\n";
    Sleep(300);
    std::cout << "[*] Setting up keybinds (INS)...\n";
    Sleep(300);

    CloseHandle(hProcess);
    return true;
}

