/**
 * ACE Engine — Layout Engine
 * Flexbox-inspired layout algorithm with support for:
 * - FlexRow / FlexColumn (with grow, shrink, basis)
 * - Stack (overlay children)
 * - Dock (top/bottom/left/right/fill)
 * - Padding, margin, gap, min/max constraints
 */

#pragma once

#include "widget.h"
#include <algorithm>

namespace ace {

class LayoutEngine {
public:
    /**
     * Compute layout for a widget tree recursively.
     */
    static void ComputeLayout(Widget* root) {
        if (!root) return;

        auto& layout = root->Layout();

        switch (layout.mode) {
            case LayoutMode::FlexRow:    LayoutFlex(root, true); break;
            case LayoutMode::FlexColumn: LayoutFlex(root, false); break;
            case LayoutMode::Stack:      LayoutStack(root); break;
            case LayoutMode::Dock:       LayoutDock(root); break;
            default: break;
        }

        // Recurse into children
        for (auto& child : root->GetChildren()) {
            ComputeLayout(child.get());
        }

        root->ClearFlag(WidgetFlags::NeedsLayout);
    }

private:
    // ========================================================================
    // FLEX LAYOUT (Row or Column)
    // ========================================================================
    static void LayoutFlex(Widget* parent, bool horizontal) {
        auto& children = parent->GetChildren();
        if (children.empty()) return;

        Rect content = parent->GetContentBounds();
        f32 mainStart = horizontal ? content.x : content.y;
        f32 mainSize  = horizontal ? content.w : content.h;
        f32 crossSize = horizontal ? content.h : content.w;
        f32 gap = parent->Layout().gap;

        // Phase 1: Measure children, sum up flex bases
        f32 totalBasis = 0;
        f32 totalGrow = 0;
        f32 totalShrink = 0;
        i32 visibleCount = 0;

        for (auto& child : children) {
            if (!child->IsVisible()) continue;
            auto& cl = child->Layout();

            f32 basis = cl.flexBasis >= 0 ? cl.flexBasis
                      : (horizontal ? cl.preferredSize.x : cl.preferredSize.y);
            if (basis <= 0) basis = horizontal ? child->GetSize().x : child->GetSize().y;

            totalBasis += basis + cl.margin.left + cl.margin.right;
            totalGrow += cl.flexGrow;
            totalShrink += cl.flexShrink;
            visibleCount++;
        }

        f32 totalGap = gap * std::max(0, visibleCount - 1);
        f32 freeSpace = mainSize - totalBasis - totalGap;

        // Phase 2: Distribute space
        f32 cursor = mainStart;
        for (auto& child : children) {
            if (!child->IsVisible()) continue;
            auto& cl = child->Layout();

            f32 basis = cl.flexBasis >= 0 ? cl.flexBasis
                      : (horizontal ? cl.preferredSize.x : cl.preferredSize.y);
            if (basis <= 0) basis = horizontal ? child->GetSize().x : child->GetSize().y;

            f32 extra = 0;
            if (freeSpace > 0 && totalGrow > 0)
                extra = freeSpace * (cl.flexGrow / totalGrow);
            else if (freeSpace < 0 && totalShrink > 0)
                extra = freeSpace * (cl.flexShrink / totalShrink);

            f32 childMainSize = std::max(0.0f, basis + extra);
            childMainSize = std::clamp(childMainSize,
                horizontal ? cl.minSize.x : cl.minSize.y,
                horizontal ? cl.maxSize.x : cl.maxSize.y);

            // Cross-axis sizing
            f32 childCrossSize = crossSize - (horizontal
                ? cl.margin.top + cl.margin.bottom
                : cl.margin.left + cl.margin.right);

            childCrossSize = std::clamp(childCrossSize,
                horizontal ? cl.minSize.y : cl.minSize.x,
                horizontal ? cl.maxSize.y : cl.maxSize.x);

            // Cross-axis alignment
            f32 crossStart = horizontal ? content.y : content.x;
            f32 crossOffset = 0;

            Alignment align = horizontal
                ? parent->Layout().alignY
                : parent->Layout().alignX;

            switch (align) {
                case Alignment::Start:   crossOffset = 0; break;
                case Alignment::Center:  crossOffset = (crossSize - childCrossSize) * 0.5f; break;
                case Alignment::End:     crossOffset = crossSize - childCrossSize; break;
                case Alignment::Stretch: childCrossSize = crossSize; break;
            }

            // Set bounds
            f32 marginBefore = horizontal ? cl.margin.left : cl.margin.top;
            f32 marginCross = horizontal ? cl.margin.top : cl.margin.left;

            if (horizontal) {
                child->SetBounds({
                    cursor + marginBefore,
                    crossStart + crossOffset + marginCross,
                    childMainSize,
                    childCrossSize
                });
            } else {
                child->SetBounds({
                    crossStart + crossOffset + marginCross,
                    cursor + marginBefore,
                    childCrossSize,
                    childMainSize
                });
            }

            cursor += childMainSize + marginBefore
                    + (horizontal ? cl.margin.right : cl.margin.bottom) + gap;
        }
    }

