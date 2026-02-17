// ============================================================================
//  AC Changer – Premium Loader
//  Compact, rounded, dark/purple gradient UI
// ============================================================================
#include <windows.h>
#include <wingdi.h>
#include <tlhelp32.h>
#include <string>
#include <fstream>
#include "GameMenu.h"

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

// ============================================================================
// DESIGN – compact window, generous rounding
// ============================================================================
#define WIN_W    270
#define WIN_H    380
#define MARGIN   18
#define FIELD_W  (WIN_W - MARGIN * 2)
#define FIELD_H  32
#define GAP      8
#define BTN_H    34
#define RAD      14
#define TITLE_H  30

// Dark black/grey palette
#define C_BG_TOP    RGB(8, 8, 8)
#define C_BG_BOT    RGB(18, 18, 20)
#define C_PANEL     RGB(16, 16, 18)
#define C_FIELD_BG  RGB(22, 22, 24)
#define C_FIELD_BR  RGB(42, 42, 46)
#define C_FIELD_FOC RGB(100, 100, 110)
#define C_BTN_BG    RGB(28, 28, 32)
#define C_BTN_HOV   RGB(42, 42, 48)
#define C_ACCENT    RGB(130, 130, 140)
#define C_ACCENT_L  RGB(180, 180, 195)
#define C_GREEN     RGB(45, 200, 120)
#define C_GREEN_DIM RGB(20, 60, 40)
#define C_RED       RGB(210, 60, 60)
#define C_TEXT      RGB(200, 200, 210)
#define C_TEXT2     RGB(110, 110, 120)
#define C_TEXT3     RGB(60, 60, 68)
#define C_BORDER    RGB(35, 35, 40)
#define C_WHITE     RGB(235, 235, 240)

// ============================================================================
// HIT AREAS – computed at paint time, consumed by click handling
// ============================================================================
struct HitRect { int x, y, w, h; };
static HitRect g_fieldRects[3]   = {};
static int     g_fieldEditIdx[3] = {};   // maps fieldRect slot -> edit index
static int     g_fieldCount      = 0;
static HitRect g_btnRect   = {};
static HitRect g_linkRect  = {};
static HitRect g_closeRect = {};

// ============================================================================
// STATE
// ============================================================================
static HINSTANCE g_hInst;
static HWND      g_hWnd;
static bool  g_loggedIn   = false;
static bool  g_signupMode = false;
static bool  g_dragging   = false;
static POINT g_dragPt;
static int   g_focus      = -1;   // edit index: -1=none 0=license 1=user 2=pass
static bool  g_btnHover   = false;
static bool  g_linkHover  = false;
static bool  g_closeHover = false;

static std::string g_user, g_pass, g_license, g_hwid;
static std::string g_status;
static COLORREF    g_statusClr = C_TEXT2;

// Hidden edit controls (off-screen, provide keyboard input buffer)
static HWND g_edits[3];
static char g_bufs[3][256] = {};

// Fonts
static HFONT fTitle, fSub, fBody, fSmall, fBtn, fIcon;

// ============================================================================
// FORWARD DECL
// ============================================================================
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Paint(HWND, HDC);
void PaintLogin(HDC, RECT&);
void PaintDash(HDC, RECT&);
void RRect(HDC, int, int, int, int, int, COLORREF, COLORREF);
void RRectFill(HDC, int, int, int, int, int, COLORREF);
int  DrawFieldRow(HDC, int, int, int, int, wchar_t, const char*, bool);
void DrawBtn(HDC, int, int, int, int, const char*, wchar_t, bool, COLORREF, COLORREF);
void DrawGlyph(HDC, int, int, int, int, wchar_t, COLORREF, int = 14);
void DrawCardIcon(HDC, int, int, int, int, int, COLORREF);
void DrawGradient(HDC, RECT&);
void DrawLogo(HDC, int, int);
void SetStat(const char*, COLORREF);
bool HitTest(HitRect&, int, int);

bool DoLogin();
bool DoSignup();
bool SaveCfg();
bool LoadCfg();
std::string GetHWID();
DWORD FindProc(const char*);
bool KillProc(const char*);
bool LaunchGame(HWND);

