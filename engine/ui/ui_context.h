/**
 * ACE Engine — UI Context
 * Central manager for the widget tree. Hybrid immediate + retained mode.
 * Handles event dispatch, layout computation, drawing, focus, tooltips.
 */

#pragma once

#include "widget.h"
#include "theme.h"
#include "animation.h"
#include "layout.h"
#include "../input/input_system.h"
#include "../render/render_backend.h"
#include "../render/font_atlas.h"
#include "../core/memory.h"

#include <stack>

namespace ace {

// ============================================================================
// IMMEDIATE MODE STATE — For hybrid im/retained API
// ============================================================================
struct ImmediateState {
    WidgetID hotWidget{0};      // Currently hovered
    WidgetID activeWidget{0};   // Currently pressed/active
    WidgetID focusedWidget{0};  // Has keyboard focus
    Vec2     mousePos;
    bool     mouseDown{false};
};

// ============================================================================
// TOOLTIP STATE
// ============================================================================
struct TooltipState {
    std::string text;
    Vec2        position;
    f32         hoverTimer{0};
    f32         showDelay{0.5f};
    bool        visible{false};
    WidgetID    ownerWidget{0};
};

// ============================================================================
// UI CONTEXT — Main entry point for all UI operations
// ============================================================================
class UIContext {
public:
    UIContext();
    ~UIContext() = default;

    // --- Initialization ---
    void Initialize(IRenderBackend* backend, u32 viewportW, u32 viewportH);

    // --- Frame lifecycle ---
    void BeginFrame(f32 deltaTime, const InputSystem& input);
    void EndFrame();

    // --- Root widget access ---
    Widget* GetRoot() { return &_root; }
    const Widget* GetRoot() const { return &_root; }

    // --- Retained mode API: Build widget tree ---
    template<typename T, typename... Args>
    T* CreateWidget(std::string_view name, Args&&... args) {
        auto widget = std::make_shared<T>(std::forward<Args>(args)...);
        widget->SetName(name);
        return static_cast<T*>(_root.AddChild(std::move(widget)));
    }

    // --- Immediate mode API ---
    bool Button(WidgetID id, Rect bounds, std::string_view label);
    bool Checkbox(WidgetID id, Rect bounds, bool& value, std::string_view label);
    f32  Slider(WidgetID id, Rect bounds, f32 value, f32 min, f32 max);
    void Label(Rect bounds, std::string_view text, Color color = Color::White());
    void Panel(Rect bounds, Color color);
    bool TextInput(WidgetID id, Rect bounds, std::string& text);

    // --- Focus management ---
    void SetFocus(Widget* widget);
    Widget* GetFocusedWidget() const { return _focusedWidget; }

    // --- Systems access ---
    DrawList&        GetDrawList()       { return _drawList; }
    ThemeEngine&     GetTheme()          { return _theme; }
    AnimationSystem& GetAnimations()     { return _animations; }
    FontAtlas&       GetFontAtlas()      { return _fontAtlas; }
    ArenaAllocator&  GetFrameAllocator() { return _frameArena; }

    const ThemeEngine& GetTheme()  const { return _theme; }
    const FontAtlas&   GetFontAtlas() const { return _fontAtlas; }

    // --- Viewport ---
    Vec2 GetViewportSize() const { return {(f32)_viewportW, (f32)_viewportH}; }
    void SetViewportSize(u32 w, u32 h) { _viewportW = w; _viewportH = h; }

    // --- Font ---
    u32 GetDefaultFont() const { return _defaultFont; }
    void SetDefaultFont(u32 fontId) { _defaultFont = fontId; }

    // --- Rendering ---
    IRenderBackend* GetBackend() const { return _backend; }

    // --- Immediate mode state ---
    const ImmediateState& GetImState() const { return _imState; }

    // --- Tooltip ---
    void ShowTooltip(std::string_view text, Vec2 pos);

    // --- Drag state ---
    bool IsDragging() const { return _dragActive; }
    Vec2 GetDragDelta() const { return _dragDelta; }

private:
    void ProcessInput(const InputSystem& input);
    void UpdateWidgetTree(Widget* widget, f32 dt);
    void DrawWidgetTree(Widget* widget);
    void DrawTooltip();

    // Core
    Widget           _root{"Root"};
    DrawList         _drawList;
    ThemeEngine      _theme;
    AnimationSystem  _animations;
    FontAtlas        _fontAtlas;
    ArenaAllocator   _frameArena{256 * 1024};
    IRenderBackend*  _backend{nullptr};

    // Immediate mode
    ImmediateState   _imState;

    // Focus
    Widget*          _focusedWidget{nullptr};

    // Tooltip
    TooltipState     _tooltip;

    // Drag
    bool             _dragActive{false};
    Vec2             _dragStart;
    Vec2             _dragDelta;

    // Viewport
    u32              _viewportW{1280};
    u32              _viewportH{720};
    f32              _deltaTime{0};

    // Font
    u32              _defaultFont{0};
};

} // namespace ace
