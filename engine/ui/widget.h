/**
 * ACE Engine — Widget Base Class
 * Foundation for all UI widgets. Supports:
 * - Hierarchical parent/child relationships
 * - Layout properties (flex, padding, margin, alignment)
 * - Animation targets
 * - Hit testing and event propagation
 * - Style overrides per-widget
 */

#pragma once

#include "../core/types.h"
#include "../core/event.h"
#include "../render/render_backend.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace ace {

// Forward declarations
class UIContext;
class ThemeEngine;
struct AnimationState;

// ============================================================================
// WIDGET FLAGS
// ============================================================================
enum class WidgetFlags : u32 {
    None           = 0,
    Visible        = 1 << 0,
    Enabled        = 1 << 1,
    Focusable      = 1 << 2,
    Draggable      = 1 << 3,
    Resizable      = 1 << 4,
    ClipChildren   = 1 << 5,
    CapturesMouse  = 1 << 6,
    CapturesKeyboard = 1 << 7,
    IsHovered      = 1 << 8,
    IsPressed      = 1 << 9,
    IsFocused      = 1 << 10,
    IsDragging     = 1 << 11,
    NeedsLayout    = 1 << 12,
    NeedsRedraw    = 1 << 13,
};

constexpr WidgetFlags operator|(WidgetFlags a, WidgetFlags b) {
    return static_cast<WidgetFlags>(static_cast<u32>(a) | static_cast<u32>(b));
}
constexpr WidgetFlags operator&(WidgetFlags a, WidgetFlags b) {
    return static_cast<WidgetFlags>(static_cast<u32>(a) & static_cast<u32>(b));
}
constexpr WidgetFlags operator~(WidgetFlags a) {
    return static_cast<WidgetFlags>(~static_cast<u32>(a));
}
constexpr bool HasFlag(WidgetFlags val, WidgetFlags flag) {
    return (static_cast<u32>(val) & static_cast<u32>(flag)) != 0;
}

// ============================================================================
// LAYOUT PROPERTIES
// ============================================================================
enum class LayoutMode : u8 {
    None,          // Manual positioning
    FlexRow,       // Horizontal flex
    FlexColumn,    // Vertical flex
    Grid,          // Grid layout
    Stack,         // Overlay stack
    Dock,          // Dock layout (top/bottom/left/right/fill)
};

enum class Alignment : u8 {
    Start, Center, End, Stretch
};

enum class DockSide : u8 {
    None, Top, Bottom, Left, Right, Fill
};

struct LayoutProps {
    LayoutMode  mode{LayoutMode::None};
    Alignment   alignX{Alignment::Start};
    Alignment   alignY{Alignment::Start};
    EdgeInsets  padding;
    EdgeInsets  margin;
    f32         flexGrow{0};
    f32         flexShrink{1};
    f32         flexBasis{-1}; // -1 = auto
    f32         gap{0};
    DockSide    dockSide{DockSide::None};
    Vec2        minSize{0, 0};
    Vec2        maxSize{99999, 99999};
    Vec2        preferredSize{0, 0};
};

// ============================================================================
// STYLE OVERRIDES (per-widget, optional)
// ============================================================================
struct WidgetStyle {
    Color       bgColor{0, 0, 0, 0};
    Color       bgColorHovered{0, 0, 0, 0};
    Color       bgColorPressed{0, 0, 0, 0};
    Color       borderColor{0, 0, 0, 0};
    Color       textColor{255, 255, 255, 255};
    f32         borderWidth{0};
    f32         borderRadius{0};
    f32         opacity{1.0f};
    bool        hasOverride{false};
};

// ============================================================================
// WIDGET — Base class for all UI elements
// ============================================================================
class Widget {
public:
    using Ptr = std::shared_ptr<Widget>;

    Widget(std::string_view name = "Widget");
    virtual ~Widget() = default;

    // --- Identity ---
    WidgetID    GetID()   const { return _id; }
    const std::string& GetName() const { return _name; }
    void        SetName(std::string_view name) { _name = name; _id = HashRuntime(name); }

    // --- Hierarchy ---
    Widget*     GetParent() const { return _parent; }
    const std::vector<Ptr>& GetChildren() const { return _children; }

    Widget* AddChild(Ptr child);
    Widget* RemoveChild(Widget* child);
    void    ClearChildren();
    Widget* FindChild(std::string_view name) const;
    Widget* FindChildRecursive(std::string_view name) const;

    template<typename T, typename... Args>
    T* Add(Args&&... args) {
        auto child = std::make_shared<T>(std::forward<Args>(args)...);
        T* ptr = child.get();
        AddChild(std::move(child));
        return ptr;
    }

