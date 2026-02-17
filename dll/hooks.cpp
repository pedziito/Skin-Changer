/*
 * AC Skin Changer - DX11 Hooks (Custom Engine)
 * Hooks IDXGISwapChain::Present + WndProc.
 * Uses ACE custom rendering engine — zero ImGui dependency.
 */

#include "core.h"
#include "../engine/ace_engine.h"

// ============================================================================
// TYPEDEFS & STATE (file-scope, internal linkage)
// ============================================================================
typedef HRESULT(WINAPI* PresentFn)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT(WINAPI* ResizeBuffersFn)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

static PresentFn oPresent = nullptr;
static ResizeBuffersFn oResizeBuffers = nullptr;
static bool engineInitialized = false;
static LARGE_INTEGER perfFreq = {};
static LARGE_INTEGER lastTime = {};

// ============================================================================
// VTABLE HOOK HELPER
// ============================================================================
struct VTableHook {
    void** vtable = nullptr;
    void* original = nullptr;
    int index = 0;
    DWORD oldProtect = 0;

    bool Hook(void** vt, int idx, void* detour) {
        vtable = vt;
        index = idx;
        original = vtable[index];
        DWORD oldProt;
        if (VirtualProtect(&vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProt)) {
            oldProtect = oldProt;
            vtable[index] = detour;
            return true;
        }
        return false;
    }

    void Unhook() {
        if (vtable && original) {
            DWORD oldProt;
            VirtualProtect(&vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProt);
            vtable[index] = original;
            VirtualProtect(&vtable[index], sizeof(void*), oldProtect, &oldProt);
            vtable = nullptr;
            original = nullptr;
        }
    }
};

static VTableHook presentHook;
static VTableHook resizeHook;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
static LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static HRESULT WINAPI   HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
static HRESULT WINAPI   HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
                                             UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT Flags);

// ============================================================================
// ENGINE INITIALIZATION
// ============================================================================
static void InitEngine(IDXGISwapChain* pSwapChain) {
    if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&G::pDevice))) {
        LogMsg("Failed to get device from swap chain");
        return;
    }
    G::pDevice->GetImmediateContext(&G::pContext);
    G::pSwapChain = pSwapChain;

    DXGI_SWAP_CHAIN_DESC desc;
    pSwapChain->GetDesc(&desc);
    G::gameWindow = desc.OutputWindow;

    ID3D11Texture2D* backBuffer = nullptr;
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (backBuffer) {
        G::pDevice->CreateRenderTargetView(backBuffer, nullptr, &G::pRenderTargetView);
        backBuffer->Release();
    }

    if (!ACE::Initialize(G::pDevice, G::pContext)) {
        LogMsg("FATAL: ACE engine initialization failed");
        return;
    }

    G::originalWndProc = (WNDPROC)SetWindowLongPtrA(G::gameWindow, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);

    QueryPerformanceFrequency(&perfFreq);
    QueryPerformanceCounter(&lastTime);

    engineInitialized = true;
    LogMsg("ACE engine initialized — custom rendering pipeline active");
}

// ============================================================================
// HOOKED WNDPROC
// ============================================================================
static LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
        G::menuOpen = !G::menuOpen;
        LogMsg("Menu toggled: %s", G::menuOpen ? "OPEN" : "CLOSED");
        return 0;
    }

    if (G::menuOpen && engineInitialized) {
        bool consumed = ACEProcessInput(ACE::gUI, (void*)hWnd, msg, (unsigned long long)wParam, (long long)lParam);

        switch (msg) {
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
            case WM_MOUSEWHEEL:
            case WM_KEYDOWN: case WM_KEYUP: case WM_SYSKEYDOWN: case WM_SYSKEYUP:
            case WM_CHAR:
                if (consumed) return 0;
                break;
        }
    }

    return CallWindowProcA(G::originalWndProc, hWnd, msg, wParam, lParam);
}

// ============================================================================
// HOOKED PRESENT
// ============================================================================
static HRESULT WINAPI HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!engineInitialized) {
        InitEngine(pSwapChain);
    }

    if (engineInitialized) {
        Inventory::ApplySkins();

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        float deltaTime = (float)(now.QuadPart - lastTime.QuadPart) / (float)perfFreq.QuadPart;
        if (deltaTime > 0.1f) deltaTime = 0.016f;
        lastTime = now;

        DXGI_SWAP_CHAIN_DESC desc;
        pSwapChain->GetDesc(&desc);

        ACE::NewFrame((float)desc.BufferDesc.Width, (float)desc.BufferDesc.Height, deltaTime);

        if (G::menuOpen) {
            Menu::Render();
        }

        ACE::Render(G::pRenderTargetView, G::pContext);
    }

    return oPresent(pSwapChain, SyncInterval, Flags);
}

