/**
 * ACE Engine — Demo Application
 *
 * Win32 window + DX11 device + full engine demo.
 * Shows docked panels, node editor, property inspector,
 * viewport with gizmos, theme switching, and more.
 *
 * Build: cl /std:c++20 /EHsc /I.. main.cpp /link d3d11.lib d3dcompiler.lib
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include "../engine/ace_engine_v2.h"
#include <cstdio>

#ifdef _WIN32

using namespace ace;

// ============================================================================
// GLOBALS
// ============================================================================
static DX11Backend      g_backend;
static FontAtlas         g_fontAtlas;
static InputSystem       g_input;
static ThemeEngine       g_theme;
static UIContext         g_ui;
static DockSystem        g_dockSystem;
static AnimationSystem   g_anims;
static NodeEditor        g_nodeEditor("Blueprint");
static GraphEditor       g_graphEditor("StateGraph");
static PropertyInspector g_inspector("Inspector");
static ViewportManager   g_viewports;
static HotReloadManager& g_hotReload = HotReloadManager::Instance();
static ScriptManager&    g_scripts = ScriptManager::Instance();

static bool  g_running = true;
static HWND  g_hwnd = nullptr;

// Demo data
struct DemoObject {
    Vec3  position = {0, 2, 0};
    Vec3  rotation = {};
    Vec3  scale = {1, 1, 1};
    Color color = {200, 100, 50, 255};
    float speed = 1.0f;
    bool  visible = true;
    std::string name = "Cube";

    ACE_REFLECT(DemoObject,
        ACE_FIELD(position),
        ACE_FIELD(rotation),
        ACE_FIELD(scale),
        ACE_FIELD(color),
        ACE_FIELD(speed),
        ACE_FIELD(visible),
        ACE_FIELD(name)
    )
};

static DemoObject g_demoObject;

// ============================================================================
// WINDOW PROCEDURE
// ============================================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            g_running = false;
            return 0;

        case WM_SIZE: {
            u32 w = LOWORD(lParam);
            u32 h = HIWORD(lParam);
            if (w > 0 && h > 0) {
                g_backend.Resize(w, h);
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            f32 x = (f32)LOWORD(lParam);
            f32 y = (f32)HIWORD(lParam);
            g_input.OnMouseMove(x, y);
            break;
        }

        case WM_LBUTTONDOWN: g_input.OnMouseDown(MouseButton::Left); break;
        case WM_LBUTTONUP:   g_input.OnMouseUp(MouseButton::Left); break;
        case WM_RBUTTONDOWN: g_input.OnMouseDown(MouseButton::Right); break;
        case WM_RBUTTONUP:   g_input.OnMouseUp(MouseButton::Right); break;
        case WM_MBUTTONDOWN: g_input.OnMouseDown(MouseButton::Middle); break;
        case WM_MBUTTONUP:   g_input.OnMouseUp(MouseButton::Middle); break;

        case WM_MOUSEWHEEL: {
            f32 delta = (f32)GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f;
            g_input.OnMouseScroll(delta);
            break;
        }

        case WM_KEYDOWN: g_input.OnKeyDown((Key)wParam); break;
        case WM_KEYUP:   g_input.OnKeyUp((Key)wParam); break;
        case WM_CHAR:    g_input.OnTextInput((char)wParam); break;
    }

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

// ============================================================================
// SETUP DEMO SCENE
// ============================================================================
void SetupDemoScene() {
    // Theme: start with Cyberpunk
    g_theme.SetActiveTheme("Cyberpunk");

    // Setup dock system
    g_dockSystem.AddTab("Viewport", nullptr, true);
    g_dockSystem.AddTab("Blueprint", nullptr, true);
    g_dockSystem.AddTab("Inspector", nullptr, true);
    g_dockSystem.AddTab("State Graph", nullptr, true);
    g_dockSystem.AddTab("Console", nullptr, true);

    // Node editor: create a simple material graph
    u32 texNode = g_nodeEditor.AddNode("Texture Sample", {100, 100}, Color{80, 120, 80, 255});
    g_nodeEditor.AddPin(texNode, "UV", PinType::Vec2, PinDirection::Input);
    g_nodeEditor.AddPin(texNode, "RGB", PinType::Vec3, PinDirection::Output);
    g_nodeEditor.AddPin(texNode, "Alpha", PinType::Float, PinDirection::Output);

    u32 mixNode = g_nodeEditor.AddNode("Lerp", {350, 80}, Color{80, 80, 150, 255});
    g_nodeEditor.AddPin(mixNode, "A", PinType::Vec3, PinDirection::Input);
    g_nodeEditor.AddPin(mixNode, "B", PinType::Vec3, PinDirection::Input);
    g_nodeEditor.AddPin(mixNode, "Factor", PinType::Float, PinDirection::Input);
    g_nodeEditor.AddPin(mixNode, "Result", PinType::Vec3, PinDirection::Output);

    u32 outputNode = g_nodeEditor.AddNode("Material Output", {600, 100}, Color{150, 60, 60, 255});
    g_nodeEditor.AddPin(outputNode, "Base Color", PinType::Vec3, PinDirection::Input);
    g_nodeEditor.AddPin(outputNode, "Normal", PinType::Vec3, PinDirection::Input);
    g_nodeEditor.AddPin(outputNode, "Roughness", PinType::Float, PinDirection::Input);
    g_nodeEditor.AddPin(outputNode, "Metallic", PinType::Float, PinDirection::Input);

    // Graph editor: create a state machine
    u32 idleState = g_graphEditor.AddNode("Idle", {100, 200});
    u32 walkState = g_graphEditor.AddNode("Walk", {350, 100});
    u32 runState  = g_graphEditor.AddNode("Run", {350, 300});
    u32 jumpState = g_graphEditor.AddNode("Jump", {600, 200});

    g_graphEditor.AddPort(idleState, "Out", false);
    g_graphEditor.AddPort(walkState, "In", true);
    g_graphEditor.AddPort(walkState, "Out", false);
    g_graphEditor.AddPort(runState, "In", true);
    g_graphEditor.AddPort(runState, "Out", false);
    g_graphEditor.AddPort(jumpState, "In", true);

    // Property inspector: inspect demo object
    g_inspector.Inspect(&g_demoObject);

    // Viewports
    g_viewports.Initialize(&g_backend);
    auto* mainVP = g_viewports.AddViewport("Perspective");
    mainVP->SetCameraMode(CameraMode::Orbit);
    mainVP->SetGizmoMode(GizmoMode::Translate);

    // Scripting
    g_scripts.Initialize(ScriptLanguage::Lua);

    // Hot reload
    g_hotReload.Initialize();
    g_hotReload.WatchDirectory("scripts/");
    g_hotReload.AddFilter(".lua");
    g_hotReload.AddFilter(".json");
    g_hotReload.AddFilter(".hlsl");
}

// ============================================================================
// MAIN LOOP FRAME
// ============================================================================
void RenderFrame(f32 dt) {
    // Begin frame
    g_input.BeginFrame();
    g_anims.Update(dt);
    g_hotReload.Update();

    // Clear to dark background
    g_backend.BeginFrame();
    g_backend.SetClearColor(Color{20, 20, 26, 255});

    // Build draw list
    DrawList drawList;

    // Get window size
    RECT rc;
    GetClientRect(g_hwnd, &rc);
    f32 w = (f32)(rc.right - rc.left);
    f32 h = (f32)(rc.bottom - rc.top);

    // Top bar
    Rect topBar{0, 0, w, 32};
    drawList.AddFilledRect(topBar, g_theme.GetColor(ThemeToken::BgOverlay));
    drawList.AddFilledRect({0, 30, w, 2}, g_theme.GetColor(ThemeToken::BorderPrimary));

    // Status bar
    Rect statusBar{0, h - 24, w, 24};
    drawList.AddFilledRect(statusBar, g_theme.GetColor(ThemeToken::BgOverlay));

    // Main content area
    Rect mainArea{0, 32, w, h - 56};

    // Dock system handles layout
    g_dockSystem.SetBounds(mainArea);
    g_dockSystem.OnUpdate(dt);
    g_dockSystem.OnDraw(drawList, g_theme);

    // Render the draw list
    Vec2 vpSize = g_backend.GetViewportSize();
    g_backend.RenderDrawList(drawList, (u32)vpSize.x, (u32)vpSize.y);
    g_backend.EndFrame();
    g_backend.Present();
}

// ============================================================================
// ENTRY POINT
// ============================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Register window class
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "ACEEngineV2";
    RegisterClassExA(&wc);

    // Create window
    g_hwnd = CreateWindowExA(
        0, "ACEEngineV2", "ACE Engine v2.0 — Editor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1600, 900,
        nullptr, nullptr, hInstance, nullptr
    );

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    // Initialize DX11
    RECT rc;
    GetClientRect(g_hwnd, &rc);
    if (!g_backend.Initialize(g_hwnd, rc.right - rc.left, rc.bottom - rc.top)) {
        MessageBoxA(nullptr, "Failed to initialize DX11", "Error", MB_OK);
        return 1;
    }

    // Load fonts
    g_fontAtlas.AddFont("C:\\Windows\\Fonts\\segoeui.ttf", 14.0f, &g_backend);

    // Setup demo scene
    SetupDemoScene();

    printf("ACE Engine v%s initialized.\n", ace::EngineVersion.string);
    printf("  Nodes: 3\n");
    printf("  Theme: Cyberpunk\n");
    printf("  Viewports: 1 (Perspective)\n");

    // Main loop
    LARGE_INTEGER freq, lastTime;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastTime);

    MSG msg = {};
    while (g_running) {
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) g_running = false;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        f32 dt = (f32)(now.QuadPart - lastTime.QuadPart) / (f32)freq.QuadPart;
        lastTime = now;

        RenderFrame(dt);
    }

    // Cleanup
    g_hotReload.Shutdown();
    g_scripts.Shutdown();
    g_backend.Shutdown();

    return 0;
}

#else
// Non-Windows stub
int main() {
    printf("ACE Engine v%d.%d.%d\n",
        ACE_ENGINE_VERSION_MAJOR, ACE_ENGINE_VERSION_MINOR, ACE_ENGINE_VERSION_PATCH);
    printf("Windows build required for DX11 backend.\n");
    printf("Vulkan/OpenGL backends coming soon.\n");
    return 0;
}
#endif
