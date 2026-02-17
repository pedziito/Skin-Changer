/**
 * ACE Engine — Property Inspector Implementation
 */

#include "property_inspector.h"

namespace ace {

PropertyInspector::PropertyInspector(std::string_view name) : Widget(name) {
    _layout.preferredSize = {300, 400};
    _layout.padding = EdgeInsets(4);
    SetFlag(WidgetFlags::ClipChildren);
}

void PropertyInspector::OnDraw(DrawList& drawList, const ThemeEngine& theme) {
    if (!_target || !_typeDesc) return;

    Rect bounds = GetContentBounds();

    // Header with type name
    Rect headerRect{bounds.x, bounds.y, bounds.w, 30};
    drawList.AddFilledRect(headerRect, theme.GetColor(ThemeToken::BgHeader));
    drawList.AddFilledRect({bounds.x, headerRect.Bottom() - 1, bounds.w, 1},
                            theme.GetColor(ThemeToken::BorderSecondary));

    // Properties
    f32 y = bounds.y + 32 - _scrollOffset;

    for (i32 i = 0; i < (i32)_typeDesc->properties.size(); ++i) {
        auto& prop = _typeDesc->properties[i];

        if (HasFlag(prop.flags, PropFlags::Hidden)) continue;

        Rect rowRect{bounds.x, y, bounds.w, _rowHeight};

        // Only draw visible rows
        if (rowRect.Bottom() > bounds.y && rowRect.y < bounds.Bottom()) {
            // Separator check
            if (HasFlag(prop.flags, PropFlags::Separator)) {
                drawList.AddFilledRect(
                    {rowRect.x + 4, rowRect.Center().y, rowRect.w - 8, 1},
                    theme.GetColor(ThemeToken::BorderSecondary));
                y += _rowHeight * 0.5f;
                continue;
            }

            if (HasFlag(prop.flags, PropFlags::Header)) {
                drawList.AddFilledRect(rowRect, theme.GetColor(ThemeToken::BgTertiary));
                // Draw header text (via font atlas in real usage)
                y += _rowHeight;
                continue;
            }

            DrawProperty(prop, rowRect, drawList, theme, i);
        }

        y += _rowHeight;
    }
}

void PropertyInspector::DrawProperty(const PropertyInfo& prop, Rect rowRect,
                                      DrawList& drawList, const ThemeEngine& theme, i32 index) {
    // Alternating row background
    if (index % 2 == 0) {
        drawList.AddFilledRect(rowRect, theme.GetColor(ThemeToken::BgSecondary));
    }

    // Hover highlight
    // (Would check mouse position here for immediate-mode hover)

    f32 labelW = rowRect.w * _labelWidth;
    Rect labelRect{rowRect.x + 4, rowRect.y, labelW - 4, rowRect.h};
    Rect editorRect{rowRect.x + labelW, rowRect.y, rowRect.w - labelW - 4, rowRect.h};

    // Label (name of property)
    // In production, render with FontAtlas. Here we draw a placeholder rect.
    drawList.AddFilledRect(
        {labelRect.x, labelRect.Center().y - 1, std::min(labelW - 8, labelW), 2},
        theme.GetColor(ThemeToken::TextSecondary).WithAlpha(40));

    // Type-specific editor
    if (HasFlag(prop.flags, PropFlags::ReadOnly)) {
        drawList.AddFilledRect(editorRect.Shrink(2), theme.GetColor(ThemeToken::BgTertiary));
        return;
    }

    switch (prop.type) {
        case PropType::Bool:   DrawBoolEditor(prop, editorRect, drawList, theme); break;
        case PropType::Int:    DrawIntEditor(prop, editorRect, drawList, theme); break;
        case PropType::Float:  DrawFloatEditor(prop, editorRect, drawList, theme); break;
        case PropType::String: DrawStringEditor(prop, editorRect, drawList, theme); break;
        case PropType::Vec2:   DrawVec2Editor(prop, editorRect, drawList, theme); break;
        case PropType::Color:  DrawColorEditor(prop, editorRect, drawList, theme); break;
        default:
            drawList.AddFilledRect(editorRect.Shrink(2), theme.GetColor(ThemeToken::InputBg));
            break;
    }
}

void PropertyInspector::DrawBoolEditor(const PropertyInfo& prop, Rect editorRect,
                                        DrawList& drawList, const ThemeEngine& theme) {
    bool value = prop.GetValue<bool>(_target);
    f32 boxSize = editorRect.h - 8;
    Rect boxRect{editorRect.x + 4, editorRect.y + 4, boxSize, boxSize};

    Color bg = value ? theme.GetColor(ThemeToken::AccentPrimary)
                     : theme.GetColor(ThemeToken::InputBg);
    drawList.AddFilledRoundRect(boxRect, bg, 3.0f);
    drawList.AddRect(boxRect, theme.GetColor(ThemeToken::BorderPrimary));

    if (value) {
        f32 cx = boxRect.x + boxSize * 0.25f;
        f32 cy = boxRect.Center().y;
        drawList.AddLine({cx, cy}, {cx + boxSize * 0.2f, cy + boxSize * 0.2f}, Color::White(), 2);
        drawList.AddLine({cx + boxSize * 0.2f, cy + boxSize * 0.2f},
                          {cx + boxSize * 0.5f, cy - boxSize * 0.2f}, Color::White(), 2);
    }
}

void PropertyInspector::DrawIntEditor(const PropertyInfo& prop, Rect editorRect,
                                       DrawList& drawList, const ThemeEngine& theme) {
    Rect fieldRect = editorRect.Shrink(2);

    if (HasFlag(prop.flags, PropFlags::DragInt)) {
        // Drag-style integer editor
        int value = prop.GetValue<int>(_target);
        f32 t = math::InvLerp(prop.rangeMin, prop.rangeMax, (f32)value);
        t = math::Saturate(t);

        drawList.AddFilledRoundRect(fieldRect, theme.GetColor(ThemeToken::InputBg), 3);
        Rect fillRect{fieldRect.x, fieldRect.y, fieldRect.w * t, fieldRect.h};
        drawList.AddFilledRoundRect(fillRect, theme.GetColor(ThemeToken::AccentPrimary).WithAlpha(100), 3);
    } else {
        drawList.AddFilledRoundRect(fieldRect, theme.GetColor(ThemeToken::InputBg), 3);
    }

    drawList.AddRect(fieldRect, theme.GetColor(ThemeToken::InputBorder));
}

void PropertyInspector::DrawFloatEditor(const PropertyInfo& prop, Rect editorRect,
                                         DrawList& drawList, const ThemeEngine& theme) {
    Rect fieldRect = editorRect.Shrink(2);

    if (HasFlag(prop.flags, PropFlags::Slider)) {
        f32 value = prop.GetValue<f32>(_target);
        f32 t = math::InvLerp(prop.rangeMin, prop.rangeMax, value);
        t = math::Saturate(t);

        drawList.AddFilledRoundRect(fieldRect, theme.GetColor(ThemeToken::InputBg), fieldRect.h * 0.5f);
        Rect fillRect{fieldRect.x, fieldRect.y, fieldRect.w * t, fieldRect.h};
        drawList.AddFilledRoundRect(fillRect, theme.GetColor(ThemeToken::AccentPrimary), fieldRect.h * 0.5f);

        // Handle
        f32 handleX = fieldRect.x + fieldRect.w * t;
        drawList.AddCircle({handleX, fieldRect.Center().y}, fieldRect.h * 0.6f, Color::White());
    } else {
        drawList.AddFilledRoundRect(fieldRect, theme.GetColor(ThemeToken::InputBg), 3);
        drawList.AddRect(fieldRect, theme.GetColor(ThemeToken::InputBorder));
    }
}

void PropertyInspector::DrawStringEditor(const PropertyInfo& prop, Rect editorRect,
                                          DrawList& drawList, const ThemeEngine& theme) {
    Rect fieldRect = editorRect.Shrink(2);
    drawList.AddFilledRoundRect(fieldRect, theme.GetColor(ThemeToken::InputBg), 3);
    drawList.AddRect(fieldRect, theme.GetColor(ThemeToken::InputBorder));
}

void PropertyInspector::DrawVec2Editor(const PropertyInfo& prop, Rect editorRect,
                                        DrawList& drawList, const ThemeEngine& theme) {
    f32 halfW = (editorRect.w - 4) * 0.5f;
    Rect xRect{editorRect.x, editorRect.y + 2, halfW, editorRect.h - 4};
    Rect yRect{editorRect.x + halfW + 4, editorRect.y + 2, halfW, editorRect.h - 4};

    // X field (red tint)
    drawList.AddFilledRoundRect(xRect, theme.GetColor(ThemeToken::InputBg), 3);
    drawList.AddFilledRect({xRect.x, xRect.y, 3, xRect.h}, Color{220, 60, 60});

    // Y field (green tint)
    drawList.AddFilledRoundRect(yRect, theme.GetColor(ThemeToken::InputBg), 3);
    drawList.AddFilledRect({yRect.x, yRect.y, 3, yRect.h}, Color{60, 180, 60});
}

void PropertyInspector::DrawColorEditor(const PropertyInfo& prop, Rect editorRect,
                                         DrawList& drawList, const ThemeEngine& theme) {
    Color value = prop.GetValue<Color>(_target);
    Rect swatchRect = editorRect.Shrink(3);

    // Checkerboard background (for alpha)
    f32 checkSize = 6;
    for (f32 cx = swatchRect.x; cx < swatchRect.Right(); cx += checkSize) {
        for (f32 cy = swatchRect.y; cy < swatchRect.Bottom(); cy += checkSize) {
            i32 ix = (i32)((cx - swatchRect.x) / checkSize);
            i32 iy = (i32)((cy - swatchRect.y) / checkSize);
            Color checkColor = ((ix + iy) % 2 == 0) ? Color{200, 200, 200} : Color{150, 150, 150};
            drawList.AddFilledRect({cx, cy, checkSize, checkSize}, checkColor);
        }
    }

    // Color swatch
    drawList.AddFilledRect(swatchRect, value);
    drawList.AddRect(swatchRect, theme.GetColor(ThemeToken::BorderPrimary));
}

bool PropertyInspector::OnMouseDown(Vec2 pos, MouseButtonEvent& e) {
    if (!_target || !_typeDesc) return false;

    Rect bounds = GetContentBounds();
    f32 y = bounds.y + 32 - _scrollOffset;

    for (i32 i = 0; i < (i32)_typeDesc->properties.size(); ++i) {
        auto& prop = _typeDesc->properties[i];
        if (HasFlag(prop.flags, PropFlags::Hidden | PropFlags::ReadOnly)) continue;

        Rect rowRect{bounds.x, y, bounds.w, _rowHeight};
        Rect editorRect{bounds.x + bounds.w * _labelWidth, y, bounds.w * (1-_labelWidth), _rowHeight};

        if (editorRect.Contains(pos)) {
            // Handle click on editor
            if (prop.type == PropType::Bool) {
                bool val = prop.GetValue<bool>(_target);
                prop.SetValue(_target, !val);
                return true;
            }

            if (prop.type == PropType::Float &&
                (HasFlag(prop.flags, PropFlags::Slider) || HasFlag(prop.flags, PropFlags::DragFloat))) {
                _editingProperty = i;
                _isDragging = true;
                _dragStartValue = prop.GetValue<f32>(_target);
                _dragStartPos = pos;
                return true;
            }

            if (prop.type == PropType::Int && HasFlag(prop.flags, PropFlags::DragInt)) {
                _editingProperty = i;
                _isDragging = true;
                _dragStartValue = (f32)prop.GetValue<int>(_target);
                _dragStartPos = pos;
                return true;
            }
        }

        y += _rowHeight;
    }

    return false;
}

bool PropertyInspector::OnMouseMove(Vec2 pos, MouseMoveEvent& e) {
    if (_isDragging && _editingProperty >= 0) {
        auto& prop = _typeDesc->properties[_editingProperty];
        Rect bounds = GetContentBounds();
        Rect editorRect{bounds.x + bounds.w * _labelWidth, 0, bounds.w * (1-_labelWidth), _rowHeight};

        if (prop.type == PropType::Float) {
            f32 delta = (pos.x - _dragStartPos.x) / editorRect.w;
            f32 newVal = _dragStartValue + delta * (prop.rangeMax - prop.rangeMin);
            newVal = math::Clamp(newVal, prop.rangeMin, prop.rangeMax);
            prop.SetValue(_target, newVal);
        } else if (prop.type == PropType::Int) {
            f32 delta = (pos.x - _dragStartPos.x) / editorRect.w;
            int newVal = (int)(_dragStartValue + delta * (prop.rangeMax - prop.rangeMin));
            newVal = (int)math::Clamp((f32)newVal, prop.rangeMin, prop.rangeMax);
            prop.SetValue(_target, newVal);
        }

        return true;
    }
    return false;
}

bool PropertyInspector::OnMouseUp(Vec2 pos, MouseButtonEvent& e) {
    _isDragging = false;
    _editingProperty = -1;
    return false;
}

bool PropertyInspector::OnMouseScroll(f32 delta) {
    f32 totalHeight = _typeDesc ? _typeDesc->properties.size() * _rowHeight + 32 : 0;
    f32 maxScroll = std::max(0.0f, totalHeight - GetBounds().h);
    _scrollOffset = math::Clamp(_scrollOffset - delta * 30.0f, 0.0f, maxScroll);
    return true;
}

} // namespace ace