// ============================================================================
// HOOKED RESIZE BUFFERS
// ============================================================================
static HRESULT WINAPI HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
                                           UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT Flags) {
    if (G::pRenderTargetView) {
        G::pRenderTargetView->Release();
        G::pRenderTargetView = nullptr;
    }

    HRESULT hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, Flags);

    if (SUCCEEDED(hr)) {
        ID3D11Texture2D* backBuffer = nullptr;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        if (backBuffer) {
            G::pDevice->CreateRenderTargetView(backBuffer, nullptr, &G::pRenderTargetView);
            backBuffer->Release();
        }
    }

    return hr;
}

// ============================================================================
// PUBLIC API
// ============================================================================
namespace Hooks {
    bool Initialize() {
        G::gameWindow = FindWindowA("SDL_app", nullptr);
        if (!G::gameWindow) G::gameWindow = FindWindowA(nullptr, "Counter-Strike 2");
        if (!G::gameWindow) {
            for (int i = 0; i < 60; i++) {
                G::gameWindow = FindWindowA("SDL_app", nullptr);
                if (!G::gameWindow) G::gameWindow = FindWindowA(nullptr, "Counter-Strike 2");
                if (G::gameWindow) break;
                Sleep(500);
            }
        }

        if (!G::gameWindow) {
            LogMsg("Could not find CS2 window");
            return false;
        }

        LogMsg("Found CS2 window: 0x%p", (void*)G::gameWindow);

        DXGI_SWAP_CHAIN_DESC scd = {};
        scd.BufferCount = 1;
        scd.BufferDesc.Width = 2;
        scd.BufferDesc.Height = 2;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = G::gameWindow;
        scd.SampleDesc.Count = 1;
        scd.Windowed = TRUE;
        scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        ID3D11Device* tmpDevice = nullptr;
        ID3D11DeviceContext* tmpContext = nullptr;
        IDXGISwapChain* tmpSwapChain = nullptr;
        D3D_FEATURE_LEVEL fl;
        D3D_FEATURE_LEVEL fls[] = { D3D_FEATURE_LEVEL_11_0 };

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            fls, 1, D3D11_SDK_VERSION, &scd,
            &tmpSwapChain, &tmpDevice, &fl, &tmpContext
        );

        if (FAILED(hr)) {
            LogMsg("Failed to create swap chain for hooking: 0x%lX", hr);
            return false;
        }

        void** scVtable = *reinterpret_cast<void***>(tmpSwapChain);
        oPresent = reinterpret_cast<PresentFn>(scVtable[8]);
        oResizeBuffers = reinterpret_cast<ResizeBuffersFn>(scVtable[13]);

        // Use memcpy to safely convert function pointers to void* (avoids MSVC WINAPI cast issues)
        void* detourPresent;
        void* detourResize;
        {
            auto fp = &HookedPresent;
            auto fr = &HookedResizeBuffers;
            memcpy(&detourPresent, &fp, sizeof(void*));
            memcpy(&detourResize, &fr, sizeof(void*));
        }
        presentHook.Hook(scVtable, 8, detourPresent);
        resizeHook.Hook(scVtable, 13, detourResize);

        tmpContext->Release();
        tmpDevice->Release();
        tmpSwapChain->Release();

        LogMsg("DX11 hooks installed (Present @ 8, ResizeBuffers @ 13) — ACE pipeline");
        return true;
    }

    void Shutdown() {
        if (G::originalWndProc && G::gameWindow) {
            SetWindowLongPtrA(G::gameWindow, GWLP_WNDPROC, (LONG_PTR)G::originalWndProc);
        }

        presentHook.Unhook();
        resizeHook.Unhook();

        if (engineInitialized) {
            ACE::Shutdown();
            engineInitialized = false;
        }

        if (G::pRenderTargetView) {
            G::pRenderTargetView->Release();
            G::pRenderTargetView = nullptr;
        }

        LogMsg("Hooks shutdown — ACE engine released");
    }
}
