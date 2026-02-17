/*
 * AC Skin Changer - DX11 Hooks
 * Hooks IDXGISwapChain::Present to render ImGui overlay.
 * Hooks WndProc to capture input when menu is open.
 * Uses VTable hooking with trampoline (14-byte absolute JMP).
 */

#include "core.h"
#include "render_engine.h"

#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_impl_win32.h"
#include "../vendor/imgui/imgui_impl_dx11.h"

// Forward declaration from imgui_impl_win32
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ============================================================================
// HOOK INFRASTRUCTURE
// ============================================================================
namespace {
    // Original function pointers
    typedef HRESULT(WINAPI* PresentFn)(IDXGISwapChain*, UINT, UINT);
    typedef HRESULT(WINAPI* ResizeBuffersFn)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    PresentFn oPresent = nullptr;
    ResizeBuffersFn oResizeBuffers = nullptr;

    bool imguiInitialized = false;

    // VTable hook using memory patching
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

    // Get DX11 VTable by creating a dummy device
    void** GetSwapChainVTable() {
        WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProcA, 0, 0,
                          GetModuleHandleA(nullptr), nullptr, nullptr, nullptr,
                          nullptr, "AC_DX11_DUMMY", nullptr };
        RegisterClassExA(&wc);
        HWND hWnd = CreateWindowA("AC_DX11_DUMMY", "", WS_OVERLAPPEDWINDOW,
                                   0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

        DXGI_SWAP_CHAIN_DESC scd = {};
        scd.BufferCount = 1;
        scd.BufferDesc.Width = 2;
        scd.BufferDesc.Height = 2;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferDesc.RefreshRate.Numerator = 60;
        scd.BufferDesc.RefreshRate.Denominator = 1;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = hWnd;
        scd.SampleDesc.Count = 1;
        scd.Windowed = TRUE;
        scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        ID3D11Device* pDevice = nullptr;
        ID3D11DeviceContext* pContext = nullptr;
        IDXGISwapChain* pSwapChain = nullptr;

        D3D_FEATURE_LEVEL featureLevel;
        D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            levels, 1, D3D11_SDK_VERSION, &scd,
            &pSwapChain, &pDevice, &featureLevel, &pContext
        );

        if (FAILED(hr)) {
            DestroyWindow(hWnd);
            UnregisterClassA("AC_DX11_DUMMY", wc.hInstance);
            return nullptr;
        }

        // Get VTable
        void** vtable = *reinterpret_cast<void***>(pSwapChain);

        // Copy vtable pointer before releasing
        static void* vtableCopy[256];
        memcpy(vtableCopy, vtable, sizeof(void*) * 256);

        pSwapChain->Release();
        pDevice->Release();
        pContext->Release();
        DestroyWindow(hWnd);
        UnregisterClassA("AC_DX11_DUMMY", wc.hInstance);

        return vtableCopy;
    }

    void InitImGui(IDXGISwapChain* pSwapChain) {
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

        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr; // Don't save imgui.ini

        // Setup backends
        ImGui_ImplWin32_Init(G::gameWindow);
        ImGui_ImplDX11_Init(G::pDevice, G::pContext);

        // Apply our custom NEVERLOSE style
        Menu::SetupStyle();

        // Hook WndProc
        G::originalWndProc = (WNDPROC)SetWindowLongPtrA(G::gameWindow, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);

        imguiInitialized = true;
        LogMsg("ImGui initialized successfully");
    }
}

// ============================================================================
// HOOKED WNDPROC
// ============================================================================
LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Toggle menu with INSERT key
    if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
        G::menuOpen = !G::menuOpen;
        LogMsg("Menu toggled: %s", G::menuOpen ? "OPEN" : "CLOSED");
        return 0;
    }

    // When menu is open, let ImGui handle input
    if (G::menuOpen && imguiInitialized) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return 0;

        // Block game input for mouse events when menu is open
        switch (msg) {
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN: case WM_LBUTTONUP:
            case WM_RBUTTONDOWN: case WM_RBUTTONUP:
            case WM_MBUTTONDOWN: case WM_MBUTTONUP:
            case WM_MOUSEWHEEL:
            case WM_KEYDOWN: case WM_KEYUP:
            case WM_CHAR:
                // Check if ImGui wants it
                if (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard)
                    return 0;
                break;
        }
    }

    return CallWindowProcA(G::originalWndProc, hWnd, msg, wParam, lParam);
}

