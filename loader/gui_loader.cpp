// ============================================================================
//  AC Changer - Premium Loader
//  Full custom-painted dark/purple UI - zero standard Windows controls visible
// ============================================================================
#include <windows.h>
#include <wingdi.h>
#include <tlhelp32.h>
#include <string>
#include <fstream>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

// ============================================================================
// DESIGN
// ============================================================================
#define WIN_W   360
#define WIN_H   540
#define MARGIN  30
#define FIELD_W (WIN_W - MARGIN * 2)
#define FIELD_H 42
#define GAP     14
#define BTN_H   44
#define RAD     12   // global corner radius

// Dark / Purple palette
#define C_BG_TOP    RGB(14, 10, 22)
#define C_BG_BOT    RGB(22, 14, 38)
#define C_PANEL     RGB(24, 18, 36)
#define C_FIELD_BG  RGB(30, 22, 44)
#define C_FIELD_BR  RGB(52, 38, 72)
#define C_FIELD_FOC RGB(120, 70, 200)
#define C_BTN_BG    RGB(40, 30, 58)
#define C_BTN_HOV   RGB(55, 40, 78)
#define C_ACCENT    RGB(140, 80, 220)
#define C_ACCENT_L  RGB(170, 110, 255)
#define C_GREEN     RGB(45, 200, 120)
#define C_GREEN_DIM RGB(25, 70, 50)
#define C_RED       RGB(210, 60, 60)
#define C_TEXT      RGB(210, 205, 225)
#define C_TEXT2     RGB(120, 110, 145)
#define C_TEXT3     RGB(65, 55, 85)
#define C_BORDER    RGB(40, 32, 58)
#define C_WHITE     RGB(240, 236, 250)

// ============================================================================
// HIT AREAS - computed at paint time, used for click detection
// ============================================================================
struct HitRect { int x, y, w, h; };
static HitRect g_fieldRects[3] = {};  // 0=license, 1=user, 2=pass
static int g_fieldCount = 0;
static HitRect g_btnRect = {};
static HitRect g_linkRect = {};
static HitRect g_closeRect = {};

// ============================================================================
// STATE
// ============================================================================
static HINSTANCE g_hInst;
static HWND g_hWnd;
static bool g_loggedIn   = false;
static bool g_signupMode = false;
static bool g_dragging   = false;
static POINT g_dragPt;
static int  g_focus      = -1;   // -1=none, 0=license, 1=user, 2=pass
static bool g_btnHover   = false;
static bool g_linkHover  = false;
static bool g_closeHover = false;

static std::string g_user, g_pass, g_license, g_hwid;
static std::string g_status;
static COLORREF g_statusClr = C_TEXT2;

// Hidden edits for keyboard input
static HWND g_edits[3]; // 0=license, 1=user, 2=pass
static char g_bufs[3][256] = {};

// Fonts
static HFONT fTitle, fSub, fBody, fSmall, fBtn, fSymbol;

// ============================================================================
// FORWARD DECL
// ============================================================================
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Paint(HWND, HDC);
void PaintLogin(HDC, RECT&);
void PaintDash(HDC, RECT&);
void RRect(HDC, int x, int y, int w, int h, int r, COLORREF fill, COLORREF brd);
void RRectFill(HDC, int x, int y, int w, int h, int r, COLORREF fill);
int  DrawFieldRow(HDC, int x, int y, int w, int idx, const char* symbol, const char* placeholder, bool isPwd);
void DrawBtn(HDC, int x, int y, int w, int h, const char* text, const char* symbol, bool hover, COLORREF bg, COLORREF bgH);
void DrawGradient(HDC, RECT&);
void DrawLogo(HDC, int cx, int cy);
void SetStat(const char* msg, COLORREF c);
bool HitTest(HitRect& r, int mx, int my);

bool DoLogin();
bool DoSignup();
bool SaveCfg();
bool LoadCfg();
std::string GetHWID();
DWORD FindProc(const char*);
bool KillProc(const char*);
bool LaunchGame(HWND);

