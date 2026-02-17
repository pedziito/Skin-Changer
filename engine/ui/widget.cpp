/**
 * ACE Engine — Widget Implementation
 */

#include "widget.h"

namespace ace {

Widget::Widget(std::string_view name) : _name(name) {
    _id = HashRuntime(name);
}

Widget* Widget::AddChild(Ptr child) {
    child->_parent = this;
    _children.push_back(std::move(child));
    MarkDirty();
    return _children.back().get();
}

Widget* Widget::RemoveChild(Widget* child) {
    for (auto it = _children.begin(); it != _children.end(); ++it) {
        if (it->get() == child) {
            child->_parent = nullptr;
            _children.erase(it);
            MarkDirty();
            return child;
        }
    }
    return nullptr;
}

void Widget::ClearChildren() {
    for (auto& c : _children) c->_parent = nullptr;
    _children.clear();
    MarkDirty();
}

Widget* Widget::FindChild(std::string_view name) const {
    for (auto& c : _children)
        if (c->_name == name) return c.get();
    return nullptr;
}

Widget* Widget::FindChildRecursive(std::string_view name) const {
    for (auto& c : _children) {
        if (c->_name == name) return c.get();
        auto* found = c->FindChildRecursive(name);
        if (found) return found;
    }
    return nullptr;
}

Rect Widget::GetContentBounds() const {
    return {
        _bounds.x + _layout.padding.left,
        _bounds.y + _layout.padding.top,
        _bounds.w - _layout.padding.Horizontal(),
        _bounds.h - _layout.padding.Vertical()
    };
}

bool Widget::HitTest(Vec2 point) const {
    return _bounds.Contains(point) && IsVisible() && IsEnabled();
}

Widget* Widget::HitTestRecursive(Vec2 point) {
    if (!IsVisible() || !_bounds.Contains(point)) return nullptr;

    // Test children in reverse (top-most first)
    for (auto it = _children.rbegin(); it != _children.rend(); ++it) {
        auto* hit = (*it)->HitTestRecursive(point);
        if (hit) return hit;
    }

    return IsEnabled() ? this : nullptr;
}

} // namespace ace
