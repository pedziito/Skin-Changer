/*
 * AC Loader — Professional DX11 Launcher
 * Built on ACE Engine v2.0 — zero ImGui dependency.
 * Login/Signup → Subscription → DLL Injection.
 * Borderless rounded window with Neverlose-inspired dark UI.
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <TlHelp32.h>
#endif

#include "../engine/ace_engine_v2.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>

#ifdef _WIN32

using namespace ace;

// ============================================================================
// GLOBALS
// ============================================================================
static DX11Backend g_backend;
static FontAtlas   g_fontAtlas;
static InputSystem g_input;
static ThemeEngine g_theme;

static u32  g_font     = 0;   // 14pt regular
static u32  g_fontBold = 0;   // 22pt for logo/headings

static HWND g_hwnd    = nullptr;
static bool g_running = true;
static int  g_width   = 480;
static int  g_height  = 580;
static constexpr int CORNER_RADIUS = 14;

// ============================================================================
// ANIMATION HELPER — smooth lerp map
// ============================================================================
static std::unordered_map<u32, f32> g_anims;
static f32 g_dt   = 0.016f;
static f32 g_time = 0.0f;

static f32 Anim(u32 id, f32 target, f32 speed = 10.0f) {
    auto& v = g_anims[id];
    v += (target - v) * (std::min)(1.0f, speed * g_dt);
    if (std::abs(v - target) < 0.002f) v = target;
    return v;
}

// ============================================================================
// HIT TEST HELPER
// ============================================================================
static bool HitTest(f32 x, f32 y, f32 w, f32 h) {
    Vec2 m = g_input.MousePos();
    return m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h;
}

// ============================================================================
// COLOR HELPERS
// ============================================================================
static Color Fade(Color c, f32 alpha) {
    return Color{c.r, c.g, c.b, u8(alpha * 255.0f)};
}
static Color Lerp(Color a, Color b, f32 t) {
    return Color{
        u8(a.r + (b.r - a.r) * t),
        u8(a.g + (b.g - a.g) * t),
        u8(a.b + (b.b - a.b) * t),
        u8(a.a + (b.a - a.a) * t)};
}

// ============================================================================
// THEME COLORS
// ============================================================================
namespace C {
    constexpr Color WindowBg      {12,  12,  16,  255};
    constexpr Color SidebarBg     {15,  15,  20,  255};
    constexpr Color CardBg        {18,  20,  28,  255};
    constexpr Color CardHover     {22,  24,  35,  255};
    constexpr Color CardSelected  {25,  28,  42,  255};
    constexpr Color InputBg       {18,  18,  26,  255};
    constexpr Color AccentBlue    {59,  130, 246, 255};
    constexpr Color AccentPurple  {139, 92,  246, 255};
    constexpr Color AccentGreen   {74,  222, 128, 255};
    constexpr Color AccentRed     {248, 113, 113, 255};
    constexpr Color AccentYellow  {250, 204, 21,  255};
    constexpr Color TextPrimary   {230, 230, 240, 255};
    constexpr Color TextSecondary {140, 140, 160, 255};
    constexpr Color TextDim       {80,  80,  100, 255};
    constexpr Color Border        {40,  40,  55,  255};
    constexpr Color BorderHover   {60,  60,  80,  255};
}

// ============================================================================
// APP STATES
// ============================================================================
enum AppScreen { SCREEN_LOGIN, SCREEN_SIGNUP, SCREEN_MAIN };
static AppScreen g_screen = SCREEN_LOGIN;

// ============================================================================
// AUTH STATE
// ============================================================================
static char g_loginUser[64]   = "";
static char g_loginPass[64]   = "";
static char g_signupUser[64]  = "";
static char g_signupPass[64]  = "";
static char g_signupPass2[64] = "";
static std::string g_authError;
static f32 g_authErrorTimer = 0;
static std::string g_loggedUser;
static f32 g_fadeAnim = 0.0f;
static u32 g_focusedItem = 0;

// ============================================================================
// SUBSCRIPTION DATA
// ============================================================================
struct SubscriptionEntry {
    std::string name;
    std::string expiryText;
    Color       accentColor;
    bool        active;
};
static std::vector<SubscriptionEntry> g_subscriptions;
static int g_selectedSub = -1;

// Injection state
static bool g_injecting = false;
static bool g_injected  = false;
static std::string g_statusMsg;
static f32 g_statusTimer = 0.0f;

// ============================================================================
// LICENSE FILE SYSTEM
// ============================================================================
static std::string GetLicensePath() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string dir = exePath;
    size_t pos = dir.find_last_of('\\');
    if (pos != std::string::npos) dir = dir.substr(0, pos + 1);
    return dir + "licenses.dat";
}

static std::string HashPassword(const std::string& pass) {
    uint32_t h = 0x811c9dc5;
    for (char c : pass) { h ^= (uint8_t)c; h *= 0x01000193; }
    char buf[16];
    snprintf(buf, sizeof(buf), "%08X", h);
    return buf;
}

static std::string GenerateLicenseKey() {
    char key[48];
    snprintf(key, sizeof(key), "AC-%04X-%04X-%04X-%04X",
             rand() & 0xFFFF, rand() & 0xFFFF,
             rand() & 0xFFFF, rand() & 0xFFFF);
    return key;
}

struct UserRecord {
    std::string username;
    std::string passHash;
    std::string licenseKey;
    time_t      created;
    time_t      expiry;
    bool        active;
};
static std::vector<UserRecord> g_users;

static void LoadUsers() {
    g_users.clear();
    std::ifstream f(GetLicensePath());
    if (!f.is_open()) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        UserRecord u;
        ss >> u.username >> u.passHash >> u.licenseKey >> u.created >> u.expiry >> u.active;
        if (!u.username.empty()) g_users.push_back(u);
    }
}

static void SaveUsers() {
    std::ofstream f(GetLicensePath());
    for (auto& u : g_users) {
        f << u.username << " " << u.passHash << " " << u.licenseKey
          << " " << u.created << " " << u.expiry << " " << u.active << "\n";
    }
}

static bool DoSignup(const char* user, const char* pass) {
    if (strlen(user) < 3) { g_authError = "Username must be 3+ characters"; g_authErrorTimer = 3; return false; }
    if (strlen(pass) < 4) { g_authError = "Password must be 4+ characters"; g_authErrorTimer = 3; return false; }
    for (auto& u : g_users) {
        if (u.username == user) { g_authError = "Username already exists"; g_authErrorTimer = 3; return false; }
    }
    UserRecord u;
    u.username = user;
    u.passHash = HashPassword(pass);
    u.licenseKey = GenerateLicenseKey();
    u.created = time(nullptr);
    u.expiry = u.created + 30 * 86400;
    u.active = true;
    g_users.push_back(u);
    SaveUsers();
    return true;
}

static bool DoLogin(const char* user, const char* pass) {
    std::string h = HashPassword(pass);
    for (auto& u : g_users) {
        if (u.username == user && u.passHash == h) {
            if (!u.active) { g_authError = "Account deactivated"; g_authErrorTimer = 3; return false; }
            if (time(nullptr) > u.expiry) { g_authError = "Subscription expired"; g_authErrorTimer = 3; return false; }
            g_loggedUser = user;
            g_subscriptions.clear();
            int daysLeft = (int)((u.expiry - time(nullptr)) / 86400);
            char expBuf[64];
            snprintf(expBuf, sizeof(expBuf), "Expires in %d days", daysLeft > 0 ? daysLeft : 0);
            g_subscriptions.push_back({
                "Counter-Strike 2", expBuf,
                Color{220, 160, 40, 255}, true
            });
            return true;
        }
    }
    g_authError = "Invalid username or password";
    g_authErrorTimer = 3;
    return false;
}

// ============================================================================
// INJECTION LOGIC
// ============================================================================
static DWORD FindProcess(const char* processName) {
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

static bool InjectDLL(DWORD processId, const char* dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) return false;
    size_t pathLen = strlen(dllPath) + 1;
    LPVOID remoteMem = VirtualAllocEx(hProcess, nullptr, pathLen,
                                       MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) { CloseHandle(hProcess); return false; }
    if (!WriteProcessMemory(hProcess, remoteMem, dllPath, pathLen, nullptr)) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess); return false;
    }
    LPVOID loadLib = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!loadLib) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess); return false;
    }
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                         (LPTHREAD_START_ROUTINE)loadLib,
                                         remoteMem, 0, nullptr);
    if (!hThread) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess); return false;
    }
    WaitForSingleObject(hThread, 10000);
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return exitCode != 0;
}

static void DoInject() {
    g_injecting = true;
    g_statusMsg = "Looking for CS2...";
    DWORD pid = FindProcess("cs2.exe");
    if (!pid) {
        g_statusMsg = "CS2 not running. Start CS2 first!";
        g_statusTimer = 4.0f; g_injecting = false; return;
    }
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string dllPath = exePath;
    size_t slash = dllPath.find_last_of('\\');
    if (slash != std::string::npos) dllPath = dllPath.substr(0, slash + 1);
    dllPath += "skin_changer.dll";
    if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        g_statusMsg = "DLL not found!";
        g_statusTimer = 4.0f; g_injecting = false; return;
    }
    if (InjectDLL(pid, dllPath.c_str())) {
        g_statusMsg = "Injected successfully!";
        g_injected = true;
    } else {
        g_statusMsg = "Injection failed. Run as Admin.";
    }
    g_statusTimer = 4.0f;
    g_injecting = false;
}

// ============================================================================
// WNDPROC — forward events to ACE InputSystem
// ============================================================================
static bool g_dragging = false;
static POINT g_dragStart = {};

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        g_running = false;
        return 0;

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            g_width  = LOWORD(lParam);
            g_height = HIWORD(lParam);
            if (g_width > 0 && g_height > 0)
                g_backend.Resize((u32)g_width, (u32)g_height);
            HRGN rgn = CreateRoundRectRgn(0, 0, g_width + 1, g_height + 1,
                                           CORNER_RADIUS * 2, CORNER_RADIUS * 2);
            SetWindowRgn(hWnd, rgn, TRUE);
        }
        return 0;

    case WM_MOUSEMOVE: {
        f32 x = (f32)(short)LOWORD(lParam);
        f32 y = (f32)(short)HIWORD(lParam);
        g_input.OnMouseMove(x, y);
        if (g_dragging) {
            POINT pt;
            GetCursorPos(&pt);
            SetWindowPos(hWnd, nullptr, pt.x - g_dragStart.x, pt.y - g_dragStart.y,
                         0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        g_input.OnMouseDown(MouseButton::Left);
        POINT pt = { (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam) };
        if (pt.y < 44 && pt.x < g_width - 80) {
            g_dragging = true;
            g_dragStart = pt;
            SetCapture(hWnd);
        }
        return 0;
    }
    case WM_LBUTTONUP:
        g_input.OnMouseUp(MouseButton::Left);
        if (g_dragging) { g_dragging = false; ReleaseCapture(); }
        return 0;
    case WM_RBUTTONDOWN: g_input.OnMouseDown(MouseButton::Right); return 0;
    case WM_RBUTTONUP:   g_input.OnMouseUp(MouseButton::Right);   return 0;
    case WM_MBUTTONDOWN: g_input.OnMouseDown(MouseButton::Middle); return 0;
    case WM_MBUTTONUP:   g_input.OnMouseUp(MouseButton::Middle);   return 0;

    case WM_MOUSEWHEEL: {
        f32 delta = (f32)GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f;
        g_input.OnMouseScroll(delta);
        return 0;
    }

    case WM_KEYDOWN: g_input.OnKeyDown((Key)(int)wParam); return 0;
    case WM_KEYUP:   g_input.OnKeyUp((Key)(int)wParam);   return 0;
    case WM_CHAR:
        if (wParam >= 32 && wParam < 127)
            g_input.OnTextInput((char)wParam);
        return 0;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

// ============================================================================
// DRAW HELPERS
// ============================================================================
static void DrawRoundedBorder(DrawList& dl, Rect r, Color bg, Color border,
                               f32 radius, f32 thickness = 1.0f) {
    // Border via two overlapping filled rects
    dl.AddFilledRoundRect(
        Rect{r.x - thickness, r.y - thickness, r.w + thickness * 2, r.h + thickness * 2},
        border, radius + thickness, 12);
    dl.AddFilledRoundRect(r, bg, radius, 12);
}

static void DrawACLogo(DrawList& dl, f32 cx, f32 cy) {
    // Multi-layer blue glow behind "AC" text
    Vec2 ts = g_fontAtlas.MeasureText(g_fontBold, "AC");

    // Glow layers
    for (int layer = 5; layer >= 1; layer--) {
        f32 spread = (f32)layer * 1.8f;
        u8 alpha = (u8)(30.0f * (1.0f - (f32)layer / 6.0f));
        Color glowCol{59, 130, 246, alpha};
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;
                g_fontAtlas.RenderText(dl, g_fontBold,
                    Vec2{cx - ts.x * 0.5f + dx * spread, cy - ts.y * 0.5f + dy * spread},
                    "AC", glowCol);
            }
        }
    }

    // Tight blue shadow
    g_fontAtlas.RenderText(dl, g_fontBold, Vec2{cx - ts.x*0.5f, cy - ts.y*0.5f + 1},
                           "AC", Color{59, 130, 246, 120});
    g_fontAtlas.RenderText(dl, g_fontBold, Vec2{cx - ts.x*0.5f + 1, cy - ts.y*0.5f},
                           "AC", Color{59, 130, 246, 80});

    // White text on top
    g_fontAtlas.RenderText(dl, g_fontBold, Vec2{cx - ts.x * 0.5f, cy - ts.y * 0.5f},
                           "AC", Color{255, 255, 255, 255});
}

static void DrawText(DrawList& dl, f32 x, f32 y, Color c, const char* text, u32 fontId = 0) {
    g_fontAtlas.RenderText(dl, fontId ? fontId : g_font, Vec2{x, y}, text, c);
}

static Vec2 TextSize(const char* text, u32 fontId = 0) {
    return g_fontAtlas.MeasureText(fontId ? fontId : g_font, text);
}

static void DrawTextCentered(DrawList& dl, Rect area, Color c, const char* text, u32 fontId = 0) {
    Vec2 ts = TextSize(text, fontId);
    f32 x = area.x + (area.w - ts.x) * 0.5f;
    f32 y = area.y + (area.h - ts.y) * 0.5f;
    DrawText(dl, x, y, c, text, fontId);
}

// ============================================================================
// BUTTON WIDGET
// ============================================================================
static bool RenderButton(DrawList& dl, const char* label, Rect r,
                          Color baseColor, bool enabled = true) {
    u32 id = Hash(label);
    bool hovered = enabled && HitTest(r.x, r.y, r.w, r.h);
    bool clicked = hovered && g_input.IsMousePressed(MouseButton::Left);
    f32 anim = Anim(id, hovered ? 1.0f : 0.0f);

    // Outer glow on hover
    if (anim > 0.01f) {
        for (int i = 3; i > 0; i--) {
            Color gc = Fade(baseColor, 0.05f * anim * (1.0f - (f32)i / 4.0f));
            dl.AddFilledRoundRect(
                Rect{r.x - i, r.y - i, r.w + i * 2, r.h + i * 2}, gc, 10.0f + i, 12);
        }
    }

    u8 alpha = enabled ? (u8)(200 + 55 * anim) : 100;
    Color bgCol{baseColor.r, baseColor.g, baseColor.b, alpha};
    dl.AddFilledRoundRect(r, bgCol, 8.0f, 12);

    // Subtle top highlight
    if (anim > 0.01f) {
        dl.AddGradientRect(
            Rect{r.x + 1, r.y + 1, r.w - 2, r.h * 0.4f},
            Fade(Color{255,255,255,255}, 0.05f * anim),
            Fade(Color{255,255,255,255}, 0.05f * anim),
            Fade(Color{255,255,255,255}, 0.0f),
            Fade(Color{255,255,255,255}, 0.0f));
    }

    Color textCol = enabled ? C::TextPrimary : Color{180, 180, 180, 180};
    DrawTextCentered(dl, r, textCol, label);

    return clicked && enabled;
}

// ============================================================================
// INPUT FIELD WIDGET
// ============================================================================
static void RenderInputField(DrawList& dl, const char* label, char* buf, int bufSize,
                              f32 x, f32 y, f32 w, bool isPassword = false) {
    f32 labelH = 16.0f;
    f32 fieldH = 36.0f;

    // Label
    DrawText(dl, x, y, C::TextSecondary, label);

    // Field
    f32 fy = y + labelH + 4;
    u32 fieldId = Hash(label);
    bool focused = g_focusedItem == fieldId;
    bool hovered = HitTest(x, fy, w, fieldH);

    Color borderCol = focused ? C::AccentBlue :
                      hovered ? C::BorderHover : C::Border;

    // Border + background
    DrawRoundedBorder(dl, Rect{x, fy, w, fieldH}, C::InputBg, borderCol, 8.0f);

    // Focus glow
    if (focused) {
        for (int i = 2; i > 0; i--) {
            Color gc = Fade(C::AccentBlue, 0.08f * (3 - i));
            dl.AddFilledRoundRect(
                Rect{x - i - 1, fy - i - 1, w + (i + 1) * 2, fieldH + (i + 1) * 2},
                gc, 9.0f + i, 12);
            dl.AddFilledRoundRect(Rect{x - 1, fy - 1, w + 2, fieldH + 2},
                                   C::InputBg, 8.0f, 12);
        }
        // Redraw main after glow
        DrawRoundedBorder(dl, Rect{x, fy, w, fieldH}, C::InputBg, C::AccentBlue, 8.0f);
    }

    // Click to focus
    if (hovered && g_input.IsMousePressed(MouseButton::Left))
        g_focusedItem = fieldId;

    // Handle text input
    if (focused) {
        int len = (int)strlen(buf);
        std::string_view input = g_input.TextInput();
        for (char c : input) {
            if (c >= 32 && c < 127 && len < bufSize - 1) {
                buf[len++] = c;
                buf[len] = '\0';
            }
        }
        if (g_input.IsKeyPressed(Key::Backspace) && len > 0) {
            buf[len - 1] = '\0';
        }
    }

    // Display text
    f32 lineH = 14.0f; // approximate line height
    const FontInstance* fi = g_fontAtlas.GetFont(g_font);
    if (fi) lineH = fi->lineHeight;

    int slen = (int)strlen(buf);
    if (slen > 0) {
        std::string displayText = isPassword ? std::string(slen, '*') : std::string(buf);
        DrawText(dl, x + 12, fy + (fieldH - lineH) * 0.5f, C::TextPrimary, displayText.c_str());

        // Cursor blink
        if (focused) {
            f32 blink = fmodf(g_time * 2.0f, 2.0f);
            if (blink < 1.0f) {
                Vec2 tw = TextSize(displayText.c_str());
                dl.AddFilledRect(Rect{x + 12 + tw.x + 1, fy + 8, 1.5f, fieldH - 16}, C::TextPrimary);
            }
        }
    } else {
        // Placeholder
        std::string ph = std::string("Enter ") + label;
        std::transform(ph.begin(), ph.end(), ph.begin(), ::tolower);
        DrawText(dl, x + 12, fy + (fieldH - lineH) * 0.5f, C::TextDim, ph.c_str());

        if (focused) {
            f32 blink = fmodf(g_time * 2.0f, 2.0f);
            if (blink < 1.0f) {
                dl.AddFilledRect(Rect{x + 12, fy + 8, 1.5f, fieldH - 16}, C::TextPrimary);
            }
        }
    }
}

// ============================================================================
// WINDOW CONTROLS (minimize + close)
// ============================================================================
static void RenderWindowControls(DrawList& dl, f32 W) {
    f32 btnSize = 18.0f;
    f32 btnY = 14.0f;
    f32 minX = W - 60;
    f32 clsX = W - 32;

    // Minimize
    u32 minId = Hash("__wnd_min");
    bool minHov = HitTest(minX, btnY, btnSize, btnSize);
    f32 minA = Anim(minId, minHov ? 1.0f : 0.0f);
    if (minA > 0.01f) {
        f32 cr = btnSize * 0.5f + 2;
        dl.AddFilledRoundRect(
            Rect{minX + btnSize*0.5f - cr, btnY + btnSize*0.5f - cr, cr*2, cr*2},
            Fade(Color{255,255,255,255}, 0.05f * minA), cr, 16);
    }
    dl.AddLine(Vec2{minX + 3, btnY + btnSize * 0.5f},
               Vec2{minX + btnSize - 3, btnY + btnSize * 0.5f},
               Color{140, 140, 160, (u8)(180 + 75 * minA)}, 1.5f);
    if (minHov && g_input.IsMousePressed(MouseButton::Left))
        ShowWindow(g_hwnd, SW_MINIMIZE);

    // Close
    u32 clsId = Hash("__wnd_cls");
    bool clsHov = HitTest(clsX, btnY, btnSize, btnSize);
    f32 clsA = Anim(clsId, clsHov ? 1.0f : 0.0f);
    if (clsA > 0.01f) {
        f32 cr = btnSize * 0.5f + 2;
        dl.AddFilledRoundRect(
            Rect{clsX + btnSize*0.5f - cr, btnY + btnSize*0.5f - cr, cr*2, cr*2},
            Fade(C::AccentRed, 0.12f * clsA), cr, 16);
    }
    f32 ccx = clsX + btnSize * 0.5f, ccy = btnY + btnSize * 0.5f;
    f32 cs = 4.0f;
    Color xCol{140, 140, 160, (u8)(180 + 75 * clsA)};
    dl.AddLine(Vec2{ccx - cs, ccy - cs}, Vec2{ccx + cs, ccy + cs}, xCol, 1.5f);
    dl.AddLine(Vec2{ccx + cs, ccy - cs}, Vec2{ccx - cs, ccy + cs}, xCol, 1.5f);
    if (clsHov && g_input.IsMousePressed(MouseButton::Left))
        g_running = false;
}

// ============================================================================
// LOGIN SCREEN
// ============================================================================
static void RenderLoginScreen(DrawList& dl, f32 W, f32 H) {
    // Background
    dl.AddFilledRoundRect(Rect{0, 0, W, H}, C::WindowBg, (f32)CORNER_RADIUS, 12);

    // Top gradient
    dl.AddGradientRect(Rect{0, 0, W, 120},
        Color{20, 25, 50, 80}, Color{20, 25, 50, 80},
        Color{12, 12, 16, 0},  Color{12, 12, 16, 0});

    // Window border
    DrawRoundedBorder(dl, Rect{0, 0, W, H}, C::WindowBg, Color{40, 40, 60, 150},
                       (f32)CORNER_RADIUS);

    RenderWindowControls(dl, W);

    // AC Logo
    DrawACLogo(dl, W * 0.5f, 65.0f);

    // Tagline
    Vec2 tts = TextSize("Skin Changer");
    DrawText(dl, W * 0.5f - tts.x * 0.5f, 92, Color{100, 100, 130, 255}, "Skin Changer");

    // Gradient separator
    f32 sepY = 118.0f;
    dl.AddGradientRect(Rect{W * 0.15f, sepY, W * 0.7f, 1},
        Color{40, 40, 60, 0},   Color{59, 130, 246, 80},
        Color{59, 130, 246, 80}, Color{40, 40, 60, 0});

    // Title
    Vec2 titleTs = TextSize("Sign in to your account");
    DrawText(dl, W * 0.5f - titleTs.x * 0.5f, 135, C::TextPrimary, "Sign in to your account");

    // Form
    f32 formX = 40, formW = W - 80, formY = 168;

    RenderInputField(dl, "Username", g_loginUser, sizeof(g_loginUser),
                     formX, formY, formW, false);
    RenderInputField(dl, "Password", g_loginPass, sizeof(g_loginPass),
                     formX, formY + 66, formW, true);

    // Error message
    if (g_authErrorTimer > 0) {
        g_authErrorTimer -= g_dt;
        f32 ea = g_authErrorTimer > 0.5f ? 1.0f : g_authErrorTimer / 0.5f;
        Vec2 ets = TextSize(g_authError.c_str());
        DrawText(dl, W * 0.5f - ets.x * 0.5f, formY + 140,
                 Fade(C::AccentRed, ea), g_authError.c_str());
    }

    // Login button
    f32 btnY = formY + 164;
    if (RenderButton(dl, "SIGN IN", Rect{formX, btnY, formW, 42}, C::AccentBlue)) {
        if (DoLogin(g_loginUser, g_loginPass)) {
            g_screen = SCREEN_MAIN;
            g_fadeAnim = 0;
        }
    }

    // Enter key shortcut
    if (g_input.IsKeyPressed(Key::Enter) && g_screen == SCREEN_LOGIN) {
        if (strlen(g_loginUser) > 0 && strlen(g_loginPass) > 0) {
            if (DoLogin(g_loginUser, g_loginPass)) {
                g_screen = SCREEN_MAIN;
                g_fadeAnim = 0;
            }
        }
    }

    // Sign up link
    Vec2 sts = TextSize("Don't have an account?");
    DrawText(dl, W * 0.5f - sts.x * 0.5f, btnY + 56, C::TextDim, "Don't have an account?");

    const char* signupLink = "Create Account";
    Vec2 slts = TextSize(signupLink);
    f32 slX = W * 0.5f - slts.x * 0.5f;
    f32 slY = btnY + 74;
    u32 slId = Hash("__signup_link");
    bool slHov = HitTest(slX, slY, slts.x, slts.y + 4);
    f32 slA = Anim(slId, slHov ? 1.0f : 0.0f);
    DrawText(dl, slX, slY, Color{59, 130, 246, (u8)(180 + 75 * slA)}, signupLink);
    if (slHov) {
        dl.AddFilledRect(Rect{slX, slY + slts.y + 1, slts.x, 1},
                          Fade(C::AccentBlue, 0.5f * slA));
    }
    if (slHov && g_input.IsMousePressed(MouseButton::Left)) {
        g_screen = SCREEN_SIGNUP;
        g_authError.clear();
        g_authErrorTimer = 0;
        memset(g_signupUser, 0, sizeof(g_signupUser));
        memset(g_signupPass, 0, sizeof(g_signupPass));
        memset(g_signupPass2, 0, sizeof(g_signupPass2));
    }

    // Footer
    Vec2 fts = TextSize("AC v3.0");
    DrawText(dl, W * 0.5f - fts.x * 0.5f, H - 28, Color{50, 50, 65, 255}, "AC v3.0");
}

// ============================================================================
// SIGNUP SCREEN
// ============================================================================
static void RenderSignupScreen(DrawList& dl, f32 W, f32 H) {
    dl.AddFilledRoundRect(Rect{0, 0, W, H}, C::WindowBg, (f32)CORNER_RADIUS, 12);
    dl.AddGradientRect(Rect{0, 0, W, 120},
        Color{20, 25, 50, 80}, Color{20, 25, 50, 80},
        Color{12, 12, 16, 0},  Color{12, 12, 16, 0});
    DrawRoundedBorder(dl, Rect{0, 0, W, H}, C::WindowBg, Color{40, 40, 60, 150},
                       (f32)CORNER_RADIUS);

    RenderWindowControls(dl, W);
    DrawACLogo(dl, W * 0.5f, 65.0f);

    Vec2 tts = TextSize("Skin Changer");
    DrawText(dl, W * 0.5f - tts.x * 0.5f, 92, Color{100, 100, 130, 255}, "Skin Changer");

    f32 sepY = 118.0f;
    dl.AddGradientRect(Rect{W * 0.15f, sepY, W * 0.7f, 1},
        Color{40, 40, 60, 0},   Color{59, 130, 246, 80},
        Color{59, 130, 246, 80}, Color{40, 40, 60, 0});

    Vec2 titleTs = TextSize("Create your account");
    DrawText(dl, W * 0.5f - titleTs.x * 0.5f, 135, C::TextPrimary, "Create your account");

    f32 formX = 40, formW = W - 80, formY = 168;

    RenderInputField(dl, "Username", g_signupUser, sizeof(g_signupUser),
                     formX, formY, formW, false);
    RenderInputField(dl, "Password", g_signupPass, sizeof(g_signupPass),
                     formX, formY + 66, formW, true);
    RenderInputField(dl, "Confirm Password", g_signupPass2, sizeof(g_signupPass2),
                     formX, formY + 132, formW, true);

    if (g_authErrorTimer > 0) {
        g_authErrorTimer -= g_dt;
        f32 ea = g_authErrorTimer > 0.5f ? 1.0f : g_authErrorTimer / 0.5f;
        Vec2 ets = TextSize(g_authError.c_str());
        DrawText(dl, W * 0.5f - ets.x * 0.5f, formY + 204,
                 Fade(C::AccentRed, ea), g_authError.c_str());
    }

    f32 btnY = formY + 224;
    if (RenderButton(dl, "CREATE ACCOUNT", Rect{formX, btnY, formW, 42}, C::AccentBlue)) {
        if (strcmp(g_signupPass, g_signupPass2) != 0) {
            g_authError = "Passwords do not match";
            g_authErrorTimer = 3;
        } else if (DoSignup(g_signupUser, g_signupPass)) {
            if (DoLogin(g_signupUser, g_signupPass)) {
                g_screen = SCREEN_MAIN;
                g_fadeAnim = 0;
            }
        }
    }

    if (g_input.IsKeyPressed(Key::Enter) && g_screen == SCREEN_SIGNUP) {
        if (strlen(g_signupUser) > 0 && strlen(g_signupPass) > 0) {
            if (strcmp(g_signupPass, g_signupPass2) != 0) {
                g_authError = "Passwords do not match";
                g_authErrorTimer = 3;
            } else if (DoSignup(g_signupUser, g_signupPass)) {
                if (DoLogin(g_signupUser, g_signupPass)) {
                    g_screen = SCREEN_MAIN;
                    g_fadeAnim = 0;
                }
            }
        }
    }

    // Back to login
    Vec2 bts = TextSize("Already have an account?");
    DrawText(dl, W * 0.5f - bts.x * 0.5f, btnY + 56, C::TextDim, "Already have an account?");

    const char* backLink = "Sign In";
    Vec2 blts = TextSize(backLink);
    f32 blX = W * 0.5f - blts.x * 0.5f;
    f32 blY = btnY + 74;
    u32 blId = Hash("__login_link");
    bool blHov = HitTest(blX, blY, blts.x, blts.y + 4);
    f32 blA = Anim(blId, blHov ? 1.0f : 0.0f);
    DrawText(dl, blX, blY, Color{59, 130, 246, (u8)(180 + 75 * blA)}, backLink);
    if (blHov && g_input.IsMousePressed(MouseButton::Left)) {
        g_screen = SCREEN_LOGIN;
        g_authError.clear();
        g_authErrorTimer = 0;
    }

    Vec2 fts = TextSize("AC v3.0");
    DrawText(dl, W * 0.5f - fts.x * 0.5f, H - 28, Color{50, 50, 65, 255}, "AC v3.0");
}

// ============================================================================
// MAIN SCREEN — Sidebar + Subscriptions + Launch
// ============================================================================
static void RenderMainScreen(DrawList& dl, f32 W, f32 H) {
    g_fadeAnim += g_dt * 3.0f;
    if (g_fadeAnim > 1.0f) g_fadeAnim = 1.0f;

    // Background
    dl.AddFilledRoundRect(Rect{0, 0, W, H}, C::WindowBg, (f32)CORNER_RADIUS, 12);
    DrawRoundedBorder(dl, Rect{0, 0, W, H}, C::WindowBg, Color{40, 40, 60, 150},
                       (f32)CORNER_RADIUS);

    // ============================================================
    // SIDEBAR
    // ============================================================
    constexpr f32 SIDEBAR_W = 120.0f;

    dl.AddFilledRoundRect(Rect{0, 0, SIDEBAR_W, H}, C::SidebarBg, (f32)CORNER_RADIUS, 12);
    // Fill right edge (not rounded)
    dl.AddFilledRect(Rect{SIDEBAR_W - CORNER_RADIUS, 0, (f32)CORNER_RADIUS, H}, C::SidebarBg);

    // Right border line
    dl.AddLine(Vec2{SIDEBAR_W, (f32)CORNER_RADIUS}, Vec2{SIDEBAR_W, H - (f32)CORNER_RADIUS},
               Color{40, 40, 55, 80}, 1.0f);

    // AC Logo in sidebar
    DrawACLogo(dl, SIDEBAR_W * 0.5f, 36.0f);

    // Separator
    f32 sepY = 56;
    dl.AddGradientRect(Rect{12, sepY, SIDEBAR_W - 24, 1},
        Color{40, 40, 60, 0},   Color{59, 130, 246, 50},
        Color{59, 130, 246, 50}, Color{40, 40, 60, 0});

    // Navigation items
    const char* navItems[] = { "Website", "Support", "Market" };
    f32 navY = sepY + 14;

    for (int i = 0; i < 3; i++) {
        f32 btnX = 8, btnW = SIDEBAR_W - 16, btnH = 32.0f;
        u32 id = Hash(navItems[i]);
        bool hovered = HitTest(btnX, navY, btnW, btnH);
        f32 anim = Anim(id, hovered ? 1.0f : 0.0f);

        if (anim > 0.01f) {
            dl.AddFilledRoundRect(Rect{btnX, navY, btnW, btnH},
                                  Fade(C::AccentBlue, 0.06f * anim), 6.0f, 8);
        }

        Color tc{140, 140, 160, (u8)(180 + 75 * anim)};
        DrawText(dl, btnX + 14, navY + (btnH - 14.0f) * 0.5f, tc, navItems[i]);

        navY += btnH + 2;
    }

    // Bottom user area
    f32 userY = H - 56;
    dl.AddGradientRect(Rect{12, userY - 8, SIDEBAR_W - 24, 1},
        Color{40, 40, 55, 0}, Color{40, 40, 55, 80},
        Color{40, 40, 55, 80}, Color{40, 40, 55, 0});

    // Avatar (rounded rect as circle)
    f32 avCX = 26, avCY = userY + 18;
    f32 avR = 14.0f;
    dl.AddFilledRoundRect(Rect{avCX - avR, avCY - avR, avR*2, avR*2},
                          Color{30, 35, 55, 255}, avR, 16);
    // Avatar border
    dl.AddFilledRoundRect(Rect{avCX - avR - 1, avCY - avR - 1, avR*2 + 2, avR*2 + 2},
                          Color{50, 55, 75, 255}, avR + 1, 16);
    dl.AddFilledRoundRect(Rect{avCX - avR, avCY - avR, avR*2, avR*2},
                          Color{30, 35, 55, 255}, avR, 16);

    // User initial
    if (!g_loggedUser.empty()) {
        char initial[2] = { (char)toupper(g_loggedUser[0]), '\0' };
        Vec2 its = TextSize(initial);
        DrawText(dl, avCX - its.x * 0.5f, avCY - its.y * 0.5f, C::AccentBlue, initial);
    }

    // Username + status
    DrawText(dl, avCX + 20, avCY - 8, C::TextPrimary, g_loggedUser.c_str());
    DrawText(dl, avCX + 20, avCY + 4, Color{74, 222, 128, 180}, "Online");

    // ============================================================
    // MAIN CONTENT — right side
    // ============================================================
    RenderWindowControls(dl, W);

    f32 contentX = SIDEBAR_W + 20;
    f32 contentY = 18;
    f32 contentW = W - SIDEBAR_W - 40;

    // Section title
    DrawText(dl, contentX, contentY, Color{240, 240, 250, 255}, "Subscription", g_fontBold);

    // Subtitle
    DrawText(dl, contentX, contentY + 24, Color{100, 100, 130, 255}, "Available subscriptions");

    // Separator
    f32 cSepY = contentY + 44;
    dl.AddGradientRect(Rect{contentX, cSepY, contentW, 1},
        Color{59, 130, 246, 60},  Color{139, 92, 246, 60},
        Color{139, 92, 246, 0},   Color{59, 130, 246, 0});

    // ============================================================
    // SUBSCRIPTION CARDS
    // ============================================================
    f32 cardY = cSepY + 16;
    f32 cardW = contentW;
    f32 cardH = 68.0f;
    f32 cardGap = 10.0f;

    for (int i = 0; i < (int)g_subscriptions.size(); i++) {
        auto& sub = g_subscriptions[i];
        f32 cX = contentX;
        f32 cY = cardY + i * (cardH + cardGap);

        u32 cardId = Hash(sub.name.c_str());
        bool hovered = HitTest(cX, cY, cardW, cardH);
        bool clicked = hovered && g_input.IsMousePressed(MouseButton::Left);
        f32 anim = Anim(cardId, hovered ? 1.0f : 0.0f);

        // Card bg
        Color bg = g_selectedSub == i ? C::CardSelected :
                   hovered ? C::CardHover : C::CardBg;

        // Selected glow
        if (g_selectedSub == i) {
            for (int gl = 4; gl > 0; gl--) {
                Color gc = Fade(C::AccentBlue, 0.03f * (1.0f - (f32)gl / 5.0f));
                dl.AddFilledRoundRect(
                    Rect{cX - (f32)gl, cY - (f32)gl, cardW + gl * 2.0f, cardH + gl * 2.0f},
                    gc, 10.0f + gl, 12);
            }
        }

        // Card border
        Color border = g_selectedSub == i ? Fade(C::AccentBlue, 0.47f) :
                       hovered ? Fade(C::AccentBlue, 0.16f + 0.24f * anim) :
                                 Color{40, 40, 55, 120};
        DrawRoundedBorder(dl, Rect{cX, cY, cardW, cardH}, bg, border, 10.0f);

        // Left accent bar on selected
        if (g_selectedSub == i) {
            dl.AddFilledRoundRect(Rect{cX + 2, cY + 8, 3, cardH - 16},
                                  C::AccentBlue, 1.5f, 4);
        }

        // Game icon
        f32 iconSize = 40.0f;
        f32 iconX = cX + 14;
        f32 iconY = cY + (cardH - iconSize) * 0.5f;
        dl.AddFilledRoundRect(Rect{iconX, iconY, iconSize, iconSize},
                              sub.accentColor, 10.0f, 8);
        // CS text on icon
        DrawTextCentered(dl, Rect{iconX, iconY, iconSize, iconSize},
                         Color{255, 255, 255, 220}, "CS");

        // Game name
        DrawText(dl, cX + 14 + iconSize + 14, cY + 15, C::TextPrimary, sub.name.c_str());

        // Expiry text
        DrawText(dl, cX + 14 + iconSize + 14, cY + 36,
                 Color{100, 100, 130, 255}, sub.expiryText.c_str());

        // Active badge
        if (sub.active) {
            const char* badge = "ACTIVE";
            Vec2 badgeTs = TextSize(badge);
            f32 bx = cX + cardW - badgeTs.x - 20;
            f32 by = cY + (cardH - 22) * 0.5f;
            dl.AddFilledRoundRect(Rect{bx - 6, by - 2, badgeTs.x + 12, badgeTs.y + 4},
                                  Fade(C::AccentGreen, 0.12f), 4.0f, 8);
            DrawText(dl, bx, by, C::AccentGreen, badge);
        }

        if (clicked) g_selectedSub = i;
    }

    // ============================================================
    // LAUNCH BUTTON
    // ============================================================
    f32 launchY = cardY + (int)g_subscriptions.size() * (cardH + cardGap) + 16;

    if (g_selectedSub >= 0) {
        Color baseCol;
        const char* launchText;

        if (g_injected) {
            baseCol = C::AccentGreen;
            launchText = "LAUNCHED";
        } else if (g_injecting) {
            baseCol = C::AccentYellow;
            launchText = "INJECTING...";
        } else {
            baseCol = C::AccentBlue;
            launchText = "LAUNCH";
        }

        if (RenderButton(dl, launchText, Rect{contentX, launchY, cardW, 42},
                          baseCol, !g_injecting && !g_injected)) {
            DoInject();
        }
    } else {
        const char* hint = "Select a subscription to continue";
        Vec2 hts = TextSize(hint);
        DrawText(dl, contentX + (contentW - hts.x) * 0.5f, launchY + 10, C::TextDim, hint);
    }

    // ============================================================
    // STATUS TOAST
    // ============================================================
    if (g_statusTimer > 0) {
        g_statusTimer -= g_dt;
        f32 a = g_statusTimer > 0.5f ? 1.0f : g_statusTimer / 0.5f;
        if (a > 0.01f) {
            f32 nW = 300, nH = 38;
            f32 nX = contentX + (contentW - nW) * 0.5f;
            f32 nY = H - 50;

            Color bg = g_injected ? Fade(C::AccentGreen, 0.86f * a)
                                  : Fade(C::AccentRed, 0.86f * a);
            dl.AddFilledRoundRect(Rect{nX, nY, nW, nH}, bg, 10.0f, 12);

            Color tc = Fade(Color{255,255,255,255}, a);
            DrawTextCentered(dl, Rect{nX, nY, nW, nH}, tc, g_statusMsg.c_str());
        }
    }

    // Logout button
    const char* logoutText = "Logout";
    Vec2 lots = TextSize(logoutText);
    f32 loX = W - lots.x - 20;
    f32 loY = H - 30;
    u32 loId = Hash("__logout");
    bool loHov = HitTest(loX, loY, lots.x, lots.y + 4);
    f32 loA = Anim(loId, loHov ? 1.0f : 0.0f);
    DrawText(dl, loX, loY, Color{120, 120, 140, (u8)(150 + 100 * loA)}, logoutText);
    if (loHov && g_input.IsMousePressed(MouseButton::Left)) {
        g_screen = SCREEN_LOGIN;
        g_loggedUser.clear();
        g_subscriptions.clear();
        g_selectedSub = -1;
        g_injected = false;
        g_injecting = false;
        memset(g_loginPass, 0, sizeof(g_loginPass));
    }
}

// ============================================================================
// ENTRY POINT
// ============================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    srand((unsigned)time(nullptr));
    LoadUsers();

    // Create borderless window
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "ACLoaderV2";
    RegisterClassExA(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    g_hwnd = CreateWindowExA(
        0, wc.lpszClassName, "AC Loader",
        WS_POPUP | WS_VISIBLE,
        (screenW - g_width) / 2, (screenH - g_height) / 2,
        g_width, g_height,
        nullptr, nullptr, hInstance, nullptr);

    // Rounded corners
    HRGN rgn = CreateRoundRectRgn(0, 0, g_width + 1, g_height + 1,
                                   CORNER_RADIUS * 2, CORNER_RADIUS * 2);
    SetWindowRgn(g_hwnd, rgn, TRUE);

    // Initialize DX11 backend
    if (!g_backend.Initialize(g_hwnd, (u32)g_width, (u32)g_height)) {
        MessageBoxA(nullptr, "Failed to initialize DX11 backend", "Error", MB_OK);
        return 1;
    }

    // Load fonts
    const char* fontPaths[] = {
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\tahoma.ttf",
        "C:\\Windows\\Fonts\\verdana.ttf",
    };
    bool fontLoaded = false;
    for (auto* p : fontPaths) {
        g_font = g_fontAtlas.AddFont(p, 14.0f, &g_backend);
        if (g_font != 0) {
            g_fontBold = g_fontAtlas.AddFont(p, 22.0f, &g_backend);
            fontLoaded = true;
            break;
        }
    }
    if (!fontLoaded) {
        MessageBoxA(nullptr, "Failed to load system font", "Error", MB_OK);
        g_backend.Shutdown();
        return 1;
    }

    ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hwnd);

    // Timing
    LARGE_INTEGER freq, last;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&last);

    // Main loop
    MSG msg = {};
    while (g_running) {
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            if (msg.message == WM_QUIT) g_running = false;
        }
        if (!g_running) break;

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        g_dt = (f32)(now.QuadPart - last.QuadPart) / (f32)freq.QuadPart;
        if (g_dt > 0.1f) g_dt = 0.016f;
        g_time += g_dt;
        last = now;

        // Begin frame
        g_input.BeginFrame();
        g_backend.BeginFrame();
        g_backend.SetClearColor(C::WindowBg);

        // Build draw list
        DrawList dl;
        f32 W = (f32)g_width;
        f32 H = (f32)g_height;

        switch (g_screen) {
        case SCREEN_LOGIN:  RenderLoginScreen(dl, W, H);  break;
        case SCREEN_SIGNUP: RenderSignupScreen(dl, W, H); break;
        case SCREEN_MAIN:   RenderMainScreen(dl, W, H);   break;
        }

        // Render
        Vec2 vpSize = g_backend.GetViewportSize();
        g_backend.RenderDrawList(dl, (u32)vpSize.x, (u32)vpSize.y);
        g_backend.EndFrame();
        g_backend.Present();
    }

    g_backend.Shutdown();
    DestroyWindow(g_hwnd);
    UnregisterClassA(wc.lpszClassName, hInstance);
    return 0;
}

#else
int main() {
    printf("AC Loader requires Windows (DX11 backend).\n");
    return 0;
}
#endif
