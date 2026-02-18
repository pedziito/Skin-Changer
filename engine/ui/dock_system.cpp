/**
 * ACE Engine — Dock System Implementation
 */

#include "dock_system.h"
#include <algorithm>

namespace ace {

DockSystem::DockSystem(std::string_view name) : Widget(name) {
    _rootNode = std::make_shared<DockNode>();
    _rootNode->id = AllocNodeId();
    SetFlag(WidgetFlags::ClipChildren);
}

u32 DockSystem::AllocNodeId() { return _nextNodeId++; }

void DockSystem::AddTab(std::string_view title, Widget* content, u32 targetNodeId) {
    DockNode* target = targetNodeId ? FindNode(targetNodeId) : _rootNode.get();
    if (!target || !target->IsLeaf()) target = _rootNode.get();

    DockNode::TabInfo tab;
    tab.id = _nextTabId++;
    tab.title = title;
    tab.content = content;
    target->tabs.push_back(std::move(tab));
    target->activeTab = static_cast<i32>(target->tabs.size()) - 1;
    MarkDirty();
}

void DockSystem::RemoveTab(u32 tabId) {
    // Search the tree for this tab
    std::function<bool(DockNode*)> removeFromNode =
        [&](DockNode* node) -> bool {
        if (!node) return false;
        if (node->IsLeaf()) {
            for (auto it = node->tabs.begin(); it != node->tabs.end(); ++it) {
                if (it->id == tabId) {
                    if (_onTabClosed) _onTabClosed(tabId);
                    node->tabs.erase(it);
                    node->activeTab = std::min(node->activeTab,
                                                (i32)node->tabs.size() - 1);
                    MarkDirty();
                    return true;
                }
            }
            return false;
        }
        return removeFromNode(node->left.get()) || removeFromNode(node->right.get());
    };
    removeFromNode(_rootNode.get());
}

void DockSystem::MoveTab(u32 tabId, u32 targetNodeId, i32 insertIndex) {
    // Find and remove tab from source
    DockNode::TabInfo movingTab;
    bool found = false;

    std::function<void(DockNode*)> extract = [&](DockNode* node) {
        if (!node || found) return;
        if (node->IsLeaf()) {
            for (auto it = node->tabs.begin(); it != node->tabs.end(); ++it) {
                if (it->id == tabId) {
                    movingTab = *it;
                    node->tabs.erase(it);
                    node->activeTab = std::min(node->activeTab,
                                                (i32)node->tabs.size() - 1);
                    found = true;
                    return;
                }
            }
        } else {
            extract(node->left.get());
            extract(node->right.get());
        }
    };
    extract(_rootNode.get());

    if (!found) return;

    // Insert into target
    DockNode* target = FindNode(targetNodeId);
    if (!target || !target->IsLeaf()) return;

    if (insertIndex < 0 || insertIndex >= (i32)target->tabs.size())
        target->tabs.push_back(movingTab);
    else
        target->tabs.insert(target->tabs.begin() + insertIndex, movingTab);

    target->activeTab = insertIndex < 0 ? (i32)target->tabs.size() - 1 : insertIndex;
    MarkDirty();
}

void DockSystem::SplitNode(u32 nodeId, SplitDirection dir, f32 ratio) {
    DockNode* node = FindNode(nodeId);
    if (!node || !node->IsLeaf()) return;

    // Move existing tabs to left child
    auto leftNode = std::make_shared<DockNode>();
    leftNode->id = AllocNodeId();
    leftNode->tabs = std::move(node->tabs);
    leftNode->activeTab = node->activeTab;

    auto rightNode = std::make_shared<DockNode>();
    rightNode->id = AllocNodeId();

    node->left = leftNode;
    node->right = rightNode;
    node->splitDir = dir;
    node->splitRatio = ratio;
    node->tabs.clear();
    node->activeTab = 0;
    MarkDirty();
}

void DockSystem::MergeNode(u32 nodeId) {
    DockNode* node = FindNode(nodeId);
    if (!node || node->IsLeaf()) return;

    // Collapse: merge all tabs from children into this node
    std::function<void(DockNode*, std::vector<DockNode::TabInfo>&)> collectTabs =
        [&](DockNode* n, std::vector<DockNode::TabInfo>& out) {
        if (!n) return;
        if (n->IsLeaf()) {
            for (auto& t : n->tabs) out.push_back(t);
        } else {
            collectTabs(n->left.get(), out);
            collectTabs(n->right.get(), out);
        }
    };

    std::vector<DockNode::TabInfo> allTabs;
    collectTabs(node, allTabs);

    node->left.reset();
    node->right.reset();
    node->splitDir = SplitDirection::None;
    node->tabs = std::move(allTabs);
    node->activeTab = 0;
    MarkDirty();
}

DockNode* DockSystem::FindNode(u32 nodeId) {
    return FindNodeRecursive(_rootNode.get(), nodeId);
}

DockNode* DockSystem::FindNodeRecursive(DockNode* node, u32 id) {
    if (!node) return nullptr;
    if (node->id == id) return node;
    auto* found = FindNodeRecursive(node->left.get(), id);
    return found ? found : FindNodeRecursive(node->right.get(), id);
}

DockNode* DockSystem::FindNodeContainingPoint(DockNode* node, Vec2 point) {
    if (!node) return nullptr;
    if (!node->bounds.Contains(point)) return nullptr;
    if (node->IsLeaf()) return node;
    auto* found = FindNodeContainingPoint(node->left.get(), point);
    return found ? found : FindNodeContainingPoint(node->right.get(), point);
}

// ============================================================================
// LAYOUT
// ============================================================================
void DockSystem::OnLayout() {
    _tabHeight = 30.0f;
    LayoutNode(_rootNode.get(), GetBounds());
}

void DockSystem::LayoutNode(DockNode* node, Rect bounds) {
    if (!node) return;
    node->bounds = bounds;

    if (node->IsLeaf()) return;

    Rect leftBounds, rightBounds;

    if (node->splitDir == SplitDirection::Horizontal) {
        f32 splitX = bounds.x + bounds.w * node->splitRatio;
        leftBounds = {bounds.x, bounds.y, splitX - bounds.x, bounds.h};
        rightBounds = {splitX, bounds.y, bounds.Right() - splitX, bounds.h};
    } else {
        f32 splitY = bounds.y + bounds.h * node->splitRatio;
        leftBounds = {bounds.x, bounds.y, bounds.w, splitY - bounds.y};
        rightBounds = {bounds.x, splitY, bounds.w, bounds.Bottom() - splitY};
    }

    LayoutNode(node->left.get(), leftBounds);
    LayoutNode(node->right.get(), rightBounds);
}

// ============================================================================
// DRAWING
// ============================================================================
void DockSystem::OnDraw(DrawList& drawList, const ThemeEngine& theme) {
    DrawNode(_rootNode.get(), drawList, theme);

    // Draw tab drag preview
    if (_isDragging) {
        // TODO: Draw floating tab ghost
    }

    // Draw drop preview
    if (_dropTarget && _dropPreview != DockSide::None) {
        Rect previewRect = _dropTarget->bounds;
        switch (_dropPreview) {
            case DockSide::Left:   previewRect.w *= 0.5f; break;
            case DockSide::Right:  previewRect.x += previewRect.w * 0.5f; previewRect.w *= 0.5f; break;
            case DockSide::Top:    previewRect.h *= 0.5f; break;
            case DockSide::Bottom: previewRect.y += previewRect.h * 0.5f; previewRect.h *= 0.5f; break;
            default: break;
        }
        drawList.AddFilledRect(previewRect, theme.GetColor(ThemeToken::AccentPrimary).WithAlpha(60));
    }
}

void DockSystem::DrawNode(DockNode* node, DrawList& drawList, const ThemeEngine& theme) {
    if (!node) return;

    if (node->IsLeaf()) {
        Rect tabBarRect{node->bounds.x, node->bounds.y, node->bounds.w, _tabHeight};
        Rect contentRect{node->bounds.x, node->bounds.y + _tabHeight,
                          node->bounds.w, node->bounds.h - _tabHeight};

        // Background
        drawList.AddFilledRect(contentRect, theme.GetColor(ThemeToken::BgSecondary));

        // Tab bar
        DrawTabBar(node, tabBarRect, drawList, theme);

        // Content border
        drawList.AddRect(node->bounds, theme.GetColor(ThemeToken::BorderSecondary));

        // Draw active tab content
        if (node->activeTab >= 0 && node->activeTab < (i32)node->tabs.size()) {
            auto* content = node->tabs[node->activeTab].content;
            if (content) {
                content->SetBounds(contentRect.Shrink(2));
            }
        }
    } else {
        DrawNode(node->left.get(), drawList, theme);
        DrawNode(node->right.get(), drawList, theme);

        // Draw splitter
        Rect splitterRect;
        if (node->splitDir == SplitDirection::Horizontal) {
            f32 splitX = node->bounds.x + node->bounds.w * node->splitRatio;
            splitterRect = {splitX - 2, node->bounds.y, 4, node->bounds.h};
        } else {
            f32 splitY = node->bounds.y + node->bounds.h * node->splitRatio;
            splitterRect = {node->bounds.x, splitY - 2, node->bounds.w, 4};
        }
        drawList.AddFilledRect(splitterRect, theme.GetColor(ThemeToken::BorderPrimary));
    }
}

void DockSystem::DrawTabBar(DockNode* node, Rect tabBarRect, DrawList& drawList, const ThemeEngine& theme) {
    drawList.AddFilledRect(tabBarRect, theme.GetColor(ThemeToken::BgHeader));

    f32 tabX = tabBarRect.x;
    f32 tabW = 120.0f;

    for (i32 i = 0; i < (i32)node->tabs.size(); ++i) {
        auto& tab = node->tabs[i];
        bool active = (i == node->activeTab);

        Rect tabRect{tabX, tabBarRect.y, tabW, tabBarRect.h};

        Color bg = active ? theme.GetColor(ThemeToken::BgSecondary)
                          : theme.GetColor(ThemeToken::BgTertiary);
        drawList.AddFilledRect(tabRect, bg);

        if (active) {
            // Active tab indicator
            drawList.AddFilledRect(
                {tabRect.x, tabRect.Bottom() - 2, tabRect.w, 2},
                theme.GetColor(ThemeToken::AccentPrimary));
        }

        // Tab border
        drawList.AddRect(tabRect, theme.GetColor(ThemeToken::BorderSecondary));

        // Close button (small X in top-right of tab)
        if (tab.closable) {
            Rect closeRect{tabRect.Right() - 18, tabRect.y + 6, 12, 12};
            Color closeColor = theme.GetColor(ThemeToken::TextSecondary);
            drawList.AddLine({closeRect.x + 2, closeRect.y + 2},
                            {closeRect.Right() - 2, closeRect.Bottom() - 2}, closeColor, 1.5f);
            drawList.AddLine({closeRect.Right() - 2, closeRect.y + 2},
                            {closeRect.x + 2, closeRect.Bottom() - 2}, closeColor, 1.5f);
        }

        tabX += tabW;
    }
}

// ============================================================================
// INPUT
// ============================================================================
void DockSystem::OnUpdate(f32 /*dt*/) {
    OnLayout();
}

bool DockSystem::OnMouseDown(Vec2 pos, MouseButtonEvent& /*e*/) {
    DockNode* hitNode = FindNodeContainingPoint(_rootNode.get(), pos);
    if (!hitNode || !hitNode->IsLeaf()) return false;

    // Check if clicked in tab bar
    Rect tabBar{hitNode->bounds.x, hitNode->bounds.y, hitNode->bounds.w, _tabHeight};
    if (tabBar.Contains(pos)) {
        f32 tabX = tabBar.x;
        f32 tabW = 120.0f;

        for (i32 i = 0; i < (i32)hitNode->tabs.size(); ++i) {
            Rect tabRect{tabX, tabBar.y, tabW, tabBar.h};

            // Check close button
            if (hitNode->tabs[i].closable) {
                Rect closeRect{tabRect.Right() - 18, tabRect.y + 6, 12, 12};
                if (closeRect.Contains(pos)) {
                    RemoveTab(hitNode->tabs[i].id);
                    return true;
                }
            }

            if (tabRect.Contains(pos)) {
                hitNode->activeTab = i;
                _isDragging = true;
                _dragTabId = hitNode->tabs[i].id;
                _dragSourceNodeId = hitNode->id;
                _dragOffset = pos - Vec2{tabX, tabBar.y};
                return true;
            }

            tabX += tabW;
        }
    }

    // Check splitter
    if (!hitNode->IsLeaf()) return false;

    return false;
}

bool DockSystem::OnMouseUp(Vec2 /*pos*/, MouseButtonEvent& /*e*/) {
    if (_isDragging) {
        // Check for drop target
        if (_dropTarget && _dropPreview != DockSide::None) {
            // Perform dock operation
            SplitDirection dir = (_dropPreview == DockSide::Left || _dropPreview == DockSide::Right)
                ? SplitDirection::Horizontal
                : SplitDirection::Vertical;

            SplitNode(_dropTarget->id, dir);

            DockNode* target = (_dropPreview == DockSide::Left || _dropPreview == DockSide::Top)
                ? _dropTarget->left.get()
                : _dropTarget->right.get();

            if (target) MoveTab(_dragTabId, target->id);
        }

        _isDragging = false;
        _dragTabId = 0;
        _dropTarget = nullptr;
        _dropPreview = DockSide::None;
        return true;
    }

    if (_isSplitterDragging) {
        _isSplitterDragging = false;
        _splitterNode = nullptr;
        return true;
    }

    return false;
}

bool DockSystem::OnMouseMove(Vec2 pos, MouseMoveEvent& /*e*/) {
    if (_isDragging) {
        // Find drop target
        DockNode* target = FindNodeContainingPoint(_rootNode.get(), pos);
        if (target && target->IsLeaf()) {
            _dropTarget = target;
            // Determine dock side
            Vec2 center = target->bounds.Center();
            Vec2 rel = pos - Vec2{target->bounds.x, target->bounds.y};

            f32 threshold = 0.3f;
            if (rel.x < target->bounds.w * threshold) _dropPreview = DockSide::Left;
            else if (rel.x > target->bounds.w * (1 - threshold)) _dropPreview = DockSide::Right;
            else if (rel.y < target->bounds.h * threshold) _dropPreview = DockSide::Top;
            else if (rel.y > target->bounds.h * (1 - threshold)) _dropPreview = DockSide::Bottom;
            else _dropPreview = DockSide::Fill;
        } else {
            _dropTarget = nullptr;
            _dropPreview = DockSide::None;
        }
        return true;
    }

    if (_isSplitterDragging && _splitterNode) {
        if (_splitterNode->splitDir == SplitDirection::Horizontal) {
            _splitterNode->splitRatio = math::Saturate(
                (pos.x - _splitterNode->bounds.x) / _splitterNode->bounds.w);
        } else {
            _splitterNode->splitRatio = math::Saturate(
                (pos.y - _splitterNode->bounds.y) / _splitterNode->bounds.h);
        }
        MarkDirty();
        return true;
    }

    return false;
}

} // namespace ace