// Overlay state
static OverlayWindow* g_pOverlayWnd = nullptr;
static volatile bool  g_overlayRunning = false;

// Overlay thread – creates the transparent window over CS2 and runs the render loop
DWORD WINAPI OverlayThreadProc(LPVOID param) {
    HINSTANCE hInst = (HINSTANCE)param;

    g_pOverlayWnd = new OverlayWindow();
    if (!g_pOverlayWnd->Create((HMODULE)hInst)) {
        delete g_pOverlayWnd;
        g_pOverlayWnd = nullptr;
        g_overlayRunning = false;
        return 1;
    }

    g_overlayRunning = true;

    // Render loop (~60 fps) – menu auto-shows on first frame
    while (g_overlayRunning) {
        g_pOverlayWnd->RunFrame();
        Sleep(16);
    }

    g_pOverlayWnd->Destroy();
    delete g_pOverlayWnd;
    g_pOverlayWnd = nullptr;
    return 0;
}

// ============================================================================
// ENTRY
// ============================================================================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    g_hInst = hInst;

    fTitle = CreateFont(18, 0,0,0, FW_BOLD,     0,0,0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI");
    fSub   = CreateFont(11, 0,0,0, FW_NORMAL,   0,0,0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI");
    fBody  = CreateFont(12, 0,0,0, FW_NORMAL,   0,0,0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI");
    fSmall = CreateFont(10, 0,0,0, FW_NORMAL,   0,0,0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI");
    fBtn   = CreateFont(12, 0,0,0, FW_SEMIBOLD, 0,0,0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe UI");
    fIcon  = CreateFont(14, 0,0,0, FW_NORMAL,   0,0,0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, "Segoe MDL2 Assets");

    WNDCLASSEX wc = { sizeof(wc) };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance      = hInst;
    wc.lpszClassName  = "ACLDR";
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground  = CreateSolidBrush(RGB(8, 8, 8));
    RegisterClassEx(&wc);

    int sx = (GetSystemMetrics(SM_CXSCREEN) - WIN_W) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - WIN_H) / 2;

    g_hWnd = CreateWindowEx(WS_EX_LAYERED, "ACLDR", "AC Changer",
        WS_POPUP, sx, sy, WIN_W, WIN_H, NULL, NULL, hInst, NULL);
    SetLayeredWindowAttributes(g_hWnd, 0, 250, LWA_ALPHA);

    // Rounded window clipping – makes the OS clip to a rounded shape
    HRGN rgn = CreateRoundRectRgn(0, 0, WIN_W + 1, WIN_H + 1, RAD * 2, RAD * 2);
    SetWindowRgn(g_hWnd, rgn, TRUE);

    ShowWindow(g_hWnd, nShow);
    UpdateWindow(g_hWnd);

    // ---- Message loop ----
    // Let TranslateMessage / DispatchMessage handle keyboard naturally.
    // Only intercept Tab / Enter so our own WndProc processes them.
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN &&
            (msg.wParam == VK_TAB || msg.wParam == VK_RETURN)) {
            SendMessage(g_hWnd, msg.message, msg.wParam, msg.lParam);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteObject(fTitle); DeleteObject(fSub); DeleteObject(fBody);
    DeleteObject(fSmall); DeleteObject(fBtn); DeleteObject(fIcon);
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
    v[0] = { rc.left,  rc.top,    (COLOR16)(8<<8), (COLOR16)(8<<8), (COLOR16)(8<<8), 0 };
    v[1] = { rc.right, rc.bottom, (COLOR16)(18<<8), (COLOR16)(18<<8), (COLOR16)(20<<8), 0 };
    GRADIENT_RECT gr = { 0, 1 };
    GradientFill(hdc, v, 2, &gr, 1, GRADIENT_FILL_RECT_V);
}

void RRect(HDC hdc, int x, int y, int w, int h, int r, COLORREF fill, COLORREF brd) {
    HBRUSH br = CreateSolidBrush(fill);
    HPEN   pn = CreatePen(PS_SOLID, 1, brd);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, br);
    HPEN   op = (HPEN)SelectObject(hdc, pn);
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
        for (int i = 0; i < 3; i++) {
            DWORD style = WS_CHILD | ES_AUTOHSCROLL;
            if (i == 2) style |= ES_PASSWORD;
            g_edits[i] = CreateWindowEx(0, "EDIT", "", style,
                -500, 0, 10, 10, hwnd, (HMENU)(UINT_PTR)(200 + i), g_hInst, NULL);
            SendMessage(g_edits[i], WM_SETFONT, (WPARAM)fBody, TRUE);
        }
        if (LoadCfg()) {
            g_loggedIn  = true;
            g_status    = "Velkommen tilbage, " + g_user + "!";
            g_statusClr = C_GREEN;
        }
        g_closeRect = { WIN_W - 32, 4, 26, 22 };
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        HDC  mem = CreateCompatibleDC(hdc);
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

        if (HitTest(g_closeRect, mx, my)) { PostQuitMessage(0); return 0; }

        // Title-bar drag
        if (my < TITLE_H && mx < WIN_W - 32) {
            g_dragging = true; g_dragPt = { mx, my };
            SetCapture(hwnd); return 0;
        }

        if (!g_loggedIn) {
            bool hitField = false;
            for (int i = 0; i < g_fieldCount; i++) {
                if (HitTest(g_fieldRects[i], mx, my)) {
                    int eidx = g_fieldEditIdx[i];
                    g_focus = eidx;
                    SetFocus(g_edits[eidx]);
                    hitField = true;
                    break;
                }
            }

            if (!hitField && HitTest(g_btnRect, mx, my)) {
                if (g_signupMode) DoSignup(); else DoLogin();
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            if (!hitField && HitTest(g_linkRect, mx, my)) {
                g_signupMode = !g_signupMode;
                g_status.clear(); g_focus = -1;
                for (int i = 0; i < 3; i++) { SetWindowText(g_edits[i], ""); g_bufs[i][0] = 0; }
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            if (!hitField) { g_focus = -1; SetFocus(hwnd); }
        } else {
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
            SetWindowPos(hwnd, NULL, pt.x - g_dragPt.x, pt.y - g_dragPt.y,
                         0, 0, SWP_NOSIZE | SWP_NOZORDER);
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
                if (g_focus < start) g_focus = start;
                else g_focus = (g_focus >= 2) ? start : g_focus + 1;
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
        bool overField = false;
        for (int i = 0; i < g_fieldCount; i++)
            if (HitTest(g_fieldRects[i], pt.x, pt.y)) { overField = true; break; }
        bool hand = HitTest(g_btnRect, pt.x, pt.y) ||
                    HitTest(g_closeRect, pt.x, pt.y) ||
                    HitTest(g_linkRect, pt.x, pt.y);
        SetCursor(LoadCursor(NULL, overField ? IDC_IBEAM : (hand ? IDC_HAND : IDC_ARROW)));
        return TRUE;
    }

    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

// ============================================================================
// MASTER PAINT
// ============================================================================
void Paint(HWND hwnd, HDC hdc) {
    RECT rc; GetClientRect(hwnd, &rc);
    DrawGradient(hdc, rc);

    SetBkMode(hdc, TRANSPARENT);

    // Title bar
    RRectFill(hdc, 0, 0, rc.right, TITLE_H, 0, RGB(6, 6, 6));

    // Separator
    HPEN sp = CreatePen(PS_SOLID, 1, C_BORDER);
    SelectObject(hdc, sp);
    MoveToEx(hdc, 0, TITLE_H, NULL); LineTo(hdc, rc.right, TITLE_H);
    DeleteObject(sp);

    // Title text
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, C_TEXT3);
    RECT tbR = { 12, 0, 180, TITLE_H };
    DrawText(hdc, "AC Changer", -1, &tbR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Close button
    if (g_closeHover)
        RRectFill(hdc, g_closeRect.x, g_closeRect.y, g_closeRect.w, g_closeRect.h, 6, RGB(60, 15, 15));
    DrawGlyph(hdc, g_closeRect.x, g_closeRect.y, g_closeRect.w, g_closeRect.h,
              0xE711, g_closeHover ? C_RED : C_TEXT3, 10);

    if (g_loggedIn) PaintDash(hdc, rc);
    else            PaintLogin(hdc, rc);
}

// ============================================================================
// LOGIN VIEW
// ============================================================================
void PaintLogin(HDC hdc, RECT& rc) {
    int cx = rc.right / 2;
    int y = TITLE_H + 14;

    // Logo
    DrawLogo(hdc, cx, y + 16);
    y += 38;

    // Title
    SelectObject(hdc, fTitle);
    SetTextColor(hdc, C_WHITE);
    RECT tr = { 0, y, rc.right, y + 22 };
    DrawText(hdc, "AC CHANGER", -1, &tr, DT_CENTER | DT_SINGLELINE);
    y += 22;

    // Subtitle
    SelectObject(hdc, fSub);
    SetTextColor(hdc, C_TEXT2);
    RECT sr = { 0, y, rc.right, y + 14 };
    DrawText(hdc, "Premium Software", -1, &sr, DT_CENTER | DT_SINGLELINE);
    y += 18;

    // Separator
    HPEN sep = CreatePen(PS_SOLID, 1, C_BORDER);
    SelectObject(hdc, sep);
    MoveToEx(hdc, MARGIN + 40, y, NULL);
    LineTo(hdc, WIN_W - MARGIN - 40, y);
    DeleteObject(sep);
    y += 12;

    // Reset field tracking
    g_fieldCount = 0;

    if (g_signupMode) {
        y = DrawFieldRow(hdc, MARGIN, y, FIELD_W, 0,
                         0xE192, "License (AC-XXXX)", false);
        y += GAP;
    }

    y = DrawFieldRow(hdc, MARGIN, y, FIELD_W, 1,
                     0xE77B, "Brugernavn", false);
    y += GAP;

    y = DrawFieldRow(hdc, MARGIN, y, FIELD_W, 2,
                     0xE72E, "Adgangskode", true);
    y += 6;

    // Forgot password
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, C_TEXT3);
    RECT fpR = { MARGIN, y, WIN_W - MARGIN, y + 12 };
    DrawText(hdc, "Glemt kode? Kontakt support.", -1, &fpR, DT_LEFT | DT_SINGLELINE);
    y += 20;

    // Button
    const char* btnTxt = g_signupMode ? "Opret Konto" : "Log Ind";
    DrawBtn(hdc, MARGIN, y, FIELD_W, BTN_H, btnTxt, 0xE76C,
            g_btnHover, C_BTN_BG, C_BTN_HOV);
    g_btnRect = { MARGIN, y, FIELD_W, BTN_H };
    y += BTN_H + 10;

    // Toggle link
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, g_linkHover ? C_ACCENT_L : C_TEXT2);
    RECT lkR = { 0, y, rc.right, y + 14 };
    if (g_signupMode)
        DrawText(hdc, "Har konto? Log ind", -1, &lkR, DT_CENTER | DT_SINGLELINE);
    else
        DrawText(hdc, "Ingen konto? Opret her", -1, &lkR, DT_CENTER | DT_SINGLELINE);
    g_linkRect = { 0, y, rc.right, 14 };
    y += 18;

    // Status
    if (!g_status.empty()) {
        SelectObject(hdc, fSmall);
        SetTextColor(hdc, g_statusClr);
        RECT stR = { MARGIN, y, WIN_W - MARGIN, y + 30 };
        DrawText(hdc, g_status.c_str(), -1, &stR, DT_CENTER | DT_WORDBREAK);
    }

    // Footer
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, RGB(35, 35, 40));
    RECT ft = { 0, rc.bottom - 16, rc.right, rc.bottom - 2 };
    DrawText(hdc, "AC Changer 2024-2026", -1, &ft, DT_CENTER | DT_SINGLELINE);
}

// ============================================================================
// DASHBOARD VIEW
// ============================================================================
void PaintDash(HDC hdc, RECT& rc) {
    int cx = rc.right / 2;
    int y = TITLE_H + 14;

    DrawLogo(hdc, cx, y + 16);
    y += 38;

    SelectObject(hdc, fTitle);
    SetTextColor(hdc, C_WHITE);
    RECT tr = { 0, y, rc.right, y + 22 };
    DrawText(hdc, "AC CHANGER", -1, &tr, DT_CENTER | DT_SINGLELINE);
    y += 22;

    SelectObject(hdc, fSub);
    SetTextColor(hdc, C_TEXT2);
    RECT sr = { 0, y, rc.right, y + 14 };
    DrawText(hdc, "Premium Software", -1, &sr, DT_CENTER | DT_SINGLELINE);
    y += 18;

    HPEN sep = CreatePen(PS_SOLID, 1, C_BORDER);
    SelectObject(hdc, sep);
    MoveToEx(hdc, MARGIN + 40, y, NULL);
    LineTo(hdc, WIN_W - MARGIN - 40, y);
    DeleteObject(sep);
    y += 14;

    // Game cards
    int cardW = 56, cardH = 56, cgap = 10;
    int totalW = cardW * 3 + cgap * 2;
    int startX = (WIN_W - totalW) / 2;

    for (int i = 0; i < 3; i++) {
        int x = startX + i * (cardW + cgap);
        COLORREF cardBrd = (i == 1) ? C_ACCENT : C_BORDER;
        RRect(hdc, x, y, cardW, cardH, 10, C_PANEL, cardBrd);
        DrawCardIcon(hdc, x, y, cardW, cardH, i, (i == 1) ? C_ACCENT_L : C_TEXT3);
    }
    y += cardH + 12;

    // Game name
    SelectObject(hdc, fBtn);
    SetTextColor(hdc, C_WHITE);
    RECT gn = { 0, y, rc.right, y + 18 };
    DrawText(hdc, "Counter-Strike 2", -1, &gn, DT_CENTER | DT_SINGLELINE);
    y += 22;

    // UNDETECTED badge
    int bw = 100, bh = 20;
    int bx = cx - bw / 2;
    RRect(hdc, bx, y, bw, bh, bh / 2, C_GREEN_DIM, C_GREEN);
    DrawGlyph(hdc, bx + 6, y, 16, bh, 0xE73E, C_GREEN, 9);
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, C_GREEN);
    RECT bd = { bx + 20, y, bx + bw, y + bh };
    DrawText(hdc, "UNDETECTED", -1, &bd, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    y += bh + 12;

    // Expiry panel
    RRect(hdc, MARGIN, y, FIELD_W, 34, RAD, C_FIELD_BG, C_FIELD_BR);
    DrawGlyph(hdc, MARGIN + 8, y, 18, 34, 0xE823, C_TEXT2, 12);
    SelectObject(hdc, fBody);
    SetTextColor(hdc, C_TEXT2);
    RECT exL = { MARGIN + 28, y, MARGIN + 150, y + 34 };
    DrawText(hdc, "Udl\xF8" "bsdato:", -1, &exL, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SetTextColor(hdc, C_GREEN);
    RECT exV = { MARGIN + 10, y, WIN_W - MARGIN - 10, y + 34 };
    DrawText(hdc, "2026-12-31", -1, &exV, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    y += 34 + 12;

    // Load button
    DrawBtn(hdc, MARGIN, y, FIELD_W, BTN_H, "Load Cheat", 0xE768,
            g_btnHover, C_BTN_BG, C_BTN_HOV);
    g_btnRect = { MARGIN, y, FIELD_W, BTN_H };
    y += BTN_H + 10;

    // Status
    if (!g_status.empty()) {
        SelectObject(hdc, fSmall);
        SetTextColor(hdc, g_statusClr);
        RECT st = { MARGIN, y, WIN_W - MARGIN, y + 30 };
        DrawText(hdc, g_status.c_str(), -1, &st, DT_CENTER | DT_WORDBREAK);
    }

    // Footer
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, RGB(35, 35, 40));
    RECT ft = { 0, rc.bottom - 16, rc.right, rc.bottom - 2 };
    DrawText(hdc, "AC Changer 2024-2026", -1, &ft, DT_CENTER | DT_SINGLELINE);
}

// ============================================================================
// DRAW GLYPH – Segoe MDL2 Assets icon
// ============================================================================
void DrawGlyph(HDC hdc, int x, int y, int w, int h, wchar_t glyph, COLORREF color, int size) {
    HFONT f = CreateFontW(size, 0,0,0, FW_NORMAL, 0,0,0,
                          DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, 0, L"Segoe MDL2 Assets");
    HFONT of = (HFONT)SelectObject(hdc, f);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    RECT r = { x, y, x + w, y + h };
    wchar_t buf[2] = { glyph, 0 };
    DrawTextW(hdc, buf, 1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, of);
    DeleteObject(f);
}

// ============================================================================
// DRAW CARD ICON – GDI vector shapes
// ============================================================================
void DrawCardIcon(HDC hdc, int x, int y, int w, int h, int type, COLORREF color) {
    int cx = x + w / 2, cy = y + h / 2;
    HPEN pen = CreatePen(PS_SOLID, 2, color);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    SelectObject(hdc, (HBRUSH)GetStockObject(NULL_BRUSH));

    switch (type) {
    case 0: {
        // Crosshair
        int r = 10;
        Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
        Ellipse(hdc, cx - r/2, cy - r/2, cx + r/2, cy + r/2);
        MoveToEx(hdc, cx - r - 3, cy, NULL); LineTo(hdc, cx - r/2, cy);
        MoveToEx(hdc, cx + r/2, cy, NULL);   LineTo(hdc, cx + r + 3, cy);
        MoveToEx(hdc, cx, cy - r - 3, NULL); LineTo(hdc, cx, cy - r/2);
        MoveToEx(hdc, cx, cy + r/2, NULL);   LineTo(hdc, cx, cy + r + 3);
        break;
    }
    case 1: {
        // Sword
        POINT blade[] = {
            { cx, cy-14 }, { cx+3, cy-10 }, { cx+2, cy+3 }, { cx+6, cy+6 },
            { cx+4, cy+8 }, { cx+1, cy+5 }, { cx, cy+11 },
            { cx-1, cy+5 }, { cx-4, cy+8 }, { cx-6, cy+6 },
            { cx-2, cy+3 }, { cx-3, cy-10 }
        };
        HBRUSH fb = CreateSolidBrush(color);
        SelectObject(hdc, fb);
        Polygon(hdc, blade, 12);
        DeleteObject(fb);
        HPEN tp = CreatePen(PS_SOLID, 2, color);
        SelectObject(hdc, tp);
        MoveToEx(hdc, cx - 8, cy + 3, NULL); LineTo(hdc, cx + 8, cy + 3);
        SelectObject(hdc, pen);
        DeleteObject(tp);
        break;
    }
    case 2: {
        // Shield
        POINT shield[] = {
            { cx, cy-12 }, { cx+10, cy-7 }, { cx+10, cy+1 },
            { cx+6, cy+9 }, { cx, cy+14 },
            { cx-6, cy+9 }, { cx-10, cy+1 }, { cx-10, cy-7 }
        };
        Polygon(hdc, shield, 8);
        MoveToEx(hdc, cx - 4, cy, NULL);
        LineTo(hdc, cx - 1, cy + 4);
        LineTo(hdc, cx + 5, cy - 4);
        break;
    }
    }

    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

// ============================================================================
// DRAW FIELD ROW – returns bottom Y, stores hit rect + edit index
// ============================================================================
int DrawFieldRow(HDC hdc, int x, int y, int w, int idx,
                 wchar_t icon, const char* placeholder, bool isPwd) {
    bool focused = (g_focus == idx);
    COLORREF brd = focused ? C_FIELD_FOC : C_FIELD_BR;

    RRect(hdc, x, y, w, FIELD_H, RAD, C_FIELD_BG, brd);

    // Focus glow
    if (focused) {
        HPEN gp = CreatePen(PS_SOLID, 1, RGB(80, 80, 90));
        SelectObject(hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
        SelectObject(hdc, gp);
        RoundRect(hdc, x - 1, y - 1, x + w + 1, y + FIELD_H + 1, RAD * 2 + 2, RAD * 2 + 2);
        DeleteObject(gp);
    }

    // Icon
    DrawGlyph(hdc, x + 6, y, 26, FIELD_H, icon, focused ? C_ACCENT_L : C_TEXT3, 13);

    // Divider
    HPEN dp = CreatePen(PS_SOLID, 1, C_BORDER);
    SelectObject(hdc, dp);
    MoveToEx(hdc, x + 36, y + 8, NULL);
    LineTo(hdc, x + 36, y + FIELD_H - 8);
    DeleteObject(dp);

    // Text
    SelectObject(hdc, fBody);
    RECT tR = { x + 44, y, x + w - 8, y + FIELD_H };

    const char* text = g_bufs[idx];
    if (strlen(text) == 0) {
        SetTextColor(hdc, C_TEXT3);
        DrawText(hdc, placeholder, -1, &tR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else if (isPwd) {
        SetTextColor(hdc, C_TEXT);
        wchar_t bullet = 0x25CF;
        std::wstring wb(strlen(text), bullet);
        RECT bR = tR;
        DrawTextW(hdc, wb.c_str(), (int)wb.size(), &bR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else {
        SetTextColor(hdc, C_TEXT);
        DrawText(hdc, text, -1, &tR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    // Blinking cursor
    if (focused) {
        SIZE sz;
        GetTextExtentPoint32(hdc, text, (int)strlen(text), &sz);
        int curX = x + 44 + (isPwd ? (int)strlen(text) * 6 : sz.cx);
        if (curX < x + w - 10) {
            HPEN cp = CreatePen(PS_SOLID, 1, C_ACCENT_L);
            SelectObject(hdc, cp);
            MoveToEx(hdc, curX + 1, y + 9, NULL);
            LineTo(hdc, curX + 1, y + FIELD_H - 9);
            DeleteObject(cp);
        }
    }

    // Store hit rect AND the real edit index
    g_fieldRects[g_fieldCount]   = { x, y, w, FIELD_H };
    g_fieldEditIdx[g_fieldCount] = idx;
    g_fieldCount++;

    return y + FIELD_H;
}

// ============================================================================
// DRAW BUTTON
// ============================================================================
void DrawBtn(HDC hdc, int x, int y, int w, int h, const char* text,
             wchar_t icon, bool hover, COLORREF bg, COLORREF bgH) {
    COLORREF c   = hover ? bgH : bg;
    COLORREF brd = hover ? C_ACCENT : C_BORDER;
    RRect(hdc, x, y, w, h, RAD, c, brd);

    if (hover) {
        HPEN gp = CreatePen(PS_SOLID, 1, RGB(70, 70, 80));
        SelectObject(hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
        SelectObject(hdc, gp);
        RoundRect(hdc, x - 1, y - 1, x + w + 1, y + h + 1, RAD * 2 + 2, RAD * 2 + 2);
        DeleteObject(gp);
    }

    COLORREF txtClr = hover ? C_WHITE : C_TEXT;

    SIZE textSz;
    SelectObject(hdc, fBtn);
    GetTextExtentPoint32(hdc, text, (int)strlen(text), &textSz);
    int totalW = 16 + 6 + textSz.cx;
    int startX = x + (w - totalW) / 2;

    DrawGlyph(hdc, startX, y, 16, h, icon, txtClr, 12);

    SelectObject(hdc, fBtn);
    SetTextColor(hdc, txtClr);
    RECT tr = { startX + 22, y, x + w, y + h };
    DrawText(hdc, text, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================================
// LOGO – gradient-filled rounded rect with "AC" text
// ============================================================================
void DrawLogo(HDC hdc, int cx, int cy) {
    int sz = 30;
    int lx = cx - sz / 2, ly = cy - sz / 2;

    // Glow rings
    for (int i = 3; i >= 1; i--) {
        HPEN gp = CreatePen(PS_SOLID, 1, RGB(40 + i*10, 40 + i*10, 45 + i*10));
        SelectObject(hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
        SelectObject(hdc, gp);
        RoundRect(hdc, lx - i*2, ly - i*2, lx + sz + i*2, ly + sz + i*2,
                  12 + i*2, 12 + i*2);
        DeleteObject(gp);
    }

    // Gradient fill clipped to rounded rect
    HRGN clip = CreateRoundRectRgn(lx, ly, lx + sz + 1, ly + sz + 1, 12, 12);
    SelectClipRgn(hdc, clip);

    TRIVERTEX v[2];
    v[0].x = lx;      v[0].y = ly;
    v[0].Red   = (COLOR16)(80  << 8);
    v[0].Green = (COLOR16)(80  << 8);
    v[0].Blue  = (COLOR16)(90  << 8);
    v[0].Alpha = 0;
    v[1].x = lx + sz; v[1].y = ly + sz;
    v[1].Red   = (COLOR16)(140 << 8);
    v[1].Green = (COLOR16)(140 << 8);
    v[1].Blue  = (COLOR16)(150 << 8);
    v[1].Alpha = 0;

    GRADIENT_RECT gr = { 0, 1 };
    GradientFill(hdc, v, 2, &gr, 1, GRADIENT_FILL_RECT_V);

    SelectClipRgn(hdc, NULL);
    DeleteObject(clip);

    // Border
    HPEN bp = CreatePen(PS_SOLID, 1, C_ACCENT);
    SelectObject(hdc, bp);
    SelectObject(hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
    RoundRect(hdc, lx, ly, lx + sz, ly + sz, 12, 12);
    DeleteObject(bp);

    // "AC" text
    HFONT logoFont = CreateFont(14, 0,0,0, FW_BOLD, 0,0,0,
                                DEFAULT_CHARSET, OUT_TT_PRECIS,
                                CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                0, "Segoe UI");
    HFONT oldF = (HFONT)SelectObject(hdc, logoFont);
    SetTextColor(hdc, C_WHITE);
    RECT lr = { lx, ly, lx + sz, ly + sz };
    DrawText(hdc, "AC", -1, &lr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldF);
    DeleteObject(logoFont);
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
    SetStat("Konto oprettet! HWID l\xE5" "st.", C_GREEN);
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
            if (CreateProcessA(p, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
                ok = true; break;
            }
        }
    }

    if (!ok) {
        HKEY hk; char sp[MAX_PATH] = "";
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                "SOFTWARE\\WOW6432Node\\Valve\\Steam", 0, KEY_READ, &hk) == ERROR_SUCCESS) {
            DWORD sz = sizeof(sp);
            RegQueryValueExA(hk, "InstallPath", NULL, NULL, (LPBYTE)sp, &sz);
            RegCloseKey(hk);
            std::string full = std::string(sp) + "\\steam.exe";
            if (CreateProcessA(full.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hProcess); CloseHandle(pi.hThread); ok = true;
            }
        }
    }

    if (!ok) { SetStat("Fejl: Steam ikke fundet!", C_RED); return false; }

    SetStat("Venter p\xE5" " Steam...", C_TEXT2);
    Sleep(8000);

    SetStat("Starter CS2...", C_TEXT2);
    ShellExecuteA(NULL, "open", "steam://rungameid/730", NULL, NULL, SW_SHOWNORMAL);

    SetStat("Venter p\xE5" " CS2...", C_TEXT2);
    DWORD cs2 = 0;
    for (int i = 0; i < 120 && !cs2; i++) {
        cs2 = FindProc("cs2.exe");
        if (!cs2) {
            char m[64]; sprintf_s(m, "Venter p\xE5" " CS2... %d sek", i + 1);
            SetStat(m, C_TEXT2); Sleep(1000);
        }
    }
    if (!cs2) { SetStat("Fejl: CS2 ikke fundet!", C_RED); return false; }

    SetStat("CS2 fundet! Forbereder...", C_ACCENT);
    Sleep(5000);

    SetStat("Injicerer overlay...", C_ACCENT);

    // Verify we can access the CS2 process
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, cs2);
    if (!h) { SetStat("Fejl: K\xF8" "r som administrator!", C_RED); return false; }
    CloseHandle(h);

    // Start the overlay window on a background thread
    HANDLE hThread = CreateThread(NULL, 0, OverlayThreadProc, (LPVOID)g_hInst, 0, NULL);
    if (!hThread) {
        SetStat("Fejl: Kunne ikke starte overlay!", C_RED);
        return false;
    }
    CloseHandle(hThread);

    // Wait briefly for overlay to initialise
    Sleep(1500);

    if (!g_overlayRunning) {
        SetStat("Fejl: Overlay kunne ikke finde CS2-vinduet!", C_RED);
        return false;
    }

    SetStat("Overlay aktiv! Tryk INSERT.", C_GREEN);
    MessageBox(hwnd, "Overlay er aktivt!\n\nMenuen vises automatisk.\nTryk INSERT for at \xE5" "bne/lukke.",
        "AC Changer", MB_OK | MB_ICONINFORMATION);
    return true;
}
