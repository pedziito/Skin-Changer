/**
 * ACE Engine — Graph Editor Implementation
 */

#include "graph_editor.h"
#include "../../input/input_system.h"
#include <algorithm>
#include <cmath>

namespace ace {

GraphEditor::GraphEditor(std::string_view name) : Widget(name) {
    _layout.preferredSize = {800, 600};
    SetFlag(WidgetFlags::ClipChildren | WidgetFlags::CapturesMouse | WidgetFlags::Focusable);
}

// ============================================================================
// NODE API
// ============================================================================
u32 GraphEditor::AddNode(std::string_view title, Vec2 position) {
    GraphNode node;
    node.id = _nextNodeId++;
    node.title = title;
    node.position = _snapToGrid ? SnapPosition(position) : position;
    _nodes.push_back(std::move(node));
    return _nodes.back().id;
}

void GraphEditor::RemoveNode(u32 nodeId) {
    _edges.erase(std::remove_if(_edges.begin(), _edges.end(),
        [nodeId](const GraphEdge& e) {
            return e.srcNodeId == nodeId || e.dstNodeId == nodeId;
        }), _edges.end());

    _nodes.erase(std::remove_if(_nodes.begin(), _nodes.end(),
        [nodeId](const GraphNode& n) { return n.id == nodeId; }), _nodes.end());
}

GraphNode* GraphEditor::GetNode(u32 nodeId) {
    for (auto& n : _nodes) if (n.id == nodeId) return &n;
    return nullptr;
}

// ============================================================================
// PORT API
// ============================================================================
u32 GraphEditor::AddPort(u32 nodeId, std::string_view name, bool isInput, Color color) {
    auto* node = GetNode(nodeId);
    if (!node) return 0;

    GraphPort port;
    port.id = _nextPortId++;
    port.name = name;
    port.isInput = isInput;
    port.color = color;
    node->ports.push_back(std::move(port));

    // Recalculate port offsets
    i32 inputIdx = 0, outputIdx = 0;
    f32 portSpacing = 22.0f;
    f32 topOffset = node->style.showHeader ? node->style.headerHeight + 8.0f : 8.0f;

    for (auto& p : node->ports) {
        if (p.isInput) {
            p.offset = {0.0f, topOffset + inputIdx * portSpacing};
            inputIdx++;
        } else {
            p.offset = {node->size.x, topOffset + outputIdx * portSpacing};
            outputIdx++;
        }
    }

    i32 maxPorts = std::max(inputIdx, outputIdx);
    node->size.y = topOffset + maxPorts * portSpacing + 12.0f;

    return node->ports.back().id;
}

// ============================================================================
// EDGE API
// ============================================================================
u32 GraphEditor::AddEdge(u32 srcNodeId, u32 srcPortId, u32 dstNodeId, u32 dstPortId) {
    if (srcNodeId == dstNodeId) return 0;

    GraphEdge edge;
    edge.id = _nextEdgeId++;
    edge.srcNodeId = srcNodeId;
    edge.srcPortId = srcPortId;
    edge.dstNodeId = dstNodeId;
    edge.dstPortId = dstPortId;
    edge.style = _defaultEdgeStyle;
    _edges.push_back(edge);

    if (onEdgeCreated) onEdgeCreated(edge.id);
    return edge.id;
}

void GraphEditor::RemoveEdge(u32 edgeId) {
    auto it = std::find_if(_edges.begin(), _edges.end(),
        [edgeId](const GraphEdge& e) { return e.id == edgeId; });
    if (it != _edges.end()) {
        if (onEdgeDeleted) onEdgeDeleted(it->id);
        _edges.erase(it);
    }
}

// ============================================================================
// GROUP API
// ============================================================================
u32 GraphEditor::AddGroup(std::string_view title, Rect bounds) {
    GraphGroup g;
    g.id = _nextGroupId++;
    g.title = title;
    g.bounds = bounds;
    _groups.push_back(std::move(g));
    return _groups.back().id;
}

void GraphEditor::RemoveGroup(u32 groupId) {
    _groups.erase(std::remove_if(_groups.begin(), _groups.end(),
        [groupId](const GraphGroup& g) { return g.id == groupId; }), _groups.end());
}

// ============================================================================
// SELECTION
// ============================================================================
void GraphEditor::SelectNode(u32 nodeId) {
    DeselectAll();
    if (auto* n = GetNode(nodeId)) {
        n->selected = true;
        if (onNodeSelected) onNodeSelected(nodeId);
    }
}

void GraphEditor::SelectMultiple(const std::vector<u32>& nodeIds) {
    for (auto id : nodeIds) {
        if (auto* n = GetNode(id)) n->selected = true;
    }
}

void GraphEditor::DeselectAll() {
    for (auto& n : _nodes) n.selected = false;
}

void GraphEditor::DeleteSelected() {
    for (auto& n : _nodes) {
        if (n.selected && onNodeDeleted) onNodeDeleted(n.id);
    }
    // Remove edges connected to selected nodes
    _edges.erase(std::remove_if(_edges.begin(), _edges.end(),
        [this](const GraphEdge& e) {
            auto* src = GetNode(e.srcNodeId);
            auto* dst = GetNode(e.dstNodeId);
            return (src && src->selected) || (dst && dst->selected);
        }), _edges.end());

    _nodes.erase(std::remove_if(_nodes.begin(), _nodes.end(),
        [](const GraphNode& n) { return n.selected; }), _nodes.end());
}

std::vector<u32> GraphEditor::GetSelectedNodeIds() const {
    std::vector<u32> ids;
    for (auto& n : _nodes) if (n.selected) ids.push_back(n.id);
    return ids;
}

// ============================================================================
// VIEW
// ============================================================================
void GraphEditor::FitToContent(f32 padding) {
    if (_nodes.empty()) return;

    f32 minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
    for (auto& n : _nodes) {
        minX = std::min(minX, n.position.x);
        minY = std::min(minY, n.position.y);
        maxX = std::max(maxX, n.position.x + n.size.x);
        maxY = std::max(maxY, n.position.y + n.size.y);
    }

    Vec2 content{maxX - minX + padding * 2, maxY - minY + padding * 2};
    Vec2 view = GetSize();

    _zoom = std::min(view.x / content.x, view.y / content.y);
    _zoom = math::Clamp(_zoom, _zoomMin, _zoomMax);

    _pan = Vec2{
        (view.x - (maxX - minX) * _zoom) * 0.5f - minX * _zoom,
        (view.y - (maxY - minY) * _zoom) * 0.5f - minY * _zoom
    };
}

void GraphEditor::CenterOnNode(u32 nodeId) {
    auto* node = GetNode(nodeId);
    if (!node) return;

    Vec2 nodeCenter = node->position + node->size * 0.5f;
    Vec2 view = GetSize();
    _pan = view * 0.5f - nodeCenter * _zoom;
}

Vec2 GraphEditor::ScreenToCanvas(Vec2 screenPos) const {
    Vec2 local = screenPos - GetPosition();
    return (local - _pan) / _zoom;
}

Vec2 GraphEditor::CanvasToScreen(Vec2 canvasPos) const {
    return canvasPos * _zoom + _pan + GetPosition();
}

Vec2 GraphEditor::SnapPosition(Vec2 pos) const {
    return {
        std::round(pos.x / _gridSize) * _gridSize,
        std::round(pos.y / _gridSize) * _gridSize
    };
}

// ============================================================================
// UPDATE
// ============================================================================
void GraphEditor::OnUpdate(f32 dt) {
    _animTime += dt;
    // Animate edge flow
    for (auto& e : _edges) {
        if (e.animated) {
            e.flowOffset += dt * 40.0f;
            if (e.flowOffset > 20.0f) e.flowOffset -= 20.0f;
        }
    }
}

// ============================================================================
// DRAWING
// ============================================================================
void GraphEditor::OnDraw(DrawList& dl, const ThemeEngine& theme) {
    Rect bounds = GetBounds();
    dl.PushClipRect(bounds);

    // Background
    dl.AddFilledRect(bounds, theme.GetColor(ThemeToken::BgPrimary));

    // Grid
    if (_showGrid) DrawGrid(dl, theme);

    // Groups (behind everything)
    DrawGroups(dl, theme);

    // Edges
    DrawEdges(dl, 0.016f, theme);

    // Edge creation preview
    if (_mode == Mode::CreatingEdge) {
        Vec2 startScreen = _edgeEndPos; // we'll compute properly
        GraphEdge previewEdge;
        previewEdge.style = _defaultEdgeStyle;
        previewEdge.color = {200, 200, 255, 150};
        previewEdge.thickness = 2.0f;

        // Find source port position
        for (auto& node : _nodes) {
            for (auto& port : node.ports) {
                if (port.id == _edgeSrcPort) {
                    Vec2 p0 = CanvasToScreen(node.position + port.offset);
                    DrawEdgeCurve(p0, _edgeEndPos, previewEdge, dl);
                }
            }
        }
    }

    // Nodes
    DrawNodes(dl, theme);

    // Selection box
    if (_mode == Mode::SelectionBox) DrawSelectionBox(dl, theme);

    // Minimap
    if (_showMinimap && !_nodes.empty()) DrawMinimap(dl, theme);

    dl.PopClipRect();
}

void GraphEditor::DrawGrid(DrawList& dl, const ThemeEngine& theme) {
    Rect bounds = GetBounds();
    f32 step = _gridSize * _zoom;
    if (step < 4.0f) return;

    Color minor = theme.GetColor(ThemeToken::BorderSecondary).WithAlpha(20);
    Color major = theme.GetColor(ThemeToken::BorderSecondary).WithAlpha(50);

    f32 offX = std::fmod(_pan.x, step);
    f32 offY = std::fmod(_pan.y, step);

    i32 ix = 0;
    for (f32 x = bounds.x + offX; x < bounds.Right(); x += step, ++ix) {
        dl.AddLine({x, bounds.y}, {x, bounds.Bottom()}, (ix % 5 == 0) ? major : minor);
    }
    i32 iy = 0;
    for (f32 y = bounds.y + offY; y < bounds.Bottom(); y += step, ++iy) {
        dl.AddLine({bounds.x, y}, {bounds.Right(), y}, (iy % 5 == 0) ? major : minor);
    }
}

void GraphEditor::DrawGroups(DrawList& dl, const ThemeEngine& theme) {
    for (auto& g : _groups) {
        if (g.collapsed) continue;
        Vec2 tl = CanvasToScreen({g.bounds.x, g.bounds.y});
        Vec2 sz = Vec2{g.bounds.w, g.bounds.h} * _zoom;
        Rect screenRect{tl.x, tl.y, sz.x, sz.y};

        dl.AddFilledRoundRect(screenRect, g.fill, 8);
        dl.AddRect(screenRect, g.border, 1.5f);
    }
}

void GraphEditor::DrawEdges(DrawList& dl, f32 dt, const ThemeEngine& theme) {
    for (auto& edge : _edges) {
        auto* srcNode = GetNode(edge.srcNodeId);
        auto* dstNode = GetNode(edge.dstNodeId);
        if (!srcNode || !dstNode) continue;

        // Find port positions
        Vec2 p0 = srcNode->position;
        Vec2 p3 = dstNode->position;

        for (auto& port : srcNode->ports) {
            if (port.id == edge.srcPortId) { p0 = srcNode->position + port.offset; break; }
        }
        for (auto& port : dstNode->ports) {
            if (port.id == edge.dstPortId) { p3 = dstNode->position + port.offset; break; }
        }

        Vec2 sp0 = CanvasToScreen(p0);
        Vec2 sp3 = CanvasToScreen(p3);

        DrawEdgeCurve(sp0, sp3, edge, dl);
    }
}

void GraphEditor::DrawEdgeCurve(Vec2 p0, Vec2 p3, const GraphEdge& edge, DrawList& dl) {
    f32 thickness = edge.thickness * _zoom;

    switch (edge.style) {
        case GraphEdgeStyle::Straight:
            dl.AddLine(p0, p3, edge.color, thickness);
            break;

        case GraphEdgeStyle::Step: {
            f32 midX = (p0.x + p3.x) * 0.5f;
            dl.AddLine(p0, {midX, p0.y}, edge.color, thickness);
            dl.AddLine({midX, p0.y}, {midX, p3.y}, edge.color, thickness);
            dl.AddLine({midX, p3.y}, p3, edge.color, thickness);
            break;
        }

        case GraphEdgeStyle::Bezier:
        default: {
            f32 tangent = std::max(50.0f * _zoom, std::abs(p3.x - p0.x) * 0.4f);
            Vec2 p1{p0.x + tangent, p0.y};
            Vec2 p2{p3.x - tangent, p3.y};
            dl.AddBezierCurve(p0, p1, p2, p3, edge.color, thickness, 32);
            break;
        }

        case GraphEdgeStyle::Spline: {
            // Approximate Catmull-Rom with Bezier control points
            f32 tangent = std::abs(p3.x - p0.x) * 0.3f;
            Vec2 p1{p0.x + tangent, p0.y};
            Vec2 p2{p3.x - tangent, p3.y};
            dl.AddBezierCurve(p0, p1, p2, p3, edge.color, thickness, 48);
            break;
        }
    }
}

void GraphEditor::DrawNodes(DrawList& dl, const ThemeEngine& theme) {
    for (auto& node : _nodes) {
        Vec2 screenPos = CanvasToScreen(node.position);
        Vec2 screenSize = node.size * _zoom;
        Rect nodeRect{screenPos.x, screenPos.y, screenSize.x, screenSize.y};

        // Custom renderer
        if (_renderer) {
            _renderer->DrawNode(node, dl, screenPos, screenSize, _zoom);
        } else {
            // Shadow
            dl.AddFilledRoundRect(nodeRect.Expand(3), theme.GetColor(ThemeToken::Shadow),
                                   node.style.borderRadius);

            // Body
            dl.AddFilledRoundRect(nodeRect, node.style.fill, node.style.borderRadius);

            // Header
            if (node.style.showHeader) {
                Rect hdr{nodeRect.x, nodeRect.y, nodeRect.w, node.style.headerHeight * _zoom};
                dl.AddFilledRoundRect(hdr, node.style.headerColor, node.style.borderRadius);
                dl.AddFilledRect({hdr.x, hdr.Bottom() - node.style.borderRadius,
                                  hdr.w, node.style.borderRadius}, node.style.headerColor);
            }

            // Selection
            if (node.selected) {
                dl.AddRect(nodeRect.Expand(2), theme.GetColor(ThemeToken::AccentPrimary), 2.0f);
            }

            // Border
            dl.AddRect(nodeRect, node.style.border, node.style.borderWidth);
        }

        // Ports
        for (auto& port : node.ports) {
            DrawPort(port, node, dl, theme);
        }
    }
}

void GraphEditor::DrawPort(const GraphPort& port, const GraphNode& node,
                            DrawList& dl, const ThemeEngine& theme) {
    Vec2 worldPos = node.position + port.offset;
    Vec2 screenPos = CanvasToScreen(worldPos);
    f32 r = 5.0f * _zoom;

    // Filled circle
    dl.AddCircle(screenPos, r, port.color);
    // Inner dot
    dl.AddCircle(screenPos, r * 0.4f, theme.GetColor(ThemeToken::BgPrimary));
}

void GraphEditor::DrawMinimap(DrawList& dl, const ThemeEngine& theme) {
    Rect bounds = GetBounds();
    f32 mmW = 150, mmH = 110;
    Rect mmRect{bounds.Right() - mmW - 10, bounds.Bottom() - mmH - 10, mmW, mmH};

    dl.AddFilledRoundRect(mmRect, theme.GetColor(ThemeToken::BgOverlay).WithAlpha(200), 6);
    dl.AddRect(mmRect, theme.GetColor(ThemeToken::BorderPrimary));

    // Find content bounds
    f32 minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
    for (auto& n : _nodes) {
        minX = std::min(minX, n.position.x);
        minY = std::min(minY, n.position.y);
        maxX = std::max(maxX, n.position.x + n.size.x);
        maxY = std::max(maxY, n.position.y + n.size.y);
    }
    f32 cw = maxX - minX + 40, ch = maxY - minY + 40;
    if (cw < 1 || ch < 1) return;

    f32 scale = std::min((mmW - 10) / cw, (mmH - 10) / ch);
    Vec2 mmOff = {mmRect.x + 5 + ((mmW - 10) - cw * scale) * 0.5f,
                  mmRect.y + 5 + ((mmH - 10) - ch * scale) * 0.5f};

    // Draw node dots
    for (auto& n : _nodes) {
        Vec2 p{(n.position.x - minX + 20) * scale + mmOff.x,
                (n.position.y - minY + 20) * scale + mmOff.y};
        Vec2 s = n.size * scale;
        dl.AddFilledRect({p.x, p.y, std::max(s.x, 3.0f), std::max(s.y, 2.0f)},
                          n.selected ? theme.GetColor(ThemeToken::AccentPrimary)
                                     : Color{150, 150, 180, 200});
    }
}

void GraphEditor::DrawSelectionBox(DrawList& dl, const ThemeEngine& theme) {
    Rect r = Rect::FromMinMax(Vec2::Min(_selStart, _selEnd), Vec2::Max(_selStart, _selEnd));
    dl.AddFilledRect(r, theme.GetColor(ThemeToken::AccentPrimary).WithAlpha(25));
    dl.AddRect(r, theme.GetColor(ThemeToken::AccentPrimary), 1.0f);
}

// ============================================================================
// INPUT
// ============================================================================
bool GraphEditor::OnMouseDown(Vec2 pos, MouseButtonEvent& e) {
    Vec2 canvas = ScreenToCanvas(pos);

    // Right-click: context menu or pan
    if (e.button == 1) {
        auto* node = HitTestNode(canvas);
        if (onContextMenu) {
            onContextMenu(node ? node->id : 0, 0);
        }
        _mode = Mode::Panning;
        return true;
    }

    // Middle click: always pan
    if (e.button == 2) {
        _mode = Mode::Panning;
        return true;
    }

    // Left click
    // Check ports first (edge creation)
    GraphNode* portOwner = nullptr;
    GraphPort* port = HitTestPort(canvas, &portOwner);
    if (port) {
        _mode = Mode::CreatingEdge;
        _edgeSrcPort = port->id;
        _edgeEndPos = pos;
        return true;
    }

    // Check nodes
    GraphNode* node = HitTestNode(canvas);
    if (node) {
        if (node->locked) return true;
        _mode = Mode::DraggingNode;
        _dragNodeId = node->id;
        _dragOffset = canvas - node->position;

        if (!node->selected) {
            if (!e.shift) DeselectAll();
            node->selected = true;
            if (onNodeSelected) onNodeSelected(node->id);
        }
        return true;
    }

    // Check groups
    auto* group = HitTestGroup(canvas);
    if (group) {
        _mode = Mode::DraggingGroup;
        return true;
    }

    // Empty space: selection box
    if (!e.shift) DeselectAll();
    _mode = Mode::SelectionBox;
    _selStart = pos;
    _selEnd = pos;
    return true;
}

bool GraphEditor::OnMouseUp(Vec2 pos, MouseButtonEvent& e) {
    if (_mode == Mode::CreatingEdge) {
        Vec2 canvas = ScreenToCanvas(pos);
        GraphNode* dstOwner = nullptr;
        GraphPort* dstPort = HitTestPort(canvas, &dstOwner);

        if (dstPort && dstPort->id != _edgeSrcPort) {
            // Find source node
            for (auto& node : _nodes) {
                for (auto& p : node.ports) {
                    if (p.id == _edgeSrcPort) {
                        // Ensure directions are opposite
                        if (p.isInput != dstPort->isInput) {
                            u32 srcN = p.isInput ? dstOwner->id : node.id;
                            u32 srcP = p.isInput ? dstPort->id : p.id;
                            u32 dstN = p.isInput ? node.id : dstOwner->id;
                            u32 dstP = p.isInput ? p.id : dstPort->id;
                            AddEdge(srcN, srcP, dstN, dstP);
                        }
                        goto done;
                    }
                }
            }
        }
        done:;
    }

    if (_mode == Mode::SelectionBox) {
        Rect selRect = Rect::FromMinMax(Vec2::Min(_selStart, _selEnd),
                                         Vec2::Max(_selStart, _selEnd));
        for (auto& node : _nodes) {
            Vec2 sp = CanvasToScreen(node.position);
            if (selRect.Contains(sp)) node.selected = true;
        }
    }

    if (_mode == Mode::DraggingNode && _snapToGrid) {
        for (auto& n : _nodes) {
            if (n.selected) {
                n.position = SnapPosition(n.position);
                if (onNodeMoved) onNodeMoved(n.id, n.position);
            }
        }
    }

    _mode = Mode::None;
    return true;
}

bool GraphEditor::OnMouseMove(Vec2 pos, MouseMoveEvent& e) {
    switch (_mode) {
        case Mode::Panning:
            _pan = _pan + e.delta;
            return true;

        case Mode::DraggingNode: {
            Vec2 canvas = ScreenToCanvas(pos);
            for (auto& n : _nodes) {
                if (n.selected && !n.locked) {
                    if (n.id == _dragNodeId) {
                        n.position = canvas - _dragOffset;
                    } else {
                        n.position = n.position + e.delta / _zoom;
                    }
                }
            }
            return true;
        }

        case Mode::CreatingEdge:
            _edgeEndPos = pos;
            return true;

        case Mode::SelectionBox:
            _selEnd = pos;
            return true;

        default:
            return false;
    }
}

bool GraphEditor::OnMouseScroll(f32 delta) {
    f32 oldZoom = _zoom;
    _zoom = math::Clamp(_zoom + delta * _zoom * 0.1f, _zoomMin, _zoomMax);

    // Zoom toward mouse cursor (preserve point under cursor)
    f32 ratio = _zoom / oldZoom;
    _pan = _pan * ratio;
    return true;
}

bool GraphEditor::OnKeyDown(KeyEvent& e) {
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
    if (e.keyCode == static_cast<i32>(Key::G)) {
        _showGrid = !_showGrid;
        return true;
    }
    return false;
}

// ============================================================================
// HIT TESTING
// ============================================================================
GraphNode* GraphEditor::HitTestNode(Vec2 canvasPos) {
    for (auto it = _nodes.rbegin(); it != _nodes.rend(); ++it) {
        if (it->GetBounds().Contains(canvasPos)) return &(*it);
    }
    return nullptr;
}

GraphPort* GraphEditor::HitTestPort(Vec2 canvasPos, GraphNode** outNode) {
    f32 hitRadius = 8.0f / _zoom;
    for (auto& node : _nodes) {
        for (auto& port : node.ports) {
            Vec2 portPos = node.position + port.offset;
            if ((portPos - canvasPos).LenSq() < hitRadius * hitRadius) {
                if (outNode) *outNode = &node;
                return &port;
            }
        }
    }
    return nullptr;
}

GraphGroup* GraphEditor::HitTestGroup(Vec2 canvasPos) {
    for (auto& g : _groups) {
        if (g.bounds.Contains(canvasPos)) return &g;
    }
    return nullptr;
}

} // namespace ace
