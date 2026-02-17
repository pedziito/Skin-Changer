/*
 * AC Skin Changer - DX11 Hooks (Custom Engine)
 * Hooks IDXGISwapChain::Present + WndProc.
 * Uses ACE custom rendering engine — zero ImGui dependency.
 */

#include "core.h"
#include "../engine/ace_engine.h"

// ============================================================================
// HOOK INFRASTRUCTURE
// ============================================================================
namespace {
    typedef HRESULT(WINAPI* PresentFn)(IDXGISwapChain*, UINT, UINT);
    typedef HRESULT(WINAPI* ResizeBuffersFn)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    PresentFn oPresent = nullptr;
    ResizeBuffersFn oResizeBuffers = nullptr;

    bool engineInitialized = false;

    // Timing
    LARGE_INTEGER perfFreq;
    LARGE_INTEGER lastTime;

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

    VTableHook presentHook;
    VTableHook resizeHook;

    // Forward declarations
    LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    HRESULT WINAPI HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    HRESULT WINAPI HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
                                        UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT Flags);

    void InitEngine(IDXGISwapChain* pSwapChain) {
        // Get device from swap chain
        if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&G::pDevice))) {
            LogMsg("Failed to get device from swap chain");
            return;
        }
        G::pDevice->GetImmediateContext(&G::pContext);
        G::pSwapChain = pSwapChain;

        // Get game window
        DXGI_SWAP_CHAIN_DESC desc;
        pSwapChain->GetDesc(&desc);
        G::gameWindow = desc.OutputWindow;

        // Create render target
        ID3D11Texture2D* backBuffer = nullptr;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        if (backBuffer) {
            G::pDevice->CreateRenderTargetView(backBuffer, nullptr, &G::pRenderTargetView);
            backBuffer->Release();
        }

        // Initialize ACE engine (renderer + font + shaders)
        if (!ACE::Initialize(G::pDevice, G::pContext)) {
            LogMsg("FATAL: ACE engine initialization failed");
            return;
        }

        // Hook WndProc
        G::originalWndProc = (WNDPROC)SetWindowLongPtrA(G::gameWindow, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);

        // Setup timing
        QueryPerformanceFrequency(&perfFreq);
        QueryPerformanceCounter(&lastTime);

        engineInitialized = true;
        LogMsg("ACE engine initialized — custom rendering pipeline active");
    }
}

// ============================================================================
// HOOKED WNDPROC — Custom Input System
// ============================================================================
LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Toggle menu with INSERT key
    if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
        G::menuOpen = !G::menuOpen;
        LogMsg("Menu toggled: %s", G::menuOpen ? "OPEN" : "CLOSED");
        return 0;
    }

    // When menu is open, feed input to ACE engine
    if (G::menuOpen && engineInitialized) {
        bool consumed = ACEProcessInput(ACE::gUI, (void*)hWnd, msg, (unsigned long long)wParam, (long long)lParam);

        // Block game input for mouse/keyboard events when menu is open
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
// HOOKED PRESENT — Custom Render Pipeline
// ============================================================================
HRESULT WINAPI HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!engineInitialized) {
        InitEngine(pSwapChain);
    }

    if (engineInitialized) {
        // Apply skins each frame
        Inventory::ApplySkins();

        // Calculate delta time
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        float deltaTime = (float)(now.QuadPart - lastTime.QuadPart) / perfFreq.QuadPart;
        if (deltaTime > 0.1f) deltaTime = 0.016f;
        lastTime = now;

        // Get display size
        DXGI_SWAP_CHAIN_DESC desc;
        pSwapChain->GetDesc(&desc);
        float displayW = (float)desc.BufferDesc.Width;
        float displayH = (float)desc.BufferDesc.Height;

        // Begin ACE frame
        ACE::NewFrame(displayW, displayH, deltaTime);

        // Render menu if open
        if (G::menuOpen) {
            Menu::Render();
        }

        // End frame and render draw lists
        ACE::Render(G::pRenderTargetView, G::pContext);
    }

    return oPresent(pSwapChain, SyncInterval, Flags);
}

// ============================================================================
// HOOKED RESIZE BUFFERS
// ============================================================================
HRESULT WINAPI HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
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
// HOOKS NAMESPACE
// ============================================================================
namespace Hooks {
    bool Initialize() {
        // Find game window first
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

        // Create dummy swap chain targeting game window for VTable hooking
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

        // Hook VTable
        void** scVtable = *reinterpret_cast<void***>(tmpSwapChain);
        oPresent = (PresentFn)scVtable[8];
        oResizeBuffers = (ResizeBuffersFn)scVtable[13];

        PresentFn pHooked = &HookedPresent;
        ResizeBuffersFn rHooked = &HookedResizeBuffers;
        presentHook.Hook(scVtable, 8, reinterpret_cast<void*>(pHooked));
        resizeHook.Hook(scVtable, 13, reinterpret_cast<void*>(rHooked));

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