    // ========================================================================
    // STACK LAYOUT (overlay all children)
    // ========================================================================
    static void LayoutStack(Widget* parent) {
        Rect content = parent->GetContentBounds();
        for (auto& child : parent->GetChildren()) {
            if (!child->IsVisible()) continue;
            auto& cl = child->Layout();

            f32 w = cl.preferredSize.x > 0 ? cl.preferredSize.x : content.w;
            f32 h = cl.preferredSize.y > 0 ? cl.preferredSize.y : content.h;

            w = std::clamp(w, cl.minSize.x, cl.maxSize.x);
            h = std::clamp(h, cl.minSize.y, cl.maxSize.y);

            f32 x = content.x, y = content.y;

            // Align within parent
            switch (cl.alignX) {
                case Alignment::Center: x += (content.w - w) * 0.5f; break;
                case Alignment::End:    x += content.w - w; break;
                default: break;
            }
            switch (cl.alignY) {
                case Alignment::Center: y += (content.h - h) * 0.5f; break;
                case Alignment::End:    y += content.h - h; break;
                default: break;
            }

            child->SetBounds({x, y, w, h});
        }
    }

    // ========================================================================
    // DOCK LAYOUT (top/bottom/left/right/fill)
    // ========================================================================
    static void LayoutDock(Widget* parent) {
        Rect remaining = parent->GetContentBounds();

        // First pass: dock non-fill children
        for (auto& child : parent->GetChildren()) {
            if (!child->IsVisible()) continue;
            auto& cl = child->Layout();
            if (cl.dockSide == DockSide::Fill || cl.dockSide == DockSide::None) continue;

            f32 size;

            switch (cl.dockSide) {
                case DockSide::Top:
                    size = cl.preferredSize.y > 0 ? cl.preferredSize.y : 30;
                    child->SetBounds({remaining.x, remaining.y, remaining.w, size});
                    remaining.y += size;
                    remaining.h -= size;
                    break;

                case DockSide::Bottom:
                    size = cl.preferredSize.y > 0 ? cl.preferredSize.y : 30;
                    child->SetBounds({remaining.x, remaining.Bottom() - size, remaining.w, size});
                    remaining.h -= size;
                    break;

                case DockSide::Left:
                    size = cl.preferredSize.x > 0 ? cl.preferredSize.x : 200;
                    child->SetBounds({remaining.x, remaining.y, size, remaining.h});
                    remaining.x += size;
                    remaining.w -= size;
                    break;

                case DockSide::Right:
                    size = cl.preferredSize.x > 0 ? cl.preferredSize.x : 200;
                    child->SetBounds({remaining.Right() - size, remaining.y, size, remaining.h});
                    remaining.w -= size;
                    break;

                default: break;
            }
        }

        // Second pass: fill remaining space
        for (auto& child : parent->GetChildren()) {
            if (!child->IsVisible()) continue;
            if (child->Layout().dockSide == DockSide::Fill) {
                child->SetBounds(remaining);
            }
        }
    }
};

} // namespace ace
