/**
 * ACE Engine — Property Inspector
 * Reflection-based auto-generated property editor.
 * Reads TypeDescriptor from the reflection system and creates
 * appropriate editor widgets for each property.
 */

#pragma once

#include "../widget.h"
#include "../theme.h"
#include "../../core/reflection.h"
#include "../../render/font_atlas.h"
#include <string>

namespace ace {

class PropertyInspector : public Widget {
public:
    PropertyInspector(std::string_view name = "PropertyInspector");

    /**
     * Bind an object for editing. Uses TypeDescriptor to enumerate fields.
     * Template version auto-discovers TypeDescriptor.
     */
    template<typename T>
    void Inspect(T* object) {
        _target = object;
        _typeDesc = &T::GetTypeDescriptor();
        MarkDirty();
    }

    /**
     * Bind with explicit TypeDescriptor (for runtime-registered types).
     */
    void Inspect(void* object, const TypeDescriptor* desc) {
        _target = object;
        _typeDesc = desc;
        MarkDirty();
    }

    void ClearInspection() {
        _target = nullptr;
        _typeDesc = nullptr;
    }

    void SetFontId(u32 id) { _fontId = id; }
    void SetRowHeight(f32 h) { _rowHeight = h; }

    void OnDraw(DrawList& drawList, const ThemeEngine& theme) override;
    bool OnMouseDown(Vec2 pos, MouseButtonEvent& e) override;
    bool OnMouseMove(Vec2 pos, MouseMoveEvent& e) override;
    bool OnMouseUp(Vec2 pos, MouseButtonEvent& e) override;
    bool OnMouseScroll(f32 delta) override;

private:
    void DrawProperty(const PropertyInfo& prop, Rect rowRect,
                      DrawList& drawList, const ThemeEngine& theme, i32 index);
    void DrawBoolEditor(const PropertyInfo& prop, Rect editorRect,
                        DrawList& drawList, const ThemeEngine& theme);
    void DrawIntEditor(const PropertyInfo& prop, Rect editorRect,
                       DrawList& drawList, const ThemeEngine& theme);
    void DrawFloatEditor(const PropertyInfo& prop, Rect editorRect,
                         DrawList& drawList, const ThemeEngine& theme);
    void DrawStringEditor(const PropertyInfo& prop, Rect editorRect,
                          DrawList& drawList, const ThemeEngine& theme);
    void DrawVec2Editor(const PropertyInfo& prop, Rect editorRect,
                        DrawList& drawList, const ThemeEngine& theme);
    void DrawColorEditor(const PropertyInfo& prop, Rect editorRect,
                         DrawList& drawList, const ThemeEngine& theme);

    void*                _target{nullptr};
    const TypeDescriptor* _typeDesc{nullptr};
    u32                  _fontId{0};
    f32                  _rowHeight{28.0f};
    f32                  _labelWidth{0.4f}; // fraction of total width
    f32                  _scrollOffset{0};

    // Drag editing state
    i32                  _editingProperty{-1};
    bool                 _isDragging{false};
    f32                  _dragStartValue{0};
    Vec2                 _dragStartPos;
};

} // namespace ace
