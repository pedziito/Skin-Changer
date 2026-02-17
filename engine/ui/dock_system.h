/**
 * ACE Engine — Dock System
 * Unreal Engine-style dockable window management.
 * Supports tab bars, split panes, drag-to-dock, floating windows.
 */

#pragma once

#include "widget.h"
#include "theme.h"
#include "../render/render_backend.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace ace {

// ============================================================================
// DOCK SPLIT DIRECTION
// ============================================================================
enum class SplitDirection : u8 {
    None,
    Horizontal,
    Vertical,
};

// ============================================================================
// DOCK NODE — Tree node in the dock layout
// ============================================================================
class DockNode {
public:
    using Ptr = std::shared_ptr<DockNode>;

    u32              id{0};
    Rect             bounds;
    SplitDirection   splitDir{SplitDirection::None};
    f32              splitRatio{0.5f};

    // If this is a leaf: tabs
    struct TabInfo {
        u32          id;
        std::string  title;
        Widget*      content{nullptr};
        bool         closable{true};
    };
    std::vector<TabInfo> tabs;
    i32                  activeTab{0};

    // If this is a branch: children
    Ptr left;
    Ptr right;

    bool IsLeaf() const { return !left && !right; }
    bool IsEmpty() const { return IsLeaf() && tabs.empty(); }

    DockNode() = default;
};

// ============================================================================
// DOCK SYSTEM — Manages the dock tree and interactions
// ============================================================================
class DockSystem : public Widget {
public:
    DockSystem(std::string_view name = "DockSystem");
    ~DockSystem() override = default;

    // --- Tab management ---
    void AddTab(std::string_view title, Widget* content, u32 targetNodeId = 0);
    void RemoveTab(u32 tabId);
    void MoveTab(u32 tabId, u32 targetNodeId, i32 insertIndex = -1);

    // --- Split operations ---
    void SplitNode(u32 nodeId, SplitDirection dir, f32 ratio = 0.5f);
    void MergeNode(u32 nodeId);

    // --- Layout ---
    void OnLayout() override;
    void OnDraw(DrawList& drawList, const ThemeEngine& theme) override;
    void OnUpdate(f32 dt) override;

    // --- Events ---
    bool OnMouseDown(Vec2 pos, MouseButtonEvent& e) override;
    bool OnMouseUp(Vec2 pos, MouseButtonEvent& e) override;
    bool OnMouseMove(Vec2 pos, MouseMoveEvent& e) override;

    // --- Query ---
    DockNode* GetRootNode() { return _rootNode.get(); }
    DockNode* FindNode(u32 nodeId);

    // --- Callbacks ---
    using TabClosedCallback = std::function<void(u32 tabId)>;
    void SetOnTabClosed(TabClosedCallback cb) { _onTabClosed = std::move(cb); }

private:
    void LayoutNode(DockNode* node, Rect bounds);
    void DrawNode(DockNode* node, DrawList& drawList, const ThemeEngine& theme);
    void DrawTabBar(DockNode* node, Rect tabBarRect, DrawList& drawList, const ThemeEngine& theme);
    DockNode* FindNodeRecursive(DockNode* node, u32 id);
    DockNode* FindNodeContainingPoint(DockNode* node, Vec2 point);
    u32 AllocNodeId();

    DockNode::Ptr       _rootNode;
    u32                 _nextNodeId{1};
    u32                 _nextTabId{1};

    // Drag state
    bool                _isDragging{false};
    u32                 _dragTabId{0};
    u32                 _dragSourceNodeId{0};
    Vec2                _dragOffset;

    // Splitter drag
    bool                _isSplitterDragging{false};
    DockNode*           _splitterNode{nullptr};

    // Drop preview
    DockSide            _dropPreview{DockSide::None};
    DockNode*           _dropTarget{nullptr};

    TabClosedCallback   _onTabClosed;

    f32                 _tabHeight{30.0f};
};

} // namespace ace
