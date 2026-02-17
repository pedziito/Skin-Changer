/**
 * ACE Engine — Node Editor (Blueprint System)
 * Visual scripting / node-based graph editor with:
 * - Draggable nodes with input/output pins
 * - Bezier curve links between pins
 * - Live link creation with drag
 * - Node selection, deletion, grouping
 * - Zoom and pan (via GraphEditor base)
 */

#pragma once

#include "../widget.h"
#include "../theme.h"
#include "../../core/types.h"
#include "../../render/render_backend.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace ace {

// ============================================================================
// PIN — Connection point on a node
// ============================================================================
enum class PinType : u8 {
    Flow,       // Execution flow
    Bool,
    Int,
    Float,
    String,
    Vec2,
    Vec3,
    Color,
    Object,
    Custom,
};

enum class PinDirection : u8 {
    Input,
    Output,
};

struct Pin {
    u32          id{0};
    std::string  name;
    PinType      type{PinType::Float};
    PinDirection direction{PinDirection::Input};
    Vec2         position;  // Computed during layout, in canvas space
    bool         connected{false};

    Color GetTypeColor() const {
        switch (type) {
            case PinType::Flow:   return {255, 255, 255};
            case PinType::Bool:   return {220, 48, 48};
            case PinType::Int:    return {68, 201, 245};
            case PinType::Float:  return {168, 230, 29};
            case PinType::String: return {243, 174, 235};
            case PinType::Vec2:   return {255, 185, 0};
            case PinType::Vec3:   return {255, 215, 0};
            case PinType::Color:  return {200, 100, 255};
            case PinType::Object: return {56, 166, 237};
            default:              return {180, 180, 180};
        }
    }
};

// ============================================================================
// NODE — Visual node in the graph
// ============================================================================
struct Node {
    u32                id{0};
    std::string        title;
    std::string        category;
    Vec2               position;        // In canvas space
    Vec2               size{180, 100};
    Color              color{99, 102, 241};
    std::vector<Pin>   inputs;
    std::vector<Pin>   outputs;
    bool               selected{false};
    bool               collapsed{false};

    // Computed bounds
    Rect GetBounds() const { return {position.x, position.y, size.x, size.y}; }
    Rect GetHeaderBounds() const { return {position.x, position.y, size.x, 28}; }
};

// ============================================================================
// LINK — Connection between two pins
// ============================================================================
struct Link {
    u32  id{0};
    u32  startPinId{0};
    u32  endPinId{0};
    u32  startNodeId{0};
    u32  endNodeId{0};
    Color color{139, 92, 246};
};

// ============================================================================
// NODE EDITOR WIDGET
// ============================================================================
class NodeEditor : public Widget {
public:
    NodeEditor(std::string_view name = "NodeEditor");

    // --- Node management ---
    u32  AddNode(std::string_view title, Vec2 position, Color color = {99, 102, 241});
    void RemoveNode(u32 nodeId);
    Node* GetNode(u32 nodeId);

    // --- Pin management ---
    u32  AddPin(u32 nodeId, std::string_view name, PinType type, PinDirection dir);
    Pin* GetPin(u32 pinId);

    // --- Link management ---
    u32  AddLink(u32 startPinId, u32 endPinId);
    void RemoveLink(u32 linkId);
    bool CanConnect(u32 startPinId, u32 endPinId) const;

    // --- Selection ---
    void SelectNode(u32 nodeId);
    void DeselectAll();
    std::vector<u32> GetSelectedNodes() const;
    void DeleteSelected();

    // --- View ---
    void SetZoom(f32 zoom) { _zoom = math::Clamp(zoom, 0.1f, 5.0f); }
    f32  GetZoom() const { return _zoom; }
    void SetPan(Vec2 pan) { _pan = pan; }
    Vec2 GetPan() const { return _pan; }
    void FitToContent();

    // --- Coordinate space ---
    Vec2 ScreenToCanvas(Vec2 screenPos) const;
    Vec2 CanvasToScreen(Vec2 canvasPos) const;

    // --- Widget overrides ---
    void OnDraw(DrawList& drawList, const ThemeEngine& theme) override;
    void OnUpdate(f32 dt) override;
    bool OnMouseDown(Vec2 pos, MouseButtonEvent& e) override;
    bool OnMouseUp(Vec2 pos, MouseButtonEvent& e) override;
    bool OnMouseMove(Vec2 pos, MouseMoveEvent& e) override;
    bool OnMouseScroll(f32 delta) override;
    bool OnKeyDown(KeyEvent& e) override;

    // --- Callbacks ---
    using NodeCallback = std::function<void(u32 nodeId)>;
    using LinkCallback = std::function<void(u32 linkId)>;
    void SetOnNodeSelected(NodeCallback cb)  { _onNodeSelected = std::move(cb); }
    void SetOnNodeDeleted(NodeCallback cb)   { _onNodeDeleted = std::move(cb); }
    void SetOnLinkCreated(LinkCallback cb)   { _onLinkCreated = std::move(cb); }

private:
    void LayoutNode(Node& node);
    void DrawGrid(DrawList& drawList, const ThemeEngine& theme);
    void DrawNode(Node& node, DrawList& drawList, const ThemeEngine& theme);
    void DrawPin(const Pin& pin, DrawList& drawList, const ThemeEngine& theme);
    void DrawLink(const Link& link, DrawList& drawList, const ThemeEngine& theme);
    void DrawLinkCreation(DrawList& drawList, const ThemeEngine& theme);
    void DrawSelectionBox(DrawList& drawList, const ThemeEngine& theme);

    Pin* FindPinAt(Vec2 canvasPos, f32 radius = 10.0f);
    Node* FindNodeAt(Vec2 canvasPos);
    std::pair<Node*, u32> FindNodeAndPinOwner(u32 pinId);

    // Data
    std::vector<Node>  _nodes;
    std::vector<Link>  _links;
    u32                _nextNodeId{1};
    u32                _nextPinId{1};
    u32                _nextLinkId{1};

    // View transform
    f32                _zoom{1.0f};
    Vec2               _pan{0, 0};

    // Interaction state
    enum class InteractionMode {
        None,
        Panning,
        DraggingNode,
        CreatingLink,
        SelectionBox,
    };
    InteractionMode    _mode{InteractionMode::None};

    // Node dragging
    u32                _dragNodeId{0};
    Vec2               _dragOffset;

    // Link creation
    u32                _linkStartPinId{0};
    Vec2               _linkEndPos;

    // Selection box
    Vec2               _selBoxStart;
    Vec2               _selBoxEnd;

    // Minimap
    bool               _showMinimap{true};

    // Callbacks
    NodeCallback       _onNodeSelected;
    NodeCallback       _onNodeDeleted;
    LinkCallback       _onLinkCreated;

    f32                _pinRadius{6.0f};
    f32                _gridSize{32.0f};
};

} // namespace ace
