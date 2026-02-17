/*
 * ACE UI — Custom Immediate-Mode UI Framework
 * Own component tree, layout engine, animation system, input system, theme.
 * Zero ImGui dependency.
 */

#pragma once

#include "ace_renderer.h"
#include <unordered_map>
#include <functional>

// ============================================================================
// THEME — NEVERLOSE INSPIRED COLOR PALETTE
// ============================================================================
namespace ACETheme {
    // Backgrounds
    constexpr uint32_t WindowBg      = ACE_COL32(13,  13,  17,  255);
    constexpr uint32_t SidebarBg     = ACE_COL32(17,  17,  22,  255);
    constexpr uint32_t CardBg        = ACE_COL32(22,  22,  30,  255);
    constexpr uint32_t CardBgHover   = ACE_COL32(28,  28,  38,  255);
    constexpr uint32_t CardBgActive  = ACE_COL32(35,  35,  48,  255);
    constexpr uint32_t InputBg       = ACE_COL32(20,  20,  28,  255);
    constexpr uint32_t PopupBg       = ACE_COL32(18,  18,  24,  240);

    // Accents
    constexpr uint32_t AccentBlue    = ACE_COL32(59,  130, 246, 255);
    constexpr uint32_t AccentBlueDim = ACE_COL32(59,  130, 246, 120);
    constexpr uint32_t AccentBlueGlow= ACE_COL32(59,  130, 246, 40);
    constexpr uint32_t AccentPurple  = ACE_COL32(139, 92,  246, 255);
    constexpr uint32_t AccentCyan    = ACE_COL32(34,  211, 238, 255);
    constexpr uint32_t AccentGreen   = ACE_COL32(74,  222, 128, 255);
    constexpr uint32_t AccentRed     = ACE_COL32(248, 113, 113, 255);
    constexpr uint32_t AccentYellow  = ACE_COL32(250, 204, 21,  255);
    constexpr uint32_t AccentOrange  = ACE_COL32(251, 146, 60,  255);

    // Text
    constexpr uint32_t TextPrimary   = ACE_COL32(230, 230, 240, 255);
    constexpr uint32_t TextSecondary = ACE_COL32(140, 140, 160, 255);
    constexpr uint32_t TextDim       = ACE_COL32(80,  80,  100, 255);
    constexpr uint32_t TextAccent    = ACE_COL32(59,  130, 246, 255);

    // Borders
    constexpr uint32_t Border        = ACE_COL32(40,  40,  55,  255);
    constexpr uint32_t BorderHover   = ACE_COL32(59,  130, 246, 180);
    constexpr uint32_t BorderActive  = ACE_COL32(59,  130, 246, 255);

    // Rarity
    constexpr uint32_t RarityConsumer   = ACE_COL32(176, 195, 217, 255);
    constexpr uint32_t RarityIndustrial = ACE_COL32(94,  152, 217, 255);
    constexpr uint32_t RarityMilSpec    = ACE_COL32(75,  105, 255, 255);
    constexpr uint32_t RarityRestricted = ACE_COL32(136, 71,  255, 255);
    constexpr uint32_t RarityClassified = ACE_COL32(211, 44,  230, 255);
    constexpr uint32_t RarityCovert     = ACE_COL32(235, 75,  75,  255);
    constexpr uint32_t RarityContraband = ACE_COL32(228, 174, 57,  255);
    constexpr uint32_t RarityGold       = ACE_COL32(255, 215, 0,   255);

    inline uint32_t GetRarityColor(int rarity) {
        switch (rarity) {
            case 1: return RarityConsumer;
            case 2: return RarityIndustrial;
            case 3: return RarityMilSpec;
            case 4: return RarityRestricted;
            case 5: return RarityClassified;
            case 6: return RarityCovert;
            case 7: return RarityContraband;
            default: return RarityMilSpec;
        }
    }

    inline const char* GetRarityName(int rarity) {
        switch (rarity) {
            case 1: return "Consumer";
            case 2: return "Industrial";
            case 3: return "Mil-Spec";
            case 4: return "Restricted";
            case 5: return "Classified";
            case 6: return "Covert";
            case 7: return "Contraband";
            default: return "Unknown";
        }
    }
}

// ============================================================================
// ANIMATION SYSTEM
// ============================================================================
namespace ACEAnim {
    inline float EaseInOutCubic(float t) {
        return t < 0.5f ? 4*t*t*t : 1 - powf(-2*t + 2, 3) / 2;
    }

    inline float SmoothLerp(float current, float target, float speed, float dt) {
        return current + (target - current) * (1.0f - expf(-speed * dt));
    }

    inline float Pulse(float time, float speed = 2.0f) {
        return (sinf(time * speed) + 1.0f) * 0.5f;
    }
}

// ============================================================================
// INPUT SYSTEM
// ============================================================================
struct ACEInputState {
    float mouseX = 0, mouseY = 0;
    bool  mouseDown[3] = {};           // current frame
    bool  mouseClicked[3] = {};        // transition down this frame
    bool  mouseReleased[3] = {};       // transition up this frame
    float mouseWheel = 0;

    bool  keyDown[256] = {};
    bool  keyPressed[256] = {};        // transition this frame
    char  inputChars[32] = {};
    int   inputCharCount = 0;

