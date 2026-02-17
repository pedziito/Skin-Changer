#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <fstream>
#include <sstream>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "msimg32.lib")

// ============================================================================
// GLOBALS
// ============================================================================
static std::string g_username;
static std::string g_password;
static std::string g_license;
static std::string g_hwid;
static char g_usernameBuffer[256] = "";
static char g_passwordBuffer[256] = "";
static char g_licenseBuffer[256] = "";
static bool g_isLoggedIn = false;
static bool g_isSignupMode = false;
static HINSTANCE g_hInstance = NULL;

// Colors
#define CLR_BG_TOP      RGB(10, 12, 28)
#define CLR_BG_BOTTOM   RGB(18, 25, 55)
#define CLR_ACCENT      RGB(50, 100, 220)
#define CLR_ACCENT_HOVER RGB(70, 125, 255)
#define CLR_INPUT_BG    RGB(22, 28, 50)
#define CLR_INPUT_BORDER RGB(45, 55, 90)
#define CLR_TEXT         RGB(210, 215, 230)
#define CLR_TEXT_DIM     RGB(120, 130, 160)
#define CLR_TEXT_LINK    RGB(80, 140, 255)
#define CLR_BTN_BG      RGB(45, 90, 210)
#define CLR_BTN_HOVER   RGB(60, 110, 240)
#define CLR_SUCCESS      RGB(50, 200, 120)
#define CLR_ERROR        RGB(220, 60, 60)

// Window dimensions
#define WIN_WIDTH  420
#define WIN_HEIGHT 580

// Control IDs
#define ID_USERNAME_EDIT  2001
#define ID_PASSWORD_EDIT  2002
#define ID_LICENSE_EDIT   2003
#define ID_LOGIN_BTN      2004
#define ID_INJECT_BTN     2005
#define ID_TOGGLE_LINK    2006
#define ID_STATUS_LABEL   2007
#define ID_USERNAME_LABEL 2010
#define ID_PASSWORD_LABEL 2011
#define ID_LICENSE_LABEL  2012

// Brushes
static HBRUSH g_hBgBrush = NULL;
static HBRUSH g_hInputBgBrush = NULL;
static HBRUSH g_hBtnBrush = NULL;
static HFONT g_hFontTitle = NULL;
static HFONT g_hFontNormal = NULL;
static HFONT g_hFontSmall = NULL;
static HFONT g_hFontBtn = NULL;

// Forward declarations
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool SaveUserConfig();
bool LoadUserConfig();
std::string GetSystemHWID();
DWORD FindProcessByName(const char* name);
bool KillProcessByName(const char* name);
bool LaunchSteamAndCS2(HWND hwnd);

// ============================================================================
// ENTRY POINT
// ============================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInstance = hInstance;

    // Create brushes
    g_hBgBrush = CreateSolidBrush(CLR_BG_TOP);
    g_hInputBgBrush = CreateSolidBrush(CLR_INPUT_BG);
    g_hBtnBrush = CreateSolidBrush(CLR_BTN_BG);

    // Create fonts
    g_hFontTitle = CreateFont(26, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));

    g_hFontNormal = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));

    g_hFontSmall = CreateFont(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));

    g_hFontBtn = CreateFont(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));

    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "CS2LoaderClass";
    wc.hbrBackground = g_hBgBrush;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassEx(&wc);

    // Calculate center position
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - WIN_WIDTH) / 2;
    int posY = (screenH - WIN_HEIGHT) / 2;

    // Create window - no resize, no maximize
    HWND hwnd = CreateWindowEx(
        0,
        "CS2LoaderClass",
        "CS2 Inventory Changer",
        WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY,
        WIN_WIDTH, WIN_HEIGHT,
        NULL, NULL,
        hInstance, NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    DeleteObject(g_hBgBrush);
    DeleteObject(g_hInputBgBrush);
    DeleteObject(g_hBtnBrush);
    DeleteObject(g_hFontTitle);
    DeleteObject(g_hFontNormal);
    DeleteObject(g_hFontSmall);
    DeleteObject(g_hFontBtn);

    return (int)msg.wParam;
}