    // --- Layout & Bounds ---
    Rect            GetBounds()         const { return _bounds; }
    Rect            GetLocalBounds()    const { return {0, 0, _bounds.w, _bounds.h}; }
    Rect            GetContentBounds()  const;
    Vec2            GetPosition()       const { return _bounds.Pos(); }
    Vec2            GetSize()           const { return _bounds.Size(); }

    void            SetPosition(Vec2 pos) { _bounds.x = pos.x; _bounds.y = pos.y; MarkDirty(); }
    void            SetSize(Vec2 size)    { _bounds.w = size.x; _bounds.h = size.y; MarkDirty(); }
    void            SetBounds(Rect r)     { _bounds = r; MarkDirty(); }

    LayoutProps&       Layout()       { return _layout; }
    const LayoutProps& Layout() const { return _layout; }

    // --- Flags ---
    WidgetFlags GetFlags()                     const { return _flags; }
    bool        HasFlag(WidgetFlags f)         const { return ace::HasFlag(_flags, f); }
    void        SetFlag(WidgetFlags f)               { _flags = _flags | f; }
    void        ClearFlag(WidgetFlags f)             { _flags = _flags & ~f; }
    void        SetVisible(bool v) { v ? SetFlag(WidgetFlags::Visible) : ClearFlag(WidgetFlags::Visible); }
    void        SetEnabled(bool e) { e ? SetFlag(WidgetFlags::Enabled) : ClearFlag(WidgetFlags::Enabled); }
    bool        IsVisible() const  { return HasFlag(WidgetFlags::Visible); }
    bool        IsEnabled() const  { return HasFlag(WidgetFlags::Enabled); }
    bool        IsHovered() const  { return HasFlag(WidgetFlags::IsHovered); }
    bool        IsPressed() const  { return HasFlag(WidgetFlags::IsPressed); }
    bool        IsFocused() const  { return HasFlag(WidgetFlags::IsFocused); }

    // --- Style ---
    WidgetStyle&       Style()       { return _style; }
    const WidgetStyle& Style() const { return _style; }

    // --- Lifecycle (override in subclasses) ---
    virtual void OnUpdate(f32 /*dt*/) {}
    virtual void OnDraw(DrawList& /*drawList*/, const ThemeEngine& /*theme*/) {}
    virtual void OnLayout() {}

    // --- Events (override in subclasses) ---
    virtual bool OnMouseEnter() { return false; }
    virtual bool OnMouseLeave() { return false; }
    virtual bool OnMouseDown(Vec2 /*pos*/, MouseButtonEvent& /*e*/) { return false; }
    virtual bool OnMouseUp(Vec2 /*pos*/, MouseButtonEvent& /*e*/) { return false; }
    virtual bool OnMouseMove(Vec2 /*pos*/, MouseMoveEvent& /*e*/) { return false; }
    virtual bool OnMouseScroll(f32 /*delta*/) { return false; }
    virtual bool OnKeyDown(KeyEvent& /*e*/) { return false; }
    virtual bool OnKeyUp(KeyEvent& /*e*/) { return false; }
    virtual bool OnTextInput(TextInputEvent& /*e*/) { return false; }
    virtual void OnFocusGained() {}
    virtual void OnFocusLost() {}
    virtual void OnDragStart(Vec2 /*pos*/) {}
    virtual void OnDragMove(Vec2 /*pos*/, Vec2 /*delta*/) {}
    virtual void OnDragEnd(Vec2 /*pos*/) {}

    // --- Hit testing ---
    virtual bool HitTest(Vec2 point) const;
    Widget*      HitTestRecursive(Vec2 point);

    // --- Callbacks ---
    using ClickCallback = std::function<void(Widget*)>;
    void SetOnClick(ClickCallback cb) { _onClick = std::move(cb); }

    // --- Dirty tracking ---
    void MarkDirty() { SetFlag(WidgetFlags::NeedsLayout | WidgetFlags::NeedsRedraw); }

    // --- Z-order ---
    i32  GetZOrder()       const { return _zOrder; }
    void SetZOrder(i32 z)        { _zOrder = z; }

    // --- Tooltip ---
    void SetTooltip(std::string_view tip) { _tooltip = tip; }
    std::string_view GetTooltip()   const { return _tooltip; }

protected:
    friend class UIContext;

    void InvokeClick() { if (_onClick) _onClick(this); }

    WidgetID           _id{0};
    std::string        _name;
    Widget*            _parent{nullptr};
    std::vector<Ptr>   _children;
    Rect               _bounds;
    LayoutProps        _layout;
    WidgetStyle        _style;
    WidgetFlags        _flags{WidgetFlags::Visible | WidgetFlags::Enabled};
    i32                _zOrder{0};
    std::string        _tooltip;
    ClickCallback      _onClick;
};

} // namespace ace