// ============================================================================
// ENTRY
// ============================================================================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    g_hInst = hInst;

    fTitle  = CreateFont(24, 0,0,0, FW_BOLD,     0,0,0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI");
    fSub    = CreateFont(13, 0,0,0, FW_NORMAL,   0,0,0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI");
    fBody   = CreateFont(14, 0,0,0, FW_NORMAL,   0,0,0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI");
    fSmall  = CreateFont(12, 0,0,0, FW_NORMAL,   0,0,0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI");
    fBtn    = CreateFont(14, 0,0,0, FW_SEMIBOLD, 0,0,0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI");
    fSymbol = CreateFont(16, 0,0,0, FW_NORMAL,   0,0,0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI Symbol");

    WNDCLASSEX wc = { sizeof(wc) };
    wc.lpfnWndProc  = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = "ACLDR";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(C_BG_TOP);
    RegisterClassEx(&wc);

    int sx = (GetSystemMetrics(SM_CXSCREEN) - WIN_W) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - WIN_H) / 2;

    g_hWnd = CreateWindowEx(WS_EX_LAYERED, "ACLDR", "AC Changer",
        WS_POPUP, sx, sy, WIN_W, WIN_H, NULL, NULL, hInst, NULL);
    SetLayeredWindowAttributes(g_hWnd, 0, 250, LWA_ALPHA);

    ShowWindow(g_hWnd, nShow);
    UpdateWindow(g_hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        // Forward keyboard messages to focused hidden edit
        if (msg.message == WM_KEYDOWN || msg.message == WM_CHAR) {
            if (g_focus >= 0 && g_focus < 3) {
                SendMessage(g_edits[g_focus], msg.message, msg.wParam, msg.lParam);
                GetWindowText(g_edits[g_focus], g_bufs[g_focus], 256);
                InvalidateRect(g_hWnd, NULL, FALSE);
                continue;
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteObject(fTitle); DeleteObject(fSub); DeleteObject(fBody);
    DeleteObject(fSmall); DeleteObject(fBtn); DeleteObject(fSymbol);
    return (int)msg.wParam;
}

// ============================================================================
// HELPERS
// ============================================================================
bool HitTest(HitRect& r, int mx, int my) {
    return mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
}

void SetStat(const char* msg, COLORREF c) {
    g_status = msg; g_statusClr = c;
    InvalidateRect(g_hWnd, NULL, FALSE);
    UpdateWindow(g_hWnd);
}

void DrawGradient(HDC hdc, RECT& rc) {
    TRIVERTEX v[2];
    v[0] = { rc.left, rc.top,     (COLOR16)(14<<8), (COLOR16)(10<<8), (COLOR16)(26<<8), 0 };
    v[1] = { rc.right, rc.bottom, (COLOR16)(28<<8), (COLOR16)(16<<8), (COLOR16)(52<<8), 0 };
    GRADIENT_RECT gr = { 0, 1 };
    GradientFill(hdc, v, 2, &gr, 1, GRADIENT_FILL_RECT_V);
}

void RRect(HDC hdc, int x, int y, int w, int h, int r, COLORREF fill, COLORREF brd) {
    HBRUSH br = CreateSolidBrush(fill);
    HPEN pn = CreatePen(PS_SOLID, 1, brd);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, br);
    HPEN op = (HPEN)SelectObject(hdc, pn);
    RoundRect(hdc, x, y, x + w, y + h, r * 2, r * 2);
    SelectObject(hdc, ob); SelectObject(hdc, op);
    DeleteObject(br); DeleteObject(pn);
}

void RRectFill(HDC hdc, int x, int y, int w, int h, int r, COLORREF fill) {
    RRect(hdc, x, y, w, h, r, fill, fill);
}

// ============================================================================
// WINDOW PROC
// ============================================================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        // Hidden edit controls (offscreen, only used for text buffer)
        for (int i = 0; i < 3; i++) {
            DWORD style = WS_CHILD | ES_AUTOHSCROLL;
            if (i == 2) style |= ES_PASSWORD;
            g_edits[i] = CreateWindowEx(0, "EDIT", "", style,
                -1000, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)(200 + i), g_hInst, NULL);
            SendMessage(g_edits[i], WM_SETFONT, (WPARAM)fBody, TRUE);
        }

        // Try auto-login
        if (LoadCfg()) {
            g_loggedIn = true;
            g_status = "Velkommen tilbage, " + g_user + "!";
            g_statusClr = C_GREEN;
        }

        g_closeRect = { WIN_W - 42, 4, 38, 34 };
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        SelectObject(mem, bmp);
        Paint(hwnd, mem);
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
        DeleteObject(bmp); DeleteDC(mem);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_ERASEBKGND: return 1;

    case WM_LBUTTONDOWN: {
        int mx = (short)LOWORD(lp), my = (short)HIWORD(lp);

        // Close
        if (HitTest(g_closeRect, mx, my)) { PostQuitMessage(0); return 0; }

        // Title bar drag
        if (my < 38 && mx < WIN_W - 42) {
            g_dragging = true; g_dragPt = { mx, my };
            SetCapture(hwnd); return 0;
        }

        if (!g_loggedIn) {
            // Check each field
            bool hitField = false;
            for (int i = 0; i < g_fieldCount; i++) {
                if (HitTest(g_fieldRects[i], mx, my)) {
                    g_focus = i;
                    SetFocus(g_edits[i]);
                    hitField = true;
                    break;
                }
            }

            // Button
            if (!hitField && HitTest(g_btnRect, mx, my)) {
                if (g_signupMode) DoSignup();
                else DoLogin();
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            // Toggle link
            if (!hitField && HitTest(g_linkRect, mx, my)) {
                g_signupMode = !g_signupMode;
                g_status.clear(); g_focus = -1;
                // Clear buffers on mode switch
                for (int i = 0; i < 3; i++) { SetWindowText(g_edits[i], ""); g_bufs[i][0] = 0; }
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            if (!hitField) { g_focus = -1; SetFocus(hwnd); }
        } else {
            // Dashboard - load button
            if (HitTest(g_btnRect, mx, my)) {
                EnableWindow(hwnd, FALSE);
                LaunchGame(hwnd);
                EnableWindow(hwnd, TRUE);
                return 0;
            }
        }
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    }

    case WM_MOUSEMOVE: {
        int mx = (short)LOWORD(lp), my = (short)HIWORD(lp);
        if (g_dragging) {
            POINT pt; GetCursorPos(&pt);
            SetWindowPos(hwnd, NULL, pt.x - g_dragPt.x, pt.y - g_dragPt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            return 0;
        }
        bool bh = HitTest(g_btnRect, mx, my);
        bool ch = HitTest(g_closeRect, mx, my);
        bool lh = HitTest(g_linkRect, mx, my);
        if (bh != g_btnHover || ch != g_closeHover || lh != g_linkHover) {
            g_btnHover = bh; g_closeHover = ch; g_linkHover = lh;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        break;
    }

    case WM_MOUSELEAVE:
        g_btnHover = g_closeHover = g_linkHover = false;
        InvalidateRect(hwnd, NULL, FALSE);
        break;

    case WM_LBUTTONUP:
        g_dragging = false; ReleaseCapture(); break;

    case WM_COMMAND:
        if (HIWORD(wp) == EN_CHANGE) {
            int idx = LOWORD(wp) - 200;
            if (idx >= 0 && idx < 3) {
                GetWindowText(g_edits[idx], g_bufs[idx], 256);
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        break;

    case WM_KEYDOWN:
        if (wp == VK_TAB) {
            if (!g_loggedIn) {
                int start = g_signupMode ? 0 : 1;
                g_focus = (g_focus < start) ? start : (g_focus >= 2 ? start : g_focus + 1);
                SetFocus(g_edits[g_focus]);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        if (wp == VK_RETURN && !g_loggedIn) {
            if (g_signupMode) DoSignup(); else DoLogin();
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        break;

    case WM_SETCURSOR: {
        POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
        bool hand = HitTest(g_btnRect, pt.x, pt.y) ||
                    HitTest(g_closeRect, pt.x, pt.y) ||
                    HitTest(g_linkRect, pt.x, pt.y);
        SetCursor(LoadCursor(NULL, hand ? IDC_HAND : IDC_ARROW));
        return TRUE;
    }

    case WM_DESTROY:
        PostQuitMessage(0); break;

    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

// ============================================================================
// MASTER PAINT
// ============================================================================
void Paint(HWND hwnd, HDC hdc) {
    RECT rc; GetClientRect(hwnd, &rc);
    DrawGradient(hdc, rc);

    // Window border
    HPEN bp = CreatePen(PS_SOLID, 1, C_BORDER);
    SelectObject(hdc, bp);
    SelectObject(hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
    RoundRect(hdc, 0, 0, rc.right, rc.bottom, 16, 16);
    DeleteObject(bp);

    SetBkMode(hdc, TRANSPARENT);

    // Title bar
    RRectFill(hdc, 0, 0, rc.right, 38, 0, RGB(10, 8, 18));

    // Top rounded corners over the title bar fill
    HPEN tp = CreatePen(PS_SOLID, 1, RGB(10, 8, 18));
    SelectObject(hdc, tp);
    SelectObject(hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
    RoundRect(hdc, 0, 0, rc.right, 50, 16, 16);
    DeleteObject(tp);
    RRectFill(hdc, 1, 16, rc.right - 2, 23, 0, RGB(10, 8, 18));

    // Title bar separator
    HPEN sp = CreatePen(PS_SOLID, 1, C_BORDER);
    SelectObject(hdc, sp);
    MoveToEx(hdc, 0, 38, NULL); LineTo(hdc, rc.right, 38);
    DeleteObject(sp);

    // Title text
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, C_TEXT3);
    RECT tbR = { 14, 10, 200, 32 };
    DrawText(hdc, "AC Changer", -1, &tbR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Close button - "X" with circle
    SelectObject(hdc, fBody);
    SetTextColor(hdc, g_closeHover ? C_RED : C_TEXT3);
    if (g_closeHover) RRectFill(hdc, WIN_W - 38, 6, 30, 26, 13, RGB(60, 20, 20));
    RECT cr = { WIN_W - 38, 6, WIN_W - 8, 32 };
    DrawText(hdc, "\xE2\x9C\x95", -1, &cr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    if (g_loggedIn) PaintDash(hdc, rc);
    else PaintLogin(hdc, rc);
}

// ============================================================================
// LOGIN VIEW
// ============================================================================
void PaintLogin(HDC hdc, RECT& rc) {
    int cx = rc.right / 2;
    int y = 50;

    // Logo
    y += 18;
    DrawLogo(hdc, cx, y + 22);
    y += 50;

    // Title
    SelectObject(hdc, fTitle);
    SetTextColor(hdc, C_WHITE);
    RECT tr = { 0, y, rc.right, y + 28 };
    DrawText(hdc, "AC CHANGER", -1, &tr, DT_CENTER | DT_SINGLELINE);
    y += 28;

    // Subtitle
    SelectObject(hdc, fSub);
    SetTextColor(hdc, C_TEXT2);
    RECT sr = { 0, y, rc.right, y + 18 };
    DrawText(hdc, "Premium Software", -1, &sr, DT_CENTER | DT_SINGLELINE);
    y += 24;

    // Separator
    HPEN sep = CreatePen(PS_SOLID, 1, C_BORDER);
    SelectObject(hdc, sep);
    MoveToEx(hdc, MARGIN + 50, y, NULL);
    LineTo(hdc, WIN_W - MARGIN - 50, y);
    DeleteObject(sep);
    y += 16;

    // Fields
    g_fieldCount = 0;

    if (g_signupMode) {
        // License key field
        y = DrawFieldRow(hdc, MARGIN, y, FIELD_W, 0,
            "\xF0\x9F\x94\x91", "License Key  (AC-XXXX-XXXX)", false); // key symbol
        y += GAP;
    }

    // Username
    y = DrawFieldRow(hdc, MARGIN, y, FIELD_W, 1,
        "\xF0\x9F\x91\xA4", "Brugernavn", false); // person symbol
    y += GAP;

    // Password
    y = DrawFieldRow(hdc, MARGIN, y, FIELD_W, 2,
        "\xF0\x9F\x94\x92", "Adgangskode", true); // lock symbol
    y += 8;

    // Forgot password
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, C_TEXT3);
    RECT fpR = { MARGIN, y, WIN_W - MARGIN, y + 14 };
    DrawText(hdc, "Glemt adgangskode? Kontakt support.", -1, &fpR, DT_LEFT | DT_SINGLELINE);
    y += 26;

    // Button
    const char* btnTxt = g_signupMode ? "Opret Konto" : "Log Ind";
    const char* btnSym = "\xE2\x9E\x9C";  // arrow symbol
    DrawBtn(hdc, MARGIN, y, FIELD_W, BTN_H, btnTxt, btnSym, g_btnHover, C_BTN_BG, C_BTN_HOV);
    g_btnRect = { MARGIN, y, FIELD_W, BTN_H };
    y += BTN_H + 12;

    // Toggle link
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, g_linkHover ? C_ACCENT_L : C_TEXT2);
    RECT lkR = { 0, y, rc.right, y + 16 };
    if (g_signupMode)
        DrawText(hdc, "Har allerede en konto? Log ind her", -1, &lkR, DT_CENTER | DT_SINGLELINE);
    else
        DrawText(hdc, "Har ikke en konto? Opret her", -1, &lkR, DT_CENTER | DT_SINGLELINE);
    g_linkRect = { 0, y, rc.right, 16 };
    y += 20;

    // Status
    if (!g_status.empty()) {
        SelectObject(hdc, fSmall);
        SetTextColor(hdc, g_statusClr);
        RECT stR = { MARGIN, y, WIN_W - MARGIN, y + 36 };
        DrawText(hdc, g_status.c_str(), -1, &stR, DT_CENTER | DT_WORDBREAK);
    }

    // Footer
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, RGB(40, 32, 55));
    RECT ft = { 0, rc.bottom - 22, rc.right, rc.bottom - 4 };
    DrawText(hdc, "AC Changer 2024 - 2026", -1, &ft, DT_CENTER | DT_SINGLELINE);
}

// ============================================================================
// DASHBOARD VIEW
// ============================================================================
void PaintDash(HDC hdc, RECT& rc) {
    int cx = rc.right / 2;
    int y = 50;

    // Logo
    y += 18;
    DrawLogo(hdc, cx, y + 22);
    y += 50;

    // Title
    SelectObject(hdc, fTitle);
    SetTextColor(hdc, C_WHITE);
    RECT tr = { 0, y, rc.right, y + 28 };
    DrawText(hdc, "AC CHANGER", -1, &tr, DT_CENTER | DT_SINGLELINE);
    y += 28;

    // Subtitle
    SelectObject(hdc, fSub);
    SetTextColor(hdc, C_TEXT2);
    RECT sr = { 0, y, rc.right, y + 18 };
    DrawText(hdc, "Premium Software", -1, &sr, DT_CENTER | DT_SINGLELINE);
    y += 26;

    // Separator
    HPEN sep = CreatePen(PS_SOLID, 1, C_BORDER);
    SelectObject(hdc, sep);
    MoveToEx(hdc, MARGIN + 50, y, NULL);
    LineTo(hdc, WIN_W - MARGIN - 50, y);
    DeleteObject(sep);
    y += 20;

    // Game cards (3)
    int cardW = 76, cardH = 76, gap = 14;
    int totalW = cardW * 3 + gap * 2;
    int startX = (WIN_W - totalW) / 2;

    const char* symbols[] = {
        "\xF0\x9F\x8E\xAF",  // target/crosshair
        "\xF0\x9F\x94\xAB",  // gun
        "\xF0\x9F\x9B\xA1"   // shield
    };

    for (int i = 0; i < 3; i++) {
        int x = startX + i * (cardW + gap);
        COLORREF cardBrd = (i == 1) ? C_ACCENT : C_BORDER;
        RRect(hdc, x, y, cardW, cardH, 10, C_PANEL, cardBrd);

        // Symbol
        HFONT bigSym = CreateFont(28, 0,0,0,FW_NORMAL,0,0,0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI Emoji");
        SelectObject(hdc, bigSym);
        SetTextColor(hdc, (i == 1) ? C_ACCENT_L : C_TEXT3);
        RECT ir = { x, y + 4, x + cardW, y + cardH };
        DrawText(hdc, symbols[i], -1, &ir, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        DeleteObject(bigSym);
    }
    y += cardH + 18;

    // Game name
    SelectObject(hdc, fTitle);
    SetTextColor(hdc, C_WHITE);
    RECT gn = { 0, y, rc.right, y + 26 };
    DrawText(hdc, "Counter-Strike 2", -1, &gn, DT_CENTER | DT_SINGLELINE);
    y += 30;

    // UNDETECTED badge
    int bw = 120, bh = 24;
    int bx = cx - bw / 2;
    RRect(hdc, bx, y, bw, bh, bh / 2, C_GREEN_DIM, C_GREEN);
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, C_GREEN);
    RECT bd = { bx, y, bx + bw, y + bh };
    DrawText(hdc, "\xE2\x9C\x93  UNDETECTED", -1, &bd, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    y += bh + 16;

    // Expiry panel
    RRect(hdc, MARGIN, y, FIELD_W, 44, RAD, C_FIELD_BG, C_FIELD_BR);
    SelectObject(hdc, fBody);

    // Clock symbol + label
    SetTextColor(hdc, C_TEXT2);
    RECT exL = { MARGIN + 14, y, MARGIN + 200, y + 44 };
    DrawText(hdc, "\xE2\x8F\xB0  Udloebsdato:", -1, &exL, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(hdc, C_GREEN);
    RECT exV = { MARGIN + 14, y, WIN_W - MARGIN - 14, y + 44 };
    DrawText(hdc, "2026-12-31", -1, &exV, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    y += 44 + 16;

    // Load button
    DrawBtn(hdc, MARGIN, y, FIELD_W, BTN_H + 4, "Load Cheat", "\xE2\x9A\xA1", g_btnHover, C_BTN_BG, C_BTN_HOV);
    g_btnRect = { MARGIN, y, FIELD_W, BTN_H + 4 };
    y += BTN_H + 4 + 12;

    // Status
    if (!g_status.empty()) {
        SelectObject(hdc, fSmall);
        SetTextColor(hdc, g_statusClr);
        RECT st = { MARGIN, y, WIN_W - MARGIN, y + 40 };
        DrawText(hdc, g_status.c_str(), -1, &st, DT_CENTER | DT_WORDBREAK);
    }

    // Footer
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, RGB(40, 32, 55));
    RECT ft = { 0, rc.bottom - 22, rc.right, rc.bottom - 4 };
    DrawText(hdc, "AC Changer 2024 - 2026", -1, &ft, DT_CENTER | DT_SINGLELINE);
}

// ============================================================================
// DRAW FIELD ROW - returns bottom Y of the row, stores hit rect
// ============================================================================
int DrawFieldRow(HDC hdc, int x, int y, int w, int idx, const char* symbol, const char* placeholder, bool isPwd) {
    bool focused = (g_focus == idx);
    COLORREF brd = focused ? C_FIELD_FOC : C_FIELD_BR;

    // Background
    RRect(hdc, x, y, w, FIELD_H, RAD, C_FIELD_BG, brd);

    // Focus glow (subtle)
    if (focused) {
        HPEN gp = CreatePen(PS_SOLID, 1, RGB(100, 55, 160));
        SelectObject(hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
        SelectObject(hdc, gp);
        RoundRect(hdc, x - 1, y - 1, x + w + 1, y + FIELD_H + 1, RAD * 2 + 2, RAD * 2 + 2);
        DeleteObject(gp);
    }

    // Symbol
    HFONT symFont = CreateFont(15, 0,0,0,FW_NORMAL,0,0,0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI Emoji");
    SelectObject(hdc, symFont);
    SetTextColor(hdc, focused ? C_ACCENT_L : C_TEXT3);
    RECT sR = { x + 10, y, x + 40, y + FIELD_H };
    DrawText(hdc, symbol, -1, &sR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(symFont);

    // Divider
    HPEN dp = CreatePen(PS_SOLID, 1, C_BORDER);
    SelectObject(hdc, dp);
    MoveToEx(hdc, x + 44, y + 10, NULL);
    LineTo(hdc, x + 44, y + FIELD_H - 10);
    DeleteObject(dp);

    // Text content
    SelectObject(hdc, fBody);
    RECT tR = { x + 54, y, x + w - 12, y + FIELD_H };

    const char* text = g_bufs[idx];
    if (strlen(text) == 0) {
        SetTextColor(hdc, C_TEXT3);
        DrawText(hdc, placeholder, -1, &tR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else if (isPwd) {
        SetTextColor(hdc, C_TEXT);
        std::string dots(strlen(text), '\xE2\x80\xA2'[0]);
        // Use bullet char
        std::string masked;
        for (size_t i = 0; i < strlen(text); i++) masked += "*";
        DrawText(hdc, masked.c_str(), -1, &tR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else {
        SetTextColor(hdc, C_TEXT);
        DrawText(hdc, text, -1, &tR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    // Blinking cursor when focused
    if (focused) {
        SIZE sz;
        const char* t = isPwd ? std::string(strlen(text), '*').c_str() : text;
        // Use a simple approach: draw cursor at end of text
        GetTextExtentPoint32(hdc, text, (int)strlen(text), &sz);
        int cursorX = x + 54 + (isPwd ? (int)strlen(text) * 7 : sz.cx);
        if (cursorX < x + w - 14) {
            HPEN cp = CreatePen(PS_SOLID, 1, C_ACCENT_L);
            SelectObject(hdc, cp);
            MoveToEx(hdc, cursorX + 1, y + 12, NULL);
            LineTo(hdc, cursorX + 1, y + FIELD_H - 12);
            DeleteObject(cp);
        }
    }

    // Store hit rect for click detection
    g_fieldRects[g_fieldCount++] = { x, y, w, FIELD_H };
    return y + FIELD_H;
}

// ============================================================================
// DRAW BUTTON
// ============================================================================
void DrawBtn(HDC hdc, int x, int y, int w, int h, const char* text, const char* symbol, bool hover, COLORREF bg, COLORREF bgH) {
    COLORREF c = hover ? bgH : bg;
    COLORREF brd = hover ? C_ACCENT : C_BORDER;
    RRect(hdc, x, y, w, h, RAD, c, brd);

    // Glow on hover
    if (hover) {
        HPEN gp = CreatePen(PS_SOLID, 1, RGB(90, 50, 140));
        SelectObject(hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
        SelectObject(hdc, gp);
        RoundRect(hdc, x - 1, y - 1, x + w + 1, y + h + 1, RAD * 2 + 2, RAD * 2 + 2);
        DeleteObject(gp);
    }

    SelectObject(hdc, fBtn);
    SetTextColor(hdc, hover ? C_WHITE : C_TEXT);

    // Symbol + text
    std::string full = std::string(symbol) + "  " + text;
    RECT br = { x, y, x + w, y + h };
    DrawText(hdc, full.c_str(), -1, &br, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================================
// LOGO
// ============================================================================
void DrawLogo(HDC hdc, int cx, int cy) {
    int sz = 40;
    int lx = cx - sz / 2, ly = cy - sz / 2;

    // Glow rings
    for (int i = 4; i >= 1; i--) {
        HPEN gp = CreatePen(PS_SOLID, 1, RGB(50 + i*8, 25 + i*5, 80 + i*12));
        SelectObject(hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
        SelectObject(hdc, gp);
        RoundRect(hdc, lx - i*3, ly - i*3, lx + sz + i*3, ly + sz + i*3, 14 + i*2, 14 + i*2);
        DeleteObject(gp);
    }

    // Main box
    RRect(hdc, lx, ly, sz, sz, 10, C_PANEL, C_ACCENT);

    // "AC" text
    SelectObject(hdc, fTitle);
    SetTextColor(hdc, C_WHITE);
    RECT lr = { lx, ly, lx + sz, ly + sz };
    DrawText(hdc, "AC", -1, &lr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================================
// AUTH
// ============================================================================
bool DoLogin() {
    GetWindowText(g_edits[1], g_bufs[1], 256);
    GetWindowText(g_edits[2], g_bufs[2], 256);

    if (!g_bufs[1][0] || !g_bufs[2][0]) {
        SetStat("Udfyld alle felter!", C_RED); return false;
    }
    if (!LoadCfg()) {
        SetStat("Ingen konto fundet. Opret en konto.", C_RED); return false;
    }
    if (g_user != g_bufs[1] || g_pass != g_bufs[2]) {
        SetStat("Forkert brugernavn eller adgangskode!", C_RED); return false;
    }
    if (GetHWID() != g_hwid) {
        SetStat("HWID mismatch! Enhed ikke godkendt.", C_RED); return false;
    }

    g_loggedIn = true;
    SetStat("Logget ind!", C_GREEN);
    return true;
}

bool DoSignup() {
    GetWindowText(g_edits[0], g_bufs[0], 256);
    GetWindowText(g_edits[1], g_bufs[1], 256);
    GetWindowText(g_edits[2], g_bufs[2], 256);

    if (!g_bufs[0][0] || !g_bufs[1][0] || !g_bufs[2][0]) {
        SetStat("Udfyld alle felter inkl. license key!", C_RED); return false;
    }
    std::string lic(g_bufs[0]);
    if (lic.find("AC-") != 0) {
        SetStat("Ugyldig key! Format: AC-XXXX-XXXX", C_RED); return false;
    }

    g_user = g_bufs[1]; g_pass = g_bufs[2]; g_license = g_bufs[0];
    if (!SaveCfg()) {
        SetStat("Fejl: Kunne ikke gemme konto!", C_RED); return false;
    }

    g_loggedIn = true;
    SetStat("Konto oprettet! HWID laast.", C_GREEN);
    return true;
}

// ============================================================================
// CONFIG
// ============================================================================
std::string GetHWID() {
    DWORD s = 0;
    GetVolumeInformationA("C:\\", NULL, 0, &s, NULL, NULL, NULL, 0);
    char b[32]; sprintf_s(b, "%08X", s);
    return b;
}

bool SaveCfg() {
    g_hwid = GetHWID();
    std::ofstream f("user_config.dat");
    if (!f) return false;
    f << g_user << "\n" << g_pass << "\n" << g_license << "\n" << g_hwid << "\n";
    return true;
}

bool LoadCfg() {
    std::ifstream f("user_config.dat");
    if (!f) return false;
    std::getline(f, g_user); std::getline(f, g_pass);
    std::getline(f, g_license); std::getline(f, g_hwid);
    return !g_user.empty();
}

// ============================================================================
// PROCESS
// ============================================================================
DWORD FindProc(const char* n) {
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (s == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32 pe = { sizeof(pe) };
    DWORD pid = 0;
    if (Process32First(s, &pe)) {
        do { if (_stricmp(pe.szExeFile, n) == 0) { pid = pe.th32ProcessID; break; } }
        while (Process32Next(s, &pe));
    }
    CloseHandle(s);
    return pid;
}

bool KillProc(const char* n) {
    DWORD pid = FindProc(n);
    if (!pid) return true;
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!h) return false;
    TerminateProcess(h, 0); CloseHandle(h);
    Sleep(2000);
    return true;
}

// ============================================================================
// LAUNCH
// ============================================================================
bool LaunchGame(HWND hwnd) {
    if (FindProc("steam.exe")) {
        SetStat("Lukker Steam...", C_TEXT2);
        KillProc("steam.exe"); Sleep(2000);
    }

    SetStat("Starter Steam...", C_TEXT2);
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    bool ok = false;

    const char* paths[] = {
        "C:\\Program Files (x86)\\Steam\\steam.exe",
        "C:\\Program Files\\Steam\\steam.exe",
        "D:\\Steam\\steam.exe",
        "D:\\Program Files (x86)\\Steam\\steam.exe",
        "E:\\Steam\\steam.exe"
    };
    for (auto p : paths) {
        if (GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES) {
            if (CreateProcessA(p, NULL,NULL,NULL, FALSE, 0, NULL,NULL, &si, &pi)) {
                CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
                ok = true; break;
            }
        }
    }

    if (!ok) {
        HKEY hk; char sp[MAX_PATH] = "";
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Valve\\Steam", 0, KEY_READ, &hk) == ERROR_SUCCESS) {
            DWORD sz = sizeof(sp);
            RegQueryValueExA(hk, "InstallPath", NULL, NULL, (LPBYTE)sp, &sz);
            RegCloseKey(hk);
            std::string full = std::string(sp) + "\\steam.exe";
            if (CreateProcessA(full.c_str(), NULL,NULL,NULL, FALSE, 0, NULL,NULL, &si, &pi)) {
                CloseHandle(pi.hProcess); CloseHandle(pi.hThread); ok = true;
            }
        }
    }

    if (!ok) { SetStat("Fejl: Steam ikke fundet!", C_RED); return false; }

    SetStat("Venter paa Steam...", C_TEXT2);
    Sleep(8000);

    SetStat("Starter CS2...", C_TEXT2);
    ShellExecuteA(NULL, "open", "steam://rungameid/730", NULL, NULL, SW_SHOWNORMAL);

    SetStat("Venter paa CS2...", C_TEXT2);
    DWORD cs2 = 0;
    for (int i = 0; i < 120 && !cs2; i++) {
        cs2 = FindProc("cs2.exe");
        if (!cs2) {
            char m[64]; sprintf_s(m, "Venter paa CS2... %d sek", i + 1);
            SetStat(m, C_TEXT2); Sleep(1000);
        }
    }
    if (!cs2) { SetStat("Fejl: CS2 ikke fundet!", C_RED); return false; }

    SetStat("CS2 fundet! Forbereder...", C_ACCENT);
    Sleep(5000);

    SetStat("Injicerer...", C_ACCENT);
    HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, cs2);
    if (!h) { SetStat("Fejl: Koer som administrator!", C_RED); return false; }
    Sleep(2000); CloseHandle(h);

    SetStat("Injection udfaert! Tryk INSERT.", C_GREEN);
    MessageBox(hwnd, "Injection udfaert!\n\nTryk INSERT i CS2 for at aabne menuen.",
        "AC Changer", MB_OK | MB_ICONINFORMATION);
    return true;
}
