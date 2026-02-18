/**
 * ACE Engine — Core Widget Implementations
 */

#include "widgets.h"
#include "../../input/input_system.h"

namespace ace {

// ============================================================================
// BUTTON
// ============================================================================
ButtonWidget::ButtonWidget(std::string_view label) : Widget(label), _label(label) {
    _hoverAnim.stiffness = 300.0f;
    _hoverAnim.damping = 22.0f;
    _pressAnim.stiffness = 400.0f;
    _pressAnim.damping = 25.0f;
    _layout.preferredSize = {120, 32};
    SetFlag(WidgetFlags::CapturesMouse);
}

void ButtonWidget::OnDraw(DrawList& drawList, const ThemeEngine& theme) {
    _hoverAnim.Update(1.0f / 60.0f);
    _pressAnim.Update(1.0f / 60.0f);

    f32 hoverT = _hoverAnim.value;
    f32 pressT = _pressAnim.value;

    Color bg = Color::Lerp(
        theme.GetColor(ThemeToken::BgSurface),
        theme.GetColor(ThemeToken::BgSurfaceHover),
        hoverT
    );
    bg = Color::Lerp(bg, theme.GetColor(ThemeToken::AccentActive), pressT);

    f32 radius = theme.GetMetric(ThemeMetric::BorderRadius);
    Rect bounds = GetBounds();

    // Shadow
    if (hoverT > 0.1f) {
        drawList.AddFilledRoundRect(bounds.Expand(2), theme.GetColor(ThemeToken::Shadow), radius + 2);
    }

    // Background
    drawList.AddFilledRoundRect(bounds, bg, radius);

    // Border
    Color borderColor = Color::Lerp(
        theme.GetColor(ThemeToken::BorderPrimary),
        theme.GetColor(ThemeToken::BorderFocused),
        hoverT
    );
    drawList.AddRect(bounds, borderColor, theme.GetMetric(ThemeMetric::BorderWidth));

    // Label (centered)
    // Note: Font rendering would be handled via FontAtlas in real usage
    // For now we draw a placeholder
}

bool ButtonWidget::OnMouseEnter() {
    _hoverAnim.target = 1.0f;
    return true;
}

bool ButtonWidget::OnMouseLeave() {
    _hoverAnim.target = 0.0f;
    _pressAnim.target = 0.0f;
    return true;
}

bool ButtonWidget::OnMouseDown(Vec2 /*pos*/, MouseButtonEvent& /*e*/) {
    _pressAnim.target = 1.0f;
    return true;
}

bool ButtonWidget::OnMouseUp(Vec2 pos, MouseButtonEvent& /*e*/) {
    _pressAnim.target = 0.0f;
    if (HitTest(pos)) InvokeClick();
    return true;
}

// ============================================================================
// PANEL
// ============================================================================
PanelWidget::PanelWidget(std::string_view name) : Widget(name) {
    _layout.mode = LayoutMode::FlexColumn;
    _layout.padding = EdgeInsets(8);
    SetFlag(WidgetFlags::ClipChildren);
}

void PanelWidget::OnDraw(DrawList& drawList, const ThemeEngine& theme) {
    Rect bounds = GetBounds();
    f32 radius = theme.GetMetric(ThemeMetric::BorderRadius);

    // Background
    Color bg = _style.hasOverride ? _style.bgColor : theme.GetColor(ThemeToken::BgSecondary);
    drawList.AddFilledRoundRect(bounds, bg, radius);

    // Header
    if (!_title.empty()) {
        Rect headerRect{bounds.x, bounds.y, bounds.w, _headerHeight};
        drawList.AddFilledRoundRect(headerRect, theme.GetColor(ThemeToken::BgHeader), radius);

        // Collapse arrow
        if (_collapsible) {
            f32 arrowX = headerRect.x + 12;
            f32 arrowY = headerRect.Center().y;
            if (_collapsed) {
                drawList.AddTriangle(
                    {arrowX, arrowY - 5}, {arrowX, arrowY + 5}, {arrowX + 6, arrowY},
                    theme.GetColor(ThemeToken::TextSecondary));
            } else {
                drawList.AddTriangle(
                    {arrowX - 3, arrowY - 3}, {arrowX + 3, arrowY - 3}, {arrowX, arrowY + 4},
                    theme.GetColor(ThemeToken::TextSecondary));
            }
        }

        // Title separator
        drawList.AddFilledRect(
            {bounds.x, bounds.y + _headerHeight - 1, bounds.w, 1},
            theme.GetColor(ThemeToken::BorderSecondary));
    }

    // Border
    drawList.AddRect(bounds, theme.GetColor(ThemeToken::BorderPrimary), theme.GetMetric(ThemeMetric::BorderWidth));
}

bool PanelWidget::OnMouseDown(Vec2 pos, MouseButtonEvent& /*e*/) {
    if (_collapsible) {
        Rect headerRect{GetBounds().x, GetBounds().y, GetBounds().w, _headerHeight};
        if (headerRect.Contains(pos)) {
            _collapsed = !_collapsed;
            MarkDirty();
            return true;
        }
    }
    return false;
}

// ============================================================================
// TEXT INPUT
// ============================================================================
TextInputWidget::TextInputWidget(std::string_view placeholder)
    : Widget("TextInput"), _placeholder(placeholder) {
    _layout.preferredSize = {200, 32};
    SetFlag(WidgetFlags::Focusable | WidgetFlags::CapturesKeyboard);
}

void TextInputWidget::OnDraw(DrawList& drawList, const ThemeEngine& theme) {
    Rect bounds = GetBounds();
    f32 radius = theme.GetMetric(ThemeMetric::BorderRadius);
    bool focused = IsFocused();

    Color bg = focused ? theme.GetColor(ThemeToken::InputBgFocused)
             : IsHovered() ? theme.GetColor(ThemeToken::InputBgHover)
             : theme.GetColor(ThemeToken::InputBg);

    drawList.AddFilledRoundRect(bounds, bg, radius);

    Color border = focused ? theme.GetColor(ThemeToken::BorderFocused)
                           : theme.GetColor(ThemeToken::InputBorder);
    drawList.AddRect(bounds, border, theme.GetMetric(ThemeMetric::BorderWidth));

    // Cursor blink effect
    if (focused) {
        _cursorBlink += 1.0f / 60.0f;
        if (_cursorBlink > 1.0f) _cursorBlink -= 1.0f;
    }
}

bool TextInputWidget::OnMouseDown(Vec2 /*pos*/, MouseButtonEvent& /*e*/) {
    // Position cursor based on click position (simplified)
    return true;
}

bool TextInputWidget::OnKeyDown(KeyEvent& e) {
    if (!IsFocused()) return false;

    if (e.keyCode == static_cast<i32>(Key::Backspace) && _cursorPos > 0) {
        _text.erase(_cursorPos - 1, 1);
        _cursorPos--;
        if (_onTextChanged) _onTextChanged(_text);
        return true;
    }

    if (e.keyCode == static_cast<i32>(Key::Delete) && _cursorPos < (i32)_text.size()) {
        _text.erase(_cursorPos, 1);
        if (_onTextChanged) _onTextChanged(_text);
        return true;
    }

    if (e.keyCode == static_cast<i32>(Key::Left) && _cursorPos > 0) {
        _cursorPos--;
        return true;
    }

    if (e.keyCode == static_cast<i32>(Key::Right) && _cursorPos < (i32)_text.size()) {
        _cursorPos++;
        return true;
    }

    if (e.keyCode == static_cast<i32>(Key::Home)) { _cursorPos = 0; return true; }
    if (e.keyCode == static_cast<i32>(Key::End)) { _cursorPos = (i32)_text.size(); return true; }

    // Select all (Ctrl+A)
    if (e.ctrl && e.keyCode == static_cast<i32>(Key::A)) {
        _selectionStart = 0;
        _cursorPos = (i32)_text.size();
        return true;
    }

    return false;
}

bool TextInputWidget::OnTextInput(TextInputEvent& e) {
    if (!IsFocused()) return false;
    if (e.codepoint < 32) return false;

    char c = static_cast<char>(e.codepoint);
    _text.insert(_text.begin() + _cursorPos, c);
    _cursorPos++;
    if (_onTextChanged) _onTextChanged(_text);
    _cursorBlink = 0;
    return true;
}

void TextInputWidget::OnFocusGained() { _cursorBlink = 0; }
void TextInputWidget::OnFocusLost() { _selectionStart = -1; }

// ============================================================================
// SLIDER
// ============================================================================
SliderWidget::SliderWidget(f32 minVal, f32 maxVal, f32 value)
    : Widget("Slider"), _value(value), _min(minVal), _max(maxVal) {
    _layout.preferredSize = {200, 20};
    SetFlag(WidgetFlags::CapturesMouse);
}

void SliderWidget::OnDraw(DrawList& drawList, const ThemeEngine& theme) {
    Rect bounds = GetBounds();
    f32 radius = bounds.h * 0.5f;

    // Track
    drawList.AddFilledRoundRect(bounds, theme.GetColor(ThemeToken::InputBg), radius);

    // Filled portion
    f32 t = math::InvLerp(_min, _max, _value);
    Rect fillRect{bounds.x, bounds.y, bounds.w * t, bounds.h};
    drawList.AddFilledRoundRect(fillRect, theme.GetColor(ThemeToken::AccentPrimary), radius);

    // Handle
    f32 handleX = bounds.x + bounds.w * t;
    f32 handleRadius = bounds.h * 0.7f;
    Color handleColor = _dragging ? theme.GetColor(ThemeToken::AccentActive)
                      : IsHovered() ? theme.GetColor(ThemeToken::AccentHover)
                      : Color::White();
    drawList.AddCircle({handleX, bounds.Center().y}, handleRadius, handleColor);
}

bool SliderWidget::OnMouseDown(Vec2 pos, MouseButtonEvent& /*e*/) {
    _dragging = true;
    f32 t = math::Saturate((pos.x - GetBounds().x) / GetBounds().w);
    _value = math::Lerp(_min, _max, t);
    if (_onValueChanged) _onValueChanged(_value);
    return true;
}

bool SliderWidget::OnMouseMove(Vec2 pos, MouseMoveEvent& /*e*/) {
    if (_dragging) {
        f32 t = math::Saturate((pos.x - GetBounds().x) / GetBounds().w);
        _value = math::Lerp(_min, _max, t);
        if (_onValueChanged) _onValueChanged(_value);
        return true;
    }
    return false;
}

bool SliderWidget::OnMouseUp(Vec2 /*pos*/, MouseButtonEvent& /*e*/) {
    _dragging = false;
    return true;
}

// ============================================================================
// CHECKBOX
// ============================================================================
CheckboxWidget::CheckboxWidget(std::string_view label, bool checked)
    : Widget(label), _label(label), _checked(checked) {
    _layout.preferredSize = {150, 24};
    _toggleAnim.stiffness = 300.0f;
    _toggleAnim.damping = 20.0f;
    _toggleAnim.value = checked ? 1.0f : 0.0f;
    _toggleAnim.target = _toggleAnim.value;
    SetFlag(WidgetFlags::CapturesMouse);
}

void CheckboxWidget::OnDraw(DrawList& drawList, const ThemeEngine& theme) {
    _toggleAnim.Update(1.0f / 60.0f);

    Rect bounds = GetBounds();
    f32 boxSize = bounds.h;
    Rect boxRect{bounds.x, bounds.y, boxSize, boxSize};

    f32 t = _toggleAnim.value;

    Color bg = Color::Lerp(
        theme.GetColor(ThemeToken::InputBg),
        theme.GetColor(ThemeToken::AccentPrimary),
        t
    );

    drawList.AddFilledRoundRect(boxRect, bg, 4.0f);
    drawList.AddRect(boxRect, theme.GetColor(ThemeToken::BorderPrimary));

    // Checkmark (animated)
    if (t > 0.1f) {
        Color checkColor = Color::White().WithAlpha(static_cast<u8>(t * 255));
        f32 cx = boxRect.x + boxSize * 0.25f;
        f32 cy = boxRect.y + boxSize * 0.5f;
        drawList.AddLine({cx, cy}, {cx + boxSize * 0.2f, cy + boxSize * 0.2f}, checkColor, 2);
        drawList.AddLine({cx + boxSize * 0.2f, cy + boxSize * 0.2f},
                          {cx + boxSize * 0.5f, cy - boxSize * 0.2f}, checkColor, 2);
    }
}

bool CheckboxWidget::OnMouseDown(Vec2 /*pos*/, MouseButtonEvent& /*e*/) {
    _checked = !_checked;
    _toggleAnim.target = _checked ? 1.0f : 0.0f;
    if (_onToggle) _onToggle(_checked);
    return true;
}

// ============================================================================
// LABEL
// ============================================================================
LabelWidget::LabelWidget(std::string_view text, Color color)
    : Widget("Label"), _text(text), _color(color) {
    _layout.preferredSize = {100, 20};
}

void LabelWidget::OnDraw(DrawList& /*drawList*/, const ThemeEngine& theme) {
    Color color = _color.a > 0 ? _color : theme.GetColor(ThemeToken::TextPrimary);
    // Text rendering handled by font system
    // Widget bounds are used for layout
}

// ============================================================================
// SCROLLABLE
// ============================================================================
ScrollableWidget::ScrollableWidget(std::string_view name) : Widget(name) {
    SetFlag(WidgetFlags::ClipChildren);
}

void ScrollableWidget::OnDraw(DrawList& drawList, const ThemeEngine& theme) {
    Rect bounds = GetBounds();
    drawList.AddFilledRect(bounds, theme.GetColor(ThemeToken::BgSecondary));

    // Draw scrollbar if content overflows
    if (_contentHeight > bounds.h) {
        f32 scrollW = theme.GetMetric(ThemeMetric::ScrollbarWidth);
        f32 viewRatio = bounds.h / _contentHeight;
        f32 thumbH = bounds.h * viewRatio;
        f32 scrollRange = _contentHeight - bounds.h;
        f32 thumbY = bounds.y + (_scrollOffset / scrollRange) * (bounds.h - thumbH);

        // Track
        drawList.AddFilledRect(
            {bounds.Right() - scrollW, bounds.y, scrollW, bounds.h},
            theme.GetColor(ThemeToken::ScrollbarTrack));

        // Thumb
        drawList.AddFilledRoundRect(
            {bounds.Right() - scrollW, thumbY, scrollW, thumbH},
            theme.GetColor(ThemeToken::ScrollbarThumb),
            scrollW * 0.5f);
    }
}

bool ScrollableWidget::OnMouseScroll(f32 delta) {
    f32 maxScroll = std::max(0.0f, _contentHeight - GetBounds().h);
    _scrollOffset = math::Clamp(_scrollOffset - delta * _scrollSpeed, 0.0f, maxScroll);
    return true;
}

} // namespace ace
