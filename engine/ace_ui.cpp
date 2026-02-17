/*
 * ACE UI — Implementation
 * Component tree, layout engine, animation, input processing, all widgets.
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "ace_ui.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// INPUT PROCESSING (WndProc handler)
// ============================================================================
bool ACEProcessInput(ACEUIContext& ctx, void* hwnd, unsigned int msg, unsigned long long wParam, long long lParam) {
    auto& in = ctx.input;
    switch (msg) {
    case WM_MOUSEMOVE:
        in.mouseX = (float)LOWORD(lParam);
        in.mouseY = (float)HIWORD(lParam);
        return true;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        in.mouseDown[0] = true;
        SetCapture((HWND)hwnd);
        return true;
    case WM_LBUTTONUP:
        in.mouseDown[0] = false;
        ReleaseCapture();
        return true;
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
        in.mouseDown[1] = true;
        return true;
    case WM_RBUTTONUP:
        in.mouseDown[1] = false;
        return true;
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
        in.mouseDown[2] = true;
        return true;
    case WM_MBUTTONUP:
        in.mouseDown[2] = false;
        return true;
    case WM_MOUSEWHEEL:
        in.mouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        return true;
    case WM_KEYDOWN: case WM_SYSKEYDOWN:
        if (wParam < 256) in.keyDown[(int)wParam] = true;
        return true;
    case WM_KEYUP: case WM_SYSKEYUP:
        if (wParam < 256) in.keyDown[(int)wParam] = false;
        return true;
    case WM_CHAR:
        if (wParam > 0 && wParam < 0x10000 && in.inputCharCount < 31) {
            in.inputChars[in.inputCharCount++] = (char)wParam;
            in.inputChars[in.inputCharCount] = 0;
        }
        return true;
    }
    return false;
}

// ============================================================================
// UI CONTEXT LIFECYCLE
// ============================================================================
void ACEUIContext::NewFrame(float dw, float dh, float dt) {
    displayW = dw;
    displayH = dh;
    deltaTime = dt;
    time += dt;

    input.NewFrame();
    drawList.Clear();
    overlayDrawList.Clear();
    hotItem = 0;
}

void ACEUIContext::EndFrame() {
    // Release active if mouse is not down
    if (!input.mouseDown[0]) {
        activeItem = 0;
        isDragging = false;
    }
    input.EndFrame();
}

// ============================================================================
// ID SYSTEM — FNV-1a hash
// ============================================================================
uint32_t ACEUIContext::GetID(const char* label) {
    uint32_t hash = 2166136261u;
    while (*label) {
        hash ^= (uint32_t)*label++;
        hash *= 16777619u;
    }
    return hash ? hash : 1; // avoid 0
}

// ============================================================================
// ANIMATION
// ============================================================================
float& ACEUIContext::Anim(uint32_t id, float defaultVal) {
    auto it = animState.find(id);
    if (it == animState.end()) {
        animState[id] = defaultVal;
        return animState[id];
    }
    return it->second;
}

float ACEUIContext::SmoothAnim(uint32_t id, float target, float speed) {
    float& val = Anim(id, target);
    val = ACEAnim::SmoothLerp(val, target, speed, deltaTime);
    return val;
}

// ============================================================================
// WINDOW
// ============================================================================
void ACEUIContext::BeginWindow(const char* title, float x, float y, float w, float h, bool draggable) {
    wnd.x = x; wnd.y = y; wnd.w = w; wnd.h = h;
    wnd.cursorX = 0; wnd.cursorY = 0;
    wnd.contentW = w;
    wnd.scrollY = 0; wnd.maxScrollY = 0;
    wnd.sameLineActive = false;
    wnd.clipX1 = x; wnd.clipY1 = y;
    wnd.clipX2 = x + w; wnd.clipY2 = y + h;

    // Dragging
    if (draggable) {
        float titleBarH = 30;
        bool inTitleBar = input.IsMouseInRect(x, y, x + w, y + titleBarH);
        if (inTitleBar && input.mouseClicked[0] && activeItem == 0) {
            isDragging = true;
            dragOffX = input.mouseX - x;
            dragOffY = input.mouseY - y;
        }
        if (isDragging && input.mouseDown[0]) {
            wnd.x = input.mouseX - dragOffX;
            wnd.y = input.mouseY - dragOffY;
        }
    }

    drawList.PushClipRect(wnd.x, wnd.y, wnd.x + wnd.w, wnd.y + wnd.h);

    // Window background
    drawList.AddRectFilled(wnd.x, wnd.y, wnd.x + wnd.w, wnd.y + wnd.h, ACETheme::WindowBg, 12.0f);
    drawList.AddRect(wnd.x, wnd.y, wnd.x + wnd.w, wnd.y + wnd.h, ACETheme::Border, 12.0f);
}

void ACEUIContext::EndWindow() {
    drawList.PopClipRect();
}

// ============================================================================
// LAYOUT
// ============================================================================
void ACEUIContext::SetCursorPos(float x, float y) {
    wnd.cursorX = x; wnd.cursorY = y;
    wnd.sameLineActive = false;
}

void ACEUIContext::SetCursorPosX(float x) {
    wnd.cursorX = x;
    wnd.sameLineActive = false;
}

ACEVec2 ACEUIContext::GetCursorPos() const {
    return { wnd.cursorX, wnd.cursorY };
}

ACEVec2 ACEUIContext::GetCursorScreenPos() const {
    return { wnd.x + wnd.cursorX, wnd.y + wnd.cursorY };
}

void ACEUIContext::SameLine(float spacing) {
    wnd.sameLineActive = true;
    wnd.sameLineSpacing = spacing;
}

void ACEUIContext::Dummy(float w, float h) {
    _AdvanceCursor(w, h);
}

float ACEUIContext::GetContentWidth() const {
    return wnd.contentW - wnd.cursorX;
}

void ACEUIContext::_AdvanceCursor(float itemW, float itemH) {
    if (wnd.sameLineActive) {
        wnd.cursorX += itemW + wnd.sameLineSpacing;
        wnd.sameLineActive = false;
    } else {
        wnd.cursorY += itemH + 6.0f; // default item spacing
    }
}

// ============================================================================
// CHILD REGION (scrollable sub-area)
// ============================================================================
void ACEUIContext::BeginChild(const char* id, float w, float h) {
    ChildState cs;
    cs.savedWnd = wnd;
    childStack.push_back(cs);

    float cx = wnd.x + wnd.cursorX;
    float cy = wnd.y + wnd.cursorY;
    if (w <= 0) w = wnd.contentW - wnd.cursorX;
    if (h <= 0) h = wnd.h - wnd.cursorY;

    // Scroll handling
    uint32_t scrollId = GetID(id);
    float& scroll = Anim(scrollId, 0);
    if (input.IsMouseInRect(cx, cy, cx + w, cy + h)) {
        scroll -= input.mouseWheel * 30.0f;
        if (scroll < 0) scroll = 0;
    }

    drawList.PushClipRect(cx, cy, cx + w, cy + h);

    wnd.x = cx; wnd.y = cy - scroll;
    wnd.w = w; wnd.h = h + scroll;
    wnd.cursorX = 0; wnd.cursorY = 0;
    wnd.contentW = w;
    wnd.startCursorY = 0;
    wnd.scrollY = scroll;
    wnd.clipX1 = cx; wnd.clipY1 = cy;
    wnd.clipX2 = cx + w; wnd.clipY2 = cy + h;
}

void ACEUIContext::EndChild() {
    drawList.PopClipRect();
    if (!childStack.empty()) {
        auto& cs = childStack.back();
        float childH = wnd.h;
        wnd = cs.savedWnd;
        childStack.pop_back();
        _AdvanceCursor(0, childH > 0 ? childH : 0);
    }
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================
bool ACEUIContext::_IsHovered(float x1, float y1, float x2, float y2) {
    return input.IsMouseInRect(x1, y1, x2, y2) &&
           input.IsMouseInRect(wnd.clipX1, wnd.clipY1, wnd.clipX2, wnd.clipY2);
}

bool ACEUIContext::_IsClicked(float x1, float y1, float x2, float y2, uint32_t id) {
    bool hovered = _IsHovered(x1, y1, x2, y2);
    if (hovered) hotItem = id;

    if (hovered && input.mouseClicked[0] && (activeItem == 0 || activeItem == id)) {
        activeItem = id;
    }

    bool clicked = false;
    if (activeItem == id && input.mouseReleased[0]) {
        if (hovered) clicked = true;
        activeItem = 0;
    }
    return clicked;
}

// ============================================================================
// TEXT WIDGETS
// ============================================================================
void ACEUIContext::Text(uint32_t color, const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    ACEVec2 pos = GetCursorScreenPos();
    drawList.AddText(pos.x, pos.y, color, buf);
    ACEVec2 ts = drawList.font ? drawList.font->CalcTextSize(buf) : ACEVec2(0, 16);
    _AdvanceCursor(ts.x, ts.y);
}

void ACEUIContext::TextShadow(uint32_t color, const char* text) {
    ACEVec2 pos = GetCursorScreenPos();
    drawList.AddTextShadow(pos.x, pos.y, color, text);
    ACEVec2 ts = drawList.font ? drawList.font->CalcTextSize(text) : ACEVec2(0, 16);
    _AdvanceCursor(ts.x, ts.y);
}

// ============================================================================
// BUTTON WIDGETS
// ============================================================================
bool ACEUIContext::Button(const char* label, float w, float h, uint32_t color) {
    uint32_t id = GetID(label);
    ACEVec2 pos = GetCursorScreenPos();

    ACEVec2 ts = drawList.font ? drawList.font->CalcTextSize(label) : ACEVec2(60, 14);
    if (w <= 0) w = ts.x + 24.0f;
    if (h <= 0) h = 32.0f;

    float x1 = pos.x, y1 = pos.y, x2 = pos.x + w, y2 = pos.y + h;
    bool hovered = _IsHovered(x1, y1, x2, y2);
    bool clicked = _IsClicked(x1, y1, x2, y2, id);
    bool held = (activeItem == id && input.mouseDown[0]);

    // Animation
    float hover = SmoothAnim(id, hovered ? 1.0f : 0.0f, 10.0f);

    // Background
    uint32_t bgCol = color;
    if (held) {
        bgCol = ACE_COL32(
            (int)(ACE_COL32_R(color) * 0.8f),
            (int)(ACE_COL32_G(color) * 0.8f),
            (int)(ACE_COL32_B(color) * 0.8f),
            ACE_COL32_A(color)
        );
    }
    drawList.AddRectFilled(x1, y1, x2, y2, bgCol, 6.0f);

    // Hover glow
    if (hover > 0.01f) {
        GlowRect(x1, y1, x2, y2, color, 6.0f, 3.0f * hover);
    }

    // Shine
    uint32_t shine = ACE_COL32(255, 255, 255, (int)(20 + 15 * hover));
    drawList.AddRectFilled(x1, y1, x2, y1 + h * 0.45f, shine, 6.0f);

    // Text (centered)
    drawList.AddTextCentered(x1, y1, x2, y2, ACE_COL32_WHITE, label);

    _AdvanceCursor(w, h);
    return clicked;
}

bool ACEUIContext::SidebarItem(const char* icon, const char* label, bool active, float width) {
    char idStr[256];
    snprintf(idStr, sizeof(idStr), "sb_%s", label);
    uint32_t id = GetID(idStr);

    ACEVec2 pos = GetCursorScreenPos();
    float h = 40.0f;
    float x1 = pos.x, y1 = pos.y, x2 = pos.x + width, y2 = pos.y + h;

    bool hovered = _IsHovered(x1, y1, x2, y2);
    bool clicked = _IsClicked(x1, y1, x2, y2, id);

    float anim = SmoothAnim(id, (hovered || active) ? 1.0f : 0.0f, 10.0f);

    // Background
    if (anim > 0.01f) {
        uint32_t bg = ACE_COL32(59, 130, 246, (int)(20 * anim));
        drawList.AddRectFilled(x1, y1, x2, y2, bg, 6.0f);
    }

    // Active indicator
    if (active) {
        drawList.AddRectFilled(x1, y1 + 6, x1 + 3, y2 - 6, ACETheme::AccentBlue, 2.0f);
    }

    // Icon + label
    float textY = y1 + (h - (drawList.font ? drawList.font->lineHeight : 14)) * 0.5f;
    drawList.AddText(x1 + 16, textY, active ? ACETheme::AccentBlue : ACETheme::TextSecondary, icon);
    drawList.AddText(x1 + 40, textY, active ? ACETheme::TextPrimary : ACETheme::TextSecondary, label);

    _AdvanceCursor(width, h);
    return clicked;
}

bool ACEUIContext::TabButton(const char* label, bool active) {
    uint32_t id = GetID(label);
    ACEVec2 ts = drawList.font ? drawList.font->CalcTextSize(label) : ACEVec2(60, 14);
    float w = ts.x + 24.0f, h = 36.0f;

    ACEVec2 pos = GetCursorScreenPos();
    float x1 = pos.x, y1 = pos.y, x2 = pos.x + w, y2 = pos.y + h;

    bool hovered = _IsHovered(x1, y1, x2, y2);
    bool clicked = _IsClicked(x1, y1, x2, y2, id);

    float anim = SmoothAnim(id, active ? 1.0f : 0.0f, 12.0f);

    // Hover BG
    if (hovered && !active)
        drawList.AddRectFilled(x1, y1, x2, y2, ACE_COL32(255, 255, 255, 8), 4.0f);

    // Text
    uint32_t textCol = active ? ACETheme::TextPrimary : (hovered ? ACETheme::TextSecondary : ACETheme::TextDim);
    drawList.AddTextCentered(x1, y1, x2, y2, textCol, label);

    // Animated underline
    if (anim > 0.01f) {
        float lineW = ts.x * anim;
        float ctrX = x1 + w * 0.5f;
        drawList.AddRectFilled(ctrX - lineW * 0.5f, y2 - 3, ctrX + lineW * 0.5f, y2 - 1, ACETheme::AccentBlue);
    }

    _AdvanceCursor(w, h);
    return clicked;
}

// ============================================================================
// TOGGLE SWITCH
// ============================================================================
bool ACEUIContext::ToggleSwitch(const char* label, bool* value) {
    uint32_t id = GetID(label);
    ACEVec2 pos = GetCursorScreenPos();
    float switchW = 40.0f, switchH = 20.0f;
    float radius = switchH * 0.5f;

    float x1 = pos.x, y1 = pos.y, x2 = pos.x + switchW, y2 = pos.y + switchH;
    bool hovered = _IsHovered(x1, y1, x2, y2);
    bool clicked = _IsClicked(x1, y1, x2, y2, id);

    if (clicked) *value = !*value;

    float anim = SmoothAnim(id, *value ? 1.0f : 0.0f, 12.0f);

    // Track
    uint32_t trackCol = *value ? ACETheme::AccentBlue : ACETheme::InputBg;
    if (hovered) trackCol = *value ? ACE_COL32(79, 150, 255, 255) : ACETheme::CardBgHover;
    drawList.AddRectFilled(x1, y1, x2, y2, trackCol, radius);
    drawList.AddRect(x1, y1, x2, y2, ACETheme::Border, radius);

    // Thumb
    float thumbX = x1 + radius + anim * (switchW - switchH);
    float thumbCY = y1 + radius;
    drawList.AddCircleFilled(thumbX, thumbCY, radius - 3, ACE_COL32(255, 255, 255, 240));

    // Glow when on
    if (*value) {
        for (int i = 0; i < 3; i++) {
            drawList.AddCircleFilled(thumbX, thumbCY, radius + i * 3.0f,
                                      ACE_COL32(59, 130, 246, (int)(30 * (1.0f - i * 0.3f))));
        }
    }

    // Label
    ACEVec2 ts = drawList.font ? drawList.font->CalcTextSize(label) : ACEVec2(60, 14);
    drawList.AddText(x2 + 8, y1 + (switchH - ts.y) * 0.5f, ACETheme::TextPrimary, label);

    _AdvanceCursor(switchW + 8 + ts.x, switchH);
    return clicked;
}

// ============================================================================
// WEAR SLIDER (with FN/MW/FT/WW/BS zones)
// ============================================================================
bool ACEUIContext::WearSlider(const char* label, float* wear) {
    uint32_t id = GetID(label);
    ACEVec2 pos = GetCursorScreenPos();
    float width = GetContentWidth() - 20;
    float totalH = 40.0f;

    float x1 = pos.x, y1 = pos.y;
    float trackY = y1 + 8, trackH = 14.0f;

    // Hit test
    float hitX1 = x1, hitY1 = y1, hitX2 = x1 + width, hitY2 = y1 + totalH;
    bool hovered = _IsHovered(hitX1, hitY1, hitX2, hitY2);
    bool held = false;

    if (hovered && input.mouseClicked[0] && (activeItem == 0 || activeItem == id))
        activeItem = id;

    if (activeItem == id) {
        held = true;
        *wear = std::clamp((input.mouseX - x1) / width, 0.0f, 1.0f);
        if (!input.mouseDown[0]) activeItem = 0;
    }

    // Wear zone data
    struct Zone { float start, end; const char* name; uint32_t color; };
    Zone zones[] = {
        { 0.000f, 0.070f, "FN", ACE_COL32(74,  222, 128, 255) },
        { 0.070f, 0.150f, "MW", ACE_COL32(59,  130, 246, 255) },
        { 0.150f, 0.380f, "FT", ACE_COL32(250, 204, 21,  255) },
        { 0.380f, 0.450f, "WW", ACE_COL32(251, 146, 60,  255) },
        { 0.450f, 1.000f, "BS", ACE_COL32(248, 113, 113, 255) },
    };

    // Draw zones
    for (auto& z : zones) {
        float zx1 = x1 + z.start * width;
        float zx2 = x1 + z.end * width;
        drawList.AddRectFilled(zx1, trackY, zx2, trackY + trackH, z.color);
        ACEVec2 ls = drawList.font ? drawList.font->CalcTextSize(z.name) : ACEVec2(16, 10);
        if (zx2 - zx1 > ls.x + 4) {
            float lx = zx1 + (zx2 - zx1 - ls.x) * 0.5f;
            drawList.AddText(lx, trackY + 1, ACE_COL32(0, 0, 0, 200), z.name);
        }
    }

    // Track border
    drawList.AddRect(x1, trackY, x1 + width, trackY + trackH, ACETheme::Border, 2.0f);

    // Thumb
    float thumbX = x1 + (*wear) * width;
    drawList.AddLine(thumbX, trackY - 2, thumbX, trackY + trackH + 2, ACE_COL32_WHITE, 2.0f);
    drawList.AddCircleFilled(thumbX, trackY + trackH * 0.5f, 6.0f, ACE_COL32_WHITE);
    drawList.AddCircle(thumbX, trackY + trackH * 0.5f, 6.0f, ACETheme::AccentBlue, 0, 2.0f);

    // Value label
    const char* condLabel = "BS";
    for (auto& z : zones) {
        if (*wear >= z.start && *wear < z.end) { condLabel = z.name; break; }
    }
    char valBuf[64];
    snprintf(valBuf, sizeof(valBuf), "%.4f (%s)", *wear, condLabel);
    ACEVec2 vs = drawList.font ? drawList.font->CalcTextSize(valBuf) : ACEVec2(80, 14);
    drawList.AddText(x1 + width - vs.x, y1 + trackH + 12, ACETheme::TextSecondary, valBuf);
    drawList.AddText(x1, y1 + trackH + 12, ACETheme::TextPrimary, label);

    _AdvanceCursor(width, totalH);
    return held;
}

bool ACEUIContext::SliderFloat(const char* label, float* v, float vMin, float vMax) {
    uint32_t id = GetID(label);
    ACEVec2 pos = GetCursorScreenPos();
    float width = GetContentWidth() - 20;
    float h = 24.0f;

    float x1 = pos.x, y1 = pos.y;
    bool hovered = _IsHovered(x1, y1, x1 + width, y1 + h);

    if (hovered && input.mouseClicked[0]) activeItem = id;
    if (activeItem == id) {
        float t = std::clamp((input.mouseX - x1) / width, 0.0f, 1.0f);
        *v = vMin + t * (vMax - vMin);
        if (!input.mouseDown[0]) activeItem = 0;
    }

    float t = std::clamp((*v - vMin) / (vMax - vMin), 0.0f, 1.0f);
    drawList.AddRectFilled(x1, y1, x1 + width, y1 + h, ACETheme::InputBg, 4.0f);
    drawList.AddRectFilled(x1, y1, x1 + width * t, y1 + h, ACETheme::AccentBlueDim, 4.0f);

    char buf[64];
    snprintf(buf, sizeof(buf), "%s: %.2f", label, *v);
    drawList.AddTextCentered(x1, y1, x1 + width, y1 + h, ACETheme::TextPrimary, buf);

    _AdvanceCursor(width, h);
    return activeItem == id;
}

// ============================================================================
// INPUT TEXT
// ============================================================================
bool ACEUIContext::InputText(const char* label, char* buf, int bufSize, float width) {
    uint32_t id = GetID(label);
    ACEVec2 pos = GetCursorScreenPos();
    float h = 32.0f;

    float x1 = pos.x, y1 = pos.y, x2 = pos.x + width, y2 = pos.y + h;
    bool hovered = _IsHovered(x1, y1, x2, y2);

    if (hovered && input.mouseClicked[0]) focusedItem = id;
    if (input.mouseClicked[0] && !hovered && focusedItem == id) focusedItem = 0;

    bool focused = (focusedItem == id);
    bool changed = false;

    // Handle text input when focused
    if (focused) {
        int len = (int)strlen(buf);
        // Backspace
        if (input.keyPressed[VK_BACK] && len > 0) {
            buf[len - 1] = 0;
            changed = true;
        }
        // Character input
        for (int i = 0; i < input.inputCharCount; i++) {
            char c = input.inputChars[i];
            if (c >= 32 && c < 127 && len < bufSize - 1) {
                buf[len] = c;
                buf[len + 1] = 0;
                changed = true;
                len++;
            }
        }
    }

    // Background
    uint32_t bgCol = focused ? ACETheme::CardBgActive : ACETheme::InputBg;
    uint32_t borderCol = focused ? ACETheme::AccentBlue : (hovered ? ACETheme::BorderHover : ACETheme::Border);
    drawList.AddRectFilled(x1, y1, x2, y2, bgCol, 6.0f);
    drawList.AddRect(x1, y1, x2, y2, borderCol, 6.0f);

    // Search icon
    drawList.AddText(x1 + 8, y1 + 8, ACETheme::TextDim, "?");

    // Text content
    if (buf[0] != '\0') {
        drawList.AddText(x1 + 28, y1 + 8, ACETheme::TextPrimary, buf);
    } else if (!focused) {
        drawList.AddText(x1 + 28, y1 + 8, ACETheme::TextDim, "Search...");
    }

    // Cursor blink
    if (focused) {
        float blink = ACEAnim::Pulse((float)time, 3.0f);
        if (blink > 0.5f) {
            ACEVec2 ts = drawList.font ? drawList.font->CalcTextSize(buf) : ACEVec2(0, 14);
            drawList.AddRectFilled(x1 + 28 + ts.x, y1 + 6, x1 + 29 + ts.x, y2 - 6, ACETheme::TextPrimary);
        }
    }

    _AdvanceCursor(width, h);
    return changed;
}

bool ACEUIContext::InputInt(const char* label, int* v, int step, int fastStep, float width) {
    uint32_t id = GetID(label);
    ACEVec2 pos = GetCursorScreenPos();
    float h = 28.0f;

    // Total width: [- btn] [value field] [+ btn]
    float btnW = 24.0f;
    float fieldW = width - btnW * 2 - 4;

    // Minus button
    {
        float bx = pos.x, by = pos.y;
        bool hov = _IsHovered(bx, by, bx + btnW, by + h);
        bool clk = _IsClicked(bx, by, bx + btnW, by + h, GetID("-btn"));
        drawList.AddRectFilled(bx, by, bx + btnW, by + h, hov ? ACETheme::CardBgHover : ACETheme::InputBg, 4.0f);
        drawList.AddTextCentered(bx, by, bx + btnW, by + h, ACETheme::TextPrimary, "-");
        if (clk) *v -= step;
    }

    // Value field
    {
        float fx = pos.x + btnW + 2, fy = pos.y;
        drawList.AddRectFilled(fx, fy, fx + fieldW, fy + h, ACETheme::InputBg, 4.0f);
        char valBuf[32];
        snprintf(valBuf, sizeof(valBuf), "%d", *v);
        drawList.AddTextCentered(fx, fy, fx + fieldW, fy + h, ACETheme::TextPrimary, valBuf);
    }

    // Plus button
    {
        float bx = pos.x + btnW + 2 + fieldW + 2, by = pos.y;
        bool hov = _IsHovered(bx, by, bx + btnW, by + h);
        bool clk = _IsClicked(bx, by, bx + btnW, by + h, GetID("+btn"));
        drawList.AddRectFilled(bx, by, bx + btnW, by + h, hov ? ACETheme::CardBgHover : ACETheme::InputBg, 4.0f);
        drawList.AddTextCentered(bx, by, bx + btnW, by + h, ACETheme::TextPrimary, "+");
        if (clk) *v += step;
    }

    _AdvanceCursor(width, h);
    return false;
}

// ============================================================================
// SKIN CARD
// ============================================================================
bool ACEUIContext::SkinCard(const char* weaponName, const char* skinName, int rarity, bool equipped, float w, float h) {
    char idBuf[256];
    snprintf(idBuf, sizeof(idBuf), "sc_%s_%s", weaponName, skinName);
    uint32_t id = GetID(idBuf);

    ACEVec2 pos = GetCursorScreenPos();
    float x1 = pos.x, y1 = pos.y, x2 = pos.x + w, y2 = pos.y + h;

    bool hovered = _IsHovered(x1, y1, x2, y2);
    bool clicked = _IsClicked(x1, y1, x2, y2, id);

    float anim = SmoothAnim(id, hovered ? 1.0f : 0.0f, 10.0f);
    uint32_t rarityCol = ACETheme::GetRarityColor(rarity);

    // Lift on hover
    float lift = anim * 2.0f;
    float cy1 = y1 - lift, cy2 = y2 - lift;

    // Card background
    drawList.AddRectFilled(x1, cy1, x2, cy2, hovered ? ACETheme::CardBgHover : ACETheme::CardBg, 8.0f);
    drawList.AddRect(x1, cy1, x2, cy2, equipped ? ACETheme::AccentBlue : ACETheme::Border, 8.0f);

    // Rarity bar at bottom
    drawList.AddRectFilled(x1 + 1, cy2 - 4, x2 - 1, cy2 - 1, rarityCol);

    // Equipped glow + badge
    if (equipped) {
        GlowRect(x1, cy1, x2, cy2, ACETheme::AccentBlue, 8.0f, 3.0f);
        float bx = x2 - 24, by = cy1 + 6;
        drawList.AddCircleFilled(bx, by, 8.0f, ACETheme::AccentBlue);
        drawList.AddLine(bx - 3, by, bx - 1, by + 3, ACE_COL32_WHITE, 2.0f);
        drawList.AddLine(bx - 1, by + 3, bx + 4, by - 3, ACE_COL32_WHITE, 2.0f);
    }

    // Weapon icon placeholder (circle with first letter)
    float iconCX = x1 + w * 0.5f, iconCY = cy1 + 35;
    drawList.AddCircleFilled(iconCX, iconCY, 18.0f, ACE_COL32(30, 30, 42, 255));
    drawList.AddCircle(iconCX, iconCY, 18.0f, rarityCol, 0, 1.5f);
    char fl[2] = { weaponName[0], 0 };
    drawList.AddTextCentered(iconCX - 8, iconCY - 7, iconCX + 8, iconCY + 7, rarityCol, fl);

    // Skin name (truncated)
    std::string dispName = skinName;
    if (drawList.font) {
        float maxTW = w - 16;
        while (drawList.font->CalcTextSize(dispName.c_str()).x > maxTW && dispName.size() > 4) {
            dispName = dispName.substr(0, dispName.size() - 4) + "...";
        }
    }
    drawList.AddText(x1 + 8, cy1 + 62, ACETheme::TextPrimary, dispName.c_str());
    drawList.AddText(x1 + 8, cy1 + 80, ACETheme::TextSecondary, weaponName);

    // Rarity text
    drawList.AddText(x1 + 8, cy1 + h - 22 - lift, rarityCol, ACETheme::GetRarityName(rarity));

    _AdvanceCursor(w, h);
    return clicked;
}

// ============================================================================
// DECORATIVE WIDGETS
// ============================================================================
void ACEUIContext::SectionHeader(const char* title) {
    ACEVec2 pos = GetCursorScreenPos();
    float width = GetContentWidth();

    drawList.AddText(pos.x, pos.y, ACETheme::TextPrimary, title);
    ACEVec2 ts = drawList.font ? drawList.font->CalcTextSize(title) : ACEVec2(80, 14);

    float lineY = pos.y + ts.y * 0.5f;
    drawList.AddLine(pos.x + ts.x + 10, lineY, pos.x + width, lineY, ACETheme::Border);

    _AdvanceCursor(width, ts.y + 8);
}

void ACEUIContext::GradientSeparator() {
    ACEVec2 pos = GetCursorScreenPos();
    float width = GetContentWidth();
    float midX = pos.x + width * 0.5f;

    drawList.AddRectFilledMultiColor(pos.x, pos.y, midX, pos.y + 1,
        ACE_COL32(40, 40, 55, 0), ACETheme::Border, ACETheme::Border, ACE_COL32(40, 40, 55, 0));
    drawList.AddRectFilledMultiColor(midX, pos.y, pos.x + width, pos.y + 1,
        ACETheme::Border, ACE_COL32(40, 40, 55, 0), ACE_COL32(40, 40, 55, 0), ACETheme::Border);

    _AdvanceCursor(width, 8);
}

void ACEUIContext::Spinner(float radius, float thickness) {
    ACEVec2 pos = GetCursorScreenPos();
    float cx = pos.x + radius, cy = pos.y + radius;

    float t = (float)time * 4.0f;
    int numSeg = 30;
    for (int i = 0; i < numSeg; i++) {
        float a1 = t + (float)i / numSeg * 2.0f * (float)M_PI;
        float a2 = t + (float)(i + 1) / numSeg * 2.0f * (float)M_PI;
        float alpha = (float)i / numSeg;
        uint32_t col = ACE_COL32(59, 130, 246, (int)(255 * alpha));
        drawList.AddLine(cx + cosf(a1) * radius, cy + sinf(a1) * radius,
                          cx + cosf(a2) * radius, cy + sinf(a2) * radius, col, thickness);
    }
    _AdvanceCursor(radius * 2, radius * 2);
}

void ACEUIContext::StatDisplay(const char* label, const char* value, uint32_t valueColor) {
    ACEVec2 pos = GetCursorScreenPos();
    drawList.AddText(pos.x, pos.y, ACETheme::TextDim, label);
    ACEVec2 ls = drawList.font ? drawList.font->CalcTextSize(label) : ACEVec2(40, 14);
    drawList.AddText(pos.x + ls.x + 8, pos.y, valueColor, value);
    ACEVec2 vs = drawList.font ? drawList.font->CalcTextSize(value) : ACEVec2(80, 14);
    _AdvanceCursor(ls.x + 8 + vs.x, ls.y + 4);
}

void ACEUIContext::DrawNotification(const char* message, float alpha) {
    if (alpha <= 0.01f) return;

    ACEVec2 ts = drawList.font ? drawList.font->CalcTextSize(message) : ACEVec2(200, 14);
    float padX = 20, padY = 10;
    float w = ts.x + padX * 2, h = ts.y + padY * 2;
    float x = displayW * 0.5f - w * 0.5f, y = 40.0f;

    uint32_t bg = ACE_COL32(22, 22, 30, (int)(220 * alpha));
    uint32_t border = ACE_COL32(59, 130, 246, (int)(200 * alpha));
    uint32_t text = ACE_COL32(230, 230, 240, (int)(255 * alpha));

    overlayDrawList.AddRectFilled(x, y, x + w, y + h, bg, 8.0f);
    overlayDrawList.AddRect(x, y, x + w, y + h, border, 8.0f);
    overlayDrawList.AddTextCentered(x, y, x + w, y + h, text, message);
}

void ACEUIContext::GlowRect(float x1, float y1, float x2, float y2, uint32_t color, float rounding, float glowSize) {
    for (int i = (int)glowSize; i > 0; i--) {
        uint32_t glowCol = ACE_COL32(
            ACE_COL32_R(color), ACE_COL32_G(color), ACE_COL32_B(color),
            (int)(15.0f * (1.0f - (float)i / glowSize))
        );
        drawList.AddRect(x1 - i, y1 - i, x2 + i, y2 + i, glowCol, rounding + i, 1.5f);
    }
}
