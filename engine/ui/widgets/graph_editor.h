/**
 * ACE Engine — Graph Editor Widget
 *
 * General-purpose zoomable/pannable graph canvas for data-flow,
 * state machines, animation blend trees, behavior trees, etc.
 * Distinct from NodeEditor: this focuses on arbitrary graph topology,
 * multi-level grouping, and extensible rendering delegates.
 */

#pragma once
#include "../widget.h"
#include "../theme.h"
#include "../../core/types.h"
#include "../../render/render_backend.h"
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <memory>

namespace ace {

// ============================================================================
// GRAPH DATA STRUCTURES
// ============================================================================

enum class GraphNodeShape : u8 {
    Rectangle,
    RoundedRect,
    Ellipse,
    Diamond,
    Hexagon,
    Capsule,
    Custom
};

struct GraphPort {
    u32  id       = 0;
    std::string name;
    Color color  = Color::White();
    Vec2  offset  = {}; // Relative to node position, computed during layout
    bool  isInput = true;
};

struct GraphNodeStyle {
    Color           fill          = {50, 50, 56, 255};
    Color           border        = {70, 70, 80, 255};
    Color           headerColor   = {80, 80, 200, 255};
    Color           textColor     = Color::White();
    GraphNodeShape  shape         = GraphNodeShape::RoundedRect;
    f32             borderRadius  = 6.0f;
    f32             borderWidth   = 1.0f;
    bool            showHeader    = true;
    f32             headerHeight  = 26.0f;
};

struct GraphNode {
    u32                 id           = 0;
    std::string         title;
    std::string         subtitle;
    Vec2                position     = {};
    Vec2                size         = {160, 80};
    GraphNodeStyle      style        = {};
    std::vector<GraphPort> ports;
    bool                selected     = false;
    bool                collapsed    = false;
    bool                locked       = false;
    u32                 groupId      = 0; // 0 = no group
    void*               userData     = nullptr;

    Rect GetBounds() const { return {position.x, position.y, size.x, size.y}; }
};

enum class GraphEdgeStyle : u8 {
    Bezier,
    Straight,
    Step,       // Right-angle routing
    Spline      // Catmull-Rom
};

struct GraphEdge {
    u32             id          = 0;
    u32             srcNodeId   = 0;
    u32             srcPortId   = 0;
    u32             dstNodeId   = 0;
    u32             dstPortId   = 0;
    Color           color       = {150, 150, 200, 255};
    f32             thickness   = 2.0f;
    GraphEdgeStyle  style       = GraphEdgeStyle::Bezier;
    bool            animated    = false;  // Marching ants / flow effect
    f32             flowOffset  = 0.0f;   // For animation
    std::string     label;
};

struct GraphGroup {
    u32             id          = 0;
    std::string     title;
    Rect            bounds      = {};
    Color           fill        = {60, 60, 80, 40};
    Color           border      = {80, 80, 120, 100};
    bool            collapsed   = false;
};

// ============================================================================
// RENDER DELEGATE (for custom node rendering)
// ============================================================================
class IGraphNodeRenderer {
public:
    virtual ~IGraphNodeRenderer() = default;
    virtual void DrawNode(const GraphNode& node, DrawList& dl, Vec2 screenPos, Vec2 screenSize, f32 zoom) = 0;
    virtual void DrawEdge(const GraphEdge& /*edge*/, Vec2 /*p0*/, Vec2 /*p3*/, DrawList& /*dl*/, f32 /*zoom*/) {}
};

// ============================================================================
// GRAPH EDITOR
// ============================================================================
class GraphEditor : public Widget {
public:
    explicit GraphEditor(std::string_view name = "GraphEditor");

    // --- Node API ---
    u32         AddNode(std::string_view title, Vec2 position = {});
    void        RemoveNode(u32 nodeId);
    GraphNode*  GetNode(u32 nodeId);
    const std::vector<GraphNode>& GetNodes() const { return _nodes; }

    // --- Port API ---
    u32         AddPort(u32 nodeId, std::string_view name, bool isInput, Color color = Color::White());

    // --- Edge API ---
    u32         AddEdge(u32 srcNodeId, u32 srcPortId, u32 dstNodeId, u32 dstPortId);
    void        RemoveEdge(u32 edgeId);
    const std::vector<GraphEdge>& GetEdges() const { return _edges; }

    // --- Group API ---
    u32         AddGroup(std::string_view title, Rect bounds);
    void        RemoveGroup(u32 groupId);

    // --- Selection ---
    void        SelectNode(u32 nodeId);
    void        SelectMultiple(const std::vector<u32>& nodeIds);
    void        DeselectAll();
    void        DeleteSelected();
    std::vector<u32> GetSelectedNodeIds() const;