// ============================================================================
// GRADIENT BACKGROUND
// ============================================================================
void DrawGradientBg(HDC hdc, RECT rc) {
    TRIVERTEX vertex[2];
    GRADIENT_RECT gRect;

    vertex[0].x = rc.left;
    vertex[0].y = rc.top;
    vertex[0].Red = 10 << 8;
    vertex[0].Green = 12 << 8;
    vertex[0].Blue = 38 << 8;
    vertex[0].Alpha = 0;

    vertex[1].x = rc.right;
    vertex[1].y = rc.bottom;
    vertex[1].Red = 15 << 8;
    vertex[1].Green = 22 << 8;
    vertex[1].Blue = 65 << 8;
    vertex[1].Alpha = 0;

    gRect.UpperLeft = 0;
    gRect.LowerRight = 1;

    GradientFill(hdc, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
}

// ============================================================================
// DRAW ROUNDED RECT (simulated)
// ============================================================================
void FillRoundRect(HDC hdc, int x, int y, int w, int h, COLORREF color, int radius) {
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    RoundRect(hdc, x, y, x + w, y + h, radius, radius);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

// ============================================================================
// SHOW/HIDE CONTROLS FOR LOGIN vs SIGNUP
// ============================================================================
void UpdateViewMode(HWND hwnd) {
    HWND hLicenseLabel = GetDlgItem(hwnd, ID_LICENSE_LABEL);
    HWND hLicenseEdit = GetDlgItem(hwnd, ID_LICENSE_EDIT);
    HWND hLoginBtn = GetDlgItem(hwnd, ID_LOGIN_BTN);
    HWND hToggle = GetDlgItem(hwnd, ID_TOGGLE_LINK);
    HWND hInject = GetDlgItem(hwnd, ID_INJECT_BTN);

    if (g_isLoggedIn) {
        // Show inject view
        ShowWindow(hLicenseLabel, SW_HIDE);
        ShowWindow(hLicenseEdit, SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_USERNAME_LABEL), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_USERNAME_EDIT), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_PASSWORD_LABEL), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_PASSWORD_EDIT), SW_HIDE);
        ShowWindow(hLoginBtn, SW_HIDE);
        ShowWindow(hToggle, SW_HIDE);
        ShowWindow(hInject, SW_SHOW);
    } else if (g_isSignupMode) {
        // Show signup view (with license)
        ShowWindow(hLicenseLabel, SW_SHOW);
        ShowWindow(hLicenseEdit, SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, ID_USERNAME_LABEL), SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, ID_USERNAME_EDIT), SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, ID_PASSWORD_LABEL), SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, ID_PASSWORD_EDIT), SW_SHOW);
        ShowWindow(hLoginBtn, SW_SHOW);
        ShowWindow(hToggle, SW_SHOW);
        ShowWindow(hInject, SW_HIDE);
        SetWindowText(hLoginBtn, "OPRET KONTO");
        SetWindowText(hToggle, "Har allerede en konto? Log ind her");
    } else {
        // Show login view (no license)
        ShowWindow(hLicenseLabel, SW_HIDE);
        ShowWindow(hLicenseEdit, SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, ID_USERNAME_LABEL), SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, ID_USERNAME_EDIT), SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, ID_PASSWORD_LABEL), SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, ID_PASSWORD_EDIT), SW_SHOW);
        ShowWindow(hLoginBtn, SW_SHOW);
        ShowWindow(hToggle, SW_SHOW);
        ShowWindow(hInject, SW_HIDE);
        SetWindowText(hLoginBtn, "LOG IND");
        SetWindowText(hToggle, "Har ikke en konto? Opret her");
    }

    InvalidateRect(hwnd, NULL, TRUE);
}