// ============================================================================
// HOOKED PRESENT
// ============================================================================
HRESULT WINAPI HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!imguiInitialized) {
        InitImGui(pSwapChain);
    }

    if (imguiInitialized) {
        // Apply skins each frame
        Inventory::ApplySkins();

        // Begin ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Render menu if open
        if (G::menuOpen) {
            Menu::Render();
        }

        // End frame and render
        ImGui::EndFrame();
        ImGui::Render();

        G::pContext->OMSetRenderTargets(1, &G::pRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return oPresent(pSwapChain, SyncInterval, Flags);
}

// ============================================================================
// HOOKED RESIZE BUFFERS
// ============================================================================
HRESULT WINAPI HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
                                     UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT Flags) {
    // Release render target before resize
    if (G::pRenderTargetView) {
        G::pRenderTargetView->Release();
        G::pRenderTargetView = nullptr;
    }

    HRESULT hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, Flags);

    // Recreate render target
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
// HOOKS NAMESPACE IMPLEMENTATION
// ============================================================================
namespace Hooks {
    bool Initialize() {
        void** vtable = GetSwapChainVTable();
        if (!vtable) {
            LogMsg("Failed to get swap chain vtable");
            return false;
        }

        // Present is index 8, ResizeBuffers is index 13
        oPresent = (PresentFn)vtable[8];
        oResizeBuffers = (ResizeBuffersFn)vtable[13];

        // We need the REAL vtable from the game's swap chain
        // The dummy approach gives us function addresses, but we need to hook the actual game's swap chain
        // Strategy: Find the game's swap chain by scanning for IDXGISwapChain in loaded modules

        // Alternative: Use the dummy vtable addresses to create MinHook-style hooks
        // For simplicity, we'll use a different approach: hook via the actual game swap chain
        // when Present is first called

        // Store the original function pointers from the dummy vtable
        // Then create a new dummy swap chain and hook its vtable
        // The real hooking happens when we detect the game's Present call

        // Actually, the cleanest approach is to use a simple IAT or direct vtable hook
        // Let's use a pattern-based approach to find the game's DXGI

        // Find DXGI module
        HMODULE hDXGI = GetModuleHandleA("dxgi.dll");
        if (!hDXGI) {
            LogMsg("dxgi.dll not loaded, waiting...");
            for (int i = 0; i < 60; i++) {
                hDXGI = GetModuleHandleA("dxgi.dll");
                if (hDXGI) break;
                Sleep(500);
            }
        }

        if (!hDXGI) {
            LogMsg("Could not find dxgi.dll");
            return false;
        }

        // Create a real hook using dummy device approach
        // We create a new swap chain targeting the GAME window
        // This lets us get the real vtable

        // Try to find the game window first
        G::gameWindow = FindWindowA("SDL_app", nullptr); // CS2 uses SDL
        if (!G::gameWindow) {
            G::gameWindow = FindWindowA(nullptr, "Counter-Strike 2");
        }
        if (!G::gameWindow) {
            // Wait for window
            for (int i = 0; i < 60; i++) {
                G::gameWindow = FindWindowA("SDL_app", nullptr);
                if (!G::gameWindow)
                    G::gameWindow = FindWindowA(nullptr, "Counter-Strike 2");
                if (G::gameWindow) break;
                Sleep(500);
            }
        }

        if (!G::gameWindow) {
            LogMsg("Could not find CS2 window");
            return false;
        }

        LogMsg("Found CS2 window: 0x%p", (void*)G::gameWindow);

        // Create dummy swap chain to get vtable
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
            LogMsg("Failed to create dummy swap chain for hooking: 0x%lX", hr);
            return false;
        }

        // Hook the vtable
        void** scVtable = *reinterpret_cast<void***>(tmpSwapChain);
        oPresent = (PresentFn)scVtable[8];
        oResizeBuffers = (ResizeBuffersFn)scVtable[13];

        // Apply VTable hooks
        presentHook.Hook(scVtable, 8, (void*)HookedPresent);
        resizeHook.Hook(scVtable, 13, (void*)HookedResizeBuffers);

        LogMsg("DX11 hooks installed (Present @ index 8, ResizeBuffers @ index 13)");

        // Don't release - the hooks reference this vtable
        // tmpSwapChain, tmpDevice, tmpContext stay alive

        return true;
    }

    void Shutdown() {
        // Restore WndProc
        if (G::originalWndProc && G::gameWindow) {
            SetWindowLongPtrA(G::gameWindow, GWLP_WNDPROC, (LONG_PTR)G::originalWndProc);
        }

        // Unhook vtable
        presentHook.Unhook();
        resizeHook.Unhook();

        // Cleanup ImGui
        if (imguiInitialized) {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }

        // Release render target
        if (G::pRenderTargetView) {
            G::pRenderTargetView->Release();
            G::pRenderTargetView = nullptr;
        }

        LogMsg("Hooks shutdown complete");
    }
}
