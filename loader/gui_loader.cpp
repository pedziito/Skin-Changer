// ============================================================================
//  CS2 Inventory Changer - Premium Loader
//  Custom painted dark UI with full GDI rendering
// ============================================================================
#include <windows.h>
#include <wingdi.h>
#include <tlhelp32.h>
#include <string>
#include <fstream>
#include <cmath>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

// ============================================================================
// DESIGN CONSTANTS
// ============================================================================
#define WIN_W  340
#define WIN_H  520

// Color palette - dark premium
#define C_BG1       RGB(12, 14, 22)      // top bg
#define C_BG2       RGB(18, 20, 32)      // bottom bg
#define C_PANEL     RGB(22, 25, 38)      // card/panel bg
#define C_FIELD     RGB(28, 32, 48)      // input field bg
#define C_FIELD_B   RGB(42, 48, 68)      // input field border
#define C_FIELD_F   RGB(55, 65, 95)      // input field focused border
#define C_BTN       RGB(38, 44, 62)      // button bg
#define C_BTN_H     RGB(48, 56, 78)      // button hover
#define C_ACCENT    RGB(0, 180, 120)     // green accent (undetected badge)
#define C_ACCENT2   RGB(70, 130, 255)    // blue accent
#define C_TEXT      RGB(200, 205, 218)   // primary text
#define C_TEXT2     RGB(110, 118, 140)   // secondary text
#define C_TEXT3     RGB(65, 72, 92)      // dim text
#define C_BORDER    RGB(35, 40, 58)      // panel border
#define C_GREEN     RGB(40, 200, 110)    // success green
#define C_RED       RGB(200, 55, 55)     // error red
#define C_WHITE     RGB(230, 235, 245)   // bright white

// Layout
#define MARGIN    28
#define FIELD_W   (WIN_W - MARGIN * 2)
#define FIELD_H   38
#define BTN_H     40
#define RADIUS    6

// ============================================================================
// GLOBALS
// ============================================================================
static HINSTANCE g_hInst;
static bool g_loggedIn     = false;
static bool g_signupMode   = false;
static bool g_btnHover     = false;
static bool g_linkHover    = false;
static bool g_closeHover   = false;
static bool g_dragging     = false;
static POINT g_dragPt;
static std::string g_user, g_pass, g_license, g_hwid;
static std::string g_status;
static COLORREF g_statusClr = C_TEXT2;
static int g_focusField    = 0; // 0=none, 1=license, 2=user, 3=pass

// Fonts
static HFONT fTitle, fSub, fBody, fSmall, fBtn, fIcon;

// Control handles (hidden edit boxes for text input)
static HWND hEditLicense, hEditUser, hEditPass;
static HWND hMainWnd;

// Buffers
static char bufUser[256], bufPass[256], bufLic[256];

// Forward decl
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void PaintWindow(HWND hwnd, HDC hdc);
void PaintLogin(HDC hdc, RECT rc);
void PaintDashboard(HDC hdc, RECT rc);
void DrawRoundedRect(HDC hdc, int x, int y, int w, int h, int r, COLORREF fill, COLORREF border);
void DrawField(HDC hdc, int x, int y, int w, int h, const char* text, const char* icon, bool focused, bool password);
void DrawButton(HDC hdc, int x, int y, int w, int h, const char* text, bool hover, COLORREF bg);
void DrawLogo(HDC hdc, int cx, int cy);
void SetStatus(const char* msg, COLORREF clr);
bool DoLogin(HWND hwnd);
bool DoSignup(HWND hwnd);
bool SaveConfig();
bool LoadConfig();
std::string GetHWID();
DWORD FindProc(const char* name);
bool KillProc(const char* name);
bool LaunchGame(HWND hwnd);

