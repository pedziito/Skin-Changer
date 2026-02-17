// ============================================================================
//  AC Changer – Loader  (NL-style design)
//  Matches the dark subscription-panel look
// ============================================================================
#include <windows.h>
#include <wingdi.h>
#include <tlhelp32.h>
#include <string>
#include <fstream>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include "GameMenu.h"

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

// ============================================================================
//  Debug logger
// ============================================================================
static void DbgLog(const char* fmt, ...) {
    FILE* f = nullptr;
    fopen_s(&f, "ac_debug.log", "a");
    if (!f) return;
    time_t now = time(nullptr);
    struct tm t; localtime_s(&t, &now);
    fprintf(f, "[%02d:%02d:%02d] ", t.tm_hour, t.tm_min, t.tm_sec);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fprintf(f, "\n");
    fclose(f);
}

// ============================================================================
//  Layout
// ============================================================================
#define WIN_W       500
#define WIN_H       400
#define SIDEBAR_W   130
#define TITLE_H     0     // no separate title bar – integrated
#define CONTENT_X   SIDEBAR_W
#define CONTENT_W   (WIN_W - SIDEBAR_W)

// ============================================================================
//  Colors – dark navy theme matching the reference
// ============================================================================
#define C_BG         RGB(15, 17, 26)
#define C_SIDEBAR    RGB(15, 17, 26)
#define C_SIDEBAR_BR RGB(30, 34, 50)
#define C_CARD       RGB(24, 28, 42)
#define C_CARD_HOV   RGB(30, 35, 52)
#define C_CARD_BR    RGB(40, 46, 65)
#define C_ACCENT     RGB(70, 130, 220)
#define C_ACCENT_L   RGB(100, 160, 245)
#define C_GREEN      RGB(45, 200, 120)
#define C_RED        RGB(210, 60, 60)
#define C_TEXT       RGB(200, 205, 220)
#define C_TEXT2      RGB(120, 128, 150)
#define C_TEXT3      RGB(70, 76, 95)
#define C_WHITE      RGB(235, 238, 245)
#define C_FIELD      RGB(24, 28, 42)
#define C_FIELD_BR   RGB(45, 50, 70)
#define C_FIELD_FC   RGB(70, 130, 220)
#define C_BTN_BG     RGB(55, 110, 200)
#define C_BTN_HOV    RGB(70, 130, 220)
#define C_ICON_CS    RGB(220, 165, 40)

// ============================================================================
//  State
// ============================================================================
struct HitRect { int x, y, w, h; };

static HINSTANCE g_hInst;
static HWND      g_hWnd;
static bool      g_loggedIn   = false;
static bool      g_signupMode = false;
static bool      g_dragging   = false;
static POINT     g_dragPt;
static int       g_focus      = -1;

static bool g_btnHover   = false;
static bool g_linkHover  = false;
static bool g_closeHover = false;
static bool g_minHover   = false;
static bool g_cardHover  = false;
static bool g_sideHover[3] = {};

static std::string g_user, g_pass, g_license, g_hwid;
static std::string g_status;
static COLORREF    g_statusClr = C_TEXT2;

static HWND g_edits[3];
static char g_bufs[3][256] = {};

static HitRect g_fieldRects[3] = {};
static int     g_fieldIdx[3]   = {};
static int     g_fieldCount    = 0;
static HitRect g_btnRect   = {};
static HitRect g_linkRect  = {};
static HitRect g_closeRect = {};
static HitRect g_minRect   = {};
static HitRect g_cardRect  = {};
static HitRect g_sideRects[3] = {};

static HFONT fTitle, fBody, fSmall, fBtn, fLogo, fNav;

// Overlay
static OverlayWindow* g_pOverlayWnd  = nullptr;
static volatile bool  g_overlayRunning = false;
static DWORD          g_cs2Pid = 0;

// ============================================================================
//  Forward declarations
// ============================================================================
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Paint(HDC);
void PaintSidebar(HDC, RECT&);
void PaintLogin(HDC, RECT&);
void PaintDash(HDC, RECT&);
void RRect(HDC, int, int, int, int, int, COLORREF, COLORREF);
void SetStat(const char*, COLORREF);
bool HitTest(HitRect&, int, int);
int  DrawField(HDC, int, int, int, int, const char*, bool);
void DrawButton(HDC, int, int, int, int, const char*, bool);

