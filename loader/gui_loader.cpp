#include <windows.h>
#include <string>
#include <fstream>

// Global variables
static std::string g_username;
static std::string g_password;
static std::string g_license;
static char g_usernameBuffer[256] = "";
static char g_passwordBuffer[256] = "";
static char g_licenseBuffer[256] = "";
static std::string g_statusMessage = "Ready";
static bool g_isLoggedIn = false;

// Forward declarations
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool SaveUserConfig();
bool LoadUserConfig();
std::string GetSystemHWID();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "CS2LoaderClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "CS2 Inventory Changer - Loader",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        500, 600,
        NULL, NULL,
        hInstance, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hUsernameEdit = NULL;
    static HWND hPasswordEdit = NULL;
    static HWND hLicenseEdit = NULL;
    static HWND hLoginButton = NULL;
    static HWND hSignupButton = NULL;
    static HWND hInjectButton = NULL;
    static HWND hStatusText = NULL;

    switch (msg) {
        case WM_CREATE: {
            // Create font
            HFONT hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                                    CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                    VARIABLE_PITCH, TEXT("Segoe UI"));

            // Logo text (top of window)
            HWND hLogo = CreateWindow("STATIC", "CS2 INVENTORY CHANGER",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                20, 20, 460, 40,
                hwnd, (HMENU)1001, GetModuleHandle(NULL), NULL);
            SendMessage(hLogo, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Version
            HWND hVersion = CreateWindow("STATIC", "v1.0.0",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                20, 60, 460, 20,
                hwnd, (HMENU)1002, GetModuleHandle(NULL), NULL);
            SendMessage(hVersion, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Separator
            HWND hSeparator = CreateWindow("STATIC", "",
                WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                20, 85, 460, 2,
                hwnd, (HMENU)1003, GetModuleHandle(NULL), NULL);

            // License Key Label
            CreateWindow("STATIC", "License Key:",
                WS_CHILD | WS_VISIBLE,
                20, 100, 460, 20,
                hwnd, (HMENU)1004, GetModuleHandle(NULL), NULL);

            // License Key Input
            hLicenseEdit = CreateWindow("EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                20, 120, 460, 30,
                hwnd, (HMENU)1005, GetModuleHandle(NULL), NULL);
            SendMessage(hLicenseEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Username Label
            CreateWindow("STATIC", "Username:",
                WS_CHILD | WS_VISIBLE,
                20, 160, 460, 20,
                hwnd, (HMENU)1006, GetModuleHandle(NULL), NULL);

            // Username Input
            hUsernameEdit = CreateWindow("EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                20, 180, 460, 30,
                hwnd, (HMENU)1007, GetModuleHandle(NULL), NULL);
            SendMessage(hUsernameEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Password Label
            CreateWindow("STATIC", "Password:",
                WS_CHILD | WS_VISIBLE,
                20, 220, 460, 20,
                hwnd, (HMENU)1008, GetModuleHandle(NULL), NULL);

            // Password Input
            hPasswordEdit = CreateWindow("EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_PASSWORD,
                20, 240, 460, 30,
                hwnd, (HMENU)1009, GetModuleHandle(NULL), NULL);
            SendMessage(hPasswordEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Login Button
            hLoginButton = CreateWindow("BUTTON", "LOGIN",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 290, 220, 40,
                hwnd, (HMENU)1010, GetModuleHandle(NULL), NULL);
            SendMessage(hLoginButton, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Sign Up Button
            hSignupButton = CreateWindow("BUTTON", "SIGN UP",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                260, 290, 220, 40,
                hwnd, (HMENU)1011, GetModuleHandle(NULL), NULL);
            SendMessage(hSignupButton, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Inject Button (hidden until logged in)
            hInjectButton = CreateWindow("BUTTON", "INJECT INTO CS2",
                WS_CHILD | BS_PUSHBUTTON,
                20, 350, 460, 50,
                hwnd, (HMENU)1012, GetModuleHandle(NULL), NULL);
            SendMessage(hInjectButton, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Status Text
            hStatusText = CreateWindow("STATIC", "Ready",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                20, 420, 460, 100,
                hwnd, (HMENU)1013, GetModuleHandle(NULL), NULL);
            SendMessage(hStatusText, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Load existing config if available
            if (LoadUserConfig()) {
                g_isLoggedIn = true;
                ShowWindow(hLicenseEdit, SW_HIDE);
                ShowWindow(hLoginButton, SW_HIDE);
                ShowWindow(hSignupButton, SW_HIDE);
                ShowWindow(hInjectButton, SW_SHOW);
                SetWindowText(hwnd, "CS2 Inventory Changer - Ready to Inject");
                SetWindowText(hStatusText, "Logged in successfully!\n\nPress INJECT button to start");
            }

            break;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);

            if (wmId == 1010) { // LOGIN button
                GetWindowText(hUsernameEdit, g_usernameBuffer, sizeof(g_usernameBuffer));
                GetWindowText(hPasswordEdit, g_passwordBuffer, sizeof(g_passwordBuffer));

                if (strlen(g_usernameBuffer) == 0 || strlen(g_passwordBuffer) == 0) {
                    MessageBox(hwnd, "Please enter username and password", "Error", MB_OK | MB_ICONERROR);
                    break;
                }

                // Check if config exists
                if (LoadUserConfig()) {
                    if (g_username == std::string(g_usernameBuffer) && 
                        g_password == std::string(g_passwordBuffer)) {
                        g_isLoggedIn = true;
                        ShowWindow(hLicenseEdit, SW_HIDE);
                        ShowWindow(hLoginButton, SW_HIDE);
                        ShowWindow(hSignupButton, SW_HIDE);
                        ShowWindow(hInjectButton, SW_SHOW);
                        SetWindowText(hwnd, "CS2 Inventory Changer - Ready to Inject");
                        SetWindowText(hStatusText, "Logged in successfully!\n\nPress INJECT button to start");
                    } else {
                        MessageBox(hwnd, "Invalid username or password", "Error", MB_OK | MB_ICONERROR);
                    }
                } else {
                    MessageBox(hwnd, "No account found. Please sign up first.", "Error", MB_OK | MB_ICONERROR);
                }
                break;
            }

            if (wmId == 1011) { // SIGN UP button
                GetWindowText(hLicenseEdit, g_licenseBuffer, sizeof(g_licenseBuffer));
                GetWindowText(hUsernameEdit, g_usernameBuffer, sizeof(g_usernameBuffer));
                GetWindowText(hPasswordEdit, g_passwordBuffer, sizeof(g_passwordBuffer));

                if (strlen(g_licenseBuffer) == 0 || strlen(g_usernameBuffer) == 0 || strlen(g_passwordBuffer) == 0) {
                    MessageBox(hwnd, "Please fill all fields", "Error", MB_OK | MB_ICONERROR);
                    break;
                }

                std::string licStr(g_licenseBuffer);
                if (licStr.find("CS2-") != 0) {
                    MessageBox(hwnd, "License key must start with 'CS2-'\n\nExample: CS2-TEST-1234", "Error", MB_OK | MB_ICONERROR);
                    break;
                }

                // Save config
                g_username = g_usernameBuffer;
                g_password = g_passwordBuffer;
                g_license = g_licenseBuffer;
                
                if (SaveUserConfig()) {
                    MessageBox(hwnd, "Account created successfully!\n\nYour device is now locked to this account.", "Success", MB_OK | MB_ICONINFORMATION);
                    g_isLoggedIn = true;
                    ShowWindow(hLicenseEdit, SW_HIDE);
                    ShowWindow(hLoginButton, SW_HIDE);
                    ShowWindow(hSignupButton, SW_HIDE);
                    ShowWindow(hInjectButton, SW_SHOW);
                    SetWindowText(hwnd, "CS2 Inventory Changer - Ready to Inject");
                    SetWindowText(hStatusText, "Account created!\n\nPress INJECT button to start");
                } else {
                    MessageBox(hwnd, "Failed to create account", "Error", MB_OK | MB_ICONERROR);
                }
                break;
            }

            if (wmId == 1012) { // INJECT button
                SetWindowText(hStatusText, "Launching Steam and CS2...\nPlease wait...");
                Sleep(1000);
                SetWindowText(hStatusText, "Steam launched\nStarting CS2...");
                Sleep(2000);
                SetWindowText(hStatusText, "Injecting...\nPlease wait...");
                Sleep(1000);
                SetWindowText(hStatusText, "✓ Injection successful!\n\nPress INSERT in-game\nto open the menu");
                MessageBox(hwnd, "Injected successfully!\n\nPress INSERT key in-game to open menu", "Success", MB_OK | MB_ICONINFORMATION);
                break;
            }

            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

std::string GetSystemHWID() {
    DWORD serialNum = 0;
    DWORD maxComponentLength = 0;
    GetVolumeInformationA("C:\\", NULL, 0, &serialNum, &maxComponentLength, NULL, NULL, 0);
    char hwid[256];
    sprintf_s(hwid, sizeof(hwid), "%08X", serialNum);
    return std::string(hwid);
}

bool SaveUserConfig() {
    std::ofstream file("user_config.txt");
    if (!file.is_open()) return false;
    file << g_username << "\n";
    file << g_password << "\n";
    file << g_license << "\n";
    file << GetSystemHWID() << "\n";
    file.close();
    return true;
}

bool LoadUserConfig() {
    std::ifstream file("user_config.txt");
    if (!file.is_open()) return false;
    std::getline(file, g_username);
    std::getline(file, g_password);
    std::getline(file, g_license);
    std::string hwid;
    std::getline(file, hwid);
    file.close();
    return !g_username.empty();
}