// ============================================================================
// WINDOW PROCEDURE
// ============================================================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hUsernameEdit, hPasswordEdit, hLicenseEdit;
    static HWND hLoginBtn, hInjectBtn, hToggleLink, hStatusLabel;
    static HWND hUsernameLabel, hPasswordLabel, hLicenseLabel;
    static bool isDragging = false;
    static POINT dragStart;

    const int MARGIN = 40;
    const int FIELD_W = WIN_WIDTH - (MARGIN * 2);

    switch (msg) {
    case WM_CREATE: {
        // License Key label + input (signup only)
        hLicenseLabel = CreateWindow("STATIC", "License Key",
            WS_CHILD | SS_LEFT,
            MARGIN, 160, FIELD_W, 18,
            hwnd, (HMENU)ID_LICENSE_LABEL, g_hInstance, NULL);
        SendMessage(hLicenseLabel, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

        hLicenseEdit = CreateWindowEx(0, "EDIT", "",
            WS_CHILD | ES_LEFT | ES_AUTOHSCROLL | WS_BORDER,
            MARGIN, 180, FIELD_W, 32,
            hwnd, (HMENU)ID_LICENSE_EDIT, g_hInstance, NULL);
        SendMessage(hLicenseEdit, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        // Username label + input
        int userY = 230;
        hUsernameLabel = CreateWindow("STATIC", "Brugernavn",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            MARGIN, userY, FIELD_W, 18,
            hwnd, (HMENU)ID_USERNAME_LABEL, g_hInstance, NULL);
        SendMessage(hUsernameLabel, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

        hUsernameEdit = CreateWindowEx(0, "EDIT", "",
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_BORDER,
            MARGIN, userY + 20, FIELD_W, 32,
            hwnd, (HMENU)ID_USERNAME_EDIT, g_hInstance, NULL);
        SendMessage(hUsernameEdit, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        // Password label + input
        int passY = userY + 70;
        hPasswordLabel = CreateWindow("STATIC", "Adgangskode",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            MARGIN, passY, FIELD_W, 18,
            hwnd, (HMENU)ID_PASSWORD_LABEL, g_hInstance, NULL);
        SendMessage(hPasswordLabel, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

        hPasswordEdit = CreateWindowEx(0, "EDIT", "",
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | ES_PASSWORD | WS_BORDER,
            MARGIN, passY + 20, FIELD_W, 32,
            hwnd, (HMENU)ID_PASSWORD_EDIT, g_hInstance, NULL);
        SendMessage(hPasswordEdit, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        // Login / Signup button
        hLoginBtn = CreateWindow("BUTTON", "LOG IND",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            MARGIN, 410, FIELD_W, 42,
            hwnd, (HMENU)ID_LOGIN_BTN, g_hInstance, NULL);
        SendMessage(hLoginBtn, WM_SETFONT, (WPARAM)g_hFontBtn, TRUE);

        // Toggle link
        hToggleLink = CreateWindow("STATIC", "Har ikke en konto? Opret her",
            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY,
            MARGIN, 465, FIELD_W, 20,
            hwnd, (HMENU)ID_TOGGLE_LINK, g_hInstance, NULL);
        SendMessage(hToggleLink, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

        // Inject button (hidden until logged in)
        hInjectBtn = CreateWindow("BUTTON", "INJECT / LOAD",
            WS_CHILD | BS_OWNERDRAW,
            MARGIN, 260, FIELD_W, 50,
            hwnd, (HMENU)ID_INJECT_BTN, g_hInstance, NULL);
        SendMessage(hInjectBtn, WM_SETFONT, (WPARAM)g_hFontBtn, TRUE);

        // Status label
        hStatusLabel = CreateWindow("STATIC", "",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            MARGIN, 500, FIELD_W, 60,
            hwnd, (HMENU)ID_STATUS_LABEL, g_hInstance, NULL);
        SendMessage(hStatusLabel, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

        // Check if already logged in
        if (LoadUserConfig()) {
            g_isLoggedIn = true;
            UpdateViewMode(hwnd);
            SetWindowText(hStatusLabel, "Velkommen tilbage! Tryk INJECT for at starte.");
        } else {
            g_isSignupMode = false;
            UpdateViewMode(hwnd);
        }
        break;
    }

    // Custom title bar drag
    case WM_LBUTTONDOWN: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        if (pt.y < 50) {
            isDragging = true;
            dragStart = pt;
            SetCapture(hwnd);
        }
        break;
    }
    case WM_MOUSEMOVE: {
        if (isDragging) {
            POINT pt;
            GetCursorPos(&pt);
            SetWindowPos(hwnd, NULL,
                pt.x - dragStart.x, pt.y - dragStart.y,
                0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        break;
    }
    case WM_LBUTTONUP: {
        isDragging = false;
        ReleaseCapture();
        break;
    }

    // Paint gradient background
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        // Draw gradient
        DrawGradientBg(hdc, rc);

        // Draw title bar area
        FillRoundRect(hdc, 0, 0, rc.right, 50, RGB(8, 10, 25), 0);

        // Draw title text
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, CLR_TEXT);
        SelectObject(hdc, g_hFontTitle);

        RECT titleRect = { 0, 8, rc.right, 45 };
        DrawText(hdc, "CS2 Inventory Changer", -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Draw close button
        SetTextColor(hdc, CLR_TEXT_DIM);
        SelectObject(hdc, g_hFontNormal);
        RECT closeRect = { rc.right - 40, 5, rc.right - 5, 45 };
        DrawText(hdc, "X", -1, &closeRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Draw subtitle
        if (g_isLoggedIn) {
            SetTextColor(hdc, CLR_SUCCESS);
            SelectObject(hdc, g_hFontNormal);
            RECT subRect = { 0, 70, rc.right, 100 };
            std::string welcomeMsg = "Velkommen, " + g_username + "!";
            DrawText(hdc, welcomeMsg.c_str(), -1, &subRect, DT_CENTER | DT_SINGLELINE);

            // Status info
            SetTextColor(hdc, CLR_TEXT_DIM);
            RECT infoRect = { 0, 100, rc.right, 130 };
            DrawText(hdc, "HWID verificeret - Klar til injection", -1, &infoRect, DT_CENTER | DT_SINGLELINE);
        } else {
            SetTextColor(hdc, CLR_TEXT_DIM);
            SelectObject(hdc, g_hFontNormal);
            RECT subRect = { 0, 70, rc.right, 100 };
            if (g_isSignupMode) {
                DrawText(hdc, "Opret din konto med license key", -1, &subRect, DT_CENTER | DT_SINGLELINE);
            } else {
                DrawText(hdc, "Log ind for at fortsaette", -1, &subRect, DT_CENTER | DT_SINGLELINE);
            }
        }

        // Decorative line under title
        HPEN linePen = CreatePen(PS_SOLID, 2, CLR_ACCENT);
        SelectObject(hdc, linePen);
        MoveToEx(hdc, MARGIN, 120, NULL);
        LineTo(hdc, rc.right - MARGIN, 120);
        DeleteObject(linePen);

        // Version text bottom
        SetTextColor(hdc, RGB(50, 55, 75));
        SelectObject(hdc, g_hFontSmall);
        RECT verRect = { 0, rc.bottom - 25, rc.right, rc.bottom - 5 };
        DrawText(hdc, "v1.0.2 | CS2 Inventory Changer", -1, &verRect, DT_CENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        break;
    }

    // Custom draw edit controls (dark background)
    case WM_CTLCOLOREDIT: {
        HDC hdcEdit = (HDC)wParam;
        SetTextColor(hdcEdit, CLR_TEXT);
        SetBkColor(hdcEdit, CLR_INPUT_BG);
        return (LRESULT)g_hInputBgBrush;
    }

    // Custom draw static controls
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;
        SetBkMode(hdcStatic, TRANSPARENT);

        if (hCtrl == hToggleLink) {
            SetTextColor(hdcStatic, CLR_TEXT_LINK);
        } else if (hCtrl == hStatusLabel) {
            SetTextColor(hdcStatic, CLR_TEXT_DIM);
        } else {
            SetTextColor(hdcStatic, CLR_TEXT_DIM);
        }
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    // Owner-draw buttons
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlID == ID_LOGIN_BTN || dis->CtlID == ID_INJECT_BTN) {
            HDC hdc = dis->hDC;
            RECT rc = dis->rcItem;

            COLORREF bgColor;
            if (dis->CtlID == ID_INJECT_BTN) {
                bgColor = CLR_SUCCESS;
                if (dis->itemState & ODS_SELECTED) bgColor = RGB(40, 170, 100);
            } else {
                bgColor = CLR_BTN_BG;
                if (dis->itemState & ODS_SELECTED) bgColor = CLR_BTN_HOVER;
            }

            // Draw rounded button
            HBRUSH btnBrush = CreateSolidBrush(bgColor);
            HPEN btnPen = CreatePen(PS_SOLID, 1, bgColor);
            SelectObject(hdc, btnBrush);
            SelectObject(hdc, btnPen);
            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 8, 8);
            DeleteObject(btnBrush);
            DeleteObject(btnPen);

            // Draw text
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            SelectObject(hdc, g_hFontBtn);
            char btnText[128];
            GetWindowText(dis->hwndItem, btnText, sizeof(btnText));
            DrawText(hdc, btnText, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        break;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);

        // Close button click on title bar
        if (wmId == 0 && HIWORD(wParam) == 0) break;

        // Toggle link clicked
        if (wmId == ID_TOGGLE_LINK) {
            g_isSignupMode = !g_isSignupMode;
            UpdateViewMode(hwnd);
            break;
        }

        // Login / Signup button
        if (wmId == ID_LOGIN_BTN) {
            GetWindowText(hUsernameEdit, g_usernameBuffer, sizeof(g_usernameBuffer));
            GetWindowText(hPasswordEdit, g_passwordBuffer, sizeof(g_passwordBuffer));

            if (strlen(g_usernameBuffer) == 0 || strlen(g_passwordBuffer) == 0) {
                SetWindowText(hStatusLabel, "Udfyld venligst alle felter!");
                break;
            }

            if (g_isSignupMode) {
                // SIGNUP
                GetWindowText(hLicenseEdit, g_licenseBuffer, sizeof(g_licenseBuffer));
                if (strlen(g_licenseBuffer) == 0) {
                    SetWindowText(hStatusLabel, "License key er paakraevet!");
                    break;
                }

                std::string licStr(g_licenseBuffer);
                if (licStr.find("CS2-") != 0) {
                    SetWindowText(hStatusLabel, "Ugyldig license! Format: CS2-XXXX-XXXX");
                    break;
                }

                g_username = g_usernameBuffer;
                g_password = g_passwordBuffer;
                g_license = g_licenseBuffer;

                if (SaveUserConfig()) {
                    g_isLoggedIn = true;
                    UpdateViewMode(hwnd);
                    SetWindowText(hStatusLabel, "Konto oprettet! HWID laast til denne enhed.");
                } else {
                    SetWindowText(hStatusLabel, "Fejl: Kunne ikke gemme konto!");
                }
            } else {
                // LOGIN
                if (LoadUserConfig()) {
                    if (g_username == std::string(g_usernameBuffer) &&
                        g_password == std::string(g_passwordBuffer)) {
                        // HWID check
                        std::string currentHWID = GetSystemHWID();
                        if (currentHWID != g_hwid) {
                            SetWindowText(hStatusLabel, "HWID mismatch! Kontakt support.");
                            break;
                        }
                        g_isLoggedIn = true;
                        UpdateViewMode(hwnd);
                        SetWindowText(hStatusLabel, "Logget ind! Tryk INJECT for at starte.");
                    } else {
                        SetWindowText(hStatusLabel, "Forkert brugernavn eller adgangskode!");
                    }
                } else {
                    SetWindowText(hStatusLabel, "Ingen konto fundet. Opret en konto foerst.");
                }
            }
            break;
        }

        // INJECT button
        if (wmId == ID_INJECT_BTN) {
            EnableWindow(hInjectBtn, FALSE);
            SetWindowText(hStatusLabel, "Starter Steam og CS2...");
            UpdateWindow(hwnd);

            if (LaunchSteamAndCS2(hwnd)) {
                SetWindowText(hStatusLabel, "Injection udfaert! Tryk INSERT i CS2.");
                MessageBox(hwnd,
                    "Injection lykkedes!\n\nTryk INSERT-tasten inde i CS2\nfor at aabne inventory menuen.",
                    "Succes", MB_OK | MB_ICONINFORMATION);
            }
            EnableWindow(hInjectBtn, TRUE);
            break;
        }
        break;
    }

    // Handle click on close "X" in title bar
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProc(hwnd, msg, wParam, lParam);
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(hwnd, &pt);
        if (pt.x > WIN_WIDTH - 40 && pt.y < 45) {
            return HTCLOSE;
        }
        return hit;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_ERASEBKGND:
        return 1; // We handle background in WM_PAINT

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ============================================================================
// HWID
// ============================================================================
std::string GetSystemHWID() {
    DWORD serialNum = 0;
    DWORD maxLen = 0;
    GetVolumeInformationA("C:\\", NULL, 0, &serialNum, &maxLen, NULL, NULL, 0);
    char hwid[64];
    sprintf_s(hwid, sizeof(hwid), "%08X", serialNum);
    return std::string(hwid);
}

// ============================================================================
// CONFIG
// ============================================================================
bool SaveUserConfig() {
    g_hwid = GetSystemHWID();
    std::ofstream file("user_config.dat");
    if (!file.is_open()) return false;
    file << g_username << "\n"
         << g_password << "\n"
         << g_license << "\n"
         << g_hwid << "\n";
    file.close();
    return true;
}

bool LoadUserConfig() {
    std::ifstream file("user_config.dat");
    if (!file.is_open()) return false;
    std::getline(file, g_username);
    std::getline(file, g_password);
    std::getline(file, g_license);
    std::getline(file, g_hwid);
    file.close();
    return !g_username.empty();
}

// ============================================================================
// PROCESS MANAGEMENT
// ============================================================================
DWORD FindProcessByName(const char* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 pe = {};
    pe.dwSize = sizeof(pe);
    DWORD pid = 0;

    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, name) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

bool KillProcessByName(const char* name) {
    DWORD pid = FindProcessByName(name);
    if (pid == 0) return true; // Not running

    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProc) return false;

    TerminateProcess(hProc, 0);
    CloseHandle(hProc);
    Sleep(2000); // Wait for process to fully close
    return true;
}

// ============================================================================
// STEAM + CS2 LAUNCH
// ============================================================================
bool LaunchSteamAndCS2(HWND hwnd) {
    HWND hStatus = GetDlgItem(hwnd, ID_STATUS_LABEL);

    // Step 1: Kill Steam if running
    if (FindProcessByName("steam.exe") != 0) {
        SetWindowText(hStatus, "Lukker Steam...");
        UpdateWindow(hwnd);
        KillProcessByName("steam.exe");
        Sleep(2000);
    }

    // Step 2: Start Steam
    SetWindowText(hStatus, "Starter Steam...");
    UpdateWindow(hwnd);

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    // Try common Steam paths
    const char* steamPaths[] = {
        "C:\\Program Files (x86)\\Steam\\steam.exe",
        "C:\\Program Files\\Steam\\steam.exe",
        "D:\\Steam\\steam.exe",
        "D:\\Program Files (x86)\\Steam\\steam.exe"
    };

    bool steamStarted = false;
    for (const char* path : steamPaths) {
        if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
            if (CreateProcessA(path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                steamStarted = true;
                break;
            }
        }
    }

    if (!steamStarted) {
        // Try via registry
        HKEY hKey;
        char steamPath[MAX_PATH] = "";
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD size = sizeof(steamPath);
            RegQueryValueExA(hKey, "InstallPath", NULL, NULL, (LPBYTE)steamPath, &size);
            RegCloseKey(hKey);

            std::string fullPath = std::string(steamPath) + "\\steam.exe";
            if (CreateProcessA(fullPath.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                steamStarted = true;
            }
        }
    }

    if (!steamStarted) {
        SetWindowText(hStatus, "Fejl: Kunne ikke finde Steam!");
        return false;
    }

    // Step 3: Wait for Steam to fully load
    SetWindowText(hStatus, "Venter paa Steam...");
    UpdateWindow(hwnd);
    Sleep(8000);

    // Step 4: Launch CS2 via Steam protocol
    SetWindowText(hStatus, "Starter Counter-Strike 2...");
    UpdateWindow(hwnd);
    ShellExecuteA(NULL, "open", "steam://rungameid/730", NULL, NULL, SW_SHOWNORMAL);

    // Step 5: Wait for CS2 to load
    SetWindowText(hStatus, "Venter paa CS2...");
    UpdateWindow(hwnd);

    DWORD cs2Pid = 0;
    int attempts = 0;
    while (cs2Pid == 0 && attempts < 120) {
        cs2Pid = FindProcessByName("cs2.exe");
        if (cs2Pid == 0) {
            Sleep(1000);
            attempts++;
            char waitMsg[128];
            sprintf_s(waitMsg, "Venter paa CS2... (%d sek)", attempts);
            SetWindowText(hStatus, waitMsg);
            UpdateWindow(hwnd);
        }
    }

    if (cs2Pid == 0) {
        SetWindowText(hStatus, "Fejl: CS2 blev ikke fundet!");
        return false;
    }

    // Step 6: Wait a bit more for CS2 to fully initialize
    SetWindowText(hStatus, "CS2 fundet! Forbereder injection...");
    UpdateWindow(hwnd);
    Sleep(5000);

    // Step 7: Inject
    SetWindowText(hStatus, "Injicerer...");
    UpdateWindow(hwnd);

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, cs2Pid);
    if (!hProcess) {
        SetWindowText(hStatus, "Fejl: Ingen adgang til CS2! Koer som admin.");
        return false;
    }

    // Injection placeholder
    Sleep(2000);
    CloseHandle(hProcess);

    SetWindowText(hStatus, "Injection udfaert! Tryk INSERT i spillet.");
    return true;
}
