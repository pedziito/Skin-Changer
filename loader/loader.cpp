/*
 * AC Loader — Premium DX11 Launcher v4.0
 * Built on ACE Engine v2.0 — zero ImGui dependency.
 * 
 * Design: Floating glass-card UI, gradient mesh background,
 *         blue-purple accent, vertical dashboard layout.
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <TlHelp32.h>
#include <Shlobj.h>
#endif

#include "../engine/ace_engine_v2.h"

#include "resource.h"

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
// DEBUG LOG — writes to ac_loader.log next to the exe
// ============================================================================
static FILE* g_logFile = nullptr;

static void LogInit() {
    char p[MAX_PATH]; GetModuleFileNameA(nullptr, p, MAX_PATH);
    std::string s = p;
    size_t i = s.find_last_of('\\');
    std::string dir = (i != std::string::npos) ? s.substr(0, i + 1) : "";
    g_logFile = fopen((dir + "ac_loader.log").c_str(), "w");
}
static void Log(const char* fmt, ...) {
    if (!g_logFile) return;
    va_list args; va_start(args, fmt);
    vfprintf(g_logFile, fmt, args);
    fprintf(g_logFile, "\n");
    fflush(g_logFile);
    va_end(args);
}

// ============================================================================
// GLOBALS
// ============================================================================
static DX11Backend g_backend;
static FontAtlas   g_fontAtlas;
static InputSystem g_input;

static u32  g_fontSm   = 0;   // 12pt
static u32  g_font     = 0;   // 14pt
static u32  g_fontMd   = 0;   // 17pt
static u32  g_fontLg   = 0;   // 24pt
static u32  g_fontXl   = 0;   // 32pt

static HWND g_hwnd    = nullptr;
static bool g_running = true;
static int  g_width   = 520;
static int  g_height  = 680;
static constexpr int CORNER = 16;

// ============================================================================
// TIMING & ANIMATION
// ============================================================================
static f32 g_dt   = 0.016f;
static f32 g_time = 0.0f;
static std::unordered_map<u32, f32> g_anims;

static f32 Anim(u32 id, f32 target, f32 speed = 12.0f) {
    auto& v = g_anims[id];
    v += (target - v) * (std::min)(1.0f, speed * g_dt);
    if (std::abs(v - target) < 0.001f) v = target;
    return v;
}

// ============================================================================
// HELPERS
// ============================================================================
static bool Hit(f32 x, f32 y, f32 w, f32 h) {
    Vec2 m = g_input.MousePos();
    return m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h;
}

static Color Fade(Color c, f32 a) {
    return Color{c.r, c.g, c.b, u8(a * 255.0f + 0.5f)};
}
static Color Mix(Color a, Color b, f32 t) {
    return Color{u8(a.r + (b.r - a.r) * t), u8(a.g + (b.g - a.g) * t),
                 u8(a.b + (b.b - a.b) * t), u8(a.a + (b.a - a.a) * t)};
}

static void Text(DrawList& dl, f32 x, f32 y, Color c, const char* t, u32 fid = 0) {
    g_fontAtlas.RenderText(dl, fid ? fid : g_font, Vec2{x, y}, t, c);
}
static Vec2 Measure(const char* t, u32 fid = 0) {
    return g_fontAtlas.MeasureText(fid ? fid : g_font, t);
}
static void TextCenter(DrawList& dl, Rect r, Color c, const char* t, u32 fid = 0) {
    Vec2 sz = Measure(t, fid);
    Text(dl, r.x + (r.w - sz.x) * 0.5f, r.y + (r.h - sz.y) * 0.5f, c, t, fid);
}

// ============================================================================
// PALETTE — deep dark with blue/purple accents
// ============================================================================
namespace P {
    // Backgrounds
    constexpr Color Bg       {8,   8,   14,  255};
    constexpr Color Card     {16,  16,  24,  245};
    constexpr Color CardHi   {22,  22,  34,  255};
    constexpr Color Surface  {20,  20,  30,  255};
    constexpr Color Input    {14,  14,  22,  255};
    constexpr Color InputFoc {18,  18,  28,  255};

    // Accent gradient endpoints
    constexpr Color Accent1  {79,  110, 255, 255};   // electric blue
    constexpr Color Accent2  {160, 80,  255, 255};   // vivid purple
    constexpr Color AccentDim{79,  110, 255, 60};

    // Status
    constexpr Color Green    {56,  230, 130, 255};
    constexpr Color Red      {255, 90,  90,  255};
    constexpr Color Yellow   {255, 210, 60,  255};
    constexpr Color Orange   {255, 150, 50,  255};

    // Text
    constexpr Color T1       {240, 240, 255, 255};
    constexpr Color T2       {140, 145, 170, 255};
    constexpr Color T3       {70,  72,  95,  255};

    // Borders
    constexpr Color Border   {35,  35,  55,  255};
    constexpr Color BorderLt {50,  50,  75,  180};
}

// ============================================================================
// APP STATE
// ============================================================================
enum Screen { LOGIN, SIGNUP, DASHBOARD };
static Screen g_screen = LOGIN;

static char g_loginUser[64]   = "";
static char g_loginPass[64]   = "";
static char g_signupUser[64]  = "";
static char g_signupPass[64]  = "";
static char g_signupPass2[64] = "";
static std::string g_authError;
static f32 g_authErrorTimer = 0;
static std::string g_loggedUser;
static u32 g_focusedField = 0;

// Subscription
struct SubInfo {
    std::string game;
    std::string expiry;
    int         daysLeft;
    bool        active;
};
static SubInfo g_sub{};
static bool g_hasSub = false;

// Injection
static bool g_injecting = false;
static bool g_injected  = false;
static std::string g_toastMsg;
static f32 g_toastTimer = 0;
static Color g_toastCol = P::Green;

// ============================================================================
// USER DATABASE
// ============================================================================
static std::string GetLicensePath() {
    char p[MAX_PATH]; GetModuleFileNameA(nullptr, p, MAX_PATH);
    std::string s = p;
    size_t i = s.find_last_of('\\');
    return (i != std::string::npos ? s.substr(0, i + 1) : "") + "licenses.dat";
}
static std::string HashPw(const std::string& pw) {
    uint32_t h = 0x811c9dc5;
    for (char c : pw) { h ^= (uint8_t)c; h *= 0x01000193; }
    char b[16]; snprintf(b, 16, "%08X", h); return b;
}
static std::string GenKey() {
    char k[48]; snprintf(k, 48, "AC-%04X-%04X-%04X-%04X",
        rand()&0xFFFF, rand()&0xFFFF, rand()&0xFFFF, rand()&0xFFFF); return k;
}

struct UserRec { std::string user, hash, key; time_t created, expiry; bool active; };
static std::vector<UserRec> g_users;

static void LoadUsers() {
    g_users.clear();
    std::ifstream f(GetLicensePath());
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line); UserRec u;
        ss >> u.user >> u.hash >> u.key >> u.created >> u.expiry >> u.active;
        if (!u.user.empty()) g_users.push_back(u);
    }
}
static void SaveUsers() {
    std::ofstream f(GetLicensePath());
    for (auto& u : g_users)
        f << u.user << " " << u.hash << " " << u.key
          << " " << u.created << " " << u.expiry << " " << u.active << "\n";
}

static bool DoSignup(const char* user, const char* pass) {
    if (strlen(user) < 3) { g_authError = "Username: min 3 chars"; g_authErrorTimer = 3; return false; }
    if (strlen(pass) < 4) { g_authError = "Password: min 4 chars"; g_authErrorTimer = 3; return false; }
    for (auto& u : g_users)
        if (u.user == user) { g_authError = "Username taken"; g_authErrorTimer = 3; return false; }
    UserRec u; u.user = user; u.hash = HashPw(pass); u.key = GenKey();
    u.created = time(nullptr); u.expiry = u.created + 30*86400; u.active = true;
    g_users.push_back(u); SaveUsers(); return true;
}

static bool DoLogin(const char* user, const char* pass) {
    std::string h = HashPw(pass);
    for (auto& u : g_users) {
        if (u.user == user && u.hash == h) {
            if (!u.active) { g_authError = "Account deactivated"; g_authErrorTimer = 3; return false; }
            if (time(nullptr) > u.expiry) { g_authError = "Subscription expired"; g_authErrorTimer = 3; return false; }
            g_loggedUser = user;
            int dl = (int)((u.expiry - time(nullptr)) / 86400);
            char buf[64]; snprintf(buf, 64, "%d days remaining", dl > 0 ? dl : 0);
            g_sub = {"Counter-Strike 2", buf, dl > 0 ? dl : 0, true};
            g_hasSub = true;
            return true;
        }
    }
    g_authError = "Invalid credentials"; g_authErrorTimer = 3; return false;
}

// ============================================================================
// INJECTION
// ============================================================================
static DWORD FindProc(const char* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32 pe; pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) {
        do { if (_stricmp(pe.szExeFile, name) == 0) { CloseHandle(snap); return pe.th32ProcessID; }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap); return 0;
}
static bool Inject(DWORD pid, const char* dll) {
    HANDLE hp = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hp) return false;
    size_t len = strlen(dll) + 1;
    LPVOID rm = VirtualAllocEx(hp, nullptr, len, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!rm) { CloseHandle(hp); return false; }
    if (!WriteProcessMemory(hp, rm, dll, len, nullptr)) {
        VirtualFreeEx(hp, rm, 0, MEM_RELEASE); CloseHandle(hp); return false;
    }
    LPVOID ll = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!ll) { VirtualFreeEx(hp, rm, 0, MEM_RELEASE); CloseHandle(hp); return false; }
    HANDLE ht = CreateRemoteThread(hp, nullptr, 0, (LPTHREAD_START_ROUTINE)ll, rm, 0, nullptr);
    if (!ht) { VirtualFreeEx(hp, rm, 0, MEM_RELEASE); CloseHandle(hp); return false; }
    WaitForSingleObject(ht, 10000);
    DWORD ec = 0; GetExitCodeThread(ht, &ec);
    CloseHandle(ht); VirtualFreeEx(hp, rm, 0, MEM_RELEASE); CloseHandle(hp);
    return ec != 0;
}
static void DoInject() {
    g_injecting = true;
    DWORD pid = FindProc("cs2.exe");
    if (!pid) { g_toastMsg = "CS2 not running - start game first"; g_toastCol = P::Red; g_toastTimer = 4; g_injecting = false; return; }

    // Extract embedded DLL from Windows resources
    HRSRC hRes = FindResource(nullptr, MAKEINTRESOURCE(IDR_SKIN_DLL), RT_RCDATA);
    if (!hRes) { g_toastMsg = "DLL resource not found"; g_toastCol = P::Red; g_toastTimer = 4; g_injecting = false; return; }
    HGLOBAL hGlob = LoadResource(nullptr, hRes);
    DWORD dwSize = SizeofResource(nullptr, hRes);
    const void* pData = LockResource(hGlob);
    if (!pData || dwSize == 0) { g_toastMsg = "Invalid DLL resource"; g_toastCol = P::Red; g_toastTimer = 4; g_injecting = false; return; }

    // Write to temp directory
    char tempDir[MAX_PATH];
    GetTempPathA(MAX_PATH, tempDir);
    std::string dllPath = std::string(tempDir) + "skin_changer.dll";
    HANDLE hFile = CreateFileA(dllPath.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        g_toastMsg = "Failed to extract DLL"; g_toastCol = P::Red; g_toastTimer = 4; g_injecting = false; return;
    }
    DWORD written = 0;
    WriteFile(hFile, pData, dwSize, &written, nullptr);
    CloseHandle(hFile);

    // Inject into CS2
    if (Inject(pid, dllPath.c_str())) {
        g_toastMsg = "Injected successfully"; g_toastCol = P::Green; g_injected = true;
    } else {
        g_toastMsg = "Injection failed - run as administrator"; g_toastCol = P::Orange;
    }
    g_toastTimer = 4; g_injecting = false;
}

// ============================================================================
// WNDPROC
// ============================================================================
static bool g_dragging = false;
static POINT g_dragPt = {};

static LRESULT CALLBACK WndProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_DESTROY: PostQuitMessage(0); g_running = false; return 0;
    case WM_SIZE:
        if (wp != SIZE_MINIMIZED) {
            g_width = LOWORD(lp); g_height = HIWORD(lp);
            if (g_width > 0 && g_height > 0) g_backend.Resize((u32)g_width, (u32)g_height);
            HRGN rgn = CreateRoundRectRgn(0, 0, g_width+1, g_height+1, CORNER*2, CORNER*2);
            SetWindowRgn(hw, rgn, TRUE);
        }
        return 0;
    case WM_MOUSEMOVE: {
        f32 x = (f32)(short)LOWORD(lp), y = (f32)(short)HIWORD(lp);
        g_input.OnMouseMove(x, y);
        if (g_dragging) { POINT pt; GetCursorPos(&pt);
            SetWindowPos(hw, nullptr, pt.x - g_dragPt.x, pt.y - g_dragPt.y, 0, 0, SWP_NOSIZE|SWP_NOZORDER); }
        return 0;
    }
    case WM_LBUTTONDOWN: {
        g_input.OnMouseDown(MouseButton::Left);
        POINT pt = {(SHORT)LOWORD(lp), (SHORT)HIWORD(lp)};
        if (pt.y < 48 && pt.x < g_width - 90) { g_dragging = true; g_dragPt = pt; SetCapture(hw); }
        return 0;
    }
    case WM_LBUTTONUP:   g_input.OnMouseUp(MouseButton::Left); if (g_dragging) { g_dragging = false; ReleaseCapture(); } return 0;
    case WM_RBUTTONDOWN: g_input.OnMouseDown(MouseButton::Right); return 0;
    case WM_RBUTTONUP:   g_input.OnMouseUp(MouseButton::Right); return 0;
    case WM_MOUSEWHEEL:  g_input.OnMouseScroll((f32)GET_WHEEL_DELTA_WPARAM(wp) / 120.0f); return 0;
    case WM_KEYDOWN:     g_input.OnKeyDown((Key)(int)wp); return 0;
    case WM_KEYUP:       g_input.OnKeyUp((Key)(int)wp); return 0;
    case WM_CHAR:        if (wp >= 32 && wp < 127) g_input.OnTextInput((char)wp); return 0;
    }
    return DefWindowProcA(hw, msg, wp, lp);
}

// ============================================================================
// DRAWING COMPONENTS
// ============================================================================

// --- Background gradient mesh ---
static void DrawBg(DrawList& dl, f32 W, f32 H) {
    dl.AddFilledRect(Rect{0, 0, W, H}, P::Bg);

    // Animated accent orbs — soft blurred glow
    f32 t = g_time * 0.3f;
    f32 orbX1 = W * 0.2f + sinf(t * 0.7f) * 40.0f;
    f32 orbY1 = H * 0.25f + cosf(t * 0.5f) * 30.0f;
    f32 orbX2 = W * 0.8f + cosf(t * 0.6f) * 35.0f;
    f32 orbY2 = H * 0.7f + sinf(t * 0.8f) * 25.0f;

    f32 orbR = 120.0f;
    for (int i = 6; i >= 1; i--) {
        f32 r = orbR * (f32)i / 3.0f;
        u8 a = (u8)(4.0f * (7 - i));
        dl.AddFilledRoundRect(Rect{orbX1 - r, orbY1 - r, r*2, r*2},
                              Color{79, 110, 255, a}, r, 24);
        dl.AddFilledRoundRect(Rect{orbX2 - r, orbY2 - r, r*2, r*2},
                              Color{160, 80, 255, a}, r, 24);
    }
}

// --- Title bar ---
static void DrawTitleBar(DrawList& dl, f32 W) {
    // Subtle top bar bg
    dl.AddGradientRect(Rect{0, 0, W, 48},
        Color{12, 12, 20, 220}, Color{12, 12, 20, 220},
        Color{12, 12, 20, 120}, Color{12, 12, 20, 120});

    // AC branding
    Vec2 ls = Measure("AC", g_fontMd);
    Text(dl, 20, (48 - ls.y) * 0.5f, P::Accent1, "AC", g_fontMd);
    Vec2 vs = Measure("LOADER", g_fontSm);
    Text(dl, 20 + ls.x + 8, (48 - vs.y) * 0.5f, P::T3, "LOADER", g_fontSm);

    // Minimize button
    f32 btnSz = 12.0f;
    f32 minX = W - 68, btnY = 18;
    u32 minId = Hash("_min");
    bool minH = Hit(minX - 6, btnY - 6, btnSz + 12, btnSz + 12);
    f32 minA = Anim(minId, minH ? 1.0f : 0.0f);
    dl.AddFilledRoundRect(Rect{minX - 8, btnY - 8, btnSz + 16, btnSz + 16},
                          Fade(Color{255,255,255,255}, 0.04f * minA), 6, 8);
    dl.AddLine(Vec2{minX, btnY + btnSz*0.5f}, Vec2{minX + btnSz, btnY + btnSz*0.5f},
               Color{160, 165, 190, (u8)(150 + 105 * minA)}, 1.5f);
    if (minH && g_input.IsMousePressed(MouseButton::Left)) ShowWindow(g_hwnd, SW_MINIMIZE);

    // Close button
    f32 clsX = W - 36;
    u32 clsId = Hash("_cls");
    bool clsH = Hit(clsX - 6, btnY - 6, btnSz + 12, btnSz + 12);
    f32 clsA = Anim(clsId, clsH ? 1.0f : 0.0f);
    dl.AddFilledRoundRect(Rect{clsX - 8, btnY - 8, btnSz + 16, btnSz + 16},
                          Fade(P::Red, 0.15f * clsA), 6, 8);
    f32 cx = clsX + btnSz*0.5f, cy = btnY + btnSz*0.5f, s = 4.0f;
    Color xc{160, 165, 190, (u8)(150 + 105 * clsA)};
    dl.AddLine(Vec2{cx-s, cy-s}, Vec2{cx+s, cy+s}, xc, 1.5f);
    dl.AddLine(Vec2{cx+s, cy-s}, Vec2{cx-s, cy+s}, xc, 1.5f);
    if (clsH && g_input.IsMousePressed(MouseButton::Left)) g_running = false;

    // Bottom separator — accent gradient line
    dl.AddGradientRect(Rect{0, 47, W, 1},
        Color{79, 110, 255, 0},  Color{79, 110, 255, 40},
        Color{160, 80, 255, 40}, Color{160, 80, 255, 0});
}

// --- Gradient accent button ---
static bool GradientButton(DrawList& dl, const char* label, Rect r, bool enabled = true) {
    u32 id = Hash(label);
    bool hov = enabled && Hit(r.x, r.y, r.w, r.h);
    bool click = hov && g_input.IsMousePressed(MouseButton::Left);
    f32 a = Anim(id, hov ? 1.0f : 0.0f);

    // Glow
    if (a > 0.01f) {
        for (int i = 4; i >= 1; i--) {
            f32 spread = (f32)i * 2.0f;
            Color gc = Fade(P::Accent1, 0.04f * a * (5.0f - i));
            dl.AddFilledRoundRect(Rect{r.x - spread, r.y - spread,
                                        r.w + spread*2, r.h + spread*2},
                                  gc, 12.0f + spread, 12);
        }
    }

    // Solid rounded rect with mixed accent
    f32 op = enabled ? (0.85f + 0.15f * a) : 0.35f;
    Color c1 = Fade(P::Accent1, op);
    Color c2 = Fade(P::Accent2, op);
    Color mixed = Mix(c1, c2, 0.5f);
    dl.AddFilledRoundRect(r, mixed, 10.0f, 12);

    // Top shine
    if (a > 0.01f) {
        dl.AddFilledRoundRect(Rect{r.x + 2, r.y + 2, r.w - 4, r.h * 0.35f},
                              Fade(Color{255,255,255,255}, 0.07f * a), 8.0f, 8);
    }

    Color tc = enabled ? Color{255,255,255,255} : Color{200,200,210,140};
    TextCenter(dl, r, tc, label, g_font);
    return click && enabled;
}

// --- Input field ---
static void InputField(DrawList& dl, const char* label, char* buf, int sz,
                        Rect r, bool password = false) {
    u32 id = Hash(label);
    bool focused = g_focusedField == id;
    bool hov = Hit(r.x, r.y, r.w, r.h);
    f32 a = Anim(id, focused ? 1.0f : (hov ? 0.4f : 0.0f));

    // Focus glow
    if (a > 0.3f) {
        Color gc = Fade(P::Accent1, 0.08f * a);
        dl.AddFilledRoundRect(Rect{r.x - 3, r.y - 3, r.w + 6, r.h + 6}, gc, 13.0f, 12);
    }

    // Border
    Color bc = Mix(P::Border, P::Accent1, a * 0.6f);
    dl.AddFilledRoundRect(Rect{r.x - 1, r.y - 1, r.w + 2, r.h + 2}, bc, 11.0f, 12);

    // Background
    Color bg = focused ? P::InputFoc : P::Input;
    dl.AddFilledRoundRect(r, bg, 10.0f, 12);

    // Click to focus
    if (hov && g_input.IsMousePressed(MouseButton::Left))
        g_focusedField = id;

    // Text input
    if (focused) {
        int len = (int)strlen(buf);
        std::string_view inp = g_input.TextInput();
        for (char c : inp)
            if (c >= 32 && c < 127 && len < sz - 1) { buf[len++] = c; buf[len] = '\0'; }
        if (g_input.IsKeyPressed(Key::Backspace) && len > 0) buf[len - 1] = '\0';
    }

    // Measure line height
    f32 lh = 14.0f;
    const FontInstance* fi = g_fontAtlas.GetFont(g_font);
    if (fi) lh = fi->lineHeight;

    int slen = (int)strlen(buf);
    f32 textY = r.y + (r.h - lh) * 0.5f;

    if (slen > 0) {
        std::string display = password ? std::string(slen, '*') : std::string(buf);
        Text(dl, r.x + 16, textY, P::T1, display.c_str());
        if (focused) {
            f32 blink = fmodf(g_time * 2.0f, 2.0f);
            if (blink < 1.0f) {
                Vec2 tw = Measure(display.c_str());
                dl.AddFilledRect(Rect{r.x + 16 + tw.x + 2, r.y + 10, 1.5f, r.h - 20}, P::Accent1);
            }
        }
    } else {
        Text(dl, r.x + 16, textY, P::T3, label);
        if (focused) {
            f32 blink = fmodf(g_time * 2.0f, 2.0f);
            if (blink < 1.0f)
                dl.AddFilledRect(Rect{r.x + 16, r.y + 10, 1.5f, r.h - 20}, P::Accent1);
        }
    }
}

// --- Glass card container ---
static void GlassCard(DrawList& dl, Rect r) {
    // Shadow layers
    for (int i = 4; i >= 1; i--) {
        f32 sp = (f32)i * 3.0f;
        dl.AddFilledRoundRect(Rect{r.x - sp + 2, r.y - sp + 4, r.w + sp*2 - 4, r.h + sp*2},
                              Fade(Color{0,0,0,255}, 0.06f * (5 - i)), 14.0f + sp, 12);
    }

    // Border
    dl.AddFilledRoundRect(Rect{r.x - 1, r.y - 1, r.w + 2, r.h + 2}, P::Border, 15.0f, 12);

    // Card bg
    dl.AddFilledRoundRect(r, P::Card, 14.0f, 12);

    // Top edge accent
    dl.AddGradientRect(Rect{r.x + 14, r.y, r.w - 28, 1},
        Fade(P::Accent1, 0.0f), Fade(P::Accent1, 0.3f),
        Fade(P::Accent2, 0.3f), Fade(P::Accent2, 0.0f));
}

// --- Status pill ---
static void Pill(DrawList& dl, f32 x, f32 y, const char* label, const char* value, Color accent) {
    Vec2 ls = Measure(label, g_fontSm);
    Vec2 vs = Measure(value, g_fontSm);
    f32 w = (std::max)(ls.x, vs.x) + 24;
    f32 h = 48.0f;

    dl.AddFilledRoundRect(Rect{x, y, w, h}, P::Surface, 10.0f, 10);
    // Accent top bar
    dl.AddFilledRoundRect(Rect{x + 8, y + 4, w - 16, 2}, Fade(accent, 0.5f), 1.0f, 4);

    Text(dl, x + 12, y + 10, P::T3, label, g_fontSm);
    Text(dl, x + 12, y + 27, accent, value, g_fontSm);
}

// --- Toast notification ---
static void DrawToast(DrawList& dl, f32 W, f32 H) {
    if (g_toastTimer <= 0) return;
    g_toastTimer -= g_dt;
    f32 a = g_toastTimer > 0.5f ? 1.0f : g_toastTimer / 0.5f;
    if (a <= 0.01f) return;

    f32 slideY = (1.0f - a) * 20.0f;
    Vec2 ts = Measure(g_toastMsg.c_str());
    f32 tw = ts.x + 40;
    f32 th = 38;
    f32 tx = (W - tw) * 0.5f;
    f32 ty = H - 60 + slideY;

    dl.AddFilledRoundRect(Rect{tx - 2, ty + 2, tw + 4, th + 2},
                          Fade(Color{0,0,0,255}, 0.3f * a), 10.0f, 12);
    dl.AddFilledRoundRect(Rect{tx, ty, tw, th}, Fade(P::Card, a), 10.0f, 12);
    dl.AddFilledRoundRect(Rect{tx + 4, ty + 8, 3, th - 16}, Fade(g_toastCol, a), 1.5f, 4);
    TextCenter(dl, Rect{tx, ty, tw, th}, Fade(P::T1, a), g_toastMsg.c_str());
}

// ============================================================================
// SCREEN: LOGIN
// ============================================================================
static void ScreenLogin(DrawList& dl, f32 W, f32 H) {
    DrawBg(dl, W, H);
    DrawTitleBar(dl, W);

    // Centered glass card
    f32 cardW = 380, cardH = 380;
    f32 cardX = (W - cardW) * 0.5f;
    f32 cardY = 80;
    GlassCard(dl, Rect{cardX, cardY, cardW, cardH});

    f32 cx = cardX + 30;
    f32 cw = cardW - 60;
    f32 cy = cardY + 28;

    // Title
    Vec2 titleSz = Measure("Welcome back", g_fontLg);
    Text(dl, cardX + (cardW - titleSz.x) * 0.5f, cy, P::T1, "Welcome back", g_fontLg);
    cy += titleSz.y + 4;

    Vec2 subSz = Measure("Sign in to continue", g_fontSm);
    Text(dl, cardX + (cardW - subSz.x) * 0.5f, cy, P::T2, "Sign in to continue", g_fontSm);
    cy += subSz.y + 28;

    // Fields
    InputField(dl, "Username", g_loginUser, sizeof(g_loginUser),
               Rect{cx, cy, cw, 44});
    cy += 56;

    InputField(dl, "Password", g_loginPass, sizeof(g_loginPass),
               Rect{cx, cy, cw, 44}, true);
    cy += 56;

    // Error
    if (g_authErrorTimer > 0) {
        g_authErrorTimer -= g_dt;
        f32 ea = g_authErrorTimer > 0.5f ? 1.0f : g_authErrorTimer / 0.5f;
        Vec2 es = Measure(g_authError.c_str(), g_fontSm);
        Text(dl, cardX + (cardW - es.x) * 0.5f, cy, Fade(P::Red, ea), g_authError.c_str(), g_fontSm);
    }
    cy += 20;

    // Sign in button
    if (GradientButton(dl, "SIGN IN", Rect{cx, cy, cw, 46})) {
        if (DoLogin(g_loginUser, g_loginPass)) { g_screen = DASHBOARD; }
    }
    cy += 58;

    // Enter key shortcut
    if (g_input.IsKeyPressed(Key::Enter) && strlen(g_loginUser) > 0 && strlen(g_loginPass) > 0) {
        if (DoLogin(g_loginUser, g_loginPass)) g_screen = DASHBOARD;
    }

    // Divider
    f32 divY = cy + 4;
    dl.AddGradientRect(Rect{cx, divY, cw * 0.42f, 1},
        Color{35,35,55,0}, Color{35,35,55,0}, P::Border, P::Border);
    Text(dl, cx + cw * 0.42f + 8, divY - 6, P::T3, "or", g_fontSm);
    dl.AddGradientRect(Rect{cx + cw * 0.58f, divY, cw * 0.42f, 1},
        P::Border, P::Border, Color{35,35,55,0}, Color{35,35,55,0});

    cy += 24;

    // Create account link
    const char* caText = "Create new account";
    Vec2 caTs = Measure(caText, g_fontSm);
    f32 caX = cardX + (cardW - caTs.x) * 0.5f;
    u32 caId = Hash("_create_link");
    bool caH = Hit(caX, cy, caTs.x, caTs.y + 4);
    f32 caA = Anim(caId, caH ? 1.0f : 0.0f);
    Text(dl, caX, cy, Mix(P::T2, P::Accent1, 0.3f + 0.7f * caA), caText, g_fontSm);
    if (caH && g_input.IsMousePressed(MouseButton::Left)) {
        g_screen = SIGNUP; g_authError.clear(); g_authErrorTimer = 0;
        memset(g_signupUser, 0, 64); memset(g_signupPass, 0, 64); memset(g_signupPass2, 0, 64);
    }

    // Footer
    Vec2 fts = Measure("AC Loader v4.0", g_fontSm);
    Text(dl, (W - fts.x) * 0.5f, H - 30, P::T3, "AC Loader v4.0", g_fontSm);

    DrawToast(dl, W, H);
}

// ============================================================================
// SCREEN: SIGNUP
// ============================================================================
static void ScreenSignup(DrawList& dl, f32 W, f32 H) {
    DrawBg(dl, W, H);
    DrawTitleBar(dl, W);

    f32 cardW = 380, cardH = 440;
    f32 cardX = (W - cardW) * 0.5f, cardY = 72;
    GlassCard(dl, Rect{cardX, cardY, cardW, cardH});

    f32 cx = cardX + 30, cw = cardW - 60;
    f32 cy = cardY + 28;

    Vec2 ts = Measure("Create account", g_fontLg);
    Text(dl, cardX + (cardW - ts.x) * 0.5f, cy, P::T1, "Create account", g_fontLg);
    cy += ts.y + 4;

    Vec2 ss = Measure("Get started with a free trial", g_fontSm);
    Text(dl, cardX + (cardW - ss.x) * 0.5f, cy, P::T2, "Get started with a free trial", g_fontSm);
    cy += ss.y + 28;

    InputField(dl, "Username", g_signupUser, sizeof(g_signupUser), Rect{cx, cy, cw, 44});
    cy += 54;
    InputField(dl, "Password", g_signupPass, sizeof(g_signupPass), Rect{cx, cy, cw, 44}, true);
    cy += 54;
    InputField(dl, "Confirm password", g_signupPass2, sizeof(g_signupPass2), Rect{cx, cy, cw, 44}, true);
    cy += 54;

    if (g_authErrorTimer > 0) {
        g_authErrorTimer -= g_dt;
        f32 ea = g_authErrorTimer > 0.5f ? 1.0f : g_authErrorTimer / 0.5f;
        Vec2 es = Measure(g_authError.c_str(), g_fontSm);
        Text(dl, cardX + (cardW - es.x) * 0.5f, cy, Fade(P::Red, ea), g_authError.c_str(), g_fontSm);
    }
    cy += 18;

    if (GradientButton(dl, "CREATE ACCOUNT", Rect{cx, cy, cw, 46})) {
        if (strcmp(g_signupPass, g_signupPass2) != 0) { g_authError = "Passwords don't match"; g_authErrorTimer = 3; }
        else if (DoSignup(g_signupUser, g_signupPass)) {
            if (DoLogin(g_signupUser, g_signupPass)) g_screen = DASHBOARD;
        }
    }
    if (g_input.IsKeyPressed(Key::Enter) && strlen(g_signupUser) > 0 && strlen(g_signupPass) > 0) {
        if (strcmp(g_signupPass, g_signupPass2) != 0) { g_authError = "Passwords don't match"; g_authErrorTimer = 3; }
        else if (DoSignup(g_signupUser, g_signupPass)) {
            if (DoLogin(g_signupUser, g_signupPass)) g_screen = DASHBOARD;
        }
    }
    cy += 58;

    // Back link
    const char* bl = "Already have an account? Sign in";
    Vec2 bls = Measure(bl, g_fontSm);
    f32 blX = cardX + (cardW - bls.x) * 0.5f;
    u32 blId = Hash("_back_link");
    bool blH = Hit(blX, cy, bls.x, bls.y + 4);
    f32 blA = Anim(blId, blH ? 1.0f : 0.0f);
    Text(dl, blX, cy, Mix(P::T2, P::Accent1, 0.3f + 0.7f * blA), bl, g_fontSm);
    if (blH && g_input.IsMousePressed(MouseButton::Left)) {
        g_screen = LOGIN; g_authError.clear(); g_authErrorTimer = 0;
    }

    Vec2 fts = Measure("AC Loader v4.0", g_fontSm);
    Text(dl, (W - fts.x) * 0.5f, H - 30, P::T3, "AC Loader v4.0", g_fontSm);
    DrawToast(dl, W, H);
}

// ============================================================================
// SCREEN: DASHBOARD
// ============================================================================
static void ScreenDashboard(DrawList& dl, f32 W, f32 H) {
    DrawBg(dl, W, H);
    DrawTitleBar(dl, W);

    // User avatar in top-right
    {
        f32 avX = W - 52, avY = 12, avR = 12;
        dl.AddFilledRoundRect(Rect{avX, avY, avR*2, avR*2},
                              Color{30, 35, 60, 255}, avR, 16);
        if (!g_loggedUser.empty()) {
            char ini[2] = {(char)toupper(g_loggedUser[0]), '\0'};
            Vec2 is = Measure(ini, g_fontSm);
            Text(dl, avX + avR - is.x * 0.5f, avY + avR - is.y * 0.5f, P::Accent1, ini, g_fontSm);
        }
        // Online dot
        dl.AddFilledRoundRect(Rect{avX + avR*2 - 5, avY + avR*2 - 5, 7, 7},
                              P::Green, 3.5f, 8);
    }

    f32 pad = 28;
    f32 contentW = W - pad * 2;
    f32 y = 68;

    // Greeting
    std::string greeting = "Hey, " + g_loggedUser;
    Text(dl, pad, y, P::T1, greeting.c_str(), g_fontLg);
    y += 32;
    Text(dl, pad, y, P::T2, "Your dashboard", g_fontSm);
    y += 32;

    // ============================================================
    // GAME HERO CARD
    // ============================================================
    {
        f32 heroH = 140;
        Rect heroR{pad, y, contentW, heroH};

        // Shadow
        dl.AddFilledRoundRect(Rect{heroR.x + 4, heroR.y + 6, heroR.w - 8, heroR.h},
                              Fade(Color{0,0,0,255}, 0.25f), 14.0f, 12);

        // Border
        dl.AddFilledRoundRect(Rect{heroR.x - 1, heroR.y - 1, heroR.w + 2, heroR.h + 2},
                              P::Border, 15.0f, 12);

        // Card bg
        dl.AddFilledRoundRect(heroR, P::Card, 14.0f, 12);

        // Top gradient overlay
        dl.AddGradientRect(Rect{heroR.x + 1, heroR.y + 1, heroR.w - 2, heroH * 0.4f},
            Fade(P::Accent1, 0.10f), Fade(P::Accent2, 0.10f),
            Fade(P::Accent2, 0.0f),  Fade(P::Accent1, 0.0f));

        // Game icon box
        f32 iconSz = 56;
        f32 iconX = heroR.x + 20;
        f32 iconY = heroR.y + (heroH - iconSz) * 0.5f;
        dl.AddFilledRoundRect(Rect{iconX, iconY, iconSz, iconSz},
                              Color{220, 160, 40, 255}, 14.0f, 10);
        TextCenter(dl, Rect{iconX, iconY, iconSz, iconSz},
                   Color{255,255,255,240}, "CS2", g_fontMd);

        // Game title + details
        f32 textX = iconX + iconSz + 20;
        Text(dl, textX, heroR.y + 30, P::T1, "Counter-Strike 2", g_fontMd);
        Text(dl, textX, heroR.y + 55, P::T2, "Skin Changer — Full Access", g_fontSm);

        // Status badge
        if (g_sub.active) {
            f32 badgeY = heroR.y + 82;
            dl.AddFilledRoundRect(Rect{textX, badgeY, 62, 22},
                                  Fade(P::Green, 0.15f), 6.0f, 8);
            TextCenter(dl, Rect{textX, badgeY, 62, 22}, P::Green, "ACTIVE", g_fontSm);
            Text(dl, textX + 72, badgeY + 4, P::T3, g_sub.expiry.c_str(), g_fontSm);
        }

        y += heroH + 20;
    }

    // ============================================================
    // STATUS PILLS ROW
    // ============================================================
    {
        char daysStr[16]; snprintf(daysStr, 16, "%d", g_sub.daysLeft);
        Pill(dl, pad, y, "DAYS LEFT", daysStr, P::Accent1);

        const char* statusStr = g_injected ? "Injected" : "Ready";
        Color statusCol = g_injected ? P::Green : P::Yellow;
        Pill(dl, pad + 100, y, "STATUS", statusStr, statusCol);

        Pill(dl, pad + 200, y, "VERSION", "4.0", P::Accent2);

        y += 68;
    }

    // ============================================================
    // LAUNCH BUTTON
    // ============================================================
    {
        const char* btnText;
        bool enabled;
        if (g_injected) { btnText = "LAUNCHED"; enabled = false; }
        else if (g_injecting) { btnText = "INJECTING..."; enabled = false; }
        else { btnText = "LAUNCH CHEAT"; enabled = true; }

        if (GradientButton(dl, btnText, Rect{pad, y, contentW, 50}, enabled)) {
            DoInject();
        }
        y += 64;
    }

    // ============================================================
    // QUICK LINKS ROW
    // ============================================================
    {
        const char* links[] = {"Changelog", "Support", "Settings"};
        f32 lx = pad;
        for (int i = 0; i < 3; i++) {
            u32 lid = Hash(links[i]);
            Vec2 ls = Measure(links[i], g_fontSm);
            f32 lw = ls.x + 24;
            bool lh = Hit(lx, y, lw, 32);
            f32 la = Anim(lid, lh ? 1.0f : 0.0f);

            dl.AddFilledRoundRect(Rect{lx, y, lw, 32},
                                  Mix(P::Surface, P::CardHi, la), 8.0f, 8);
            TextCenter(dl, Rect{lx, y, lw, 32},
                       Mix(P::T2, P::T1, la), links[i], g_fontSm);
            lx += lw + 10;
        }
        y += 48;
    }

    // ============================================================
    // LOGOUT
    // ============================================================
    {
        const char* lo = "Sign out";
        Vec2 los = Measure(lo, g_fontSm);
        f32 loX = (W - los.x) * 0.5f;
        u32 loId = Hash("_signout");
        bool loH = Hit(loX, y, los.x, los.y + 4);
        f32 loA = Anim(loId, loH ? 1.0f : 0.0f);
        Text(dl, loX, y, Mix(P::T3, P::Red, loA * 0.7f), lo, g_fontSm);
        if (loH && g_input.IsMousePressed(MouseButton::Left)) {
            g_screen = LOGIN; g_loggedUser.clear(); g_hasSub = false;
            g_injected = false; g_injecting = false;
            memset(g_loginPass, 0, 64);
        }
    }

    DrawToast(dl, W, H);
}

// ============================================================================
// ENTRY POINT
// ============================================================================
static int LoaderMain(HINSTANCE hInstance);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    LogInit();
    Log("=== AC Loader starting ===");
    __try {
        return LoaderMain(hInstance);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("CRASH: unhandled exception 0x%08X", GetExceptionCode());
        MessageBoxA(nullptr,
            "AC Loader crashed unexpectedly.\n\n"
            "Check ac_loader.log for details.\n"
            "Try running as Administrator.",
            "AC Loader - Error", MB_OK | MB_ICONERROR);
        return 1;
    }
}

static int LoaderMain(HINSTANCE hInstance) {
    Log("Step 1: Init");
    srand((unsigned)time(nullptr));
    LoadUsers();
    Log("Step 2: Users loaded (%d)", (int)g_users.size());

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "ACLoaderV4";
    RegisterClassExA(&wc);
    Log("Step 3: Window class registered");

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    g_hwnd = CreateWindowExA(WS_EX_APPWINDOW, wc.lpszClassName, "AC Loader",
        WS_POPUP | WS_VISIBLE,
        (sw - g_width) / 2, (sh - g_height) / 2, g_width, g_height,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_hwnd) {
        Log("FAIL: CreateWindowExA error=%lu", GetLastError());
        MessageBoxA(nullptr, "Failed to create window", "Error", MB_OK); return 1;
    }
    Log("Step 4: Window created %dx%d", g_width, g_height);

    HRGN rgn = CreateRoundRectRgn(0, 0, g_width+1, g_height+1, CORNER*2, CORNER*2);
    SetWindowRgn(g_hwnd, rgn, TRUE);

    Log("Step 5: Initializing DX11...");
    if (!g_backend.Initialize(g_hwnd, (u32)g_width, (u32)g_height)) {
        Log("FAIL: DX11 init failed");
        MessageBoxA(nullptr, "DX11 init failed.\n\nCheck ac_loader.log", "Error", MB_OK);
        return 1;
    }
    Log("Step 6: DX11 OK");

    // Load fonts — bold/heavy weights for premium feel
    auto TryFont = [](const char* paths[], int count, f32 size) -> u32 {
        for (int i = 0; i < count; i++) {
            u32 id = g_fontAtlas.AddFont(paths[i], size, &g_backend);
            if (id != 0) return id;
        }
        return 0;
    };

    // Heavy weight for headings (Black > Bold > Regular fallback)
    const char* heavyFonts[] = {
        "C:\\Windows\\Fonts\\seguibl.ttf",   // Segoe UI Black
        "C:\\Windows\\Fonts\\segoeuib.ttf",  // Segoe UI Bold
        "C:\\Windows\\Fonts\\arialbd.ttf",   // Arial Bold
        "C:\\Windows\\Fonts\\arial.ttf",
    };
    // Bold for body/buttons
    const char* boldFonts[] = {
        "C:\\Windows\\Fonts\\segoeuib.ttf",  // Segoe UI Bold
        "C:\\Windows\\Fonts\\seguisb.ttf",   // Segoe UI Semibold
        "C:\\Windows\\Fonts\\arialbd.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
    };
    // Semibold for small/body text
    const char* bodyFonts[] = {
        "C:\\Windows\\Fonts\\seguisb.ttf",   // Segoe UI Semibold
        "C:\\Windows\\Fonts\\segoeui.ttf",   // Segoe UI Regular
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\tahoma.ttf",
    };

    g_fontSm = TryFont(bodyFonts,  4, 13.0f);
    g_font   = TryFont(boldFonts,  4, 15.0f);
    g_fontMd = TryFont(boldFonts,  4, 18.0f);
    g_fontLg = TryFont(heavyFonts, 4, 26.0f);
    g_fontXl = TryFont(heavyFonts, 4, 34.0f);

    bool loaded = (g_fontSm && g_font && g_fontMd && g_fontLg && g_fontXl);
    if (!loaded) {
        Log("FAIL: No system font found");
        MessageBoxA(nullptr, "No system font found.\nCheck ac_loader.log", "Error", MB_OK);
        g_backend.Shutdown(); return 1;
    }
    Log("Step 7: Fonts loaded (bold/heavy weights)");

    ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hwnd);
    Log("Step 8: Window shown — entering render loop");

    LARGE_INTEGER freq, last;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&last);

    MSG msg = {};
    while (g_running) {
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessageA(&msg);
            if (msg.message == WM_QUIT) g_running = false;
        }
        if (!g_running) break;

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        g_dt = (f32)(now.QuadPart - last.QuadPart) / (f32)freq.QuadPart;
        if (g_dt > 0.1f) g_dt = 0.016f;
        g_time += g_dt;
        last = now;

        g_input.BeginFrame();
        g_backend.BeginFrame();
        g_backend.SetClearColor(P::Bg);

        DrawList dl;
        f32 W = (f32)g_width, H = (f32)g_height;

        switch (g_screen) {
        case LOGIN:     ScreenLogin(dl, W, H);     break;
        case SIGNUP:    ScreenSignup(dl, W, H);     break;
        case DASHBOARD: ScreenDashboard(dl, W, H);  break;
        }

        Vec2 vp = g_backend.GetViewportSize();
        g_backend.RenderDrawList(dl, (u32)vp.x, (u32)vp.y);
        g_backend.EndFrame();
        g_backend.Present();
    }

    g_backend.Shutdown();
    DestroyWindow(g_hwnd);
    UnregisterClassA(wc.lpszClassName, hInstance);
    Log("Shutdown complete");
    if (g_logFile) fclose(g_logFile);
    return 0;
}

#else
int main() {
    printf("AC Loader requires Windows (DX11).\n");
    return 0;
}
#endif