    // Previous frame
    bool  _prevMouseDown[3] = {};
    bool  _prevKeyDown[256] = {};

    // Call at start of frame to compute transitions
    void NewFrame() {
        for (int i = 0; i < 3; i++) {
            mouseClicked[i]  = mouseDown[i] && !_prevMouseDown[i];
            mouseReleased[i] = !mouseDown[i] && _prevMouseDown[i];
        }
        for (int i = 0; i < 256; i++) {
            keyPressed[i] = keyDown[i] && !_prevKeyDown[i];
        }
    }

    // Call at end of frame to copy current -> prev
    void EndFrame() {
        memcpy(_prevMouseDown, mouseDown, sizeof(mouseDown));
        memcpy(_prevKeyDown, keyDown, sizeof(keyDown));
        mouseWheel = 0;
        inputCharCount = 0;
    }

    bool IsMouseInRect(float x1, float y1, float x2, float y2) const {
        return mouseX >= x1 && mouseY >= y1 && mouseX < x2 && mouseY < y2;
    }
};

// ============================================================================
// UI CONTEXT — IMMEDIATE MODE
// ============================================================================
class ACEUIContext {
public:
    // Drawing
    ACEDrawList drawList;
    ACEDrawList overlayDrawList;  // for notifications, tooltips

    // Input
    ACEInputState input;

    // Time
    float  deltaTime = 0.016f;
    double time = 0;

    // Display
    float displayW = 1920, displayH = 1080;

    // Hot/Active/Focus tracking
    uint32_t hotItem = 0;
    uint32_t activeItem = 0;
    uint32_t focusedItem = 0;

    // Persistent animation states
    std::unordered_map<uint32_t, float> animState;

    // Window/layout state
    struct WindowState {
        float x, y, w, h;           // window position and size
        float cursorX, cursorY;      // current cursor position within window
        float contentW;              // available content width
        float scrollY;
        float maxScrollY;
        bool  sameLineActive;
        float sameLineX;
        float sameLineSpacing;
        float startCursorY;          // for child regions
        float clipX1, clipY1, clipX2, clipY2;  // active clip rect
    };
    WindowState wnd;

    // Child window stack
    struct ChildState {
        WindowState savedWnd;
    };
    std::vector<ChildState> childStack;

    // Dragging state
    bool   isDragging = false;
    float  dragOffX = 0, dragOffY = 0;

    // ---- LIFECYCLE ----
    void NewFrame(float displayW, float displayH, float deltaTime);
    void EndFrame();

    // ---- ID SYSTEM ----
    uint32_t GetID(const char* label);

    // ---- ANIMATION ----
    float& Anim(uint32_t id, float defaultVal = 0);
    float  SmoothAnim(uint32_t id, float target, float speed = 10.0f);

    // ---- WINDOW ----
    void BeginWindow(const char* title, float x, float y, float w, float h, bool draggable = true);
    void EndWindow();

    // ---- LAYOUT ----
    void SetCursorPos(float x, float y);
    void SetCursorPosX(float x);
    ACEVec2 GetCursorPos() const;
    ACEVec2 GetCursorScreenPos() const;
    void SameLine(float spacing = 8.0f);
    void Dummy(float w, float h);
    float GetContentWidth() const;

    // ---- CHILD REGION (scrollable) ----
    void BeginChild(const char* id, float w, float h);
    void EndChild();

    // ---- WIDGETS ----
    // Text
    void Text(uint32_t color, const char* fmt, ...);
    void TextShadow(uint32_t color, const char* text);

    // Buttons
    bool Button(const char* label, float w = 0, float h = 32, uint32_t color = ACETheme::AccentBlue);
    bool SidebarItem(const char* icon, const char* label, bool active, float width = 200);
    bool TabButton(const char* label, bool active);

    // Toggle
    bool ToggleSwitch(const char* label, bool* value);

    // Sliders
    bool WearSlider(const char* label, float* wear);
    bool SliderFloat(const char* label, float* v, float vMin, float vMax);

    // Input
    bool InputText(const char* label, char* buf, int bufSize, float width = 300);
    bool InputInt(const char* label, int* v, int step = 1, int fastStep = 10, float width = 100);

    // Skin-specific widgets
    bool SkinCard(const char* weaponName, const char* skinName, int rarity, bool equipped, float w = 160, float h = 120);

    // Decorative
    void SectionHeader(const char* title);
    void GradientSeparator();
    void Spinner(float radius = 10.0f, float thickness = 3.0f);
    void StatDisplay(const char* label, const char* value, uint32_t valueColor = ACETheme::TextPrimary);
    void DrawNotification(const char* message, float alpha);
    void GlowRect(float x1, float y1, float x2, float y2, uint32_t color, float rounding = 6, float glowSize = 4);

private:
    // Internal helpers
    bool _IsHovered(float x1, float y1, float x2, float y2);
    bool _IsClicked(float x1, float y1, float x2, float y2, uint32_t id);
    void _AdvanceCursor(float itemW, float itemH);
};

// ============================================================================
// WNDPROC HANDLER — feeds input to ACEUIContext
// ============================================================================
// Call from your WndProc hook. Returns true if input was consumed.
bool ACEProcessInput(ACEUIContext& ctx, void* hwnd, unsigned int msg, unsigned long long wParam, long long lParam);
