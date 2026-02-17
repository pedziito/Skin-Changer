/*
 * AC Loader — Professional GUI Launcher
 * Custom DX11 rendering engine. Login/Signup → Subscription → Launch.
 * "AC" branding: white bold text with blue shadow glow (no box).
 * Truly rounded window via SetWindowRgn. Professional dark UI.
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <TlHelp32.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")

#include "../engine/ace_renderer.h"
#include "../engine/ace_ui.h"

// ============================================================================
// GLOBALS
// ============================================================================
static ID3D11Device*            g_device = nullptr;
static ID3D11DeviceContext*     g_context = nullptr;
static IDXGISwapChain*          g_swapChain = nullptr;
static ID3D11RenderTargetView*  g_rtv = nullptr;
static HWND                     g_hwnd = nullptr;
static bool                     g_running = true;
static int                      g_width = 480;
static int                      g_height = 560;
static constexpr int            CORNER_RADIUS = 14;

static ACEFont      g_font;
static ACEFont      g_fontLarge;  // For the AC logo
static ACERenderer  g_renderer;
static ACEUIContext g_ui;

// ============================================================================
// APP STATES
// ============================================================================
enum AppScreen {
    SCREEN_LOGIN,
    SCREEN_SIGNUP,
    SCREEN_MAIN
};
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
static float g_authErrorTimer = 0;
static std::string g_loggedUser;
static float g_fadeAnim = 0.0f;

// ============================================================================
// LICENSE / SUBSCRIPTION DATA
// ============================================================================
struct LicenseData {
    std::string key;
    std::string username;
    time_t      created;
    time_t      expiry;
    bool        active;
};

struct SubscriptionEntry {
    std::string name;
    std::string expiryText;
    uint32_t    accentColor;
    bool        active;
};

static std::vector<SubscriptionEntry> g_subscriptions;
static int  g_selectedSub = -1;

// Injection state
static bool g_injecting = false;
static bool g_injected  = false;
static std::string g_statusMsg;
static float g_statusTimer = 0.0f;

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
    // Simple hash for local storage (not cryptographic, but sufficient for local auth)
    uint32_t h = 0x811c9dc5;
    for (char c : pass) {
        h ^= (uint8_t)c;
        h *= 0x01000193;
    }
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
    u.expiry = u.created + 30 * 86400; // 30 day trial
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
            // Setup subscription entries
            g_subscriptions.clear();
            int daysLeft = (int)((u.expiry - time(nullptr)) / 86400);
            char expBuf[64];
            snprintf(expBuf, sizeof(expBuf), "Expires in %d days", daysLeft > 0 ? daysLeft : 0);
            g_subscriptions.push_back({
                "Counter-Strike 2",
                expBuf,
                ACE_COL32(220, 160, 40, 255),
                true
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
    if (!loadLib) { VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE); CloseHandle(hProcess); return false; }
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                         (LPTHREAD_START_ROUTINE)loadLib,
                                         remoteMem, 0, nullptr);
    if (!hThread) { VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE); CloseHandle(hProcess); return false; }
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
// DX11 SETUP
// ============================================================================
static bool CreateDeviceD3D() {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL fl;
    D3D_FEATURE_LEVEL fls[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        fls, 2, D3D11_SDK_VERSION,
        &sd, &g_swapChain, &g_device, &fl, &g_context);
    if (FAILED(hr)) return false;
    ID3D11Texture2D* bb = nullptr;
    g_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    if (bb) { g_device->CreateRenderTargetView(bb, nullptr, &g_rtv); bb->Release(); }
    return true;
}

static void CleanupDevice() {
    if (g_rtv)       { g_rtv->Release();       g_rtv = nullptr; }
    if (g_swapChain) { g_swapChain->Release();  g_swapChain = nullptr; }
    if (g_context)   { g_context->Release();    g_context = nullptr; }
    if (g_device)    { g_device->Release();     g_device = nullptr; }
}

static void CreateRenderTarget() {
    ID3D11Texture2D* bb = nullptr;
    g_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    if (bb) { g_device->CreateRenderTargetView(bb, nullptr, &g_rtv); bb->Release(); }
}

// ============================================================================
// WNDPROC
// ============================================================================
static bool g_dragging = false;
static POINT g_dragStart = {};

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ACEProcessInput(g_ui, (void*)hWnd, msg, (unsigned long long)wParam, (long long)lParam);
    switch (msg) {
    case WM_SIZE:
        if (g_device && wParam != SIZE_MINIMIZED) {
            if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
            g_swapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam),
                                        DXGI_FORMAT_UNKNOWN, 0);
            g_width = LOWORD(lParam);
            g_height = HIWORD(lParam);
            CreateRenderTarget();
            // Update window region for rounded corners
            HRGN rgn = CreateRoundRectRgn(0, 0, g_width + 1, g_height + 1,
                                           CORNER_RADIUS * 2, CORNER_RADIUS * 2);
            SetWindowRgn(hWnd, rgn, TRUE);
        }
        return 0;
    case WM_LBUTTONDOWN: {
        POINT pt = { (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam) };
        if (pt.y < 44 && pt.x < g_width - 80) {
            g_dragging = true;
            g_dragStart = pt;
            SetCapture(hWnd);
        }
        return 0;
    }
    case WM_MOUSEMOVE:
        if (g_dragging) {
            POINT pt;
            GetCursorPos(&pt);
            SetWindowPos(hWnd, nullptr,
                         pt.x - g_dragStart.x, pt.y - g_dragStart.y,
                         0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return 0;
    case WM_LBUTTONUP:
        if (g_dragging) { g_dragging = false; ReleaseCapture(); }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        g_running = false;
        return 0;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

// ============================================================================
// HELPER: Draw AC logo (white text + blue shadow/glow, standalone — no box)
// ============================================================================
static void DrawACLogo(ACEDrawList& dl, float cx, float cy, ACEFont* largeFont, ACEFont* smallFont) {
    const char* logoText = "AC";

    // Use large font for logo if available, otherwise fallback
    ACEFont* prevFont = dl.font;
    if (largeFont && largeFont->fontSize > 0) {
        dl.font = largeFont;
    }

    ACEVec2 ts = dl.font->CalcTextSize(logoText);

    // Multi-layer blue glow behind text
    for (int layer = 5; layer >= 1; layer--) {
        float spread = (float)layer * 1.8f;
        int alpha = (int)(30.0f * (1.0f - (float)layer / 6.0f));
        uint32_t glowCol = ACE_COL32(59, 130, 246, alpha);
        // Draw glow in 8 directions
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;
                dl.AddText(cx - ts.x * 0.5f + dx * spread,
                           cy - ts.y * 0.5f + dy * spread,
                           glowCol, logoText);
            }
        }
    }

    // Core blue shadow (tight, directly behind)
    dl.AddText(cx - ts.x * 0.5f + 0, cy - ts.y * 0.5f + 1, ACE_COL32(59, 130, 246, 120), logoText);
    dl.AddText(cx - ts.x * 0.5f + 1, cy - ts.y * 0.5f + 0, ACE_COL32(59, 130, 246, 80), logoText);
    dl.AddText(cx - ts.x * 0.5f - 1, cy - ts.y * 0.5f + 0, ACE_COL32(59, 130, 246, 80), logoText);
    dl.AddText(cx - ts.x * 0.5f + 0, cy - ts.y * 0.5f - 1, ACE_COL32(59, 130, 246, 60), logoText);

    // White text on top
    dl.AddText(cx - ts.x * 0.5f, cy - ts.y * 0.5f,
               ACE_COL32(255, 255, 255, 255), logoText);

    dl.font = prevFont;
}

// ============================================================================
// HELPER: Animated button with glow
// ============================================================================
static bool RenderButton(ACEUIContext& ctx, const char* label, float x, float y, float w, float h,
                          uint32_t baseColor, bool enabled = true) {
    uint32_t id = ctx.GetID(label);
    bool hovered = enabled && ctx.input.IsMouseInRect(x, y, x + w, y + h);
    bool clicked = hovered && ctx.input.mouseClicked[0];
    float anim = ctx.SmoothAnim(id, hovered ? 1.0f : 0.0f);

    uint8_t r = ACE_COL32_R(baseColor);
    uint8_t g = ACE_COL32_G(baseColor);
    uint8_t b = ACE_COL32_B(baseColor);

    // Outer glow on hover
    if (anim > 0.01f) {
        for (int i = 3; i > 0; i--) {
            uint32_t gc = ACE_COL32(r, g, b, (int)(12.0f * anim * (1.0f - (float)i / 4.0f)));
            ctx.drawList.AddRectFilled(x - i, y - i, x + w + i, y + h + i, gc, 10.0f + i);
        }
    }

    int alpha = enabled ? (int)(200 + 55 * anim) : 100;
    ctx.drawList.AddRectFilled(x, y, x + w, y + h,
                                ACE_COL32(r, g, b, alpha), 8.0f);

    // Subtle top highlight
    ctx.drawList.AddRectFilledMultiColor(
        x + 1, y + 1, x + w - 1, y + h * 0.4f,
        ACE_COL32(255, 255, 255, (int)(12 * anim)), ACE_COL32(255, 255, 255, (int)(12 * anim)),
        ACE_COL32(255, 255, 255, 0), ACE_COL32(255, 255, 255, 0));

    ACEVec2 ts = ctx.drawList.font->CalcTextSize(label);
    uint32_t textCol = enabled ? ACE_COL32(255, 255, 255, 255) : ACE_COL32(180, 180, 180, 180);
    ctx.drawList.AddText(x + (w - ts.x) * 0.5f, y + (h - ts.y) * 0.5f, textCol, label);

    return clicked && enabled;
}

// ============================================================================
// HELPER: Input field with label
// ============================================================================
static void RenderInputField(ACEUIContext& ctx, const char* label, char* buf, int bufSize,
                              float x, float y, float w, bool isPassword = false) {
    float labelH = 16.0f;
    float fieldH = 36.0f;

    // Label
    ctx.drawList.AddText(x, y, ACETheme::TextSecondary, label);

    // Field background
    float fy = y + labelH + 4;
    uint32_t fieldId = ctx.GetID(label);
    bool focused = ctx.focusedItem == fieldId;
    bool hovered = ctx.input.IsMouseInRect(x, fy, x + w, fy + fieldH);

    uint32_t borderCol = focused ? ACETheme::AccentBlue :
                         hovered ? ACE_COL32(60, 60, 80, 255) :
                                   ACE_COL32(40, 40, 55, 200);

    ctx.drawList.AddRectFilled(x, fy, x + w, fy + fieldH,
                                ACE_COL32(18, 18, 26, 255), 8.0f);
    ctx.drawList.AddRect(x, fy, x + w, fy + fieldH, borderCol, 8.0f, 1.0f);

    // Focus glow
    if (focused) {
        for (int i = 2; i > 0; i--) {
            ctx.drawList.AddRect(x - i, fy - i, x + w + i, fy + fieldH + i,
                                  ACE_COL32(59, 130, 246, (int)(20 * (3 - i))), 8.0f + i, 1.0f);
        }
    }

    if (hovered && ctx.input.mouseClicked[0]) {
        ctx.focusedItem = fieldId;
    }

    // Handle text input
    if (focused) {
        int len = (int)strlen(buf);
        for (int i = 0; i < ctx.input.inputCharCount && len < bufSize - 1; i++) {
            char c = ctx.input.inputChars[i];
            if (c >= 32 && c < 127) {
                buf[len++] = c;
                buf[len] = '\0';
            }
        }
        if (ctx.input.keyPressed[VK_BACK] && len > 0) {
            buf[len - 1] = '\0';
        }
    }

    // Display text or placeholder
    int slen = (int)strlen(buf);
    if (slen > 0) {
        if (isPassword) {
            std::string masked(slen, '*');
            ctx.drawList.AddText(x + 12, fy + (fieldH - ctx.drawList.font->lineHeight) * 0.5f,
                                  ACETheme::TextPrimary, masked.c_str());
        } else {
            ctx.drawList.AddText(x + 12, fy + (fieldH - ctx.drawList.font->lineHeight) * 0.5f,
                                  ACETheme::TextPrimary, buf);
        }
        // Cursor blink
        if (focused) {
            float blink = fmodf((float)ctx.time * 2.0f, 2.0f);
            if (blink < 1.0f) {
                const char* displayText = isPassword ? std::string(slen, '*').c_str() : buf;
                float tw = ctx.drawList.font->CalcTextSize(isPassword ? std::string(slen, '*').c_str() : buf).x;
                ctx.drawList.AddRectFilled(x + 12 + tw + 1, fy + 8, x + 13 + tw + 1, fy + fieldH - 8,
                                            ACETheme::TextPrimary, 0);
            }
        }
    } else {
        // Placeholder
        std::string ph = std::string("Enter ") + label;
        std::transform(ph.begin(), ph.end(), ph.begin(), ::tolower);
        ctx.drawList.AddText(x + 12, fy + (fieldH - ctx.drawList.font->lineHeight) * 0.5f,
                              ACE_COL32(60, 60, 80, 255), ph.c_str());
        // Cursor
        if (focused) {
            float blink = fmodf((float)ctx.time * 2.0f, 2.0f);
            if (blink < 1.0f) {
                ctx.drawList.AddRectFilled(x + 12, fy + 8, x + 13, fy + fieldH - 8,
                                            ACETheme::TextPrimary, 0);
            }
        }
    }
}

// ============================================================================
// WINDOW CONTROLS (minimize + close)
// ============================================================================
static void RenderWindowControls(ACEUIContext& ctx, float W) {
    float btnSize = 18.0f;
    float btnY = 14.0f;
    float minX = W - 60;
    float clsX = W - 32;

    // Minimize
    uint32_t minId = ctx.GetID("__min");
    bool minHov = ctx.input.IsMouseInRect(minX, btnY, minX + btnSize, btnY + btnSize);
    float minA = ctx.SmoothAnim(minId, minHov ? 1.0f : 0.0f);
    if (minA > 0.01f)
        ctx.drawList.AddCircleFilled(minX + btnSize * 0.5f, btnY + btnSize * 0.5f,
                                      btnSize * 0.5f + 2, ACE_COL32(255, 255, 255, (int)(12 * minA)));
    ctx.drawList.AddLine(minX + 3, btnY + btnSize * 0.5f,
                         minX + btnSize - 3, btnY + btnSize * 0.5f,
                         ACE_COL32(140, 140, 160, (int)(180 + 75 * minA)), 1.5f);
    if (minHov && ctx.input.mouseClicked[0])
        ShowWindow(g_hwnd, SW_MINIMIZE);

    // Close
    uint32_t clsId = ctx.GetID("__cls");
    bool clsHov = ctx.input.IsMouseInRect(clsX, btnY, clsX + btnSize, btnY + btnSize);
    float clsA = ctx.SmoothAnim(clsId, clsHov ? 1.0f : 0.0f);
    if (clsA > 0.01f)
        ctx.drawList.AddCircleFilled(clsX + btnSize * 0.5f, btnY + btnSize * 0.5f,
                                      btnSize * 0.5f + 2, ACE_COL32(255, 60, 60, (int)(30 * clsA)));
    float ccx = clsX + btnSize * 0.5f, ccy = btnY + btnSize * 0.5f;
    float cs = 4.0f;
    uint32_t xCol = ACE_COL32(140, 140, 160, (int)(180 + 75 * clsA));
    ctx.drawList.AddLine(ccx - cs, ccy - cs, ccx + cs, ccy + cs, xCol, 1.5f);
    ctx.drawList.AddLine(ccx + cs, ccy - cs, ccx - cs, ccy + cs, xCol, 1.5f);
    if (clsHov && ctx.input.mouseClicked[0])
        g_running = false;
}

// ============================================================================
// RENDER: LOGIN SCREEN
// ============================================================================
static void RenderLoginScreen(ACEUIContext& ctx, float W, float H) {
    // Background
    ctx.drawList.AddRectFilled(0, 0, W, H, ACE_COL32(12, 12, 16, 255), (float)CORNER_RADIUS);

    // Subtle top gradient
    ctx.drawList.AddRectFilledMultiColor(
        0, 0, W, 120,
        ACE_COL32(20, 25, 50, 80), ACE_COL32(20, 25, 50, 80),
        ACE_COL32(12, 12, 16, 0), ACE_COL32(12, 12, 16, 0));

    // Border
    ctx.drawList.AddRect(0, 0, W, H, ACE_COL32(40, 40, 60, 150), (float)CORNER_RADIUS, 1.0f);

    RenderWindowControls(ctx, W);

    // AC Logo — centered, standalone white text with blue glow
    DrawACLogo(ctx.drawList, W * 0.5f, 65.0f, &g_fontLarge, &g_font);

    // Tagline
    const char* tagline = "Skin Changer";
    ACEVec2 tts = ctx.drawList.font->CalcTextSize(tagline);
    ctx.drawList.AddText(W * 0.5f - tts.x * 0.5f, 90, ACE_COL32(100, 100, 130, 255), tagline);

    // Separator
    float sepY = 115;
    ctx.drawList.AddRectFilledMultiColor(
        W * 0.15f, sepY, W * 0.85f, sepY + 1,
        ACE_COL32(40, 40, 60, 0), ACE_COL32(59, 130, 246, 80),
        ACE_COL32(59, 130, 246, 80), ACE_COL32(40, 40, 60, 0));

    // Title
    const char* title = "Sign in to your account";
    ACEVec2 titleTs = ctx.drawList.font->CalcTextSize(title);
    ctx.drawList.AddText(W * 0.5f - titleTs.x * 0.5f, 132,
                          ACETheme::TextPrimary, title);

    // Form area
    float formX = 40;
    float formW = W - 80;
    float formY = 162;

    RenderInputField(ctx, "Username", g_loginUser, sizeof(g_loginUser),
                     formX, formY, formW, false);
    RenderInputField(ctx, "Password", g_loginPass, sizeof(g_loginPass),
                     formX, formY + 66, formW, true);

    // Error message
    if (g_authErrorTimer > 0) {
        g_authErrorTimer -= ctx.deltaTime;
        float ea = g_authErrorTimer > 0.5f ? 1.0f : g_authErrorTimer / 0.5f;
        ACEVec2 ets = ctx.drawList.font->CalcTextSize(g_authError.c_str());
        ctx.drawList.AddText(W * 0.5f - ets.x * 0.5f, formY + 140,
                              ACE_COL32(248, 113, 113, (int)(255 * ea)),
                              g_authError.c_str());
    }

    // Login button
    float btnY = formY + 160;
    if (RenderButton(ctx, "SIGN IN", formX, btnY, formW, 40,
                     ACETheme::AccentBlue, true)) {
        // Handle Enter key too
        if (DoLogin(g_loginUser, g_loginPass)) {
            g_screen = SCREEN_MAIN;
            g_fadeAnim = 0;
        }
    }

    // Handle Enter key for login
    if (ctx.input.keyPressed[VK_RETURN] && g_screen == SCREEN_LOGIN) {
        if (strlen(g_loginUser) > 0 && strlen(g_loginPass) > 0) {
            if (DoLogin(g_loginUser, g_loginPass)) {
                g_screen = SCREEN_MAIN;
                g_fadeAnim = 0;
            }
        }
    }

    // Sign up link
    const char* signupText = "Don't have an account?";
    ACEVec2 sts = ctx.drawList.font->CalcTextSize(signupText);
    ctx.drawList.AddText(W * 0.5f - sts.x * 0.5f, btnY + 54,
                          ACETheme::TextDim, signupText);

    const char* signupLink = "Create Account";
    ACEVec2 slts = ctx.drawList.font->CalcTextSize(signupLink);
    float slX = W * 0.5f - slts.x * 0.5f;
    float slY = btnY + 72;
    uint32_t slId = ctx.GetID("__signup_link");
    bool slHov = ctx.input.IsMouseInRect(slX, slY, slX + slts.x, slY + slts.y + 4);
    float slA = ctx.SmoothAnim(slId, slHov ? 1.0f : 0.0f);
    ctx.drawList.AddText(slX, slY,
                          ACE_COL32(59, 130, 246, (int)(180 + 75 * slA)), signupLink);
    if (slHov) {
        ctx.drawList.AddRectFilled(slX, slY + slts.y + 1, slX + slts.x, slY + slts.y + 2,
                                    ACE_COL32(59, 130, 246, (int)(120 * slA)), 0);
    }
    if (slHov && ctx.input.mouseClicked[0]) {
        g_screen = SCREEN_SIGNUP;
        g_authError.clear();
        g_authErrorTimer = 0;
        memset(g_signupUser, 0, sizeof(g_signupUser));
        memset(g_signupPass, 0, sizeof(g_signupPass));
        memset(g_signupPass2, 0, sizeof(g_signupPass2));
    }

    // Footer
    ctx.drawList.AddText(W * 0.5f - 40, H - 28, ACE_COL32(50, 50, 65, 255), "AC v3.0");
}

// ============================================================================
// RENDER: SIGNUP SCREEN
// ============================================================================
static void RenderSignupScreen(ACEUIContext& ctx, float W, float H) {
    ctx.drawList.AddRectFilled(0, 0, W, H, ACE_COL32(12, 12, 16, 255), (float)CORNER_RADIUS);
    ctx.drawList.AddRectFilledMultiColor(
        0, 0, W, 120,
        ACE_COL32(20, 25, 50, 80), ACE_COL32(20, 25, 50, 80),
        ACE_COL32(12, 12, 16, 0), ACE_COL32(12, 12, 16, 0));
    ctx.drawList.AddRect(0, 0, W, H, ACE_COL32(40, 40, 60, 150), (float)CORNER_RADIUS, 1.0f);

    RenderWindowControls(ctx, W);

    DrawACLogo(ctx.drawList, W * 0.5f, 65.0f, &g_fontLarge, &g_font);

    const char* tagline = "Skin Changer";
    ACEVec2 tts = ctx.drawList.font->CalcTextSize(tagline);
    ctx.drawList.AddText(W * 0.5f - tts.x * 0.5f, 90, ACE_COL32(100, 100, 130, 255), tagline);

    float sepY = 115;
    ctx.drawList.AddRectFilledMultiColor(
        W * 0.15f, sepY, W * 0.85f, sepY + 1,
        ACE_COL32(40, 40, 60, 0), ACE_COL32(59, 130, 246, 80),
        ACE_COL32(59, 130, 246, 80), ACE_COL32(40, 40, 60, 0));

    const char* title = "Create your account";
    ACEVec2 titleTs = ctx.drawList.font->CalcTextSize(title);
    ctx.drawList.AddText(W * 0.5f - titleTs.x * 0.5f, 132, ACETheme::TextPrimary, title);

    float formX = 40, formW = W - 80, formY = 162;

    RenderInputField(ctx, "Username", g_signupUser, sizeof(g_signupUser),
                     formX, formY, formW, false);
    RenderInputField(ctx, "Password", g_signupPass, sizeof(g_signupPass),
                     formX, formY + 66, formW, true);
    RenderInputField(ctx, "Confirm Password", g_signupPass2, sizeof(g_signupPass2),
                     formX, formY + 132, formW, true);

    if (g_authErrorTimer > 0) {
        g_authErrorTimer -= ctx.deltaTime;
        float ea = g_authErrorTimer > 0.5f ? 1.0f : g_authErrorTimer / 0.5f;
        ACEVec2 ets = ctx.drawList.font->CalcTextSize(g_authError.c_str());
        ctx.drawList.AddText(W * 0.5f - ets.x * 0.5f, formY + 200,
                              ACE_COL32(248, 113, 113, (int)(255 * ea)),
                              g_authError.c_str());
    }

    float btnY = formY + 218;
    if (RenderButton(ctx, "CREATE ACCOUNT", formX, btnY, formW, 40,
                     ACETheme::AccentBlue, true)) {
        if (strcmp(g_signupPass, g_signupPass2) != 0) {
            g_authError = "Passwords do not match";
            g_authErrorTimer = 3;
        } else if (DoSignup(g_signupUser, g_signupPass)) {
            // Auto-login after signup
            if (DoLogin(g_signupUser, g_signupPass)) {
                g_screen = SCREEN_MAIN;
                g_fadeAnim = 0;
            }
        }
    }

    if (ctx.input.keyPressed[VK_RETURN] && g_screen == SCREEN_SIGNUP) {
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
    const char* backText = "Already have an account?";
    ACEVec2 bts = ctx.drawList.font->CalcTextSize(backText);
    ctx.drawList.AddText(W * 0.5f - bts.x * 0.5f, btnY + 54, ACETheme::TextDim, backText);

    const char* backLink = "Sign In";
    ACEVec2 blts = ctx.drawList.font->CalcTextSize(backLink);
    float blX = W * 0.5f - blts.x * 0.5f;
    float blY = btnY + 72;
    uint32_t blId = ctx.GetID("__login_link");
    bool blHov = ctx.input.IsMouseInRect(blX, blY, blX + blts.x, blY + blts.y + 4);
    float blA = ctx.SmoothAnim(blId, blHov ? 1.0f : 0.0f);
    ctx.drawList.AddText(blX, blY,
                          ACE_COL32(59, 130, 246, (int)(180 + 75 * blA)), backLink);
    if (blHov && ctx.input.mouseClicked[0]) {
        g_screen = SCREEN_LOGIN;
        g_authError.clear();
        g_authErrorTimer = 0;
    }

    ctx.drawList.AddText(W * 0.5f - 40, H - 28, ACE_COL32(50, 50, 65, 255), "AC v3.0");
}

// ============================================================================
// RENDER: MAIN / SUBSCRIPTION SCREEN
// ============================================================================
static void RenderMainScreen(ACEUIContext& ctx, float W, float H) {
    // Fade-in animation
    g_fadeAnim += ctx.deltaTime * 3.0f;
    if (g_fadeAnim > 1.0f) g_fadeAnim = 1.0f;

    // Background
    ctx.drawList.AddRectFilled(0, 0, W, H, ACE_COL32(12, 12, 16, 255), (float)CORNER_RADIUS);
    ctx.drawList.AddRect(0, 0, W, H, ACE_COL32(40, 40, 60, 150), (float)CORNER_RADIUS, 1.0f);

    // ============================================================
    // SIDEBAR
    // ============================================================
    constexpr float SIDEBAR_W = 120.0f;

    // Sidebar bg (left portion — rounded on left only)
    ctx.drawList.AddRectFilled(0, 0, SIDEBAR_W, H, ACE_COL32(15, 15, 20, 255), (float)CORNER_RADIUS);
    ctx.drawList.AddRectFilled(SIDEBAR_W - CORNER_RADIUS, 0, SIDEBAR_W, H,
                               ACE_COL32(15, 15, 20, 255), 0); // Fill right edge

    // Subtle right border
    ctx.drawList.AddLine(SIDEBAR_W, CORNER_RADIUS, SIDEBAR_W, H - CORNER_RADIUS,
                         ACE_COL32(40, 40, 55, 80), 1.0f);

    // AC Logo in sidebar — white text, blue glow, no box
    DrawACLogo(ctx.drawList, SIDEBAR_W * 0.5f, 36.0f, &g_fontLarge, &g_font);

    // Separator under logo
    float sepY = 56;
    ctx.drawList.AddRectFilledMultiColor(
        12, sepY, SIDEBAR_W - 12, sepY + 1,
        ACE_COL32(40, 40, 60, 0), ACE_COL32(59, 130, 246, 50),
        ACE_COL32(59, 130, 246, 50), ACE_COL32(40, 40, 60, 0));

    // Navigation items
    const char* navItems[] = { "Website", "Support", "Market" };
    const char* navIcons[] = { ">", "?", "$" }; // Simple text icons
    float navY = sepY + 14;

    for (int i = 0; i < 3; i++) {
        float btnX = 8;
        float btnW = SIDEBAR_W - 16;
        float btnH = 32.0f;

        uint32_t id = ctx.GetID(navItems[i]);
        bool hovered = ctx.input.IsMouseInRect(btnX, navY, btnX + btnW, navY + btnH);
        float anim = ctx.SmoothAnim(id, hovered ? 1.0f : 0.0f);

        if (anim > 0.01f) {
            ctx.drawList.AddRectFilled(btnX, navY, btnX + btnW, navY + btnH,
                                       ACE_COL32(59, 130, 246, (int)(15 * anim)), 6.0f);
        }

        uint32_t tc = ACE_COL32(140, 140, 160, (int)(180 + 75 * anim));
        float textX = btnX + 14;
        ctx.drawList.AddText(textX, navY + (btnH - ctx.drawList.font->lineHeight) * 0.5f,
                              tc, navItems[i]);

        navY += btnH + 2;
    }

    // Bottom user area
    float userY = H - 56;
    ctx.drawList.AddRectFilledMultiColor(
        12, userY - 8, SIDEBAR_W - 12, userY - 7,
        ACE_COL32(40, 40, 55, 0), ACE_COL32(40, 40, 55, 80),
        ACE_COL32(40, 40, 55, 80), ACE_COL32(40, 40, 55, 0));

    // Avatar
    float avCX = 26, avCY = userY + 18;
    ctx.drawList.AddCircleFilled(avCX, avCY, 14.0f, ACE_COL32(30, 35, 55, 255));
    ctx.drawList.AddCircle(avCX, avCY, 14.0f, ACE_COL32(50, 55, 75, 255), 0, 1.0f);
    // User initial
    if (!g_loggedUser.empty()) {
        char initial[2] = { (char)toupper(g_loggedUser[0]), '\0' };
        ACEVec2 its = ctx.drawList.font->CalcTextSize(initial);
        ctx.drawList.AddText(avCX - its.x * 0.5f, avCY - its.y * 0.5f,
                              ACE_COL32(59, 130, 246, 255), initial);
    }

    // Username
    ctx.drawList.AddText(avCX + 20, avCY - 8,
                          ACETheme::TextPrimary, g_loggedUser.c_str());
    ctx.drawList.AddText(avCX + 20, avCY + 4,
                          ACE_COL32(74, 222, 128, 180), "Online");

    // ============================================================
    // MAIN CONTENT — right side
    // ============================================================
    RenderWindowControls(ctx, W);

    float contentX = SIDEBAR_W + 20;
    float contentY = 18;
    float contentW = W - SIDEBAR_W - 40;

    // Section title
    ctx.drawList.AddTextShadow(contentX, contentY,
                                ACE_COL32(240, 240, 250, 255), "Subscription");

    // Subtitle
    ctx.drawList.AddText(contentX, contentY + 20,
                          ACE_COL32(100, 100, 130, 255), "Available subscriptions");

    // Separator
    float cSepY = contentY + 40;
    ctx.drawList.AddRectFilledMultiColor(
        contentX, cSepY, contentX + contentW, cSepY + 1,
        ACE_COL32(59, 130, 246, 60), ACE_COL32(139, 92, 246, 60),
        ACE_COL32(139, 92, 246, 0), ACE_COL32(59, 130, 246, 0));

    // ============================================================
    // SUBSCRIPTION CARDS
    // ============================================================
    float cardY = cSepY + 16;
    float cardW = contentW;
    float cardH = 68.0f;
    float cardGap = 10.0f;

    for (int i = 0; i < (int)g_subscriptions.size(); i++) {
        auto& sub = g_subscriptions[i];
        float cX = contentX;
        float cY = cardY + i * (cardH + cardGap);

        uint32_t cardId = ctx.GetID(sub.name.c_str());
        bool hovered = ctx.input.IsMouseInRect(cX, cY, cX + cardW, cY + cardH);
        bool clicked = hovered && ctx.input.mouseClicked[0];
        float anim = ctx.SmoothAnim(cardId, hovered ? 1.0f : 0.0f);

        // Card bg
        uint32_t bg = g_selectedSub == i ? ACE_COL32(25, 28, 42, 255)
                    : hovered            ? ACE_COL32(22, 24, 35, 255)
                                         : ACE_COL32(18, 20, 28, 255);

        ctx.drawList.AddRectFilled(cX, cY, cX + cardW, cY + cardH, bg, 10.0f);

        // Border
        uint32_t border = g_selectedSub == i ? ACE_COL32(59, 130, 246, 120)
                        : hovered            ? ACE_COL32(59, 130, 246, (int)(40 + 60 * anim))
                                             : ACE_COL32(40, 40, 55, 120);
        ctx.drawList.AddRect(cX, cY, cX + cardW, cY + cardH, border, 10.0f, 1.0f);

        // Left accent bar on selected
        if (g_selectedSub == i) {
            ctx.drawList.AddRectFilled(cX + 1, cY + 8, cX + 4, cY + cardH - 8,
                                        ACETheme::AccentBlue, 2.0f);
        }

        // Selected glow
        if (g_selectedSub == i) {
            for (int gl = 4; gl > 0; gl--) {
                uint32_t gc = ACE_COL32(59, 130, 246, (int)(8.0f * (1.0f - (float)gl / 5.0f)));
                ctx.drawList.AddRect(cX - gl, cY - gl, cX + cardW + gl, cY + cardH + gl,
                                     gc, 10.0f + gl, 1.0f);
            }
        }

        // Game icon
        float iconSize = 40.0f;
        float iconR = 10.0f;
        float iconX = cX + 14;
        float iconY = cY + (cardH - iconSize) * 0.5f;
        ctx.drawList.AddRectFilled(iconX, iconY, iconX + iconSize, iconY + iconSize,
                                   sub.accentColor, iconR);
        ctx.drawList.AddRect(iconX, iconY, iconX + iconSize, iconY + iconSize,
                             ACE_COL32(255, 255, 255, 20), iconR, 1.0f);
        // CS2 text on icon
        ACEVec2 icts = ctx.drawList.font->CalcTextSize("CS");
        ctx.drawList.AddText(iconX + (iconSize - icts.x) * 0.5f,
                             iconY + (iconSize - icts.y) * 0.5f,
                             ACE_COL32(255, 255, 255, 220), "CS");

        // Game name
        ctx.drawList.AddText(cX + 14 + iconSize + 14, cY + 15,
                              ACETheme::TextPrimary, sub.name.c_str());

        // Expiry text
        ctx.drawList.AddText(cX + 14 + iconSize + 14, cY + 36,
                              ACE_COL32(100, 100, 130, 255), sub.expiryText.c_str());

        // Active badge
        if (sub.active) {
            const char* badge = "ACTIVE";
            ACEVec2 bts = ctx.drawList.font->CalcTextSize(badge);
            float bx = cX + cardW - bts.x - 20;
            float by = cY + (cardH - 22) * 0.5f;
            ctx.drawList.AddRectFilled(bx - 6, by - 2, bx + bts.x + 6, by + bts.y + 2,
                                        ACE_COL32(74, 222, 128, 30), 4.0f);
            ctx.drawList.AddText(bx, by, ACE_COL32(74, 222, 128, 255), badge);
        }

        if (clicked) g_selectedSub = i;
    }

    // ============================================================
    // LAUNCH BUTTON
    // ============================================================
    float launchY = cardY + (int)g_subscriptions.size() * (cardH + cardGap) + 16;

    if (g_selectedSub >= 0) {
        uint32_t baseCol;
        const char* launchText;

        if (g_injected) {
            baseCol = ACE_COL32(74, 222, 128, 255);
            launchText = "LAUNCHED";
        } else if (g_injecting) {
            baseCol = ACE_COL32(250, 204, 21, 255);
            launchText = "INJECTING...";
        } else {
            baseCol = ACETheme::AccentBlue;
            launchText = "LAUNCH";
        }

        if (RenderButton(ctx, launchText, contentX, launchY, cardW, 42,
                          baseCol, !g_injecting && !g_injected)) {
            DoInject();
        }
    } else {
        // Disabled hint
        const char* hint = "Select a subscription to continue";
        ACEVec2 hts = ctx.drawList.font->CalcTextSize(hint);
        ctx.drawList.AddText(contentX + (contentW - hts.x) * 0.5f, launchY + 10,
                              ACE_COL32(60, 60, 80, 255), hint);
    }

    // ============================================================
    // STATUS TOAST
    // ============================================================
    if (g_statusTimer > 0) {
        g_statusTimer -= ctx.deltaTime;
        float a = g_statusTimer > 0.5f ? 1.0f : g_statusTimer / 0.5f;
        if (a > 0.01f) {
            ctx.overlayDrawList.font = ctx.drawList.font;
            float nW = 300, nH = 38;
            float nX = contentX + (contentW - nW) * 0.5f;
            float nY = H - 50;

            uint32_t bg = g_injected
                ? ACE_COL32(74, 222, 128, (int)(220 * a))
                : ACE_COL32(248, 113, 113, (int)(220 * a));
            ctx.overlayDrawList.AddRectFilled(nX, nY, nX + nW, nY + nH, bg, 10.0f);

            uint32_t tc = ACE_COL32(255, 255, 255, (int)(255 * a));
            ACEVec2 nts = ctx.overlayDrawList.font->CalcTextSize(g_statusMsg.c_str());
            ctx.overlayDrawList.AddText(nX + (nW - nts.x) * 0.5f,
                                         nY + (nH - nts.y) * 0.5f,
                                         tc, g_statusMsg.c_str());
        }
    }

    // Logout button (bottom right)
    const char* logoutText = "Logout";
    ACEVec2 lots = ctx.drawList.font->CalcTextSize(logoutText);
    float loX = W - lots.x - 20;
    float loY = H - 30;
    uint32_t loId = ctx.GetID("__logout");
    bool loHov = ctx.input.IsMouseInRect(loX, loY, loX + lots.x, loY + lots.y + 4);
    float loA = ctx.SmoothAnim(loId, loHov ? 1.0f : 0.0f);
    ctx.drawList.AddText(loX, loY,
                          ACE_COL32(120, 120, 140, (int)(150 + 100 * loA)), logoutText);
    if (loHov && ctx.input.mouseClicked[0]) {
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
// RENDER DISPATCH
// ============================================================================
static void RenderUI() {
    auto& ctx = g_ui;
    float W = (float)g_width;
    float H = (float)g_height;

    switch (g_screen) {
    case SCREEN_LOGIN:  RenderLoginScreen(ctx, W, H);  break;
    case SCREEN_SIGNUP: RenderSignupScreen(ctx, W, H); break;
    case SCREEN_MAIN:   RenderMainScreen(ctx, W, H);   break;
    }
}

// ============================================================================
// ENTRY POINT
// ============================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    srand((unsigned)time(nullptr));

    // Load existing user database
    LoadUsers();

    // Create borderless window
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "ACLoader";
    RegisterClassExA(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    g_hwnd = CreateWindowExA(
        0,
        wc.lpszClassName, "AC Loader",
        WS_POPUP | WS_VISIBLE,
        (screenW - g_width) / 2, (screenH - g_height) / 2,
        g_width, g_height,
        nullptr, nullptr, hInstance, nullptr);

    // Apply rounded window region for truly rounded corners
    HRGN rgn = CreateRoundRectRgn(0, 0, g_width + 1, g_height + 1,
                                   CORNER_RADIUS * 2, CORNER_RADIUS * 2);
    SetWindowRgn(g_hwnd, rgn, TRUE);

    if (!CreateDeviceD3D()) {
        CleanupDevice();
        UnregisterClassA(wc.lpszClassName, hInstance);
        return 1;
    }

    ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hwnd);

    // Init ACE renderer
    if (!g_renderer.Initialize(g_device, g_context)) {
        MessageBoxA(g_hwnd, "Failed to initialize ACE renderer", "Error", MB_OK);
        CleanupDevice();
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
        if (g_font.LoadFromFile(p, 14.0f)) { fontLoaded = true; break; }
    }
    if (!fontLoaded) {
        MessageBoxA(g_hwnd, "Failed to load system font", "Error", MB_OK);
        CleanupDevice(); return 1;
    }

    // Large font for AC logo
    const char* boldPaths[] = {
        "C:\\Windows\\Fonts\\segoeuib.ttf",   // Segoe UI Bold
        "C:\\Windows\\Fonts\\arialbd.ttf",    // Arial Bold
        "C:\\Windows\\Fonts\\tahomabd.ttf",   // Tahoma Bold
        "C:\\Windows\\Fonts\\segoeui.ttf",    // Fallback to regular
        "C:\\Windows\\Fonts\\arial.ttf",
    };
    bool largeFontLoaded = false;
    for (auto* p : boldPaths) {
        if (g_fontLarge.LoadFromFile(p, 28.0f)) { largeFontLoaded = true; break; }
    }
    // If large font fails, we fall back to regular font in DrawACLogo

    g_renderer.CreateFontTexture(g_font);
    if (largeFontLoaded) {
        // We need a second texture for the large font — share atlas won't work
        // For now, use the same atlas approach. The large font will use the regular font's atlas
        // This requires the logo to use the regular font. We'll handle this in DrawACLogo.
        g_renderer.CreateFontTexture(g_fontLarge);
    }

    g_ui.drawList.font = &g_font;
    g_ui.overlayDrawList.font = &g_font;

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
        float dt = (float)(now.QuadPart - last.QuadPart) / (float)freq.QuadPart;
        if (dt > 0.1f) dt = 0.016f;
        last = now;

        RECT rc;
        GetClientRect(g_hwnd, &rc);
        float w = (float)(rc.right - rc.left);
        float h = (float)(rc.bottom - rc.top);

        g_ui.NewFrame(w, h, dt);
        g_ui.wnd.x = 0; g_ui.wnd.y = 0;
        g_ui.wnd.w = w; g_ui.wnd.h = h;
        g_ui.wnd.cursorX = 0; g_ui.wnd.cursorY = 0;
        g_ui.wnd.contentW = w;

        RenderUI();

        g_ui.EndFrame();

        float clearColor[4] = { 0.047f, 0.047f, 0.063f, 1.0f };
        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_context->ClearRenderTargetView(g_rtv, clearColor);

        g_renderer.RenderDrawList(g_ui.drawList, w, h);
        if (!g_ui.overlayDrawList.vtxBuffer.empty())
            g_renderer.RenderDrawList(g_ui.overlayDrawList, w, h);

        g_swapChain->Present(1, 0);
    }

    g_renderer.Shutdown();
    g_font.Free();
    g_fontLarge.Free();
    CleanupDevice();
    DestroyWindow(g_hwnd);
    UnregisterClassA(wc.lpszClassName, hInstance);

    return 0;
}
