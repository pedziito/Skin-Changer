/**
 * ACE Engine — Node Editor Implementation
 */

#include "node_editor.h"
#include <algorithm>

namespace ace {

NodeEditor::NodeEditor(std::string_view name) : Widget(name) {
    _layout.preferredSize = {800, 600};
    SetFlag(WidgetFlags::ClipChildren | WidgetFlags::CapturesMouse | WidgetFlags::Focusable);
}

// ============================================================================
// NODE MANAGEMENT
// ============================================================================
u32 NodeEditor::AddNode(std::string_view title, Vec2 position, Color color) {
    Node node;
    node.id = _nextNodeId++;
    node.title = title;
    node.position = position;
    node.color = color;
    _nodes.push_back(std::move(node));
    return _nodes.back().id;
}

void NodeEditor::RemoveNode(u32 nodeId) {
    // Remove links connected to this node
    _links.erase(std::remove_if(_links.begin(), _links.end(),
        [nodeId](const Link& l) { return l.startNodeId == nodeId || l.endNodeId == nodeId; }),
        _links.end());

    _nodes.erase(std::remove_if(_nodes.begin(), _nodes.end(),
        [nodeId](const Node& n) { return n.id == nodeId; }),
        _nodes.end());
}

Node* NodeEditor::GetNode(u32 nodeId) {
    for (auto& n : _nodes) if (n.id == nodeId) return &n;
    return nullptr;
}

// ============================================================================
// PIN MANAGEMENT
// ============================================================================
u32 NodeEditor::AddPin(u32 nodeId, std::string_view name, PinType type, PinDirection dir) {
    auto* node = GetNode(nodeId);
    if (!node) return 0;

    Pin pin;
    pin.id = _nextPinId++;
    pin.name = name;
    pin.type = type;
    pin.direction = dir;

    if (dir == PinDirection::Input)
        node->inputs.push_back(std::move(pin));
    else
        node->outputs.push_back(std::move(pin));

    LayoutNode(*node);
    return (dir == PinDirection::Input)
        ? node->inputs.back().id
        : node->outputs.back().id;
}

Pin* NodeEditor::GetPin(u32 pinId) {
    for (auto& node : _nodes) {
        for (auto& pin : node.inputs) if (pin.id == pinId) return &pin;
        for (auto& pin : node.outputs) if (pin.id == pinId) return &pin;
    }
    return nullptr;
}

// ============================================================================
// LINK MANAGEMENT
// ============================================================================
u32 NodeEditor::AddLink(u32 startPinId, u32 endPinId) {
    if (!CanConnect(startPinId, endPinId)) return 0;

    // Find parent nodes
    auto [startNode, startIdx] = FindNodeAndPinOwner(startPinId);
    auto [endNode, endIdx] = FindNodeAndPinOwner(endPinId);
    if (!startNode || !endNode) return 0;

    Pin* startPin = GetPin(startPinId);
    Pin* endPin = GetPin(endPinId);
    if (!startPin || !endPin) return 0;

    startPin->connected = true;
    endPin->connected = true;

    Link link;
    link.id = _nextLinkId++;
    link.startPinId = startPinId;
    link.endPinId = endPinId;
    link.startNodeId = startNode->id;
    link.endNodeId = endNode->id;
    link.color = startPin->GetTypeColor();
    _links.push_back(link);

    if (_onLinkCreated) _onLinkCreated(link.id);
    return link.id;
}

void NodeEditor::RemoveLink(u32 linkId) {
    auto it = std::find_if(_links.begin(), _links.end(),
        [linkId](const Link& l) { return l.id == linkId; });
    if (it != _links.end()) {
        // Clear connected flags
        if (auto* pin = GetPin(it->startPinId)) pin->connected = false;
        if (auto* pin = GetPin(it->endPinId)) pin->connected = false;
        _links.erase(it);
    }
}

bool NodeEditor::CanConnect(u32 startPinId, u32 endPinId) const {
    if (startPinId == endPinId) return false;

    const Pin* start = nullptr;
    const Pin* end = nullptr;
    const Node* startNode = nullptr;
    const Node* endNode = nullptr;

    for (auto& node : _nodes) {
        for (auto& pin : node.inputs) {
            if (pin.id == startPinId) { start = &pin; startNode = &node; }
            if (pin.id == endPinId) { end = &pin; endNode = &node; }
        }
        for (auto& pin : node.outputs) {
            if (pin.id == startPinId) { start = &pin; startNode = &node; }
            if (pin.id == endPinId) { end = &pin; endNode = &node; }
        }
    }

    if (!start || !end) return false;
    if (startNode == endNode) return false; // No self-connection
    if (start->direction == end->direction) return false; // Must be input -> output
    if (start->type != end->type && start->type != PinType::Flow && end->type != PinType::Flow)
        return false; // Type mismatch (flow connects to anything)

    return true;
}

// ============================================================================
// SELECTION
// ============================================================================
void NodeEditor::SelectNode(u32 nodeId) {
    for (auto& n : _nodes) n.selected = (n.id == nodeId);
    if (_onNodeSelected) _onNodeSelected(nodeId);
}

void NodeEditor::DeselectAll() {
    for (auto& n : _nodes) n.selected = false;
}

std::vector<u32> NodeEditor::GetSelectedNodes() const {
    std::vector<u32> result;
    for (auto& n : _nodes) if (n.selected) result.push_back(n.id);
    return result;
}

void NodeEditor::DeleteSelected() {
    for (auto& n : _nodes) {
        if (n.selected) {
            if (_onNodeDeleted) _onNodeDeleted(n.id);
        }
    }
    // Remove links first
    _links.erase(std::remove_if(_links.begin(), _links.end(),
        [this](const Link& l) {
            auto* node = GetNode(l.startNodeId);
            auto* node2 = GetNode(l.endNodeId);
            return (node && node->selected) || (node2 && node2->selected);
        }), _links.end());

    _nodes.erase(std::remove_if(_nodes.begin(), _nodes.end(),
        [](const Node& n) { return n.selected; }), _nodes.end());
}

// ============================================================================
// VIEW
// ============================================================================
void NodeEditor::FitToContent() {
    if (_nodes.empty()) return;

    f32 minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
    for (auto& n : _nodes) {
        minX = std::min(minX, n.position.x);
        minY = std::min(minY, n.position.y);
        maxX = std::max(maxX, n.position.x + n.size.x);
        maxY = std::max(maxY, n.position.y + n.size.y);
    }

    Vec2 contentSize{maxX - minX, maxY - minY};
    Vec2 viewSize = GetSize();

    _zoom = std::min(viewSize.x / (contentSize.x + 100), viewSize.y / (contentSize.y + 100));
    _zoom = math::Clamp(_zoom, 0.1f, 2.0f);

    _pan = Vec2{(viewSize.x - contentSize.x * _zoom) * 0.5f - minX * _zoom,
                 (viewSize.y - contentSize.y * _zoom) * 0.5f - minY * _zoom};
}

Vec2 NodeEditor::ScreenToCanvas(Vec2 screenPos) const {
    Vec2 local = screenPos - GetPosition();
    return (local - _pan) / _zoom;
}

Vec2 NodeEditor::CanvasToScreen(Vec2 canvasPos) const {
    return canvasPos * _zoom + _pan + GetPosition();
}

// ============================================================================
// LAYOUT
// ============================================================================
void NodeEditor::LayoutNode(Node& node) {
    f32 headerH = 28.0f;
    f32 pinSpacing = 22.0f;
    f32 pinMarginTop = headerH + 8.0f;

    i32 maxPins = std::max((i32)node.inputs.size(), (i32)node.outputs.size());
    node.size.y = pinMarginTop + maxPins * pinSpacing + 12.0f;

    // Position input pins (left side)
    for (i32 i = 0; i < (i32)node.inputs.size(); ++i) {
        node.inputs[i].position = {
            node.position.x,
            node.position.y + pinMarginTop + i * pinSpacing
        };
    }

    // Position output pins (right side)
    for (i32 i = 0; i < (i32)node.outputs.size(); ++i) {
        node.outputs[i].position = {
            node.position.x + node.size.x,
            node.position.y + pinMarginTop + i * pinSpacing
        };
    }
}

// ============================================================================
// UPDATE
// ============================================================================
void NodeEditor::OnUpdate(f32 dt) {
    for (auto& node : _nodes) LayoutNode(node);
}

// ============================================================================
// DRAWING
// ============================================================================
void NodeEditor::OnDraw(DrawList& drawList, const ThemeEngine& theme) {
    Rect bounds = GetBounds();
    drawList.PushClipRect(bounds);

    // Background
    drawList.AddFilledRect(bounds, theme.GetColor(ThemeToken::BgPrimary));

    // Grid
    DrawGrid(drawList, theme);

    // Links (draw behind nodes)
    for (auto& link : _links) DrawLink(link, drawList, theme);

    // Link being created
    if (_mode == InteractionMode::CreatingLink) {
        DrawLinkCreation(drawList, theme);
    }

    // Nodes
    for (auto& node : _nodes) DrawNode(node, drawList, theme);

    // Selection box
    if (_mode == InteractionMode::SelectionBox) {
        DrawSelectionBox(drawList, theme);
    }

    // Minimap (small overview in corner)
    if (_showMinimap && !_nodes.empty()) {
        Rect mmRect{bounds.Right() - 160, bounds.Bottom() - 120, 150, 110};
        drawList.AddFilledRoundRect(mmRect, theme.GetColor(ThemeToken::BgOverlay), 4);
        drawList.AddRect(mmRect, theme.GetColor(ThemeToken::BorderPrimary));
    }

    drawList.PopClipRect();
}

void NodeEditor::DrawGrid(DrawList& drawList, const ThemeEngine& theme) {
    Rect bounds = GetBounds();
    Color gridColor = theme.GetColor(ThemeToken::BorderSecondary).WithAlpha(30);
    Color gridColorMajor = theme.GetColor(ThemeToken::BorderSecondary).WithAlpha(60);

    f32 step = _gridSize * _zoom;
    if (step < 4.0f) return; // Too zoomed out

    f32 offsetX = std::fmod(_pan.x, step);
    f32 offsetY = std::fmod(_pan.y, step);

    i32 majorEvery = 4;
    i32 ix = 0, iy = 0;

    for (f32 x = bounds.x + offsetX; x < bounds.Right(); x += step, ++ix) {
        Color c = (ix % majorEvery == 0) ? gridColorMajor : gridColor;
        drawList.AddLine({x, bounds.y}, {x, bounds.Bottom()}, c);
    }
    for (f32 y = bounds.y + offsetY; y < bounds.Bottom(); y += step, ++iy) {
        Color c = (iy % majorEvery == 0) ? gridColorMajor : gridColor;
        drawList.AddLine({bounds.x, y}, {bounds.Right(), y}, c);
    }
}

void NodeEditor::DrawNode(Node& node, DrawList& drawList, const ThemeEngine& theme) {
    Vec2 screenPos = CanvasToScreen(node.position);
    Vec2 screenSize = node.size * _zoom;
    Rect nodeRect{screenPos.x, screenPos.y, screenSize.x, screenSize.y};
    Rect headerRect{nodeRect.x, nodeRect.y, nodeRect.w, 28 * _zoom};

    // Shadow
    drawList.AddFilledRoundRect(nodeRect.Expand(4), theme.GetColor(ThemeToken::Shadow), 8);

    // Body
    drawList.AddFilledRoundRect(nodeRect, theme.GetColor(ThemeToken::NodeBg), 6);

    // Header
    drawList.AddFilledRoundRect(headerRect, node.color, 6);
    // Bottom corners of header are square
    drawList.AddFilledRect({headerRect.x, headerRect.Bottom() - 6, headerRect.w, 6}, node.color);

    // Selection highlight
    if (node.selected) {
        drawList.AddRect(nodeRect.Expand(1), theme.GetColor(ThemeToken::AccentPrimary), 2);
    }

    // Border
    drawList.AddRect(nodeRect, theme.GetColor(ThemeToken::NodeBorder));

    // Pins
    for (auto& pin : node.inputs) DrawPin(pin, drawList, theme);
    for (auto& pin : node.outputs) DrawPin(pin, drawList, theme);
}

void NodeEditor::DrawPin(const Pin& pin, DrawList& drawList, const ThemeEngine& theme) {
    Vec2 screenPos = CanvasToScreen(pin.position);
    f32 r = _pinRadius * _zoom;

    Color pinColor = pin.GetTypeColor();
    if (pin.connected) {
        drawList.AddCircle(screenPos, r, pinColor);
    } else {
        // Hollow circle for unconnected
        drawList.AddCircle(screenPos, r, pinColor);
        drawList.AddCircle(screenPos, r * 0.5f, theme.GetColor(ThemeToken::NodeBg));
    }
}

void NodeEditor::DrawLink(const Link& link, DrawList& drawList, const ThemeEngine& theme) {
    Pin* startPin = GetPin(link.startPinId);
    Pin* endPin = GetPin(link.endPinId);
    if (!startPin || !endPin) return;

    Vec2 p0 = CanvasToScreen(startPin->position);
    Vec2 p3 = CanvasToScreen(endPin->position);

    // Control points for smooth bezier
    f32 tangentLen = std::max(50.0f * _zoom, std::abs(p3.x - p0.x) * 0.5f);
    Vec2 p1{p0.x + tangentLen, p0.y};
    Vec2 p2{p3.x - tangentLen, p3.y};

    drawList.AddBezierCurve(p0, p1, p2, p3, link.color, 2.0f * _zoom, 32);
}

void NodeEditor::DrawLinkCreation(DrawList& drawList, const ThemeEngine& theme) {
    Pin* startPin = GetPin(_linkStartPinId);
    if (!startPin) return;

    Vec2 p0 = CanvasToScreen(startPin->position);
    Vec2 p3 = _linkEndPos;

    f32 tangentLen = std::max(50.0f * _zoom, std::abs(p3.x - p0.x) * 0.5f);
    Vec2 p1, p2;

    if (startPin->direction == PinDirection::Output) {
        p1 = {p0.x + tangentLen, p0.y};
        p2 = {p3.x - tangentLen, p3.y};
    } else {
        p1 = {p0.x - tangentLen, p0.y};
        p2 = {p3.x + tangentLen, p3.y};
    }

    drawList.AddBezierCurve(p0, p1, p2, p3, startPin->GetTypeColor().WithAlpha(180), 2.0f, 32);
}

void NodeEditor::DrawSelectionBox(DrawList& drawList, const ThemeEngine& theme) {
    Rect selRect = Rect::FromMinMax(
        Vec2::Min(_selBoxStart, _selBoxEnd),
        Vec2::Max(_selBoxStart, _selBoxEnd));

    drawList.AddFilledRect(selRect, theme.GetColor(ThemeToken::AccentPrimary).WithAlpha(30));
    drawList.AddRect(selRect, theme.GetColor(ThemeToken::AccentPrimary), 1.0f);
}

// ============================================================================
// INPUT HANDLING
// ============================================================================
bool NodeEditor::OnMouseDown(Vec2 pos, MouseButtonEvent& e) {
    Vec2 canvasPos = ScreenToCanvas(pos);

    if (e.button == 1) { // Right-click: pan
        _mode = InteractionMode::Panning;
        return true;
    }

    // Check pin click first
    Pin* pin = FindPinAt(canvasPos);
    if (pin) {
        _mode = InteractionMode::CreatingLink;
        _linkStartPinId = pin->id;
        _linkEndPos = pos;
        return true;
    }

    // Check node click
    Node* node = FindNodeAt(canvasPos);
    if (node) {
        _mode = InteractionMode::DraggingNode;
        _dragNodeId = node->id;
        _dragOffset = canvasPos - node->position;

        // Selection
        if (!node->selected) {
            DeselectAll();
            node->selected = true;
            if (_onNodeSelected) _onNodeSelected(node->id);
        }
        return true;
    }

    // Empty space: start selection box
    DeselectAll();
    _mode = InteractionMode::SelectionBox;
    _selBoxStart = pos;
    _selBoxEnd = pos;
    return true;
}

bool NodeEditor::OnMouseUp(Vec2 pos, MouseButtonEvent& e) {
    if (_mode == InteractionMode::CreatingLink) {
        Vec2 canvasPos = ScreenToCanvas(pos);
        Pin* endPin = FindPinAt(canvasPos);
        if (endPin && endPin->id != _linkStartPinId) {
            AddLink(_linkStartPinId, endPin->id);
        }
    }

    if (_mode == InteractionMode::SelectionBox) {
        // Select nodes in box
        Rect selRect = Rect::FromMinMax(
            Vec2::Min(_selBoxStart, _selBoxEnd),
            Vec2::Max(_selBoxStart, _selBoxEnd));

        for (auto& node : _nodes) {
            Vec2 screenPos = CanvasToScreen(node.position);
            if (selRect.Contains(screenPos)) {
                node.selected = true;
            }
        }
    }

    _mode = InteractionMode::None;
    return true;
}

bool NodeEditor::OnMouseMove(Vec2 pos, MouseMoveEvent& e) {
    switch (_mode) {
        case InteractionMode::Panning:
            _pan = _pan + e.delta;
            return true;

        case InteractionMode::DraggingNode: {
            Vec2 canvasPos = ScreenToCanvas(pos);
            // Move selected nodes
            for (auto& node : _nodes) {
                if (node.selected) {
                    if (node.id == _dragNodeId) {
                        node.position = canvasPos - _dragOffset;
                    } else {
                        node.position = node.position + e.delta / _zoom;
                    }
                }
            }
            return true;
        }

        case InteractionMode::CreatingLink:
            _linkEndPos = pos;
            return true;

        case InteractionMode::SelectionBox:
            _selBoxEnd = pos;
            return true;

        default:
            return false;
    }
}

bool NodeEditor::OnMouseScroll(f32 delta) {
    f32 oldZoom = _zoom;
    _zoom = math::Clamp(_zoom + delta * 0.1f, 0.1f, 5.0f);

    // Zoom towards mouse position (keep point under cursor stable)
    // pan_new = mouse - (mouse - pan_old) * (zoom_new / zoom_old)
    // Simplified:
    f32 ratio = _zoom / oldZoom;
    Vec2 mouseLocal = GetPosition() - _pan; // approximate
    _pan = _pan * ratio;

    return true;
}

bool NodeEditor::OnKeyDown(KeyEvent& e) {
    if (e.keyCode == static_cast<i32>(Key::Delete)) {
        DeleteSelected();
        return true;
    }
    if (e.ctrl && e.keyCode == static_cast<i32>(Key::A)) {
        for (auto& n : _nodes) n.selected = true;
        return true;
    }
    if (e.keyCode == static_cast<i32>(Key::F)) {
        FitToContent();
        return true;
    }
    return false;
}

// ============================================================================
// HELPERS
// ============================================================================
Pin* NodeEditor::FindPinAt(Vec2 canvasPos, f32 radius) {
    for (auto& node : _nodes) {
        for (auto& pin : node.inputs) {
            if ((pin.position - canvasPos).LenSq() < radius * radius)
                return &pin;
        }
        for (auto& pin : node.outputs) {
            if ((pin.position - canvasPos).LenSq() < radius * radius)
                return &pin;
        }
    }
    return nullptr;
}

Node* NodeEditor::FindNodeAt(Vec2 canvasPos) {
    // Reverse order: topmost first
    for (auto it = _nodes.rbegin(); it != _nodes.rend(); ++it) {
        if (it->GetBounds().Contains(canvasPos)) return &(*it);
    }
    return nullptr;
}

std::pair<Node*, u32> NodeEditor::FindNodeAndPinOwner(u32 pinId) {
    for (auto& node : _nodes) {
        for (auto& pin : node.inputs)
            if (pin.id == pinId) return {&node, pin.id};
        for (auto& pin : node.outputs)
            if (pin.id == pinId) return {&node, pin.id};
    }
    return {nullptr, 0};
}

} // namespace ace
