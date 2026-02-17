/**
 * ACE Engine — Viewport & Gizmo Implementation
 */

#include "viewport.h"
#include "../../input/input_system.h"
#include <cmath>
#include <algorithm>

namespace ace {

// ============================================================================
// CAMERA
// ============================================================================
Mat4 Camera::GetViewMatrix() const {
    Vec3 f = (target - position).Normalized();
    Vec3 r = f.Cross(up).Normalized();
    Vec3 u = r.Cross(f);

    Mat4 m;
    m.m[0][0] = r.x;  m.m[0][1] = u.x;  m.m[0][2] = -f.x;  m.m[0][3] = 0;
    m.m[1][0] = r.y;  m.m[1][1] = u.y;  m.m[1][2] = -f.y;  m.m[1][3] = 0;
    m.m[2][0] = r.z;  m.m[2][1] = u.z;  m.m[2][2] = -f.z;  m.m[2][3] = 0;
    m.m[3][0] = -r.Dot(position);
    m.m[3][1] = -u.Dot(position);
    m.m[3][2] =  f.Dot(position);
    m.m[3][3] = 1;
    return m;
}

Mat4 Camera::GetProjectionMatrix(f32 aspect) const {
    Mat4 m = {};
    if (orthographic) {
        f32 r = orthoSize * aspect;
        f32 t = orthoSize;
        m.m[0][0] = 1.0f / r;
        m.m[1][1] = 1.0f / t;
        m.m[2][2] = -2.0f / (farPlane - nearPlane);
        m.m[3][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        m.m[3][3] = 1.0f;
    } else {
        f32 tanHalf = std::tan(fov * 0.5f * 3.14159265f / 180.0f);
        m.m[0][0] = 1.0f / (aspect * tanHalf);
        m.m[1][1] = 1.0f / tanHalf;
        m.m[2][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        m.m[2][3] = -1.0f;
        m.m[3][2] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
    }
    return m;
}

// ============================================================================
// TRANSFORM
// ============================================================================
Mat4 Transform3D::GetMatrix() const {
    // Simple Euler rotation: Y * X * Z order
    f32 cx = std::cos(rotation.x * 3.14159265f / 180.0f);
    f32 sx = std::sin(rotation.x * 3.14159265f / 180.0f);
    f32 cy = std::cos(rotation.y * 3.14159265f / 180.0f);
    f32 sy = std::sin(rotation.y * 3.14159265f / 180.0f);
    f32 cz = std::cos(rotation.z * 3.14159265f / 180.0f);
    f32 sz = std::sin(rotation.z * 3.14159265f / 180.0f);

    Mat4 m;
    m.m[0][0] = (cy * cz + sy * sx * sz) * scale.x;
    m.m[0][1] = (cx * sz) * scale.x;
    m.m[0][2] = (-sy * cz + cy * sx * sz) * scale.x;
    m.m[1][0] = (-cy * sz + sy * sx * cz) * scale.y;
    m.m[1][1] = (cx * cz) * scale.y;
    m.m[1][2] = (sy * sz + cy * sx * cz) * scale.y;
    m.m[2][0] = (sy * cx) * scale.z;
    m.m[2][1] = (-sx) * scale.z;
    m.m[2][2] = (cy * cx) * scale.z;
    m.m[3][0] = position.x;
    m.m[3][1] = position.y;
    m.m[3][2] = position.z;
    m.m[3][3] = 1.0f;
    return m;
}

// ============================================================================
// VIEWPORT
// ============================================================================
Viewport::Viewport(std::string_view name) : Widget(name) {
    _layout.preferredSize = {400, 300};
    SetFlag(WidgetFlags::ClipChildren | WidgetFlags::CapturesMouse | WidgetFlags::Focusable);
}

void Viewport::Initialize(IRenderBackend* backend, u32 width, u32 height) {
    _backend = backend;
    _rtWidth = width;
    _rtHeight = height;
    if (_backend) {
        _renderTarget = _backend->CreateRenderTarget(width, height);
    }
    UpdateOrbitCamera();
}

void Viewport::Resize(u32 width, u32 height) {
    if (_rtWidth == width && _rtHeight == height) return;
    _rtWidth = width;
    _rtHeight = height;
    if (_backend && _renderTarget) {
        // Recreate render target
        _renderTarget = _backend->CreateRenderTarget(width, height);
    }
}

Vec2 Viewport::WorldToScreen(Vec3 worldPos) const {
    Vec2 vpSize = GetSize();
    f32 aspect = vpSize.x / std::max(vpSize.y, 1.0f);
    Mat4 view = _camera.GetViewMatrix();
    Mat4 proj = _camera.GetProjectionMatrix(aspect);

    // Transform to clip space
    f32 x = worldPos.x, y = worldPos.y, z = worldPos.z;
    f32 cx = view.m[0][0]*x + view.m[1][0]*y + view.m[2][0]*z + view.m[3][0];
    f32 cy = view.m[0][1]*x + view.m[1][1]*y + view.m[2][1]*z + view.m[3][1];
    f32 cz = view.m[0][2]*x + view.m[1][2]*y + view.m[2][2]*z + view.m[3][2];
    f32 cw = view.m[0][3]*x + view.m[1][3]*y + view.m[2][3]*z + view.m[3][3];

    f32 px = proj.m[0][0]*cx + proj.m[1][0]*cy + proj.m[2][0]*cz + proj.m[3][0];
    f32 py = proj.m[0][1]*cx + proj.m[1][1]*cy + proj.m[2][1]*cz + proj.m[3][1];
    f32 pw = proj.m[0][3]*cx + proj.m[1][3]*cy + proj.m[2][3]*cz + proj.m[3][3];

    if (std::abs(pw) < 0.0001f) return {-1, -1}; // Behind camera

    f32 ndcX = px / pw;
    f32 ndcY = py / pw;

    Vec2 pos = GetPosition();
    return {
        pos.x + (ndcX * 0.5f + 0.5f) * vpSize.x,
        pos.y + (1.0f - (ndcY * 0.5f + 0.5f)) * vpSize.y
    };
}

Vec3 Viewport::ScreenToWorldRay(Vec2 screenPos) const {
    Vec2 vpSize = GetSize();
    Vec2 local = screenPos - GetPosition();
    f32 ndcX = (local.x / vpSize.x) * 2.0f - 1.0f;
    f32 ndcY = 1.0f - (local.y / vpSize.y) * 2.0f;

    // Simplified: return forward direction offset by NDC
    Vec3 forward = _camera.GetForward();
    Vec3 right = _camera.GetRight();
    Vec3 up = right.Cross(forward);

    f32 aspect = vpSize.x / std::max(vpSize.y, 1.0f);
    f32 tanHalf = std::tan(_camera.fov * 0.5f * 3.14159265f / 180.0f);

    return (forward + right * (ndcX * tanHalf * aspect) + up * (ndcY * tanHalf)).Normalized();
}

void Viewport::UpdateOrbitCamera() {
    f32 yawRad   = _orbitYaw * 3.14159265f / 180.0f;
    f32 pitchRad = _orbitPitch * 3.14159265f / 180.0f;

    _camera.position.x = _camera.target.x + _orbitDistance * std::cos(pitchRad) * std::sin(yawRad);
    _camera.position.y = _camera.target.y + _orbitDistance * std::sin(pitchRad);
    _camera.position.z = _camera.target.z + _orbitDistance * std::cos(pitchRad) * std::cos(yawRad);
}

// ============================================================================
// UPDATE
// ============================================================================
void Viewport::OnUpdate(f32 dt) {
    Vec2 sz = GetSize();
    u32 w = (u32)std::max(sz.x, 1.0f);
    u32 h = (u32)std::max(sz.y, 1.0f);
    if (w != _rtWidth || h != _rtHeight) Resize(w, h);
}

// ============================================================================
// DRAWING
// ============================================================================
void Viewport::OnDraw(DrawList& dl, const ThemeEngine& theme) {
    Rect bounds = GetBounds();
    dl.PushClipRect(bounds);

    // Background (dark viewport bg)
    dl.AddFilledRect(bounds, Color{30, 30, 35, 255});

    // Render scene content via callback
    if (onRenderScene) {
        onRenderScene(dl, _camera, GetSize());
    }

    // Grid (2D projected representation)
    if (_showGrid) DrawGrid3D(dl);

    // Gizmo
    if (_selectedTransform && _gizmo.mode != GizmoMode::None) {
        DrawGizmo(dl, theme);
    }

    // Toolbar overlay
    DrawToolbar(dl, theme);

    // Orientation cube (top-right corner)
    DrawOrientationCube(dl);

    // View info (bottom-left)
    DrawViewInfo(dl, theme);

    dl.PopClipRect();
}

void Viewport::DrawToolbar(DrawList& dl, const ThemeEngine& theme) {
    Rect bounds = GetBounds();
    Rect toolbar{bounds.x + 4, bounds.y + 4, 200, 28};

    dl.AddFilledRoundRect(toolbar, theme.GetColor(ThemeToken::BgOverlay).WithAlpha(200), 4);

    // Mode indicators
    Color activeColor = theme.GetColor(ThemeToken::AccentPrimary);
    Color inactiveColor = theme.GetColor(ThemeToken::TextSecondary);

    // T R S buttons
    f32 bx = toolbar.x + 6;
    f32 by = toolbar.y + 4;
    f32 bw = 20, bh = 20;

    auto drawModeBtn = [&](const char* label, GizmoMode mode, f32 x) {
        Color c = (_gizmo.mode == mode) ? activeColor : inactiveColor;
        dl.AddFilledRoundRect({x, by, bw, bh}, c.WithAlpha(40), 3);
        dl.AddRect({x, by, bw, bh}, c, 1.0f);
    };

    drawModeBtn("T", GizmoMode::Translate, bx);
    drawModeBtn("R", GizmoMode::Rotate, bx + 24);
    drawModeBtn("S", GizmoMode::Scale, bx + 48);

    // Camera mode indicator
    const char* camLabel = "Orbit";
    if (_cameraMode == CameraMode::FPS) camLabel = "FPS";
    else if (_cameraMode == CameraMode::Fly) camLabel = "Fly";
    (void)camLabel;
}

void Viewport::DrawGrid3D(DrawList& dl) {
    // Project 3D grid lines to 2D
    Vec2 center = WorldToScreen({0, 0, 0});
    if (center.x < -1000 || center.y < -1000) return;

    Color gridColor = Color{80, 80, 80, 60};
    Color xAxisColor = Color{220, 50, 50, 180};
    Color zAxisColor = Color{50, 50, 220, 180};

    // Draw major axes
    Vec2 xEnd = WorldToScreen({_gridExtent, 0, 0});
    Vec2 xStart = WorldToScreen({-_gridExtent, 0, 0});
    Vec2 zEnd = WorldToScreen({0, 0, _gridExtent});
    Vec2 zStart = WorldToScreen({0, 0, -_gridExtent});

    dl.AddLine(xStart, xEnd, xAxisColor, 1.5f);
    dl.AddLine(zStart, zEnd, zAxisColor, 1.5f);

    // Grid lines
    for (f32 i = -_gridExtent; i <= _gridExtent; i += _gridSize) {
        if (std::abs(i) < 0.01f) continue; // Skip axes

        Vec2 p0 = WorldToScreen({i, 0, -_gridExtent});
        Vec2 p1 = WorldToScreen({i, 0, _gridExtent});
        Vec2 p2 = WorldToScreen({-_gridExtent, 0, i});
        Vec2 p3 = WorldToScreen({_gridExtent, 0, i});

        dl.AddLine(p0, p1, gridColor);
        dl.AddLine(p2, p3, gridColor);
    }
}

void Viewport::DrawGizmo(DrawList& dl, const ThemeEngine& theme) {
    if (!_selectedTransform) return;

    Vec2 center = WorldToScreen(_selectedTransform->position);
    Rect bounds = GetBounds();
    if (!bounds.Contains(center)) return;

    switch (_gizmo.mode) {
        case GizmoMode::Translate: DrawGizmoTranslate(dl, center); break;
        case GizmoMode::Rotate:    DrawGizmoRotate(dl, center); break;
        case GizmoMode::Scale:     DrawGizmoScale(dl, center); break;
        case GizmoMode::Universal:
            DrawGizmoTranslate(dl, center);
            DrawGizmoRotate(dl, center);
            break;
        default: break;
    }
}

void Viewport::DrawGizmoTranslate(DrawList& dl, Vec2 center) {
    Color xColor = Color{220, 60, 60, 255};
    Color yColor = Color{60, 200, 60, 255};
    Color zColor = Color{60, 80, 220, 255};

    f32 axisLen = 80.0f;
    f32 arrowSize = 10.0f;

    // Highlight active axis
    auto getColor = [&](GizmoAxis axis, Color base) -> Color {
        if (_gizmo.hoveredAxis == axis || _gizmo.activeAxis == axis)
            return Color{255, 220, 50, 255};
        return base;
    };

    // X axis (right)
    Vec2 xEnd = WorldToScreen(_selectedTransform->position + Vec3{3, 0, 0});
    dl.AddLine(center, xEnd, getColor(GizmoAxis::X, xColor), 2.5f);
    // Arrow
    Vec2 xDir = (xEnd - center).Normalized();
    dl.AddLine(xEnd, xEnd - xDir * arrowSize + Vec2{0, -arrowSize * 0.5f},
               getColor(GizmoAxis::X, xColor), 2.0f);
    dl.AddLine(xEnd, xEnd - xDir * arrowSize + Vec2{0, arrowSize * 0.5f},
               getColor(GizmoAxis::X, xColor), 2.0f);

    // Y axis (up)
    Vec2 yEnd = WorldToScreen(_selectedTransform->position + Vec3{0, 3, 0});
    dl.AddLine(center, yEnd, getColor(GizmoAxis::Y, yColor), 2.5f);
    Vec2 yDir = (yEnd - center).Normalized();
    dl.AddLine(yEnd, yEnd - yDir * arrowSize + Vec2{arrowSize * 0.5f, 0},
               getColor(GizmoAxis::Y, yColor), 2.0f);
    dl.AddLine(yEnd, yEnd - yDir * arrowSize + Vec2{-arrowSize * 0.5f, 0},
               getColor(GizmoAxis::Y, yColor), 2.0f);

    // Z axis
    Vec2 zEnd = WorldToScreen(_selectedTransform->position + Vec3{0, 0, 3});
    dl.AddLine(center, zEnd, getColor(GizmoAxis::Z, zColor), 2.5f);
    Vec2 zDir = (zEnd - center).Normalized();
    dl.AddLine(zEnd, zEnd - zDir * arrowSize + Vec2{arrowSize * 0.3f, -arrowSize * 0.3f},
               getColor(GizmoAxis::Z, zColor), 2.0f);
    dl.AddLine(zEnd, zEnd - zDir * arrowSize + Vec2{-arrowSize * 0.3f, arrowSize * 0.3f},
               getColor(GizmoAxis::Z, zColor), 2.0f);

    // XY plane
    Vec2 xyCorner1 = center + (xEnd - center) * 0.3f;
    Vec2 xyCorner2 = center + (yEnd - center) * 0.3f;
    Vec2 xyCorner3 = xyCorner1 + (yEnd - center) * 0.3f;
    Color xyColor = getColor(GizmoAxis::XY, Color{220, 200, 60, 80});
    dl.AddFilledRect({std::min(xyCorner1.x, xyCorner2.x), std::min(xyCorner1.y, xyCorner2.y),
                      std::abs(xyCorner3.x - center.x), std::abs(xyCorner3.y - center.y)},
                      xyColor);
}

void Viewport::DrawGizmoRotate(DrawList& dl, Vec2 center) {
    Color xColor = Color{220, 60, 60, 200};
    Color yColor = Color{60, 200, 60, 200};
    Color zColor = Color{60, 80, 220, 200};

    f32 radius = 60.0f;
    i32 segments = 48;

    auto getColor = [&](GizmoAxis axis, Color base) -> Color {
        if (_gizmo.hoveredAxis == axis || _gizmo.activeAxis == axis)
            return Color{255, 220, 50, 255};
        return base;
    };

    // X ring (YZ plane)
    for (i32 i = 0; i < segments; ++i) {
        f32 a0 = (f32)i / segments * 6.2831853f;
        f32 a1 = (f32)(i + 1) / segments * 6.2831853f;
        Vec2 p0{center.x, center.y + std::cos(a0) * radius};
        Vec2 p1{center.x, center.y + std::cos(a1) * radius};
        p0.x += std::sin(a0) * radius * 0.3f;
        p1.x += std::sin(a1) * radius * 0.3f;
        dl.AddLine(p0, p1, getColor(GizmoAxis::X, xColor), 1.5f);
    }

    // Y ring (XZ plane) — flat horizontal ellipse
    for (i32 i = 0; i < segments; ++i) {
        f32 a0 = (f32)i / segments * 6.2831853f;
        f32 a1 = (f32)(i + 1) / segments * 6.2831853f;
        Vec2 p0{center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius * 0.3f};
        Vec2 p1{center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius * 0.3f};
        dl.AddLine(p0, p1, getColor(GizmoAxis::Y, yColor), 1.5f);
    }

    // Z ring (XY plane) — full circle
    for (i32 i = 0; i < segments; ++i) {
        f32 a0 = (f32)i / segments * 6.2831853f;
        f32 a1 = (f32)(i + 1) / segments * 6.2831853f;
        Vec2 p0{center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius};
        Vec2 p1{center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius};
        dl.AddLine(p0, p1, getColor(GizmoAxis::Z, zColor), 1.5f);
    }
}

void Viewport::DrawGizmoScale(DrawList& dl, Vec2 center) {
    Color xColor = Color{220, 60, 60, 255};
    Color yColor = Color{60, 200, 60, 255};
    Color zColor = Color{60, 80, 220, 255};

    auto getColor = [&](GizmoAxis axis, Color base) -> Color {
        if (_gizmo.hoveredAxis == axis || _gizmo.activeAxis == axis)
            return Color{255, 220, 50, 255};
        return base;
    };

    // X
    Vec2 xEnd = WorldToScreen(_selectedTransform->position + Vec3{3, 0, 0});
    dl.AddLine(center, xEnd, getColor(GizmoAxis::X, xColor), 2.0f);
    dl.AddFilledRect({xEnd.x - 4, xEnd.y - 4, 8, 8}, getColor(GizmoAxis::X, xColor));

    // Y
    Vec2 yEnd = WorldToScreen(_selectedTransform->position + Vec3{0, 3, 0});
    dl.AddLine(center, yEnd, getColor(GizmoAxis::Y, yColor), 2.0f);
    dl.AddFilledRect({yEnd.x - 4, yEnd.y - 4, 8, 8}, getColor(GizmoAxis::Y, yColor));

    // Z
    Vec2 zEnd = WorldToScreen(_selectedTransform->position + Vec3{0, 0, 3});
    dl.AddLine(center, zEnd, getColor(GizmoAxis::Z, zColor), 2.0f);
    dl.AddFilledRect({zEnd.x - 4, zEnd.y - 4, 8, 8}, getColor(GizmoAxis::Z, zColor));

    // Center cube (uniform scale)
    dl.AddFilledRect({center.x - 5, center.y - 5, 10, 10},
                      getColor(GizmoAxis::XYZ, Color{200, 200, 200, 180}));
}

void Viewport::DrawOrientationCube(DrawList& dl) {
    Rect bounds = GetBounds();
    Vec2 cubeCenter = {bounds.Right() - 40, bounds.y + 40};
    f32 sz = 14.0f;

    // Simple projected axes
    auto projAxis = [&](Vec3 dir) -> Vec2 {
        Vec3 camRight = _camera.GetRight();
        Vec3 camUp = camRight.Cross(_camera.GetForward());
        return {cubeCenter.x + dir.Dot(camRight) * sz * 2,
                cubeCenter.y - dir.Dot(camUp) * sz * 2};
    };

    // Draw axis lines from center
    Vec2 px = projAxis({1, 0, 0}), py = projAxis({0, 1, 0}), pz = projAxis({0, 0, 1});

    dl.AddFilledRoundRect({cubeCenter.x - sz, cubeCenter.y - sz, sz * 2, sz * 2},
                           Color{40, 40, 50, 180}, 4);
    dl.AddLine(cubeCenter, px, Color{220, 60, 60, 220}, 2.0f);
    dl.AddLine(cubeCenter, py, Color{60, 200, 60, 220}, 2.0f);
    dl.AddLine(cubeCenter, pz, Color{60, 80, 220, 220}, 2.0f);
}

void Viewport::DrawViewInfo(DrawList& dl, const ThemeEngine& theme) {
    Rect bounds = GetBounds();
    Rect info{bounds.x + 4, bounds.Bottom() - 22, 120, 18};
    dl.AddFilledRoundRect(info, Color{20, 20, 28, 180}, 3);
}

// ============================================================================
// INPUT
// ============================================================================
bool Viewport::OnMouseDown(Vec2 pos, MouseButtonEvent& e) {
    _lastMousePos = pos;

    // Alt + Left: orbit
    if (e.button == 0 && e.alt) {
        _isOrbiting = true;
        return true;
    }

    // Alt + Middle or Middle: pan
    if (e.button == 2 || (e.button == 0 && e.shift)) {
        _isPanning = true;
        return true;
    }

    // Right: orbit (alternative)
    if (e.button == 1) {
        _isOrbiting = true;
        return true;
    }

    // Left click: check gizmo first
    if (e.button == 0 && _selectedTransform && _gizmo.mode != GizmoMode::None) {
        GizmoAxis axis = HitTestGizmo(pos);
        if (axis != GizmoAxis::None) {
            _gizmo.activeAxis = axis;
            _gizmo.isDragging = true;
            _gizmo.initialPosition = _selectedTransform->position;
            _gizmo.initialRotation = _selectedTransform->rotation;
            _gizmo.initialScale = _selectedTransform->scale;
            return true;
        }
    }

    // Left click on viewport: picking
    if (e.button == 0 && onViewportClick) {
        Vec3 rayDir = ScreenToWorldRay(pos);
        onViewportClick(_camera.position, rayDir);
    }

    return true;
}

bool Viewport::OnMouseUp(Vec2 pos, MouseButtonEvent& e) {
    _isOrbiting = false;
    _isPanning = false;

    if (_gizmo.isDragging) {
        _gizmo.isDragging = false;
        _gizmo.activeAxis = GizmoAxis::None;
        if (_selectedTransform && onTransformChanged) {
            onTransformChanged(*_selectedTransform);
        }
    }
    return true;
}

bool Viewport::OnMouseMove(Vec2 pos, MouseMoveEvent& e) {
    Vec2 delta = pos - _lastMousePos;
    _lastMousePos = pos;

    if (_isOrbiting && _cameraMode == CameraMode::Orbit) {
        _orbitYaw   += delta.x * 0.5f;
        _orbitPitch -= delta.y * 0.5f;
        _orbitPitch = math::Clamp(_orbitPitch, -89.0f, 89.0f);
        UpdateOrbitCamera();
        return true;
    }

    if (_isPanning) {
        Vec3 right = _camera.GetRight();
        Vec3 up = right.Cross(_camera.GetForward());
        f32 panSpeed = _orbitDistance * 0.003f;
        _camera.target = _camera.target - right * (delta.x * panSpeed) + up * (delta.y * panSpeed);
        UpdateOrbitCamera();
        return true;
    }

    if (_gizmo.isDragging && _selectedTransform) {
        f32 sensitivity = 0.05f;
        u8 axis = static_cast<u8>(_gizmo.activeAxis);

        if (_gizmo.mode == GizmoMode::Translate) {
            if (axis & (u8)GizmoAxis::X) _selectedTransform->position.x += delta.x * sensitivity;
            if (axis & (u8)GizmoAxis::Y) _selectedTransform->position.y -= delta.y * sensitivity;
            if (axis & (u8)GizmoAxis::Z) _selectedTransform->position.z += delta.x * sensitivity;
        } else if (_gizmo.mode == GizmoMode::Rotate) {
            if (axis & (u8)GizmoAxis::X) _selectedTransform->rotation.x += delta.y * 0.5f;
            if (axis & (u8)GizmoAxis::Y) _selectedTransform->rotation.y += delta.x * 0.5f;
            if (axis & (u8)GizmoAxis::Z) _selectedTransform->rotation.z += delta.x * 0.5f;
        } else if (_gizmo.mode == GizmoMode::Scale) {
            f32 scaleDelta = (delta.x + delta.y) * 0.01f;
            if (axis & (u8)GizmoAxis::X) _selectedTransform->scale.x += scaleDelta;
            if (axis & (u8)GizmoAxis::Y) _selectedTransform->scale.y += scaleDelta;
            if (axis & (u8)GizmoAxis::Z) _selectedTransform->scale.z += scaleDelta;
        }
        return true;
    }

    // Hover gizmo test
    if (_selectedTransform && _gizmo.mode != GizmoMode::None && !_gizmo.isDragging) {
        _gizmo.hoveredAxis = HitTestGizmo(pos);
    }

    return false;
}

bool Viewport::OnMouseScroll(f32 delta) {
    _orbitDistance -= delta * _orbitDistance * 0.1f;
    _orbitDistance = math::Clamp(_orbitDistance, 0.5f, 500.0f);
    UpdateOrbitCamera();
    return true;
}

bool Viewport::OnKeyDown(KeyEvent& e) {
    if (e.keyCode == static_cast<i32>(Key::W)) {
        SetGizmoMode(GizmoMode::Translate);
        return true;
    }
    if (e.keyCode == static_cast<i32>(Key::E)) {
        SetGizmoMode(GizmoMode::Rotate);
        return true;
    }
    if (e.keyCode == static_cast<i32>(Key::R)) {
        SetGizmoMode(GizmoMode::Scale);
        return true;
    }
    if (e.keyCode == static_cast<i32>(Key::F)) {
        // Focus on origin
        _camera.target = _selectedTransform ? _selectedTransform->position : Vec3{};
        UpdateOrbitCamera();
        return true;
    }
    if (e.keyCode == static_cast<i32>(Key::Num5)) {
        _camera.orthographic = !_camera.orthographic;
        return true;
    }
    return false;
}

GizmoAxis Viewport::HitTestGizmo(Vec2 pos) {
    if (!_selectedTransform) return GizmoAxis::None;

    Vec2 center = WorldToScreen(_selectedTransform->position);
    Vec2 xEnd = WorldToScreen(_selectedTransform->position + Vec3{3, 0, 0});
    Vec2 yEnd = WorldToScreen(_selectedTransform->position + Vec3{0, 3, 0});
    Vec2 zEnd = WorldToScreen(_selectedTransform->position + Vec3{0, 0, 3});

    f32 hitThreshold = 12.0f;

    // Point-to-line-segment distance test
    auto distToSegment = [](Vec2 p, Vec2 a, Vec2 b) -> f32 {
        Vec2 ab = b - a;
        Vec2 ap = p - a;
        f32 t = math::Clamp(ap.Dot(ab) / std::max(ab.Dot(ab), 0.0001f), 0.0f, 1.0f);
        Vec2 closest = a + ab * t;
        return (p - closest).Len();
    };

    if (distToSegment(pos, center, xEnd) < hitThreshold) return GizmoAxis::X;
    if (distToSegment(pos, center, yEnd) < hitThreshold) return GizmoAxis::Y;
    if (distToSegment(pos, center, zEnd) < hitThreshold) return GizmoAxis::Z;

    // Center handle (uniform)
    if ((pos - center).Len() < 10.0f) return GizmoAxis::XYZ;

    return GizmoAxis::None;
}

// ============================================================================
// VIEWPORT MANAGER
// ============================================================================
void ViewportManager::Initialize(IRenderBackend* backend) {
    _backend = backend;
}

Viewport* ViewportManager::AddViewport(std::string_view name) {
    auto vp = std::make_unique<Viewport>(name);
    vp->Initialize(_backend, 400, 300);
    auto* ptr = vp.get();
    _viewports.push_back(std::move(vp));
    if (!_active) _active = ptr;
    LayoutViewports();
    return ptr;
}

void ViewportManager::RemoveViewport(std::string_view name) {
    auto it = std::find_if(_viewports.begin(), _viewports.end(),
        [name](const auto& vp) { return vp->GetName() == name; });
    if (it != _viewports.end()) {
        if (_active == it->get()) _active = nullptr;
        _viewports.erase(it);
        if (_active == nullptr && !_viewports.empty())
            _active = _viewports[0].get();
        LayoutViewports();
    }
}

Viewport* ViewportManager::GetViewport(std::string_view name) {
    for (auto& vp : _viewports) if (vp->GetName() == name) return vp.get();
    return nullptr;
}

void ViewportManager::SetBounds(Rect bounds) {
    _bounds = bounds;
    LayoutViewports();
}

void ViewportManager::LayoutViewports() {
    if (_viewports.empty()) return;

    switch (_layout) {
        case ViewportLayout::Single:
            if (!_viewports.empty()) {
                _viewports[0]->SetPosition({_bounds.x, _bounds.y});
                _viewports[0]->SetSize({_bounds.w, _bounds.h});
            }
            break;

        case ViewportLayout::SplitH:
            for (i32 i = 0; i < std::min((i32)_viewports.size(), 2); ++i) {
                _viewports[i]->SetPosition({_bounds.x + i * _bounds.w * 0.5f, _bounds.y});
                _viewports[i]->SetSize({_bounds.w * 0.5f, _bounds.h});
            }
            break;

        case ViewportLayout::SplitV:
            for (i32 i = 0; i < std::min((i32)_viewports.size(), 2); ++i) {
                _viewports[i]->SetPosition({_bounds.x, _bounds.y + i * _bounds.h * 0.5f});
                _viewports[i]->SetSize({_bounds.w, _bounds.h * 0.5f});
            }
            break;

        case ViewportLayout::Quad:
            for (i32 i = 0; i < std::min((i32)_viewports.size(), 4); ++i) {
                f32 col = (f32)(i % 2);
                f32 row = (f32)(i / 2);
                _viewports[i]->SetPosition({_bounds.x + col * _bounds.w * 0.5f,
                                             _bounds.y + row * _bounds.h * 0.5f});
                _viewports[i]->SetSize({_bounds.w * 0.5f, _bounds.h * 0.5f});
            }
            break;

        case ViewportLayout::ThreeLeft:
            if (_viewports.size() >= 1) {
                _viewports[0]->SetPosition({_bounds.x, _bounds.y});
                _viewports[0]->SetSize({_bounds.w * 0.6f, _bounds.h});
            }
            for (i32 i = 1; i < std::min((i32)_viewports.size(), 3); ++i) {
                _viewports[i]->SetPosition({_bounds.x + _bounds.w * 0.6f,
                                             _bounds.y + (i - 1) * _bounds.h * 0.5f});
                _viewports[i]->SetSize({_bounds.w * 0.4f, _bounds.h * 0.5f});
            }
            break;

        case ViewportLayout::ThreeTop:
            if (_viewports.size() >= 1) {
                _viewports[0]->SetPosition({_bounds.x, _bounds.y});
                _viewports[0]->SetSize({_bounds.w, _bounds.h * 0.6f});
            }
            for (i32 i = 1; i < std::min((i32)_viewports.size(), 3); ++i) {
                _viewports[i]->SetPosition({_bounds.x + (i - 1) * _bounds.w * 0.5f,
                                             _bounds.y + _bounds.h * 0.6f});
                _viewports[i]->SetSize({_bounds.w * 0.5f, _bounds.h * 0.4f});
            }
            break;
    }
}

void ViewportManager::Update(f32 dt) {
    for (auto& vp : _viewports) vp->OnUpdate(dt);
}

void ViewportManager::Draw(DrawList& dl, const ThemeEngine& theme) {
    for (auto& vp : _viewports) vp->OnDraw(dl, theme);
}

bool ViewportManager::HandleMouseDown(Vec2 pos, MouseButtonEvent& e) {
    for (auto& vp : _viewports) {
        if (vp->GetBounds().Contains(pos)) {
            _active = vp.get();
            return vp->OnMouseDown(pos, e);
        }
    }
    return false;
}

bool ViewportManager::HandleMouseUp(Vec2 pos, MouseButtonEvent& e) {
    if (_active) return _active->OnMouseUp(pos, e);
    return false;
}

bool ViewportManager::HandleMouseMove(Vec2 pos, MouseMoveEvent& e) {
    if (_active) return _active->OnMouseMove(pos, e);
    return false;
}

} // namespace ace
