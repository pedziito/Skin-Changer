/*
 * AC Loader — Subscription-Based GUI Launcher
 * Clean dark UI with ACE custom rendering engine (DX11).
 * Matches the AC theme across loader, skin changer, and admin tools.
 * "AC" branding — smooth rounded corners, no pixelated edges.
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

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

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
static int                      g_width = 470;
static int                      g_height = 420;

static ACEFont      g_font;
static ACERenderer  g_renderer;
static ACEUIContext g_ui;

// ============================================================================
// SUBSCRIPTION DATA
// ============================================================================
struct SubscriptionEntry {
    std::string name;
    std::string expiryText;
    uint32_t    iconColor;
    bool        active;
};

static std::vector<SubscriptionEntry> g_subscriptions;
static std::string g_username = "User";
static int  g_selectedSub = -1;

// Injection state
static bool g_injecting = false;
static bool g_injected = false;
static std::string g_statusMsg;
static float g_statusTimer = 0.0f;

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
    if (!remoteMem) {
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, remoteMem, dllPath, pathLen, nullptr)) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    LPVOID loadLibAddr = (LPVOID)GetProcAddress(
        GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!loadLibAddr) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                         (LPTHREAD_START_ROUTINE)loadLibAddr,
                                         remoteMem, 0, nullptr);
    if (!hThread) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
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
        g_statusTimer = 4.0f;
        g_injecting = false;
        return;
    }

    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string dllPath = exePath;
    size_t lastSlash = dllPath.find_last_of('\\');
    if (lastSlash != std::string::npos)
        dllPath = dllPath.substr(0, lastSlash + 1);
    dllPath += "skin_changer.dll";

    if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        g_statusMsg = "DLL not found!";
        g_statusTimer = 4.0f;
        g_injecting = false;
        return;
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
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
    if (g_swapChain) { g_swapChain->Release(); g_swapChain = nullptr; }
    if (g_context) { g_context->Release(); g_context = nullptr; }
    if (g_device) { g_device->Release(); g_device = nullptr; }
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
        }
        return 0;
    case WM_LBUTTONDOWN: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        // Drag from top area (not on buttons)
        if (pt.y < 40 && pt.x < g_width - 70) {
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
            RECT wr;
            GetWindowRect(hWnd, &wr);
            SetWindowPos(hWnd, nullptr,
                         pt.x - g_dragStart.x,
                         pt.y - g_dragStart.y,
                         0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return 0;
    case WM_LBUTTONUP:
        if (g_dragging) {
            g_dragging = false;
            ReleaseCapture();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        g_running = false;
        return 0;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

// ============================================================================
// UI RENDERING
// ============================================================================
static void RenderUI() {
    auto& ctx = g_ui;
    float W = (float)g_width;
    float H = (float)g_height;

    // ---- Full window background with rounded corners ----
    ctx.drawList.AddRectFilled(0, 0, W, H, ACE_COL32(13, 13, 18, 255), 12.0f);
    ctx.drawList.AddRect(0, 0, W, H, ACE_COL32(35, 35, 50, 200), 12.0f, 1.0f);

    // ============================================================
    // LAYOUT
    // ============================================================
    constexpr float SIDEBAR_W = 110.0f;
    constexpr float PAD       = 16.0f;

    // ============================================================
    // SIDEBAR
    // ============================================================
    ctx.drawList.AddRectFilled(0, 0, SIDEBAR_W, H,
                               ACE_COL32(16, 16, 22, 255), 12.0f);

    // Clip sidebar right corners (fill overlap)
    ctx.drawList.AddRectFilled(SIDEBAR_W - 12, 0, SIDEBAR_W, H,
                               ACE_COL32(16, 16, 22, 255), 0);

    // Right edge
    ctx.drawList.AddLine(SIDEBAR_W, 12, SIDEBAR_W, H - 12,
                         ACE_COL32(35, 35, 50, 100), 1.0f);

    // ---- AC Brand Logo ----
    float logoSize = 44.0f;
    float logoX = (SIDEBAR_W - logoSize) * 0.5f;
    float logoY = 22.0f;
    float logoR = logoSize * 0.22f;

    // Logo background
    ctx.drawList.AddRectFilled(logoX, logoY, logoX + logoSize, logoY + logoSize,
                               ACE_COL32(22, 35, 70, 255), logoR);
    ctx.drawList.AddRect(logoX, logoY, logoX + logoSize, logoY + logoSize,
                         ACE_COL32(59, 130, 246, 60), logoR, 1.0f);

    // "AC" text centered
    const char* logoText = "AC";
    ACEVec2 logoTs = ctx.drawList.font->CalcTextSize(logoText);
    ctx.drawList.AddText(logoX + (logoSize - logoTs.x) * 0.5f,
                         logoY + (logoSize - logoTs.y) * 0.5f,
                         ACE_COL32(70, 150, 255, 255), logoText);

    // ---- Sidebar Navigation ----
    const char* navItems[] = { "Website", "Support", "Market" };
    float navY = logoY + logoSize + 28;

    for (int i = 0; i < 3; i++) {
        float btnX = 8;
        float btnW = SIDEBAR_W - 16;
        float btnH = 30.0f;

        uint32_t id = ctx.GetID(navItems[i]);
        bool hovered = ctx.input.IsMouseInRect(btnX, navY, btnX + btnW, navY + btnH);
        float anim = ctx.SmoothAnim(id, hovered ? 1.0f : 0.0f);

        if (anim > 0.01f) {
            uint32_t bg = ACE_COL32(59, 130, 246, (int)(18 * anim));
            ctx.drawList.AddRectFilled(btnX, navY, btnX + btnW, navY + btnH,
                                       bg, 6.0f);
        }

        uint32_t textCol = hovered ? ACETheme::TextPrimary : ACETheme::TextSecondary;
        ACEVec2 ts = ctx.drawList.font->CalcTextSize(navItems[i]);
        ctx.drawList.AddText(btnX + (btnW - ts.x) * 0.5f,
                             navY + (btnH - ts.y) * 0.5f,
                             textCol, navItems[i]);

        navY += btnH + 4;
    }

    // ---- Bottom user area ----
    float userY = H - 50;
    ctx.drawList.AddRectFilledMultiColor(
        12, userY - 6, SIDEBAR_W - 12, userY - 5,
        ACE_COL32(35, 35, 50, 0), ACE_COL32(35, 35, 50, 100),
        ACE_COL32(35, 35, 50, 100), ACE_COL32(35, 35, 50, 0));

    // Avatar circle
    float avatarCX = 24, avatarCY = userY + 16;
    ctx.drawList.AddCircleFilled(avatarCX, avatarCY, 14.0f,
                                 ACE_COL32(40, 40, 55, 255), 48);
    ctx.drawList.AddCircle(avatarCX, avatarCY, 14.0f,
                           ACE_COL32(55, 55, 75, 255), 48, 1.0f);

    // Username
    ctx.drawList.AddText(avatarCX + 22, avatarCY - 7,
                         ACETheme::TextPrimary, g_username.c_str());

    // ============================================================
    // MAIN CONTENT AREA
    // ============================================================
    float contentX = SIDEBAR_W + PAD;
    float contentY = PAD;
    float contentW = W - SIDEBAR_W - PAD * 2;

    // ---- Window controls (minimize + close) ----
    {
        float btnSize = 20.0f;
        float btnY = 12.0f;
        float minX = W - 58;
        float clsX = W - 30;

        // Minimize
        uint32_t minId = ctx.GetID("__min");
        bool minHov = ctx.input.IsMouseInRect(minX, btnY, minX + btnSize, btnY + btnSize);
        float minA = ctx.SmoothAnim(minId, minHov ? 1.0f : 0.0f);
        if (minA > 0.01f)
            ctx.drawList.AddRectFilled(minX - 3, btnY - 3, minX + btnSize + 3, btnY + btnSize + 3,
                                       ACE_COL32(255, 255, 255, (int)(12 * minA)), 4.0f);
        ctx.drawList.AddLine(minX + 4, btnY + btnSize * 0.5f,
                             minX + btnSize - 4, btnY + btnSize * 0.5f,
                             ACE_COL32(150, 150, 170, 255), 1.5f);
        if (minHov && ctx.input.mouseClicked[0])
            ShowWindow(g_hwnd, SW_MINIMIZE);

        // Close
        uint32_t clsId = ctx.GetID("__cls");
        bool clsHov = ctx.input.IsMouseInRect(clsX, btnY, clsX + btnSize, btnY + btnSize);
        float clsA = ctx.SmoothAnim(clsId, clsHov ? 1.0f : 0.0f);
        if (clsA > 0.01f)
            ctx.drawList.AddRectFilled(clsX - 3, btnY - 3, clsX + btnSize + 3, btnY + btnSize + 3,
                                       ACE_COL32(255, 60, 60, (int)(35 * clsA)), 4.0f);
        float ccx = clsX + btnSize * 0.5f, ccy = btnY + btnSize * 0.5f;
        float cs = 4.5f;
        ctx.drawList.AddLine(ccx - cs, ccy - cs, ccx + cs, ccy + cs,
                             ACE_COL32(150, 150, 170, 255), 1.5f);
        ctx.drawList.AddLine(ccx + cs, ccy - cs, ccx - cs, ccy + cs,
                             ACE_COL32(150, 150, 170, 255), 1.5f);
        if (clsHov && ctx.input.mouseClicked[0])
            g_running = false;
    }

    // ---- Title ----
    ctx.drawList.AddText(contentX, contentY + 2,
                         ACE_COL32(235, 235, 245, 255), "Subscription");

    // ---- Subtitle ----
    ctx.drawList.AddText(contentX, contentY + 22,
                         ACE_COL32(59, 130, 246, 180), "Available subscriptions");

    // ============================================================
    // SUBSCRIPTION CARDS
    // ============================================================
    float cardY = contentY + 54;
    float cardW = contentW;
    float cardH = 64.0f;
    float cardGap = 8.0f;

    for (int i = 0; i < (int)g_subscriptions.size(); i++) {
        auto& sub = g_subscriptions[i];

        float cX = contentX;
        float cY = cardY + i * (cardH + cardGap);

        uint32_t cardId = ctx.GetID(sub.name.c_str());
        bool hovered = ctx.input.IsMouseInRect(cX, cY, cX + cardW, cY + cardH);
        bool clicked = hovered && ctx.input.mouseClicked[0];
        float anim = ctx.SmoothAnim(cardId, hovered ? 1.0f : 0.0f);

        // Card background
        uint32_t cardBg = ACE_COL32(22, 22, 30, 255);
        if (hovered) cardBg = ACE_COL32(26, 26, 36, 255);
        if (g_selectedSub == i) cardBg = ACE_COL32(28, 30, 42, 255);

        ctx.drawList.AddRectFilled(cX, cY, cX + cardW, cY + cardH, cardBg, 10.0f);

        // Card border
        uint32_t borderCol = ACE_COL32(40, 40, 55, 160);
        if (hovered) borderCol = ACE_COL32(59, 130, 246, (int)(50 + 80 * anim));
        if (g_selectedSub == i) borderCol = ACE_COL32(59, 130, 246, 140);
        ctx.drawList.AddRect(cX, cY, cX + cardW, cY + cardH, borderCol, 10.0f, 1.0f);

        // Game name
        ctx.drawList.AddText(cX + 14, cY + 14,
                             ACE_COL32(230, 230, 240, 255), sub.name.c_str());

        // Expiry
        ctx.drawList.AddText(cX + 14, cY + 36,
                             ACE_COL32(120, 120, 140, 255), sub.expiryText.c_str());

        // Game icon — smooth rounded square
        float iconSize = 36.0f;
        float iconR = 8.0f;
        float iconX = cX + cardW - iconSize - 14;
        float iconY = cY + (cardH - iconSize) * 0.5f;
        ctx.drawList.AddRectFilled(iconX, iconY, iconX + iconSize, iconY + iconSize,
                                   sub.iconColor, iconR);
        ctx.drawList.AddRect(iconX, iconY, iconX + iconSize, iconY + iconSize,
                             ACE_COL32(255, 255, 255, 18), iconR, 1.0f);

        // Selected glow
        if (g_selectedSub == i) {
            for (int g = 3; g > 0; g--) {
                uint32_t gc = ACE_COL32(59, 130, 246,
                                        (int)(8.0f * (1.0f - (float)g / 3.0f)));
                ctx.drawList.AddRect(cX - g, cY - g, cX + cardW + g, cY + cardH + g,
                                     gc, 10.0f + g, 1.5f);
            }
        }

        if (clicked) g_selectedSub = i;
    }

    // ---- Launch button ----
    float launchY = cardY + (int)g_subscriptions.size() * (cardH + cardGap) + 16;
    float launchH = 38.0f;

    if (g_selectedSub >= 0) {
        uint32_t launchId = ctx.GetID("__launch");
        bool launchHov = ctx.input.IsMouseInRect(contentX, launchY,
                                                  contentX + cardW, launchY + launchH);
        float launchA = ctx.SmoothAnim(launchId, launchHov ? 1.0f : 0.0f);

        uint32_t launchBg;
        const char* launchText;

        if (g_injected) {
            launchBg = ACE_COL32(74, 222, 128, (int)(180 + 75 * launchA));
            launchText = "LAUNCHED";
        } else if (g_injecting) {
            launchBg = ACE_COL32(250, 204, 21, 180);
            launchText = "INJECTING...";
        } else {
            launchBg = ACE_COL32(59, 130, 246, (int)(200 + 55 * launchA));
            launchText = "LAUNCH";
        }

        ctx.drawList.AddRectFilled(contentX, launchY, contentX + cardW, launchY + launchH,
                                   launchBg, 8.0f);

        if (launchA > 0.01f && !g_injected) {
            ctx.drawList.AddRectFilled(contentX - 2, launchY - 2,
                                       contentX + cardW + 2, launchY + launchH + 2,
                                       ACE_COL32(59, 130, 246, (int)(18 * launchA)), 10.0f);
        }

        ACEVec2 ts = ctx.drawList.font->CalcTextSize(launchText);
        ctx.drawList.AddText(contentX + (cardW - ts.x) * 0.5f,
                             launchY + (launchH - ts.y) * 0.5f,
                             ACE_COL32(255, 255, 255, 255), launchText);

        if (launchHov && ctx.input.mouseClicked[0] && !g_injecting && !g_injected)
            DoInject();
    }

    // ---- Status toast ----
    if (g_statusTimer > 0) {
        g_statusTimer -= ctx.deltaTime;
        float a = g_statusTimer > 0.5f ? 1.0f : g_statusTimer / 0.5f;
        if (a > 0.01f) {
            ctx.overlayDrawList.font = ctx.drawList.font;
            float nW = 280, nH = 34;
            float nX = contentX + (contentW - nW) * 0.5f;
            float nY = H - 50;

            uint32_t bg = g_injected
                ? ACE_COL32(74, 222, 128, (int)(200 * a))
                : ACE_COL32(248, 113, 113, (int)(200 * a));
            ctx.overlayDrawList.AddRectFilled(nX, nY, nX + nW, nY + nH, bg, 8.0f);

            uint32_t tc = ACE_COL32(255, 255, 255, (int)(255 * a));
            ACEVec2 nts = ctx.overlayDrawList.font->CalcTextSize(g_statusMsg.c_str());
            ctx.overlayDrawList.AddText(nX + (nW - nts.x) * 0.5f,
                                         nY + (nH - nts.y) * 0.5f,
                                         tc, g_statusMsg.c_str());
        }
    }
}

// ============================================================================
// ENTRY POINT
// ============================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    srand((unsigned)time(nullptr));

    // Setup subscriptions
    g_subscriptions.push_back({
        "Counter-Strike: Global Offensive",
        "Expires in 16 days",
        ACE_COL32(220, 160, 40, 255),   // Orange/gold icon
        true
    });

    // Get Windows username
    char* envUser = nullptr;
    size_t envLen = 0;
    if (_dupenv_s(&envUser, &envLen, "USERNAME") == 0 && envUser) {
        g_username = envUser;
        free(envUser);
    }

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
        WS_EX_LAYERED,
        wc.lpszClassName, "AC Loader",
        WS_POPUP,
        (screenW - g_width) / 2, (screenH - g_height) / 2,
        g_width, g_height,
        nullptr, nullptr, hInstance, nullptr);

    SetLayeredWindowAttributes(g_hwnd, 0, 255, LWA_ALPHA);

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
        CleanupDevice();
        return 1;
    }
    g_renderer.CreateFontTexture(g_font);
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

        float clearColor[4] = { 0.05f, 0.05f, 0.07f, 1.0f };
        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_context->ClearRenderTargetView(g_rtv, clearColor);

        g_renderer.RenderDrawList(g_ui.drawList, w, h);
        if (!g_ui.overlayDrawList.vtxBuffer.empty())
            g_renderer.RenderDrawList(g_ui.overlayDrawList, w, h);

        g_swapChain->Present(1, 0);
    }

    g_renderer.Shutdown();
    g_font.Free();
    CleanupDevice();
    DestroyWindow(g_hwnd);
    UnregisterClassA(wc.lpszClassName, hInstance);

    return 0;
}