bool DoLogin();
bool DoSignup();
bool SaveCfg();
bool LoadCfg();
std::string GetHWID();
DWORD FindProc(const char*);
bool  KillProc(const char*);
bool  LaunchGame(HWND);

// ============================================================================
//  Overlay thread
// ============================================================================
DWORD WINAPI OverlayThreadProc(LPVOID param) {
    DbgLog("=== OverlayThreadProc START ===");
    HINSTANCE hInst = (HINSTANCE)param;
    g_pOverlayWnd = new OverlayWindow();
    DbgLog("Calling OverlayWindow::Create with PID=%d...", (int)g_cs2Pid);
    if (!g_pOverlayWnd->Create((HMODULE)hInst, g_cs2Pid)) {
        DbgLog("ERROR: OverlayWindow::Create FAILED");
        delete g_pOverlayWnd;
        g_pOverlayWnd = nullptr;
        g_overlayRunning = false;
        return 1;
    }
    DbgLog("OverlayWindow::Create succeeded, starting render loop");
    g_overlayRunning = true;
    while (g_overlayRunning) {
        g_pOverlayWnd->RunFrame();
        Sleep(16);
    }
    DbgLog("Render loop ended, destroying overlay");
    g_pOverlayWnd->Destroy();
    delete g_pOverlayWnd;
    g_pOverlayWnd = nullptr;
    return 0;
}

// ============================================================================
//  WinMain
// ============================================================================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    g_hInst = hInst;

    fLogo  = CreateFont(28, 0, 0, 0, FW_BOLD,   0, 0, 0, DEFAULT_CHARSET,
                        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                        0, "Segoe UI");
    fTitle = CreateFont(20, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
                        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                        0, "Segoe UI");
    fNav   = CreateFont(13, 0, 0, 0, FW_NORMAL,   0, 0, 0, DEFAULT_CHARSET,
                        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                        0, "Segoe UI");
    fBody  = CreateFont(13, 0, 0, 0, FW_NORMAL,   0, 0, 0, DEFAULT_CHARSET,
                        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                        0, "Segoe UI");
    fSmall = CreateFont(11, 0, 0, 0, FW_NORMAL,   0, 0, 0, DEFAULT_CHARSET,
                        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                        0, "Segoe UI");
    fBtn   = CreateFont(13, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
                        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                        0, "Segoe UI");

    WNDCLASSEX wc = { sizeof(wc) };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance      = hInst;
    wc.lpszClassName  = "ACLDR";
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground  = CreateSolidBrush(C_BG);
    RegisterClassEx(&wc);

    int sx = (GetSystemMetrics(SM_CXSCREEN) - WIN_W) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - WIN_H) / 2;

    g_hWnd = CreateWindowEx(WS_EX_LAYERED, "ACLDR", "AC Changer",
        WS_POPUP, sx, sy, WIN_W, WIN_H, NULL, NULL, hInst, NULL);
    SetLayeredWindowAttributes(g_hWnd, 0, 248, LWA_ALPHA);

    HRGN rgn = CreateRoundRectRgn(0, 0, WIN_W + 1, WIN_H + 1, 16, 16);
    SetWindowRgn(g_hWnd, rgn, TRUE);

    ShowWindow(g_hWnd, nShow);
    UpdateWindow(g_hWnd);

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

    DeleteObject(fLogo); DeleteObject(fTitle); DeleteObject(fNav);
    DeleteObject(fBody); DeleteObject(fSmall); DeleteObject(fBtn);
    return (int)msg.wParam;
}

// ============================================================================
//  Helpers
// ============================================================================
bool HitTest(HitRect& r, int mx, int my) {
    return mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
}