    // --- View Controls ---
    void    SetZoom(f32 zoom)          { _zoom = math::Clamp(zoom, _zoomMin, _zoomMax); }
    f32     GetZoom() const            { return _zoom; }
    void    SetPan(Vec2 pan)           { _pan = pan; }
    Vec2    GetPan() const             { return _pan; }
    void    SetZoomRange(f32 mn, f32 mx) { _zoomMin = mn; _zoomMax = mx; }
    void    FitToContent(f32 padding = 60.0f);
    void    CenterOnNode(u32 nodeId);

    // --- Coordinate transforms ---
    Vec2 ScreenToCanvas(Vec2 screenPos) const;
    Vec2 CanvasToScreen(Vec2 canvasPos) const;

    // --- Configuration ---
    void SetGridSize(f32 s)                    { _gridSize = s; }
    void SetSnapToGrid(bool v)                 { _snapToGrid = v; }
    void SetShowGrid(bool v)                   { _showGrid = v; }
    void SetShowMinimap(bool v)                { _showMinimap = v; }
    void SetEdgeStyle(GraphEdgeStyle style)    { _defaultEdgeStyle = style; }
    void SetRenderer(std::shared_ptr<IGraphNodeRenderer> r) { _renderer = std::move(r); }

    // --- Callbacks ---
    std::function<void(u32)>           onNodeSelected;
    std::function<void(u32)>           onNodeDeleted;
    std::function<void(u32)>           onEdgeCreated;
    std::function<void(u32)>           onEdgeDeleted;
    std::function<void(u32, Vec2)>     onNodeMoved;
    std::function<void(u32, u32)>      onContextMenu; // nodeId (0=canvas), portId (0=none)

protected:
    void OnUpdate(f32 dt) override;
    void OnDraw(DrawList& drawList, const ThemeEngine& theme) override;
    bool OnMouseDown(Vec2 pos, MouseButtonEvent& e) override;
    bool OnMouseUp(Vec2 pos, MouseButtonEvent& e) override;
    bool OnMouseMove(Vec2 pos, MouseMoveEvent& e) override;
    bool OnMouseScroll(f32 delta) override;
    bool OnKeyDown(KeyEvent& e) override;

private:
    // Data
    std::vector<GraphNode>  _nodes;
    std::vector<GraphEdge>  _edges;
    std::vector<GraphGroup> _groups;
    u32 _nextNodeId  = 1;
    u32 _nextEdgeId  = 1;
    u32 _nextPortId  = 1;
    u32 _nextGroupId = 1;

    // View
    Vec2  _pan   = {};
    f32   _zoom  = 1.0f;
    f32   _zoomMin = 0.1f;
    f32   _zoomMax = 5.0f;

    // Grid
    f32   _gridSize   = 20.0f;
    bool  _snapToGrid = false;
    bool  _showGrid   = true;
    bool  _showMinimap = true;

    // Interaction
    enum class Mode : u8 {
        None,
        Panning,
        DraggingNode,
        CreatingEdge,
        SelectionBox,
        DraggingGroup
    } _mode = Mode::None;

    u32   _dragNodeId = 0;
    Vec2  _dragOffset = {};
    u32   _edgeSrcPort = 0;
    Vec2  _edgeEndPos  = {};
    Vec2  _selStart    = {};
    Vec2  _selEnd      = {};

    // Render
    GraphEdgeStyle _defaultEdgeStyle = GraphEdgeStyle::Bezier;
    std::shared_ptr<IGraphNodeRenderer> _renderer;

    // -- Drawing helpers --
    void DrawGrid(DrawList& dl, const ThemeEngine& theme);
    void DrawGroups(DrawList& dl, const ThemeEngine& theme);
    void DrawEdges(DrawList& dl, f32 dt, const ThemeEngine& theme);
    void DrawNodes(DrawList& dl, const ThemeEngine& theme);
    void DrawEdgeCurve(Vec2 p0, Vec2 p3, const GraphEdge& edge, DrawList& dl);
    void DrawPort(const GraphPort& port, const GraphNode& node, DrawList& dl, const ThemeEngine& theme);
    void DrawMinimap(DrawList& dl, const ThemeEngine& theme);
    void DrawSelectionBox(DrawList& dl, const ThemeEngine& theme);

    // -- Hit testing --
    GraphNode* HitTestNode(Vec2 canvasPos);
    GraphPort* HitTestPort(Vec2 canvasPos, GraphNode** outNode = nullptr);
    GraphGroup* HitTestGroup(Vec2 canvasPos);

    // -- Snapping --
    Vec2 SnapPosition(Vec2 pos) const;

    f32 _animTime = 0.0f; // for flow animation
};

} // namespace ace
