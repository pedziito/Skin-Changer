/*
 * AC License Generator — Standalone GUI Application
 * NEVERLOSE-themed admin tool for generating, validating, and revoking licenses.
 * Uses ACE custom rendering engine (DX11) — same visual style as the skin changer.
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

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
static int                      g_width = 900;
static int                      g_height = 620;

static ACEFont      g_font;
static ACERenderer  g_renderer;
static ACEUIContext g_ui;

// ============================================================================
// LICENSE SYSTEM
// ============================================================================
struct LicenseEntry {
    std::string key;
    std::string username;
    std::string created;
    std::string expiry;
    bool active;
};

static std::vector<LicenseEntry> g_licenses;
static char g_username[128] = "";
static char g_filename[128] = "license.key";
static char g_validateFile[128] = "";
static int  g_expiryDays = 30;
static int  g_currentPage = 0;  // 0=Generate, 1=Validate, 2=Manage, 3=About

// Notification
static std::string g_notification;
static float g_notifTimer = 0;
static uint32_t g_notifColor = 0;

static void ShowNotif(const char* msg, uint32_t col = ACETheme::AccentGreen) {
    g_notification = msg;
    g_notifTimer = 3.0f;
    g_notifColor = col;
}

static std::string GenerateKey() {
    time_t now = time(nullptr);
    struct tm ti;
    localtime_s(&ti, &now);
    char key[64];
    snprintf(key, sizeof(key), "CS2-%04d-%08X-%02d-%08X",
             ti.tm_year + 1900, (unsigned)(rand() ^ (rand() << 16)),
             ti.tm_mon + 1, (unsigned)(rand() ^ (rand() << 16)));
    return key;
}

static std::string TimeStr(time_t t) {
    struct tm ti;
    localtime_s(&ti, &t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &ti);
    return buf;
}

static bool CreateLicense(const char* filename, const char* username, int days) {
    std::string key = GenerateKey();
    time_t now = time(nullptr);
    time_t exp = now + (time_t)days * 86400;

    std::ofstream f(filename);
    if (!f.is_open()) return false;

    f << key << "\n";
    f << "USERNAME=" << username << "\n";
    f << "CREATED=" << TimeStr(now) << "\n";
    f << "EXPIRY=" << TimeStr(exp) << "\n";
    f << "ACTIVE=true\n";
    f.close();

    LicenseEntry e;
    e.key = key;
    e.username = username;
    e.created = TimeStr(now);
    e.expiry = TimeStr(exp);
    e.active = true;
    g_licenses.push_back(e);

    return true;
}

static bool ValidateLicenseFile(const char* filename, LicenseEntry& out) {
    std::ifstream f(filename);
    if (!f.is_open()) return false;

    out = {};
    std::string line;
    while (std::getline(f, line)) {
        if (line.find("CS2-") == 0) out.key = line;
        else if (line.find("USERNAME=") == 0) out.username = line.substr(9);
        else if (line.find("CREATED=") == 0) out.created = line.substr(8);
        else if (line.find("EXPIRY=") == 0) out.expiry = line.substr(7);
        else if (line.find("ACTIVE=true") != std::string::npos) out.active = true;
        else if (line.find("ACTIVE=false") != std::string::npos) out.active = false;
    }
    f.close();
    return !out.key.empty();
}

static bool RevokeLicenseFile(const char* filename) {
    std::ifstream inf(filename);
    if (!inf.is_open()) return false;

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(inf, line)) {
        if (line.find("ACTIVE=") != std::string::npos)
            lines.push_back("ACTIVE=false");
        else
            lines.push_back(line);
    }
    inf.close();

    std::ofstream outf(filename);
    for (auto& l : lines) outf << l << "\n";
    outf.close();
    return true;
}

// ============================================================================
// DX11 SETUP
// ============================================================================
static bool CreateDeviceD3D() {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
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
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ACEProcessInput(g_ui, (void*)hWnd, msg, (unsigned long long)wParam, (long long)lParam);

    switch (msg) {
    case WM_SIZE:
        if (g_device && wParam != SIZE_MINIMIZED) {
            if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
            g_swapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            g_width = LOWORD(lParam);
            g_height = HIWORD(lParam);
            CreateRenderTarget();
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

    // Full-screen background
    ctx.drawList.AddRectFilled(0, 0, W, H, ACETheme::WindowBg, 0);

    // Window chrome
    float mx = 20, my = 20;
    float mw = W - 40, mh = H - 40;

    ctx.drawList.AddRectFilled(mx, my, mx + mw, my + mh, ACE_COL32(15, 15, 20, 255), 12.0f);
    ctx.drawList.AddRect(mx, my, mx + mw, my + mh, ACETheme::Border, 12.0f, 1.0f);

    // Title bar
    float titleH = 44;
    ctx.drawList.AddRectFilledMultiColor(
        mx, my, mx + mw, my + titleH,
        ACE_COL32(20, 20, 30, 255), ACE_COL32(20, 20, 30, 255),
        ACE_COL32(15, 15, 22, 255), ACE_COL32(15, 15, 22, 255));
    ctx.drawList.AddTextShadow(mx + 18, my + 13, ACETheme::AccentBlue, "AC LICENSE GENERATOR");
    ctx.drawList.AddText(mx + mw - 100, my + 15, ACETheme::TextDim, "v3.0 Admin");
    ctx.drawList.AddRectFilledMultiColor(
        mx, my + titleH - 2, mx + mw, my + titleH,
        ACETheme::AccentBlue, ACETheme::AccentPurple,
        ACETheme::AccentPurple, ACETheme::AccentBlue);

    // Sidebar
    float sideW = 180;
    float sideY = my + titleH;
    float sideH = mh - titleH;
    ctx.drawList.AddRectFilled(mx, sideY, mx + sideW, my + mh, ACETheme::SidebarBg, 0);
    ctx.drawList.AddLine(mx + sideW, sideY, mx + sideW, my + mh, ACETheme::Border, 1.0f);

    // Sidebar branding
    ctx.drawList.AddText(mx + 16, sideY + 10, ACETheme::AccentBlue, "ADMIN PANEL");
    ctx.drawList.AddText(mx + 16, sideY + 28, ACETheme::TextDim, "License Manager");
    ctx.drawList.AddRectFilledMultiColor(
        mx + 12, sideY + 48, mx + sideW - 12, sideY + 49,
        ACETheme::AccentBlue, ACETheme::AccentPurple,
        ACETheme::AccentPurple, ACETheme::AccentBlue);

    // Sidebar buttons
    const char* pageNames[] = { "Generate", "Validate", "Manage", "About" };
    const char* pageIcons[] = { "+", "?", "L", "i" };
    float btnY = sideY + 60;

    for (int i = 0; i < 4; i++) {
        float btnH = 36;
        uint32_t id = ctx.GetID(pageNames[i]);
        bool active = (g_currentPage == i);
        bool hovered = ctx.input.IsMouseInRect(mx, btnY, mx + sideW, btnY + btnH);
        bool clicked = hovered && ctx.input.mouseClicked[0];

        float anim = ctx.SmoothAnim(id, active ? 1.0f : (hovered ? 0.5f : 0.0f));

        if (anim > 0.01f) {
            uint32_t bg = ACE_COL32(59, 130, 246, (int)(30 * anim));
            ctx.drawList.AddRectFilled(mx + 4, btnY, mx + sideW - 4, btnY + btnH, bg, 6.0f);
        }
        if (active) {
            ctx.drawList.AddRectFilled(mx + 2, btnY + 6, mx + 5, btnY + btnH - 6,
                                        ACETheme::AccentBlue, 2.0f);
        }

        uint32_t tc = active ? ACETheme::TextPrimary :
                      hovered ? ACETheme::TextPrimary : ACETheme::TextSecondary;
        ctx.drawList.AddText(mx + 20, btnY + 10, ACETheme::AccentBlue, pageIcons[i]);
        ctx.drawList.AddText(mx + 40, btnY + 10, tc, pageNames[i]);

        if (clicked) g_currentPage = i;
        btnY += btnH + 2;
    }

    // Bottom stats
    float botY = my + mh - 50;
    ctx.drawList.AddRectFilledMultiColor(
        mx + 12, botY, mx + sideW - 12, botY + 1,
        ACETheme::Border, ACETheme::Border,
        ACE_COL32(40, 40, 55, 0), ACE_COL32(40, 40, 55, 0));
    char statBuf[64];
    snprintf(statBuf, sizeof(statBuf), "%zu licenses", g_licenses.size());
    ctx.drawList.AddText(mx + 16, botY + 8, ACETheme::AccentGreen, statBuf);
    ctx.drawList.AddText(mx + 16, botY + 26, ACETheme::TextDim, "ACE Engine v3.0");

    // Content area
    float cx = mx + sideW + 16;
    float cy = my + titleH + 16;
    float cw = mw - sideW - 32;
    float ch = mh - titleH - 32;

    // ---- PAGE: GENERATE ----
    if (g_currentPage == 0) {
        ctx.drawList.AddTextShadow(cx, cy, ACETheme::TextPrimary, "Generate New License");
        ctx.drawList.AddRectFilledMultiColor(cx, cy + 20, cx + cw, cy + 21,
            ACETheme::AccentBlue, ACETheme::AccentPurple,
            ACETheme::AccentPurple, ACETheme::AccentBlue);

        // Card
        float cardY = cy + 36;
        ctx.drawList.AddRectFilled(cx, cardY, cx + cw, cardY + 240, ACETheme::CardBg, 8.0f);
        ctx.drawList.AddRect(cx, cardY, cx + cw, cardY + 240, ACETheme::Border, 8.0f, 1.0f);

        // Username
        ctx.drawList.AddText(cx + 16, cardY + 16, ACETheme::TextSecondary, "Username");
        ctx.SetCursorPos(cx + 16 - ctx.wnd.x, cardY + 36 - ctx.wnd.y);
        ctx.InputText("##user", g_username, sizeof(g_username), cw - 32);

        // Filename
        ctx.drawList.AddText(cx + 16, cardY + 76, ACETheme::TextSecondary, "Output File");
        ctx.SetCursorPos(cx + 16 - ctx.wnd.x, cardY + 96 - ctx.wnd.y);
        ctx.InputText("##file", g_filename, sizeof(g_filename), cw - 32);

        // Expiry days
        ctx.drawList.AddText(cx + 16, cardY + 136, ACETheme::TextSecondary, "Expiry (days)");
        ctx.SetCursorPos(cx + 16 - ctx.wnd.x, cardY + 156 - ctx.wnd.y);
        ctx.InputInt("##days", &g_expiryDays, 1, 10, 120);
        if (g_expiryDays < 1) g_expiryDays = 1;
        if (g_expiryDays > 3650) g_expiryDays = 3650;

        // Generate button
        float btnX = cx + 16, btnBY = cardY + 200, btnW = cw - 32, btnBH = 32;
        uint32_t gId = ctx.GetID("gen_btn");
        bool gHov = ctx.input.IsMouseInRect(btnX, btnBY, btnX + btnW, btnBY + btnBH);
        float gAnim = ctx.SmoothAnim(gId, gHov ? 1.0f : 0.0f);

        uint32_t gBg = ACE_COL32(59, 130, 246, (int)(200 + 55 * gAnim));
        ctx.drawList.AddRectFilled(btnX, btnBY, btnX + btnW, btnBY + btnBH, gBg, 6.0f);
        if (gAnim > 0.01f) {
            ctx.drawList.AddRectFilled(btnX - 2, btnBY - 2, btnX + btnW + 2, btnBY + btnBH + 2,
                ACE_COL32(59, 130, 246, (int)(30 * gAnim)), 8.0f);
        }
        ACEVec2 ts = ctx.drawList.font->CalcTextSize("GENERATE LICENSE");
        ctx.drawList.AddText(btnX + (btnW - ts.x) / 2, btnBY + (btnBH - ts.y) / 2,
                             ACE_COL32(255, 255, 255, 255), "GENERATE LICENSE");

        if (gHov && ctx.input.mouseClicked[0]) {
            if (g_username[0] && g_filename[0]) {
                if (CreateLicense(g_filename, g_username, g_expiryDays)) {
                    ShowNotif("License generated successfully!");
                } else {
                    ShowNotif("Failed to create license file!", ACETheme::AccentRed);
                }
            } else {
                ShowNotif("Fill in username and filename!", ACETheme::AccentYellow);
            }
        }

        // Recent license display
        if (!g_licenses.empty()) {
            auto& last = g_licenses.back();
            float infoY = cardY + 260;
            ctx.drawList.AddRectFilled(cx, infoY, cx + cw, infoY + 100, ACETheme::CardBg, 8.0f);
            ctx.drawList.AddText(cx + 16, infoY + 8, ACETheme::AccentGreen, "Last Generated:");
            ctx.drawList.AddText(cx + 16, infoY + 28, ACETheme::TextPrimary, last.key.c_str());

            char info[256];
            snprintf(info, sizeof(info), "User: %s  |  Expires: %s",
                     last.username.c_str(), last.expiry.c_str());
            ctx.drawList.AddText(cx + 16, infoY + 50, ACETheme::TextSecondary, info);

            snprintf(info, sizeof(info), "File: %s", g_filename);
            ctx.drawList.AddText(cx + 16, infoY + 70, ACETheme::TextDim, info);
        }
    }

    // ---- PAGE: VALIDATE ----
    else if (g_currentPage == 1) {
        ctx.drawList.AddTextShadow(cx, cy, ACETheme::TextPrimary, "Validate License");
        ctx.drawList.AddRectFilledMultiColor(cx, cy + 20, cx + cw, cy + 21,
            ACETheme::AccentBlue, ACETheme::AccentPurple,
            ACETheme::AccentPurple, ACETheme::AccentBlue);

        float cardY = cy + 36;
        ctx.drawList.AddRectFilled(cx, cardY, cx + cw, cardY + 100, ACETheme::CardBg, 8.0f);

        ctx.drawList.AddText(cx + 16, cardY + 16, ACETheme::TextSecondary, "License File Path");
        ctx.SetCursorPos(cx + 16 - ctx.wnd.x, cardY + 36 - ctx.wnd.y);
        ctx.InputText("##vfile", g_validateFile, sizeof(g_validateFile), cw - 32);

        // Validate button
        float btnX = cx + 16, btnBY = cardY + 72, btnW = 160, btnBH = 28;
        uint32_t vId = ctx.GetID("val_btn");
        bool vHov = ctx.input.IsMouseInRect(btnX, btnBY, btnX + btnW, btnBY + btnBH);
        ctx.drawList.AddRectFilled(btnX, btnBY, btnX + btnW, btnBY + btnBH,
            vHov ? ACETheme::AccentBlue : ACETheme::AccentBlueDim, 4.0f);
        ACEVec2 vts = ctx.drawList.font->CalcTextSize("VALIDATE");
        ctx.drawList.AddText(btnX + (btnW - vts.x) / 2, btnBY + (btnBH - vts.y) / 2,
                             ACE_COL32(255, 255, 255, 255), "VALIDATE");

        static LicenseEntry valResult;
        static bool valDone = false;
        static bool valValid = false;

        if (vHov && ctx.input.mouseClicked[0] && g_validateFile[0]) {
            valValid = ValidateLicenseFile(g_validateFile, valResult);
            valDone = true;
        }

        if (valDone) {
            float resY = cardY + 120;
            ctx.drawList.AddRectFilled(cx, resY, cx + cw, resY + 140, ACETheme::CardBg, 8.0f);

            if (valValid) {
                ctx.drawList.AddText(cx + 16, resY + 8,
                    valResult.active ? ACETheme::AccentGreen : ACETheme::AccentRed,
                    valResult.active ? "STATUS: VALID & ACTIVE" : "STATUS: REVOKED");

                ctx.drawList.AddText(cx + 16, resY + 32, ACETheme::TextSecondary, "Key:");
                ctx.drawList.AddText(cx + 60, resY + 32, ACETheme::TextPrimary, valResult.key.c_str());

                ctx.drawList.AddText(cx + 16, resY + 52, ACETheme::TextSecondary, "User:");
                ctx.drawList.AddText(cx + 60, resY + 52, ACETheme::TextPrimary, valResult.username.c_str());

                ctx.drawList.AddText(cx + 16, resY + 72, ACETheme::TextSecondary, "Created:");
                ctx.drawList.AddText(cx + 80, resY + 72, ACETheme::TextPrimary, valResult.created.c_str());

                ctx.drawList.AddText(cx + 16, resY + 92, ACETheme::TextSecondary, "Expiry:");
                ctx.drawList.AddText(cx + 80, resY + 92, ACETheme::TextPrimary, valResult.expiry.c_str());

                // Revoke button
                if (valResult.active) {
                    float rX = cx + 16, rY2 = resY + 112, rW = 120, rH = 28;
                    uint32_t rId = ctx.GetID("rev_val_btn");
                    bool rHov = ctx.input.IsMouseInRect(rX, rY2, rX + rW, rY2 + rH);
                    ctx.drawList.AddRectFilled(rX, rY2, rX + rW, rY2 + rH,
                        rHov ? ACETheme::AccentRed : ACE_COL32(248, 113, 113, 120), 4.0f);
                    ACEVec2 rts = ctx.drawList.font->CalcTextSize("REVOKE");
                    ctx.drawList.AddText(rX + (rW - rts.x) / 2, rY2 + (rH - rts.y) / 2,
                                         ACE_COL32(255, 255, 255, 255), "REVOKE");
                    if (rHov && ctx.input.mouseClicked[0]) {
                        if (RevokeLicenseFile(g_validateFile)) {
                            valResult.active = false;
                            ShowNotif("License revoked!");
                        }
                    }
                }
            } else {
                ctx.drawList.AddText(cx + 16, resY + 16, ACETheme::AccentRed,
                                     "INVALID — file not found or corrupt");
            }
        }
    }

    // ---- PAGE: MANAGE ----
    else if (g_currentPage == 2) {
        ctx.drawList.AddTextShadow(cx, cy, ACETheme::TextPrimary, "License History");
        ctx.drawList.AddRectFilledMultiColor(cx, cy + 20, cx + cw, cy + 21,
            ACETheme::AccentBlue, ACETheme::AccentPurple,
            ACETheme::AccentPurple, ACETheme::AccentBlue);

        if (g_licenses.empty()) {
            ctx.drawList.AddText(cx + 20, cy + 50, ACETheme::TextDim,
                                 "No licenses generated this session.");
            ctx.drawList.AddText(cx + 20, cy + 70, ACETheme::TextDim,
                                 "Go to Generate to create one.");
        } else {
            float listY = cy + 36;
            for (int i = (int)g_licenses.size() - 1; i >= 0; i--) {
                auto& lic = g_licenses[i];
                float entH = 70;

                if (listY + entH > my + mh - 20) break; // clip

                ctx.drawList.AddRectFilled(cx, listY, cx + cw, listY + entH,
                                            ACETheme::CardBg, 6.0f);

                // Status indicator
                uint32_t statusCol = lic.active ? ACETheme::AccentGreen : ACETheme::AccentRed;
                ctx.drawList.AddCircleFilled(cx + 14, listY + 14, 5, statusCol, 12);

                ctx.drawList.AddText(cx + 26, listY + 6, ACETheme::TextPrimary, lic.key.c_str());

                char info[256];
                snprintf(info, sizeof(info), "User: %s  |  Expires: %s  |  %s",
                         lic.username.c_str(), lic.expiry.c_str(),
                         lic.active ? "Active" : "Revoked");
                ctx.drawList.AddText(cx + 26, listY + 26, ACETheme::TextSecondary, info);

                snprintf(info, sizeof(info), "Created: %s", lic.created.c_str());
                ctx.drawList.AddText(cx + 26, listY + 46, ACETheme::TextDim, info);

                listY += entH + 6;
            }
        }
    }

    // ---- PAGE: ABOUT ----
    else if (g_currentPage == 3) {
        ctx.drawList.AddTextShadow(cx, cy, ACETheme::TextPrimary, "About");
        ctx.drawList.AddRectFilledMultiColor(cx, cy + 20, cx + cw, cy + 21,
            ACETheme::AccentBlue, ACETheme::AccentPurple,
            ACETheme::AccentPurple, ACETheme::AccentBlue);

        float cardY = cy + 36;
        ctx.drawList.AddRectFilled(cx, cardY, cx + cw, cardY + 260, ACETheme::CardBg, 8.0f);

        ctx.drawList.AddText(cx + 16, cardY + 12, ACETheme::AccentBlue, "AC License Generator");
        ctx.drawList.AddText(cx + 16, cardY + 32, ACETheme::TextSecondary,
                             "Admin tool for CS2 Skin Changer v3.0");
        ctx.drawList.AddText(cx + 16, cardY + 52, ACETheme::TextDim,
                             "Custom ACE rendering engine — zero ImGui dependency.");

        ctx.drawList.AddRectFilledMultiColor(
            cx + 16, cardY + 74, cx + cw - 16, cardY + 75,
            ACETheme::Border, ACETheme::Border,
            ACE_COL32(40, 40, 55, 0), ACE_COL32(40, 40, 55, 0));

        ctx.drawList.AddText(cx + 16, cardY + 84, ACETheme::TextSecondary, "Features:");

        const char* features[] = {
            "License key generation (CS2-YYYY-XXXX-MM-XXXX)",
            "License validation & status check",
            "License revocation",
            "Configurable expiry (1-3650 days)",
            "NEVERLOSE-themed dark UI",
            "DX11 hardware-accelerated rendering",
            "Custom shader pipeline (HLSL)",
            "stb_truetype font rendering",
        };
        for (int i = 0; i < 8; i++) {
            ctx.drawList.AddText(cx + 32, cardY + 104 + i * 18,
                                 ACETheme::AccentGreen, "+");
            ctx.drawList.AddText(cx + 48, cardY + 104 + i * 18,
                                 ACETheme::TextPrimary, features[i]);
        }
    }

    // ---- NOTIFICATION TOAST ----
    if (g_notifTimer > 0) {
        g_notifTimer -= ctx.deltaTime;
        float a = g_notifTimer > 0.5f ? 1.0f : g_notifTimer / 0.5f;
        if (a > 0.01f) {
            ctx.overlayDrawList.font = ctx.drawList.font;
            float nW = 300, nH = 36;
            float nX = mx + mw - nW - 16;
            float nY = my + mh - nH - 16;

            uint32_t cR = ACE_COL32_R(g_notifColor);
            uint32_t cG = ACE_COL32_G(g_notifColor);
            uint32_t cB = ACE_COL32_B(g_notifColor);
            uint32_t nc = ACE_COL32(cR, cG, cB, (int)(200 * a));

            ctx.overlayDrawList.AddRectFilled(nX, nY, nX + nW, nY + nH, nc, 6.0f);
            uint32_t tc = ACE_COL32(255, 255, 255, (int)(255 * a));
            ACEVec2 nts = ctx.overlayDrawList.font->CalcTextSize(g_notification.c_str());
            ctx.overlayDrawList.AddText(nX + (nW - nts.x) / 2, nY + (nH - nts.y) / 2,
                                         tc, g_notification.c_str());
        }
    }
}

// ============================================================================
// ENTRY POINT
// ============================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    srand((unsigned)time(nullptr));

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "ACLicenseGen";
    RegisterClassExA(&wc);

    g_hwnd = CreateWindowExA(
        0, wc.lpszClassName, "AC License Generator — Admin Panel",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, g_width, g_height,
        nullptr, nullptr, hInstance, nullptr);

    if (!CreateDeviceD3D()) {
        CleanupDevice();
        UnregisterClassA(wc.lpszClassName, hInstance);
        return 1;
    }

    ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hwnd);

    // Initialize ACE engine
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

        // Begin ACE frame
        g_ui.NewFrame(w, h, dt);

        // Wrap entire screen as a window for layout
        g_ui.wnd.x = 0; g_ui.wnd.y = 0;
        g_ui.wnd.w = w; g_ui.wnd.h = h;
        g_ui.wnd.cursorX = 0; g_ui.wnd.cursorY = 0;
        g_ui.wnd.contentW = w;

        // Render UI
        RenderUI();

        // End frame
        g_ui.EndFrame();

        // Clear and render
        float clearColor[4] = { 0.05f, 0.05f, 0.07f, 1.0f };
        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_context->ClearRenderTargetView(g_rtv, clearColor);

        g_renderer.RenderDrawList(g_ui.drawList, w, h);
        if (!g_ui.overlayDrawList.vtxBuffer.empty())
            g_renderer.RenderDrawList(g_ui.overlayDrawList, w, h);

        g_swapChain->Present(1, 0); // VSync
    }

    // Cleanup
    g_renderer.Shutdown();
    g_font.Free();
    CleanupDevice();
    DestroyWindow(g_hwnd);
    UnregisterClassA(wc.lpszClassName, hInstance);

    return 0;
}