void SetStat(const char* msg, COLORREF c) {
    g_status = msg; g_statusClr = c;
    InvalidateRect(g_hWnd, NULL, FALSE);
    UpdateWindow(g_hWnd);
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

int DrawField(HDC hdc, int x, int y, int w, int editIdx,
              const char* placeholder, bool isPwd) {
    bool focused = (g_focus == editIdx);
    COLORREF brd = focused ? C_FIELD_FC : C_FIELD_BR;
    RRect(hdc, x, y, w, 36, 6, C_FIELD, brd);

    SelectObject(hdc, fBody);
    SetBkMode(hdc, TRANSPARENT);
    RECT tR = { x + 14, y, x + w - 14, y + 36 };

    const char* text = g_bufs[editIdx];
    if (strlen(text) == 0) {
        SetTextColor(hdc, C_TEXT3);
        DrawText(hdc, placeholder, -1, &tR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else if (isPwd) {
        SetTextColor(hdc, C_TEXT);
        std::string dots(strlen(text), '*');
        DrawText(hdc, dots.c_str(), -1, &tR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else {
        SetTextColor(hdc, C_TEXT);
        DrawText(hdc, text, -1, &tR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    g_fieldRects[g_fieldCount] = { x, y, w, 36 };
    g_fieldIdx[g_fieldCount]   = editIdx;
    g_fieldCount++;
    return y + 36;
}

void DrawButton(HDC hdc, int x, int y, int w, int h,
                const char* text, bool hover) {
    COLORREF bg = hover ? C_BTN_HOV : C_BTN_BG;
    RRect(hdc, x, y, w, h, 6, bg, bg);
    SelectObject(hdc, fBtn);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);
    RECT r = { x, y, x + w, y + h };
    DrawText(hdc, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================================
//  WndProc
// ============================================================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {

    case WM_CREATE:
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
        g_closeRect = { WIN_W - 34, 8, 22, 22 };
        g_minRect   = { WIN_W - 60, 8, 22, 22 };
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        SelectObject(mem, bmp);
        Paint(mem);
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
        DeleteObject(bmp); DeleteDC(mem);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_ERASEBKGND: return 1;

    case WM_LBUTTONDOWN: {
        int mx = (short)LOWORD(lp), my = (short)HIWORD(lp);
        if (HitTest(g_closeRect, mx, my)) { PostQuitMessage(0); return 0; }
        if (HitTest(g_minRect, mx, my)) { ShowWindow(hwnd, SW_MINIMIZE); return 0; }

        // Drag from top area or sidebar
        if (my < 40 || mx < SIDEBAR_W) {
            g_dragging = true; g_dragPt = { mx, my };
            SetCapture(hwnd); return 0;
        }

        if (!g_loggedIn) {
            bool hit = false;
            for (int i = 0; i < g_fieldCount; i++) {
                if (HitTest(g_fieldRects[i], mx, my)) {
                    g_focus = g_fieldIdx[i];
                    SetFocus(g_edits[g_focus]);
                    hit = true; break;
                }
            }
            if (!hit && HitTest(g_btnRect, mx, my)) {
                if (g_signupMode) DoSignup(); else DoLogin();
                InvalidateRect(hwnd, NULL, FALSE); return 0;
            }
            if (!hit && HitTest(g_linkRect, mx, my)) {
                g_signupMode = !g_signupMode;
                g_status.clear(); g_focus = -1;
                for (int i = 0; i < 3; i++) {
                    SetWindowText(g_edits[i], ""); g_bufs[i][0] = 0;
                }
                InvalidateRect(hwnd, NULL, FALSE); return 0;
            }
            if (!hit) { g_focus = -1; SetFocus(hwnd); }
        } else {
            if (HitTest(g_cardRect, mx, my)) {
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
        bool mh = HitTest(g_minRect, mx, my);
        bool lh = HitTest(g_linkRect, mx, my);
        bool crh = g_loggedIn && HitTest(g_cardRect, mx, my);
        bool sh[3] = {};
        for (int i = 0; i < 3; i++) sh[i] = HitTest(g_sideRects[i], mx, my);

        bool changed = (bh != g_btnHover || ch != g_closeHover ||
                        mh != g_minHover || lh != g_linkHover || crh != g_cardHover);
        for (int i = 0; i < 3; i++) if (sh[i] != g_sideHover[i]) changed = true;

        if (changed) {
            g_btnHover = bh; g_closeHover = ch; g_minHover = mh;
            g_linkHover = lh; g_cardHover = crh;
            for (int i = 0; i < 3; i++) g_sideHover[i] = sh[i];
            InvalidateRect(hwnd, NULL, FALSE);
        }
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        break;
    }

    case WM_MOUSELEAVE:
        g_btnHover = g_closeHover = g_minHover = g_linkHover = g_cardHover = false;
        for (int i = 0; i < 3; i++) g_sideHover[i] = false;
        InvalidateRect(hwnd, NULL, FALSE); break;

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
        if (wp == VK_TAB && !g_loggedIn) {
            int start = g_signupMode ? 0 : 1;
            if (g_focus < start) g_focus = start;
            else g_focus = (g_focus >= 2) ? start : g_focus + 1;
            SetFocus(g_edits[g_focus]);
            InvalidateRect(hwnd, NULL, FALSE);
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
        bool field = false;
        for (int i = 0; i < g_fieldCount; i++)
            if (HitTest(g_fieldRects[i], pt.x, pt.y)) { field = true; break; }
        bool hand = HitTest(g_btnRect, pt.x, pt.y) ||
                    HitTest(g_closeRect, pt.x, pt.y) ||
                    HitTest(g_minRect, pt.x, pt.y) ||
                    HitTest(g_linkRect, pt.x, pt.y) ||
                    (g_loggedIn && HitTest(g_cardRect, pt.x, pt.y));
        for (int i = 0; i < 3; i++)
            if (HitTest(g_sideRects[i], pt.x, pt.y)) hand = true;
        SetCursor(LoadCursor(NULL, field ? IDC_IBEAM : (hand ? IDC_HAND : IDC_ARROW)));
        return TRUE;
    }

    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

// ============================================================================
//  Sidebar
// ============================================================================
void PaintSidebar(HDC hdc, RECT& rc) {
    // Sidebar background
    RECT sb = { 0, 0, SIDEBAR_W, rc.bottom };
    HBRUSH sbr = CreateSolidBrush(C_SIDEBAR);
    FillRect(hdc, &sb, sbr);
    DeleteObject(sbr);

    // Right border line
    HPEN bp = CreatePen(PS_SOLID, 1, C_SIDEBAR_BR);
    HPEN obp = (HPEN)SelectObject(hdc, bp);
    MoveToEx(hdc, SIDEBAR_W - 1, 0, NULL);
    LineTo(hdc, SIDEBAR_W - 1, rc.bottom);
    SelectObject(hdc, obp);
    DeleteObject(bp);

    SetBkMode(hdc, TRANSPARENT);

    // "AC" logo – top left
    SelectObject(hdc, fLogo);
    SetTextColor(hdc, C_ACCENT);
    RECT lr = { 18, 14, SIDEBAR_W, 50 };
    DrawText(hdc, "AC", -1, &lr, DT_LEFT | DT_SINGLELINE);

    // Navigation items
    const char* navItems[] = { "Website", "Support", "Market" };
    int ny = 80;
    for (int i = 0; i < 3; i++) {
        g_sideRects[i] = { 0, ny, SIDEBAR_W - 1, 30 };
        if (g_sideHover[i]) {
            RECT hr = { 4, ny, SIDEBAR_W - 5, ny + 30 };
            HBRUSH hb = CreateSolidBrush(RGB(25, 30, 45));
            FillRect(hdc, &hr, hb);
            DeleteObject(hb);
        }
        SelectObject(hdc, fNav);
        SetTextColor(hdc, g_sideHover[i] ? C_WHITE : C_TEXT2);
        RECT nr = { 22, ny, SIDEBAR_W - 8, ny + 30 };
        DrawText(hdc, navItems[i], -1, &nr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        ny += 32;
    }

    // User avatar + name at bottom
    int avatarY = rc.bottom - 48;
    // Circle avatar
    HBRUSH ab = CreateSolidBrush(RGB(50, 55, 75));
    HPEN   ap = CreatePen(PS_SOLID, 1, RGB(50, 55, 75));
    SelectObject(hdc, ab); SelectObject(hdc, ap);
    Ellipse(hdc, 16, avatarY, 16 + 30, avatarY + 30);
    DeleteObject(ab); DeleteObject(ap);

    // User initial in circle
    SelectObject(hdc, fBtn);
    SetTextColor(hdc, C_WHITE);
    RECT ir = { 16, avatarY, 46, avatarY + 30 };
    if (!g_user.empty()) {
        char init[2] = { (char)toupper(g_user[0]), 0 };
        DrawText(hdc, init, -1, &ir, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // Username
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, C_TEXT);
    RECT ur = { 52, avatarY, SIDEBAR_W - 8, avatarY + 30 };
    const char* displayName = g_loggedIn ? g_user.c_str() : "Ikke logget ind";
    DrawText(hdc, displayName, -1, &ur, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================================
//  Paint
// ============================================================================
void Paint(HDC hdc) {
    RECT rc; GetClientRect(g_hWnd, &rc);

    // Background
    HBRUSH bg = CreateSolidBrush(C_BG);
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    SetBkMode(hdc, TRANSPARENT);

    // Sidebar
    PaintSidebar(hdc, rc);

    // Window buttons (min / close) – top right
    // Minimize "–"
    if (g_minHover) {
        RRect(hdc, g_minRect.x - 2, g_minRect.y - 2,
              g_minRect.w + 4, g_minRect.h + 4, 4,
              RGB(30, 35, 52), RGB(30, 35, 52));
    }
    SelectObject(hdc, fBody);
    SetTextColor(hdc, g_minHover ? C_WHITE : C_TEXT3);
    RECT mnR = { g_minRect.x, g_minRect.y, g_minRect.x + g_minRect.w, g_minRect.y + g_minRect.h };
    DrawText(hdc, "\xE2\x80\x93", -1, &mnR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Close "x"
    if (g_closeHover) {
        RRect(hdc, g_closeRect.x - 2, g_closeRect.y - 2,
              g_closeRect.w + 4, g_closeRect.h + 4, 4,
              RGB(60, 20, 20), RGB(60, 20, 20));
    }
    SetTextColor(hdc, g_closeHover ? C_RED : C_TEXT3);
    RECT clR = { g_closeRect.x, g_closeRect.y, g_closeRect.x + g_closeRect.w, g_closeRect.y + g_closeRect.h };
    DrawText(hdc, "x", -1, &clR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Content
    if (g_loggedIn) PaintDash(hdc, rc);
    else            PaintLogin(hdc, rc);
}

// ============================================================================
//  Login (shown in right content area)
// ============================================================================
void PaintLogin(HDC hdc, RECT& rc) {
    int cx = CONTENT_X + CONTENT_W / 2;
    int lx = CONTENT_X + 30;
    int rw = CONTENT_W - 60;
    int y = 60;
    g_fieldCount = 0;

    SelectObject(hdc, fTitle);
    SetTextColor(hdc, C_WHITE);
    RECT tR = { CONTENT_X, y, rc.right, y + 28 };
    DrawText(hdc, g_signupMode ? "Opret Konto" : "Log Ind", -1, &tR,
             DT_CENTER | DT_SINGLELINE);
    y += 32;

    SelectObject(hdc, fSmall);
    SetTextColor(hdc, C_TEXT2);
    RECT sR = { CONTENT_X, y, rc.right, y + 16 };
    DrawText(hdc, g_signupMode ? "Opret en ny konto" : "Log ind p\xE5 din konto",
             -1, &sR, DT_CENTER | DT_SINGLELINE);
    y += 40;

    if (g_signupMode) {
        y = DrawField(hdc, lx, y, rw, 0, "License key (AC-XXXX)", false);
        y += 10;
    }
    y = DrawField(hdc, lx, y, rw, 1, "Brugernavn", false);
    y += 10;
    y = DrawField(hdc, lx, y, rw, 2, "Adgangskode", true);
    y += 24;

    DrawButton(hdc, lx, y, rw, 38,
               g_signupMode ? "Opret Konto" : "Log Ind", g_btnHover);
    g_btnRect = { lx, y, rw, 38 };
    y += 38 + 16;

    SelectObject(hdc, fSmall);
    SetTextColor(hdc, g_linkHover ? C_ACCENT_L : C_TEXT2);
    RECT lR = { CONTENT_X, y, rc.right, y + 16 };
    DrawText(hdc, g_signupMode ? "Har du en konto? Log ind" :
             "Ingen konto? Opret her", -1, &lR, DT_CENTER | DT_SINGLELINE);
    g_linkRect = { CONTENT_X, y, CONTENT_W, 16 };
    y += 26;

    if (!g_status.empty()) {
        SelectObject(hdc, fSmall);
        SetTextColor(hdc, g_statusClr);
        RECT stR = { lx, y, lx + rw, y + 36 };
        DrawText(hdc, g_status.c_str(), -1, &stR, DT_CENTER | DT_WORDBREAK);
    }
}

// ============================================================================
//  Dashboard – subscription panel
// ============================================================================
void PaintDash(HDC hdc, RECT& rc) {
    int lx = CONTENT_X + 24;
    int rw = CONTENT_W - 48;
    int y = 22;

    // Title: "Subscription"
    SelectObject(hdc, fTitle);
    SetTextColor(hdc, C_WHITE);
    RECT tR = { lx, y, lx + rw, y + 26 };
    DrawText(hdc, "Subscription", -1, &tR, DT_LEFT | DT_SINGLELINE);
    y += 26;

    // Subtitle
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, C_TEXT2);
    RECT sR = { lx, y, lx + rw, y + 16 };
    DrawText(hdc, "Available subscriptions", -1, &sR, DT_LEFT | DT_SINGLELINE);
    y += 36;

    // ── CS2 Card ──
    int cardH = 64;
    COLORREF ccBg = g_cardHover ? C_CARD_HOV : C_CARD;
    COLORREF ccBr = g_cardHover ? C_ACCENT : C_CARD_BR;
    RRect(hdc, lx, y, rw, cardH, 8, ccBg, ccBr);
    g_cardRect = { lx, y, rw, cardH };

    // CS2 icon circle (orange/gold)
    int iconX = lx + rw - 50;
    int iconY = y + (cardH - 34) / 2;
    HBRUSH ib = CreateSolidBrush(C_ICON_CS);
    HPEN   ip = CreatePen(PS_SOLID, 1, C_ICON_CS);
    SelectObject(hdc, ib); SelectObject(hdc, ip);
    Ellipse(hdc, iconX, iconY, iconX + 34, iconY + 34);
    DeleteObject(ib); DeleteObject(ip);

    // "CS" text in icon
    SelectObject(hdc, fBtn);
    SetTextColor(hdc, RGB(20, 15, 5));
    RECT icR = { iconX, iconY, iconX + 34, iconY + 34 };
    DrawText(hdc, "CS", -1, &icR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Game name
    SelectObject(hdc, fBtn);
    SetTextColor(hdc, C_WHITE);
    RECT gnR = { lx + 16, y + 12, iconX - 8, y + 30 };
    DrawText(hdc, "Counter-Strike 2", -1, &gnR, DT_LEFT | DT_SINGLELINE);

    // Expiry
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, C_TEXT2);
    RECT exR = { lx + 16, y + 32, iconX - 8, y + 48 };
    DrawText(hdc, "Expires in 16 days", -1, &exR, DT_LEFT | DT_SINGLELINE);

    y += cardH + 14;

    // ── Status info card ──
    RRect(hdc, lx, y, rw, 50, 8, C_CARD, C_CARD_BR);
    SelectObject(hdc, fBody);
    SetTextColor(hdc, C_TEXT2);
    RECT stlR = { lx + 16, y, lx + 120, y + 50 };
    DrawText(hdc, "Status:", -1, &stlR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SetTextColor(hdc, C_GREEN);
    RECT strR = { lx + 16, y, lx + rw - 16, y + 50 };
    DrawText(hdc, "UNDETECTED", -1, &strR, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    y += 50 + 14;

    // ── User info card ──
    RRect(hdc, lx, y, rw, 50, 8, C_CARD, C_CARD_BR);
    SetTextColor(hdc, C_TEXT2);
    RECT ulR = { lx + 16, y, lx + 120, y + 50 };
    DrawText(hdc, "Bruger:", -1, &ulR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SetTextColor(hdc, C_WHITE);
    RECT urR = { lx + 16, y, lx + rw - 16, y + 50 };
    DrawText(hdc, g_user.c_str(), -1, &urR, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    y += 50 + 20;

    // Click instruction
    SelectObject(hdc, fSmall);
    SetTextColor(hdc, C_TEXT3);
    RECT crR = { lx, y, lx + rw, y + 16 };
    DrawText(hdc, "Klik p\xE5 CS2-kortet for at starte", -1, &crR,
             DT_CENTER | DT_SINGLELINE);
    y += 24;

    // Status
    if (!g_status.empty()) {
        SelectObject(hdc, fSmall);
        SetTextColor(hdc, g_statusClr);
        RECT stR = { lx, y, lx + rw, y + 40 };
        DrawText(hdc, g_status.c_str(), -1, &stR, DT_CENTER | DT_WORDBREAK);
    }
}

// ============================================================================
//  Auth
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
    SetStat("Konto oprettet! HWID l\xE5st.", C_GREEN);
    return true;
}

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
//  Process
// ============================================================================
DWORD FindProc(const char* n) {
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (s == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32 pe = { sizeof(pe) };
    DWORD pid = 0;
    if (Process32First(s, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, n) == 0) { pid = pe.th32ProcessID; break; }
        } while (Process32Next(s, &pe));
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
//  Launch
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

    SetStat("Venter p\xE5 Steam...", C_TEXT2);
    Sleep(8000);

    SetStat("Starter CS2...", C_TEXT2);
    ShellExecuteA(NULL, "open", "steam://rungameid/730", NULL, NULL, SW_SHOWNORMAL);

    SetStat("Venter p\xE5 CS2...", C_TEXT2);
    DWORD cs2 = 0;
    for (int i = 0; i < 120 && !cs2; i++) {
        cs2 = FindProc("cs2.exe");
        if (!cs2) {
            char m[64]; sprintf_s(m, "Venter p\xE5 CS2... %d sek", i + 1);
            SetStat(m, C_TEXT2); Sleep(1000);
        }
    }
    if (!cs2) { SetStat("Fejl: CS2 ikke fundet!", C_RED); return false; }

    DbgLog("CS2 fundet! PID=%d", (int)cs2);
    g_cs2Pid = cs2;  // Store for overlay thread
    SetStat("CS2 fundet! Forbereder...", C_TEXT2);
    Sleep(5000);

    SetStat("Injicerer overlay...", C_TEXT2);

    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, cs2);
    DbgLog("OpenProcess(cs2 PID=%d) => %p (LastError=%d)", (int)cs2, (void*)h, (int)GetLastError());
    if (!h) { SetStat("Fejl: K\xF8r som administrator!", C_RED); return false; }
    CloseHandle(h);

    DbgLog("Checking for CS2 window before overlay thread...");
    HWND testHwnd = FindWindowA(nullptr, "Counter-Strike 2");
    DbgLog("Pre-check FindWindowA('Counter-Strike 2') => %p", (void*)testHwnd);
    if (testHwnd) {
        RECT wr; GetWindowRect(testHwnd, &wr);
        DbgLog("  CS2 window rect=(%d,%d,%d,%d)", wr.left, wr.top, wr.right, wr.bottom);
    }

    DbgLog("Creating overlay thread...");
    HANDLE hThread = CreateThread(NULL, 0, OverlayThreadProc, (LPVOID)g_hInst, 0, NULL);
    if (!hThread) {
        DbgLog("ERROR: CreateThread failed! LastError=%d", (int)GetLastError());
        SetStat("Fejl: Kunne ikke starte overlay!", C_RED);
        return false;
    }
    DbgLog("Overlay thread created, polling for ready...");
    CloseHandle(hThread);

    // Poll for up to 15 seconds (the overlay may need time to find CS2 window)
    bool overlayOk = false;
    for (int i = 0; i < 30; i++) {
        Sleep(500);
        if (g_overlayRunning) { overlayOk = true; break; }
    }

    DbgLog("After wait: g_overlayRunning=%d g_pOverlayWnd=%p", (int)g_overlayRunning, (void*)g_pOverlayWnd);
    if (!overlayOk) {
        DbgLog("ERROR: Overlay did not start successfully!");
        SetStat("Fejl: Overlay kunne ikke starte! Se ac_debug.log", C_RED);
        return false;
    }

    DbgLog("=== Overlay ready! ===");
    SetStat("Overlay aktiv! Tryk INSERT for menu.", C_GREEN);
    MessageBox(hwnd,
        "Overlay er aktivt!\n\nMenuen vises automatisk.\nTryk INSERT for at \xE5" "bne/lukke.\n\nDebug log: ac_debug.log",
        "AC Changer", MB_OK | MB_ICONINFORMATION);
    return true;
}
