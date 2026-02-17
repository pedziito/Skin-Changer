/**
 * ACE Engine — Button Widget
 * Retained-mode button with hover/press states, icons, and animation.
 */

#pragma once

#include "../widget.h"
#include "../theme.h"
#include "../animation.h"
#include "../../render/font_atlas.h"

namespace ace {

class ButtonWidget : public Widget {
public:
    ButtonWidget(std::string_view label = "Button");

    void SetLabel(std::string_view label) { _label = label; }
    std::string_view GetLabel() const { return _label; }

    void SetFontId(u32 id) { _fontId = id; }

    void OnDraw(DrawList& drawList, const ThemeEngine& theme) override;
    bool OnMouseEnter() override;
    bool OnMouseLeave() override;
    bool OnMouseDown(Vec2 pos, MouseButtonEvent& e) override;
    bool OnMouseUp(Vec2 pos, MouseButtonEvent& e) override;

private:
    std::string     _label;
    u32             _fontId{0};
    SpringAnimation _hoverAnim;
    SpringAnimation _pressAnim;
};

// ============================================================================
// PANEL WIDGET — Container with background and optional title
// ============================================================================
class PanelWidget : public Widget {
public:
    PanelWidget(std::string_view name = "Panel");

    void SetTitle(std::string_view title) { _title = title; }
    void SetCollapsible(bool v) { _collapsible = v; }
    void SetCollapsed(bool v) { _collapsed = v; }
    bool IsCollapsed() const { return _collapsed; }

    void OnDraw(DrawList& drawList, const ThemeEngine& theme) override;
    bool OnMouseDown(Vec2 pos, MouseButtonEvent& e) override;

private:
    std::string _title;
    bool        _collapsible{false};
    bool        _collapsed{false};
    f32         _headerHeight{32.0f};
    u32         _fontId{0};
};

// ============================================================================
// TEXT INPUT WIDGET — Editable text field with cursor
// ============================================================================
class TextInputWidget : public Widget {
public:
    TextInputWidget(std::string_view placeholder = "");

    void SetText(std::string_view text) { _text = text; }
    std::string_view GetText() const { return _text; }

    void SetPlaceholder(std::string_view ph) { _placeholder = ph; }
    void SetPassword(bool v) { _isPassword = v; }

    void OnDraw(DrawList& drawList, const ThemeEngine& theme) override;
    bool OnMouseDown(Vec2 pos, MouseButtonEvent& e) override;
    bool OnKeyDown(KeyEvent& e) override;
    bool OnTextInput(TextInputEvent& e) override;
    void OnFocusGained() override;
    void OnFocusLost() override;

    using TextChangedCallback = std::function<void(const std::string&)>;
    void SetOnTextChanged(TextChangedCallback cb) { _onTextChanged = std::move(cb); }

private:
    std::string _text;
    std::string _placeholder;
    i32         _cursorPos{0};
    i32         _selectionStart{-1};
    bool        _isPassword{false};
    f32         _cursorBlink{0};
    u32         _fontId{0};
    TextChangedCallback _onTextChanged;
};

// ============================================================================
// SLIDER WIDGET — Horizontal value slider
// ============================================================================
class SliderWidget : public Widget {
public:
    SliderWidget(f32 minVal = 0, f32 maxVal = 1, f32 value = 0.5f);

    f32  GetValue() const { return _value; }
    void SetValue(f32 v) { _value = math::Clamp(v, _min, _max); }
    void SetRange(f32 min, f32 max) { _min = min; _max = max; }

    void OnDraw(DrawList& drawList, const ThemeEngine& theme) override;
    bool OnMouseDown(Vec2 pos, MouseButtonEvent& e) override;
    bool OnMouseMove(Vec2 pos, MouseMoveEvent& e) override;
    bool OnMouseUp(Vec2 pos, MouseButtonEvent& e) override;

    using ValueChangedCallback = std::function<void(f32)>;
    void SetOnValueChanged(ValueChangedCallback cb) { _onValueChanged = std::move(cb); }

private:
    f32 _value;
    f32 _min;
    f32 _max;
    bool _dragging{false};
    ValueChangedCallback _onValueChanged;
};

// ============================================================================
// CHECKBOX WIDGET
// ============================================================================
class CheckboxWidget : public Widget {
public:
    CheckboxWidget(std::string_view label = "Checkbox", bool checked = false);

    bool IsChecked() const { return _checked; }
    void SetChecked(bool v) { _checked = v; }

    void OnDraw(DrawList& drawList, const ThemeEngine& theme) override;
    bool OnMouseDown(Vec2 pos, MouseButtonEvent& e) override;

    using ToggleCallback = std::function<void(bool)>;
    void SetOnToggle(ToggleCallback cb) { _onToggle = std::move(cb); }

private:
    std::string   _label;
    bool          _checked;
    u32           _fontId{0};
    SpringAnimation _toggleAnim;
    ToggleCallback  _onToggle;
};

// ============================================================================
// LABEL WIDGET — Static text display
// ============================================================================
class LabelWidget : public Widget {
public:
    LabelWidget(std::string_view text = "", Color color = Color::White());

    void SetText(std::string_view text) { _text = text; }
    void SetColor(Color c) { _color = c; }
    void SetFontId(u32 id) { _fontId = id; }

    void OnDraw(DrawList& drawList, const ThemeEngine& theme) override;

private:
    std::string _text;
    Color       _color;
    u32         _fontId{0};
};

// ============================================================================
// SEPARATOR WIDGET
// ============================================================================
class SeparatorWidget : public Widget {
public:
    SeparatorWidget(bool horizontal = true) : Widget("Separator"), _horizontal(horizontal) {
        _layout.preferredSize = _horizontal ? Vec2{0, 1} : Vec2{1, 0};
        _layout.flexGrow = _horizontal ? 1.0f : 0.0f;
    }

    void OnDraw(DrawList& drawList, const ThemeEngine& theme) override {
        drawList.AddFilledRect(GetBounds(), theme.GetColor(ThemeToken::BorderSecondary));
    }

private:
    bool _horizontal;
};

// ============================================================================
// SCROLLBAR WIDGET
// ============================================================================
class ScrollableWidget : public Widget {
public:
    ScrollableWidget(std::string_view name = "Scrollable");

    f32  GetScrollOffset() const { return _scrollOffset; }
    void SetContentHeight(f32 h) { _contentHeight = h; }

    void OnDraw(DrawList& drawList, const ThemeEngine& theme) override;
    bool OnMouseScroll(f32 delta) override;

private:
    f32 _scrollOffset{0};
    f32 _contentHeight{0};
    f32 _scrollSpeed{30.0f};
};

} // namespace ace