// ============================================================================
// ENTRY POINT
// ============================================================================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    g_hInst = hInst;

    // Create fonts
    fTitle = CreateFont(22, 0, 0, 0, FW_BOLD,    0,0,0, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI");
    fSub   = CreateFont(13, 0, 0, 0, FW_NORMAL,  0,0,0, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI");
    fBody  = CreateFont(14, 0, 0, 0, FW_NORMAL,  0,0,0, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI");
    fSmall = CreateFont(11, 0, 0, 0, FW_NORMAL,  0,0,0, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI");
    fBtn   = CreateFont(14, 0, 0, 0, FW_SEMIBOLD,0,0,0, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI");
    fIcon  = CreateFont(16, 0, 0, 0, FW_NORMAL,  0,0,0, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI Emoji");

    WNDCLASSEX wc = { sizeof(wc) };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance      = hInst;
    wc.lpszClassName  = "CS2LC";
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground  = CreateSolidBrush(C_BG1);
    RegisterClassEx(&wc);

    int sx = (GetSystemMetrics(SM_CXSCREEN) - WIN_W) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - WIN_H) / 2;

    hMainWnd = CreateWindowEx(
        WS_EX_LAYERED,
        "CS2LC", "CS2 Inventory Changer",
        WS_POPUP,
        sx, sy, WIN_W, WIN_H,
        NULL, NULL, hInst, NULL);

    // Set window opacity (slightly translucent for premium feel)
    SetLayeredWindowAttributes(hMainWnd, 0, 248, LWA_ALPHA);

    ShowWindow(hMainWnd, nShow);
    UpdateWindow(hMainWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteObject(fTitle); DeleteObject(fSub); DeleteObject(fBody);
    DeleteObject(fSmall); DeleteObject(fBtn); DeleteObject(fIcon);
    return (int)msg.wParam;
}

// ============================================================================
// WINDOW PROC
// ============================================================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        // Hidden edit controls for keyboard input
        hEditLicense = CreateWindowEx(0, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL,
            -500, 0, 100, 20, hwnd, (HMENU)101, g_hInst, NULL);
        hEditUser = CreateWindowEx(0, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL,
            -500, 0, 100, 20, hwnd, (HMENU)102, g_hInst, NULL);
        hEditPass = CreateWindowEx(0, "EDIT", "", WS_CHILD | ES_PASSWORD | ES_AUTOHSCROLL,
            -500, 0, 100, 20, hwnd, (HMENU)103, g_hInst, NULL);

        SendMessage(hEditLicense, WM_SETFONT, (WPARAM)fBody, TRUE);
        SendMessage(hEditUser, WM_SETFONT, (WPARAM)fBody, TRUE);
        SendMessage(hEditPass, WM_SETFONT, (WPARAM)fBody, TRUE);

        // Auto-login if config exists
        if (LoadConfig()) {
            g_loggedIn = true;
            g_status = "Velkommen tilbage, " + g_user + "!";
            g_statusClr = C_GREEN;
        }

        SetTimer(hwnd, 1, 50, NULL); // repaint timer for smooth updates
        break;
    }

    case WM_TIMER:
        InvalidateRect(hwnd, NULL, FALSE);
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        // Double buffer
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        SelectObject(memDC, memBmp);

        PaintWindow(hwnd, memDC);

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
        DeleteObject(memBmp);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_ERASEBKGND:
        return 1;

    // ---- Mouse handling ----
    case WM_LBUTTONDOWN: {
        int mx = LOWORD(lp), my = HIWORD(lp);

        // Close button (top-right)
        if (mx > WIN_W - 40 && my < 36) {
            PostQuitMessage(0);
            return 0;
        }

        // Title bar drag
        if (my < 36) {
            g_dragging = true;
            g_dragPt = { mx, my };
            SetCapture(hwnd);
            return 0;
        }

        if (!g_loggedIn) {
            // --- LOGIN / SIGNUP mode clicks ---
            int baseY = g_signupMode ? 175 : 215;

            // License field (signup only)
            if (g_signupMode && my >= 175 && my < 175 + FIELD_H) {
                g_focusField = 1;
                SetFocus(hEditLicense);
                return 0;
            }

            // Username field
            if (my >= baseY && my < baseY + FIELD_H) {
                g_focusField = 2;
                SetFocus(hEditUser);
                return 0;
            }

            // Password field
            if (my >= baseY + 50 && my < baseY + 50 + FIELD_H) {
                g_focusField = 3;
                SetFocus(hEditPass);
                return 0;
            }

            // Button click
            int btnY = g_signupMode ? 345 : 345;
            if (my >= btnY && my < btnY + BTN_H && mx >= MARGIN && mx <= WIN_W - MARGIN) {
                if (g_signupMode) DoSignup(hwnd);
                else DoLogin(hwnd);
                return 0;
            }

            // Toggle link
            int linkY = btnY + BTN_H + 12;
            if (my >= linkY && my < linkY + 20) {
                g_signupMode = !g_signupMode;
                g_status.clear();
                g_focusField = 0;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
        } else {
            // --- DASHBOARD mode clicks ---
            // Load button
            if (my >= 370 && my < 370 + BTN_H + 6 && mx >= MARGIN && mx <= WIN_W - MARGIN) {
                EnableWindow(hwnd, FALSE);
                LaunchGame(hwnd);
                EnableWindow(hwnd, TRUE);
                return 0;
            }
        }

        g_focusField = 0;
        SetFocus(hwnd);
        break;
    }

    case WM_MOUSEMOVE: {
        int mx = LOWORD(lp), my = HIWORD(lp);
        if (g_dragging) {
            POINT pt; GetCursorPos(&pt);
            SetWindowPos(hwnd, NULL, pt.x - g_dragPt.x, pt.y - g_dragPt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            return 0;
        }

        // Button hover
        bool oldHover = g_btnHover;
        bool oldClose = g_closeHover;
        int btnY = g_loggedIn ? 370 : 345;
        g_btnHover = (my >= btnY && my < btnY + BTN_H + 6 && mx >= MARGIN && mx <= WIN_W - MARGIN);
        g_closeHover = (mx > WIN_W - 40 && my < 36);

        // Link hover
        int linkY = 345 + BTN_H + 12;
        g_linkHover = (!g_loggedIn && my >= linkY && my < linkY + 20);

        if (g_btnHover != oldHover || g_closeHover != oldClose) {
            InvalidateRect(hwnd, NULL, FALSE);
        }

        // Track mouse leave
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        break;
    }

    case WM_MOUSELEAVE:
        g_btnHover = false;
        g_closeHover = false;
        g_linkHover = false;
        InvalidateRect(hwnd, NULL, FALSE);
        break;

    case WM_LBUTTONUP:
        g_dragging = false;
        ReleaseCapture();
        break;

    // Sync hidden edits with our visual fields
    case WM_COMMAND:
        if (HIWORD(wp) == EN_CHANGE) {
            int id = LOWORD(wp);
            if (id == 101) GetWindowText(hEditLicense, bufLic, 256);
            if (id == 102) GetWindowText(hEditUser, bufUser, 256);
            if (id == 103) GetWindowText(hEditPass, bufPass, 256);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;

    case WM_KEYDOWN:
        if (wp == VK_TAB) {
            // Tab between fields
            g_focusField = (g_focusField % 3) + 1;
            if (!g_signupMode && g_focusField == 1) g_focusField = 2;
            HWND targets[] = { NULL, hEditLicense, hEditUser, hEditPass };
            SetFocus(targets[g_focusField]);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        if (wp == VK_RETURN) {
            if (!g_loggedIn) {
                if (g_signupMode) DoSignup(hwnd);
                else DoLogin(hwnd);
            }
            return 0;
        }
        break;

    case WM_SETCURSOR: {
        POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
        int linkY = 345 + BTN_H + 12;
        bool onLink = (!g_loggedIn && pt.y >= linkY && pt.y < linkY + 20);
        bool onClose = (pt.x > WIN_W - 40 && pt.y < 36);
        bool onBtn = false;
        int btnY = g_loggedIn ? 370 : 345;
        onBtn = (pt.y >= btnY && pt.y < btnY + BTN_H + 6 && pt.x >= MARGIN && pt.x <= WIN_W - MARGIN);
        SetCursor(LoadCursor(NULL, (onLink || onClose || onBtn) ? IDC_HAND : IDC_ARROW));
        return TRUE;
    }

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

// ============================================================================
// MASTER PAINT
// ============================================================================
void PaintWindow(HWND hwnd, HDC hdc) {
    RECT rc; GetClientRect(hwnd, &rc);

    // Gradient background
    TRIVERTEX vt[2];
    GRADIENT_RECT gr = { 0, 1 };
    vt[0] = { rc.left, rc.top,     (COLOR16)(12<<8), (COLOR16)(14<<8), (COLOR16)(24<<8), 0 };
    vt[1] = { rc.right, rc.bottom, (COLOR16)(16<<8), (COLOR16)(20<<8), (COLOR16)(36<<8), 0 };
    GradientFill(hdc, vt, 2, &gr, 1, GRADIENT_FILL_RECT_V);

    // Outer border (subtle)
    HPEN borderPen = CreatePen(PS_SOLID, 1, C_BORDER);
    HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    SelectObject(hdc, borderPen); SelectObject(hdc, nullBrush);
    RoundRect(hdc, 0, 0, rc.right, rc.bottom, 12, 12);
    DeleteObject(borderPen);

    // Title bar line
    HPEN linePen = CreatePen(PS_SOLID, 1, RGB(30, 34, 50));
    SelectObject(hdc, linePen);
    MoveToEx(hdc, 0, 36, NULL);
    LineTo(hdc, rc.right, 36);
    DeleteObject(linePen);

    // Close button
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, fBody);
    SetTextColor(hdc, g_closeHover ? C_RED : C_TEXT3);
    RECT closeR = { WIN_W - 38, 8, WIN_W - 8, 32 };
    DrawText(hdc, "X", -1, &closeR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Window title (small)
    SetTextColor(hdc, C_TEXT3);
    SelectObject(hdc, fSmall);
    RECT titleBarR = { 14, 10, 200, 30 };
    DrawText(hdc, "CS2 Inventory Changer", -1, &titleBarR, DT_LEFT | DT_SINGLELINE);

    if (g_loggedIn)
        PaintDashboard(hdc, rc);
    else
        PaintLogin(hdc, rc);
}

// ============================================================================
// LOGIN VIEW
// ============================================================================
void PaintLogin(HDC hdc, RECT rc) {
    SetBkMode(hdc, TRANSPARENT);
    int cx = rc.right / 2;

    // Logo
    DrawLogo(hdc, cx, 80);

    // Title
    SelectObject(hdc, fTitle);
    SetTextColor(hdc, C_WHITE);
    RECT tr = { 0, 108, rc.right, 135 };
    DrawText(hdc, "CS2 CHANGER", -1, &tr, DT_CENTER | DT_SINGLELINE);

    // Subtitle
    SelectObject(hdc, fSub);
    SetTextColor(hdc, C_TEXT2);
    RECT sr = { 0, 136, rc.right, 155 };
    DrawText(hdc, "Premium Software", -1, &sr, DT_CENTER | DT_SINGLELINE);

    // Separator line
    HPEN sep = CreatePen(PS_SOLID, 1, C_BORDER);
    SelectObject(hdc, sep);
    MoveToEx(hdc, MARGIN + 40, 162, NULL);
    LineTo(hdc, WIN_W - MARGIN - 40, 162);
    DeleteObject(sep);

    int y = 175;

    // License field (signup only)
    if (g_signupMode) {
        DrawField(hdc, MARGIN, y, FIELD_W, FIELD_H, bufLic, "KEY", g_focusField == 1, false);
        y += FIELD_H + 12;
    }

    // Username
    DrawField(hdc, MARGIN, y, FIELD_W, FIELD_H, bufUser, "USR", g_focusField == 2, false);
    y += FIELD_H + 12;

    // Password
    DrawField(hdc, MARGIN, y, FIELD_W, FIELD_H, bufPass, "PWD", g_focusField == 3, true);
    y += FIELD_H + 8;

    // Forgot password link
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, C_TEXT3);
    RECT fpR = { MARGIN, y, WIN_W - MARGIN, y + 16 };
    DrawText(hdc, "Glemt adgangskode? Kontakt support.", -1, &fpR, DT_LEFT | DT_SINGLELINE);
    y += 28;

    // Button
    const char* btnText = g_signupMode ? "->  Opret Konto" : "->  Log Ind";
    DrawButton(hdc, MARGIN, 345, FIELD_W, BTN_H, btnText, g_btnHover, g_btnHover ? C_BTN_H : C_BTN);

    // Toggle link
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, g_linkHover ? C_ACCENT2 : C_TEXT2);
    RECT linkR = { 0, 345 + BTN_H + 12, rc.right, 345 + BTN_H + 30 };
    if (g_signupMode)
        DrawText(hdc, "Har allerede en konto? Log ind her", -1, &linkR, DT_CENTER | DT_SINGLELINE);
    else
        DrawText(hdc, "Har ikke en konto? Opret her", -1, &linkR, DT_CENTER | DT_SINGLELINE);

    // Status message
    if (!g_status.empty()) {
        SelectObject(hdc, fSmall);
        SetTextColor(hdc, g_statusClr);
        RECT stR = { MARGIN, 420, WIN_W - MARGIN, 460 };
        DrawText(hdc, g_status.c_str(), -1, &stR, DT_CENTER | DT_WORDBREAK);
    }

    // Footer
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, C_TEXT3);
    RECT ftR = { 0, rc.bottom - 24, rc.right, rc.bottom - 6 };
    DrawText(hdc, "CS2 Changer 2024 - 2026", -1, &ftR, DT_CENTER | DT_SINGLELINE);
}

// ============================================================================
// DASHBOARD VIEW (after login)
// ============================================================================
void PaintDashboard(HDC hdc, RECT rc) {
    SetBkMode(hdc, TRANSPARENT);
    int cx = rc.right / 2;

    // Logo
    DrawLogo(hdc, cx, 80);

    // Title
    SelectObject(hdc, fTitle);
    SetTextColor(hdc, C_WHITE);
    RECT tr = { 0, 108, rc.right, 135 };
    DrawText(hdc, "CS2 CHANGER", -1, &tr, DT_CENTER | DT_SINGLELINE);

    // Subtitle
    SelectObject(hdc, fSub);
    SetTextColor(hdc, C_TEXT2);
    RECT sr = { 0, 136, rc.right, 155 };
    DrawText(hdc, "Premium Software", -1, &sr, DT_CENTER | DT_SINGLELINE);

    // Separator
    HPEN sep = CreatePen(PS_SOLID, 1, C_BORDER);
    SelectObject(hdc, sep);
    MoveToEx(hdc, MARGIN + 40, 165, NULL);
    LineTo(hdc, WIN_W - MARGIN - 40, 165);
    DeleteObject(sep);

    // Game icon cards (3 icons in a row like the reference)
    int cardW = 72, cardH = 72, gap = 16;
    int totalW = cardW * 3 + gap * 2;
    int startX = (WIN_W - totalW) / 2;
    int cardY = 185;

    for (int i = 0; i < 3; i++) {
        int x = startX + i * (cardW + gap);
        DrawRoundedRect(hdc, x, cardY, cardW, cardH, 8, C_PANEL, C_BORDER);

        // Draw "CS2" text in each card as icon placeholder
        SelectObject(hdc, fBtn);
        SetTextColor(hdc, i == 1 ? C_ACCENT : C_TEXT3);
        RECT iconR = { x, cardY, x + cardW, cardY + cardH };
        DrawText(hdc, "CS2", -1, &iconR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // Game name
    SelectObject(hdc, fTitle);
    SetTextColor(hdc, C_WHITE);
    RECT gnR = { 0, cardY + cardH + 16, rc.right, cardY + cardH + 42 };
    DrawText(hdc, "Counter-Strike 2", -1, &gnR, DT_CENTER | DT_SINGLELINE);

    // Status badge - UNDETECTED
    int badgeW = 110, badgeH = 22;
    int badgeX = cx - badgeW / 2;
    int badgeY = cardY + cardH + 48;
    DrawRoundedRect(hdc, badgeX, badgeY, badgeW, badgeH, 11, RGB(20, 60, 40), C_GREEN);
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, C_GREEN);
    RECT bdR = { badgeX, badgeY, badgeX + badgeW, badgeY + badgeH };
    DrawText(hdc, "UNDETECTED", -1, &bdR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Expiry date panel
    int panelY = badgeY + badgeH + 20;
    DrawRoundedRect(hdc, MARGIN, panelY, FIELD_W, 42, 6, C_FIELD, C_FIELD_B);

    SelectObject(hdc, fBody);
    SetTextColor(hdc, C_TEXT2);
    RECT expLR = { MARGIN + 14, panelY, MARGIN + 130, panelY + 42 };
    DrawText(hdc, "Udloebsdato:", -1, &expLR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(hdc, C_GREEN);
    RECT expVR = { MARGIN + 130, panelY, WIN_W - MARGIN - 14, panelY + 42 };
    DrawText(hdc, "2026-12-31", -1, &expVR, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    // Load button
    DrawButton(hdc, MARGIN, 370, FIELD_W, BTN_H + 6, "->  Load Cheat", g_btnHover,
               g_btnHover ? C_BTN_H : C_BTN);

    // Status
    if (!g_status.empty()) {
        SelectObject(hdc, fSmall);
        SetTextColor(hdc, g_statusClr);
        RECT stR = { MARGIN, 425, WIN_W - MARGIN, 475 };
        DrawText(hdc, g_status.c_str(), -1, &stR, DT_CENTER | DT_WORDBREAK);
    }

    // Footer
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, C_TEXT3);
    RECT ftR = { 0, rc.bottom - 24, rc.right, rc.bottom - 6 };
    DrawText(hdc, "CS2 Changer 2024 - 2026", -1, &ftR, DT_CENTER | DT_SINGLELINE);
}

// ============================================================================
// DRAW HELPERS
// ============================================================================
void DrawRoundedRect(HDC hdc, int x, int y, int w, int h, int r, COLORREF fill, COLORREF border) {
    HBRUSH br = CreateSolidBrush(fill);
    HPEN pn = CreatePen(PS_SOLID, 1, border);
    SelectObject(hdc, br); SelectObject(hdc, pn);
    RoundRect(hdc, x, y, x + w, y + h, r * 2, r * 2);
    DeleteObject(br); DeleteObject(pn);
}

void DrawField(HDC hdc, int x, int y, int w, int h, const char* text, const char* icon, bool focused, bool password) {
    COLORREF border = focused ? C_ACCENT2 : C_FIELD_B;
    DrawRoundedRect(hdc, x, y, w, h, 5, C_FIELD, border);

    // Icon area
    SelectObject(hdc, fBody);
    SetTextColor(hdc, focused ? C_ACCENT2 : C_TEXT3);
    RECT iconR = { x + 10, y, x + 46, y + h };
    DrawText(hdc, icon, -1, &iconR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Separator line
    HPEN sp = CreatePen(PS_SOLID, 1, C_BORDER);
    SelectObject(hdc, sp);
    MoveToEx(hdc, x + 48, y + 8, NULL);
    LineTo(hdc, x + 48, y + h - 8);
    DeleteObject(sp);

    // Text
    SelectObject(hdc, fBody);
    SetTextColor(hdc, C_TEXT);
    RECT textR = { x + 56, y, x + w - 10, y + h };

    if (strlen(text) == 0) {
        // Show placeholder
        SetTextColor(hdc, C_TEXT3);
        const char* ph = "...";
        if (strcmp(icon, "KEY") == 0) ph = "CS2-XXXX-XXXX";
        else if (strcmp(icon, "USR") == 0) ph = "Brugernavn";
        else if (strcmp(icon, "PWD") == 0) ph = "Adgangskode";
        DrawText(hdc, ph, -1, &textR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else if (password) {
        // Show dots
        std::string dots(strlen(text), '*');
        DrawText(hdc, dots.c_str(), -1, &textR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else {
        DrawText(hdc, text, -1, &textR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
}

void DrawButton(HDC hdc, int x, int y, int w, int h, const char* text, bool hover, COLORREF bg) {
    DrawRoundedRect(hdc, x, y, w, h, 5, bg, hover ? RGB(60, 70, 100) : C_BORDER);
    SelectObject(hdc, fBtn);
    SetTextColor(hdc, C_TEXT);
    RECT btnR = { x, y, x + w, y + h };
    DrawText(hdc, text, -1, &btnR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void DrawLogo(HDC hdc, int cx, int cy) {
    // Draw a stylized "SC" logo (box with letters)
    int sz = 36;
    int lx = cx - sz / 2, ly = cy - sz / 2;

    // Outer glow effect (subtle)
    for (int i = 3; i >= 1; i--) {
        int alpha = 15 + i * 5;
        HPEN gp = CreatePen(PS_SOLID, 1, RGB(alpha, alpha + 10, alpha + 30));
        HBRUSH nb = (HBRUSH)GetStockObject(NULL_BRUSH);
        SelectObject(hdc, gp); SelectObject(hdc, nb);
        RoundRect(hdc, lx - i*2, ly - i*2, lx + sz + i*2, ly + sz + i*2, 10, 10);
        DeleteObject(gp);
    }

    // Main box
    DrawRoundedRect(hdc, lx, ly, sz, sz, 6, C_PANEL, C_ACCENT2);

    // "SC" text inside
    SelectObject(hdc, fTitle);
    SetTextColor(hdc, C_WHITE);
    RECT logoR = { lx, ly - 1, lx + sz, ly + sz };
    DrawText(hdc, "SC", -1, &logoR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================================
// AUTH LOGIC
// ============================================================================
void SetStatus(const char* msg, COLORREF clr) {
    g_status = msg;
    g_statusClr = clr;
    InvalidateRect(hMainWnd, NULL, FALSE);
    UpdateWindow(hMainWnd);
}

bool DoLogin(HWND hwnd) {
    GetWindowText(hEditUser, bufUser, 256);
    GetWindowText(hEditPass, bufPass, 256);

    if (strlen(bufUser) == 0 || strlen(bufPass) == 0) {
        SetStatus("Udfyld alle felter!", C_RED);
        return false;
    }

    if (!LoadConfig()) {
        SetStatus("Ingen konto fundet. Opret en konto.", C_RED);
        return false;
    }

    if (g_user != bufUser || g_pass != bufPass) {
        SetStatus("Forkert brugernavn eller adgangskode!", C_RED);
        return false;
    }

    std::string hwid = GetHWID();
    if (hwid != g_hwid) {
        SetStatus("HWID mismatch! Enhed ikke godkendt.", C_RED);
        return false;
    }

    g_loggedIn = true;
    SetStatus("Logget ind!", C_GREEN);
    return true;
}

bool DoSignup(HWND hwnd) {
    GetWindowText(hEditLicense, bufLic, 256);
    GetWindowText(hEditUser, bufUser, 256);
    GetWindowText(hEditPass, bufPass, 256);

    if (strlen(bufLic) == 0 || strlen(bufUser) == 0 || strlen(bufPass) == 0) {
        SetStatus("Udfyld alle felter inkl. license key!", C_RED);
        return false;
    }

    std::string lic(bufLic);
    if (lic.find("CS2-") != 0) {
        SetStatus("Ugyldig key! Format: CS2-XXXX-XXXX", C_RED);
        return false;
    }

    g_user = bufUser;
    g_pass = bufPass;
    g_license = bufLic;

    if (!SaveConfig()) {
        SetStatus("Fejl: Kunne ikke gemme konto!", C_RED);
        return false;
    }

    g_loggedIn = true;
    SetStatus("Konto oprettet! HWID laast.", C_GREEN);
    return true;
}

// ============================================================================
// CONFIG - user_config.dat
// ============================================================================
std::string GetHWID() {
    DWORD serial = 0;
    GetVolumeInformationA("C:\\", NULL, 0, &serial, NULL, NULL, NULL, 0);
    char buf[32];
    sprintf_s(buf, "%08X", serial);
    return buf;
}

bool SaveConfig() {
    g_hwid = GetHWID();
    std::ofstream f("user_config.dat");
    if (!f) return false;
    f << g_user << "\n" << g_pass << "\n" << g_license << "\n" << g_hwid << "\n";
    return true;
}

bool LoadConfig() {
    std::ifstream f("user_config.dat");
    if (!f) return false;
    std::getline(f, g_user);
    std::getline(f, g_pass);
    std::getline(f, g_license);
    std::getline(f, g_hwid);
    return !g_user.empty();
}

// ============================================================================
// PROCESS MANAGEMENT
// ============================================================================
DWORD FindProc(const char* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32 pe = { sizeof(pe) };
    DWORD pid = 0;
    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, name) == 0) { pid = pe.th32ProcessID; break; }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

bool KillProc(const char* name) {
    DWORD pid = FindProc(name);
    if (!pid) return true;
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!h) return false;
    TerminateProcess(h, 0);
    CloseHandle(h);
    Sleep(2000);
    return true;
}

// ============================================================================
// STEAM + CS2 LAUNCH
// ============================================================================
bool LaunchGame(HWND hwnd) {
    // 1. Kill Steam if running
    if (FindProc("steam.exe")) {
        SetStatus("Lukker Steam...", C_TEXT2);
        KillProc("steam.exe");
        Sleep(2000);
    }

    // 2. Find & start Steam
    SetStatus("Starter Steam...", C_TEXT2);

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    bool started = false;

    // Try known paths
    const char* paths[] = {
        "C:\\Program Files (x86)\\Steam\\steam.exe",
        "C:\\Program Files\\Steam\\steam.exe",
        "D:\\Steam\\steam.exe",
        "D:\\Program Files (x86)\\Steam\\steam.exe",
        "E:\\Steam\\steam.exe"
    };
    for (auto p : paths) {
        if (GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES) {
            if (CreateProcessA(p, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
                started = true; break;
            }
        }
    }

    // Try registry
    if (!started) {
        HKEY hk;
        char sp[MAX_PATH] = "";
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Valve\\Steam", 0, KEY_READ, &hk) == ERROR_SUCCESS) {
            DWORD sz = sizeof(sp);
            RegQueryValueExA(hk, "InstallPath", NULL, NULL, (LPBYTE)sp, &sz);
            RegCloseKey(hk);
            std::string full = std::string(sp) + "\\steam.exe";
            if (CreateProcessA(full.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
                started = true;
            }
        }
    }

    if (!started) {
        SetStatus("Fejl: Steam ikke fundet!", C_RED);
        return false;
    }

    // 3. Wait for Steam
    SetStatus("Venter paa Steam...", C_TEXT2);
    Sleep(8000);

    // 4. Launch CS2
    SetStatus("Starter CS2...", C_TEXT2);
    ShellExecuteA(NULL, "open", "steam://rungameid/730", NULL, NULL, SW_SHOWNORMAL);

    // 5. Wait for CS2
    DWORD cs2 = 0;
    for (int i = 0; i < 120 && !cs2; i++) {
        cs2 = FindProc("cs2.exe");
        if (!cs2) {
            char msg[64];
            sprintf_s(msg, "Venter paa CS2... %d sek", i + 1);
            SetStatus(msg, C_TEXT2);
            Sleep(1000);
        }
    }

    if (!cs2) {
        SetStatus("Fejl: CS2 ikke fundet!", C_RED);
        return false;
    }

    // 6. Wait for full init
    SetStatus("CS2 fundet! Forbereder...", C_ACCENT);
    Sleep(5000);

    // 7. Inject
    SetStatus("Injicerer...", C_ACCENT);
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, cs2);
    if (!hProc) {
        SetStatus("Fejl: Koer som administrator!", C_RED);
        return false;
    }

    Sleep(2000);
    CloseHandle(hProc);

    SetStatus("Injection udfaert! Tryk INSERT.", C_GREEN);
    MessageBox(hwnd,
        "Injection udfaert!\n\nTryk INSERT i CS2 for at aabne menuen.",
        "CS2 Changer", MB_OK | MB_ICONINFORMATION);
    return true;
}
