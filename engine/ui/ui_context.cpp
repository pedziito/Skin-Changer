/**
 * ACE Engine — UI Context Implementation
 */

#include "ui_context.h"

namespace ace {

UIContext::UIContext() {
    _root.Layout().mode = LayoutMode::Dock;
}

void UIContext::Initialize(IRenderBackend* backend, u32 viewportW, u32 viewportH) {
    _backend = backend;
    _viewportW = viewportW;
    _viewportH = viewportH;
    _root.SetBounds({0, 0, (f32)viewportW, (f32)viewportH});
}

void UIContext::BeginFrame(f32 deltaTime, const InputSystem& input) {
    _deltaTime = deltaTime;
    _drawList.Clear();
    _frameArena.Reset();

    _imState.mousePos = input.MousePos();
    _imState.mouseDown = input.IsMouseDown(MouseButton::Left);

    // Update animations
    _animations.Update(deltaTime);

    // Process input events against widget tree
    ProcessInput(input);

    // Update tooltip
    if (_tooltip.visible) {
        _tooltip.hoverTimer += deltaTime;
    }
}

void UIContext::EndFrame() {
    // Layout pass
    _root.SetBounds({0, 0, (f32)_viewportW, (f32)_viewportH});
    LayoutEngine::ComputeLayout(&_root);

    // Update pass
    UpdateWidgetTree(&_root, _deltaTime);

    // Draw pass
    DrawWidgetTree(&_root);

    // Draw tooltip on top
    DrawTooltip();

    // Submit to backend
    if (_backend) {
        _backend->RenderDrawList(_drawList, _viewportW, _viewportH);
    }
}

// ============================================================================
// IMMEDIATE MODE API
// ============================================================================
bool UIContext::Button(WidgetID id, Rect bounds, std::string_view label) {
    bool hovered = bounds.Contains(_imState.mousePos);
    bool pressed = false;

    if (hovered) {
        _imState.hotWidget = id;
        if (_imState.mouseDown) {
            _imState.activeWidget = id;
        }
    }

    if (_imState.activeWidget == id && !_imState.mouseDown) {
        pressed = hovered;
        _imState.activeWidget = 0;
    }

    // Draw
    Color bgColor = _theme.GetColor(ThemeToken::BgSurface);
    if (_imState.activeWidget == id)     bgColor = _theme.GetColor(ThemeToken::AccentActive);
    else if (hovered)                    bgColor = _theme.GetColor(ThemeToken::BgSurfaceHover);

    f32 radius = _theme.GetMetric(ThemeMetric::BorderRadius);
    _drawList.AddFilledRoundRect(bounds, bgColor, radius);

    // Border
    Color borderColor = hovered
        ? _theme.GetColor(ThemeToken::BorderFocused)
        : _theme.GetColor(ThemeToken::BorderPrimary);
    _drawList.AddRect(bounds, borderColor, _theme.GetMetric(ThemeMetric::BorderWidth));

    // Label
    if (!label.empty() && _defaultFont) {
        Vec2 textSize = _fontAtlas.MeasureText(_defaultFont, label);
        Vec2 textPos = {
            bounds.x + (bounds.w - textSize.x) * 0.5f,
            bounds.y + (bounds.h - textSize.y) * 0.5f
        };
        _fontAtlas.RenderText(_drawList, _defaultFont, textPos, label,
                              _theme.GetColor(ThemeToken::TextPrimary));
    }

    return pressed;
}

bool UIContext::Checkbox(WidgetID id, Rect bounds, bool& value, std::string_view label) {
    f32 boxSize = bounds.h;
    Rect boxRect{bounds.x, bounds.y, boxSize, boxSize};

    bool hovered = boxRect.Contains(_imState.mousePos);
    bool toggled = false;

    if (hovered) {
        _imState.hotWidget = id;
        if (_imState.mouseDown && _imState.activeWidget != id) {
            _imState.activeWidget = id;
        }
    }

    if (_imState.activeWidget == id && !_imState.mouseDown) {
        if (hovered) { value = !value; toggled = true; }
        _imState.activeWidget = 0;
    }

    // Draw box
    Color bg = value ? _theme.GetColor(ThemeToken::AccentPrimary)
                     : _theme.GetColor(ThemeToken::InputBg);
    if (hovered) bg = value ? _theme.GetColor(ThemeToken::AccentHover)
                            : _theme.GetColor(ThemeToken::InputBgHover);

    _drawList.AddFilledRoundRect(boxRect, bg, 4.0f);
    _drawList.AddRect(boxRect, _theme.GetColor(ThemeToken::BorderPrimary));

    // Checkmark
    if (value) {
        f32 cx = boxRect.x + boxSize * 0.25f;
        f32 cy = boxRect.y + boxSize * 0.5f;
        _drawList.AddLine({cx, cy}, {cx + boxSize * 0.2f, cy + boxSize * 0.2f}, Color::White(), 2);
        _drawList.AddLine({cx + boxSize * 0.2f, cy + boxSize * 0.2f},
                          {cx + boxSize * 0.5f, cy - boxSize * 0.2f}, Color::White(), 2);
    }

    // Label
    if (!label.empty() && _defaultFont) {
        Vec2 labelPos{bounds.x + boxSize + 8, bounds.y + (bounds.h - 14) * 0.5f};
        _fontAtlas.RenderText(_drawList, _defaultFont, labelPos, label,
                              _theme.GetColor(ThemeToken::TextPrimary));
    }

    return toggled;
}

f32 UIContext::Slider(WidgetID id, Rect bounds, f32 value, f32 min, f32 max) {
    bool hovered = bounds.Contains(_imState.mousePos);

    if (hovered && _imState.mouseDown) {
        _imState.activeWidget = id;
    }

    if (_imState.activeWidget == id) {
        if (_imState.mouseDown) {
            f32 t = math::Saturate((_imState.mousePos.x - bounds.x) / bounds.w);
            value = math::Lerp(min, max, t);
        } else {
            _imState.activeWidget = 0;
        }
    }

    // Draw track
    _drawList.AddFilledRoundRect(bounds, _theme.GetColor(ThemeToken::InputBg), bounds.h * 0.5f);

    // Draw filled portion
    f32 t = math::InvLerp(min, max, value);
    Rect fillRect{bounds.x, bounds.y, bounds.w * t, bounds.h};
    _drawList.AddFilledRoundRect(fillRect, _theme.GetColor(ThemeToken::AccentPrimary), bounds.h * 0.5f);

    // Draw handle
    f32 handleX = bounds.x + bounds.w * t;
    _drawList.AddCircle({handleX, bounds.Center().y}, bounds.h * 0.7f,
                        hovered ? _theme.GetColor(ThemeToken::AccentHover) : Color::White());

    return value;
}

void UIContext::Label(Rect bounds, std::string_view text, Color color) {
    if (_defaultFont) {
        _fontAtlas.RenderText(_drawList, _defaultFont, bounds.Pos(), text, color, bounds.w);
    }
}

void UIContext::Panel(Rect bounds, Color color) {
    f32 radius = _theme.GetMetric(ThemeMetric::BorderRadius);
    _drawList.AddFilledRoundRect(bounds, color, radius);
}

bool UIContext::TextInput(WidgetID id, Rect bounds, std::string& text) {
    bool hovered = bounds.Contains(_imState.mousePos);
    bool focused = _imState.focusedWidget == id;
    bool changed = false;

    if (hovered && _imState.mouseDown) {
        _imState.focusedWidget = id;
        focused = true;
    }

    // Draw background
    Color bg = focused ? _theme.GetColor(ThemeToken::InputBgFocused)
             : hovered ? _theme.GetColor(ThemeToken::InputBgHover)
             : _theme.GetColor(ThemeToken::InputBg);

    _drawList.AddFilledRoundRect(bounds, bg, _theme.GetMetric(ThemeMetric::BorderRadius));

    Color border = focused ? _theme.GetColor(ThemeToken::BorderFocused)
                           : _theme.GetColor(ThemeToken::InputBorder);
    _drawList.AddRect(bounds, border, _theme.GetMetric(ThemeMetric::BorderWidth));

    // Render text
    if (_defaultFont) {
        Vec2 textPos{bounds.x + 8, bounds.y + (bounds.h - 14) * 0.5f};
        std::string_view displayText = text.empty() ? "..." : std::string_view(text);
        Color textColor = text.empty() ? _theme.GetColor(ThemeToken::TextDisabled)
                                       : _theme.GetColor(ThemeToken::TextPrimary);
        _fontAtlas.RenderText(_drawList, _defaultFont, textPos, displayText, textColor,
                              bounds.w - 16);
    }

    // Cursor blink
    if (focused && _defaultFont) {
        Vec2 textSize = _fontAtlas.MeasureText(_defaultFont, text);
        f32 cursorX = bounds.x + 8 + textSize.x;
        f32 blink = std::fmod(_deltaTime * 2.0f, 2.0f);
        if (blink < 1.0f) {
            _drawList.AddFilledRect({cursorX, bounds.y + 4, 2, bounds.h - 8},
                                     _theme.GetColor(ThemeToken::TextPrimary));
        }
    }

    return changed;
}

void UIContext::SetFocus(Widget* widget) {
    if (_focusedWidget) {
        _focusedWidget->ClearFlag(WidgetFlags::IsFocused);
        _focusedWidget->OnFocusLost();
    }
    _focusedWidget = widget;
    if (widget) {
        widget->SetFlag(WidgetFlags::IsFocused);
        widget->OnFocusGained();
    }
}

void UIContext::ShowTooltip(std::string_view text, Vec2 pos) {
    _tooltip.text = text;
    _tooltip.position = pos;
    _tooltip.visible = true;
}

// ============================================================================
// INTERNAL
// ============================================================================
void UIContext::ProcessInput(const InputSystem& input) {
    Vec2 mousePos = input.MousePos();

    // Hit test from root
    Widget* hit = _root.HitTestRecursive(mousePos);

    // Mouse enter/leave for widgets
    static Widget* lastHovered = nullptr;
    if (hit != lastHovered) {
        if (lastHovered) {
            lastHovered->ClearFlag(WidgetFlags::IsHovered);
            lastHovered->OnMouseLeave();
        }
        if (hit) {
            hit->SetFlag(WidgetFlags::IsHovered);
            hit->OnMouseEnter();
        }
        lastHovered = hit;
    }

    // Tooltip
    if (hit && !hit->GetTooltip().empty()) {
        _tooltip.hoverTimer += _deltaTime;
        if (_tooltip.hoverTimer > _tooltip.showDelay) {
            ShowTooltip(hit->GetTooltip(), mousePos + Vec2{12, 16});
        }
        _tooltip.ownerWidget = hit->GetID();
    } else {
        _tooltip.visible = false;
        _tooltip.hoverTimer = 0;
    }

    // Mouse button events
    if (input.IsMousePressed(MouseButton::Left)) {
        MouseButtonEvent e{0, true, mousePos};
        if (hit) {
            hit->SetFlag(WidgetFlags::IsPressed);
            hit->OnMouseDown(mousePos, e);
            SetFocus(hit->HasFlag(WidgetFlags::Focusable) ? hit : nullptr);
        }
    }

    if (input.IsMouseReleased(MouseButton::Left)) {
        MouseButtonEvent e{0, false, mousePos};
        // Check if released on same widget that was pressed
        for (auto& child : _root.GetChildren()) {
            if (child->HasFlag(WidgetFlags::IsPressed)) {
                child->ClearFlag(WidgetFlags::IsPressed);
                child->OnMouseUp(mousePos, e);
                if (child->HitTest(mousePos)) {
                    child->InvokeClick();
                }
            }
        }
    }

    // Keyboard events
    // Forward to focused widget
    // (simplified — extend for full key routing)
}

void UIContext::UpdateWidgetTree(Widget* widget, f32 dt) {
    if (!widget->IsVisible()) return;
    widget->OnUpdate(dt);
    for (auto& child : widget->GetChildren())
        UpdateWidgetTree(child.get(), dt);
}

void UIContext::DrawWidgetTree(Widget* widget) {
    if (!widget->IsVisible()) return;

    if (widget->HasFlag(WidgetFlags::ClipChildren)) {
        _drawList.PushClipRect(widget->GetBounds());
    }

    widget->OnDraw(_drawList, _theme);

    for (auto& child : widget->GetChildren())
        DrawWidgetTree(child.get());

    if (widget->HasFlag(WidgetFlags::ClipChildren)) {
        _drawList.PopClipRect();
    }
}

void UIContext::DrawTooltip() {
    if (!_tooltip.visible || _tooltip.text.empty()) return;

    Vec2 textSize = _defaultFont ? _fontAtlas.MeasureText(_defaultFont, _tooltip.text) : Vec2{100, 20};
    Rect bg{_tooltip.position.x, _tooltip.position.y,
            textSize.x + 16, textSize.y + 8};

    _drawList.AddFilledRoundRect(bg, _theme.GetColor(ThemeToken::TooltipBg), 4.0f);
    _drawList.AddRect(bg, _theme.GetColor(ThemeToken::BorderPrimary));

    if (_defaultFont) {
        _fontAtlas.RenderText(_drawList, _defaultFont,
                              bg.Pos() + Vec2{8, 4}, _tooltip.text,
                              _theme.GetColor(ThemeToken::TooltipText));
    }
}

} // namespace ace
