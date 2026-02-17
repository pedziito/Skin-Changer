/**
 * ACE Engine — Viewport Widget
 *
 * Renders a 3D scene (or off-screen content) into a texture,
 * then displays it as a widget. Supports multiple viewports,
 * camera controls, overlay gizmos, and grid rendering.
 */

#pragma once
#include "../widget.h"
#include "../theme.h"
#include "../../core/types.h"
#include "../../render/render_backend.h"
#include <functional>
#include <vector>
#include <string>

namespace ace {

// ============================================================================
// CAMERA
// ============================================================================
struct Camera {
    Vec3 position  = {0, 5, -10};
    Vec3 target    = {0, 0, 0};
    Vec3 up        = {0, 1, 0};
    f32  fov       = 60.0f;
    f32  nearPlane = 0.1f;
    f32  farPlane  = 1000.0f;
    bool orthographic = false;
    f32  orthoSize    = 10.0f;

    Mat4 GetViewMatrix() const;
    Mat4 GetProjectionMatrix(f32 aspect) const;
    Vec3 GetForward() const { return (target - position).Normalized(); }
    Vec3 GetRight() const   { return GetForward().Cross(up).Normalized(); }
};

enum class CameraMode : u8 {
    Orbit,    // Maya-style orbit around target
    FPS,      // First-person WASD + mouse
    Fly       // Free-fly with no constraints
};

// ============================================================================
// GIZMO TYPES
// ============================================================================
enum class GizmoMode : u8 {
    None,
    Translate,
    Rotate,
    Scale,
    Universal // All three combined
};

enum class GizmoSpace : u8 {
    World,
    Local
};

enum class GizmoAxis : u8 {
    None  = 0,
    X     = 1,
    Y     = 2,
    Z     = 4,
    XY    = X | Y,
    XZ    = X | Z,
    YZ    = Y | Z,
    XYZ   = X | Y | Z,
    View  = 8  // Screen-aligned plane
};

struct GizmoState {
    GizmoMode  mode  = GizmoMode::Translate;
    GizmoSpace space = GizmoSpace::World;
    GizmoAxis  activeAxis = GizmoAxis::None;
    GizmoAxis  hoveredAxis = GizmoAxis::None;
    bool       isDragging = false;
    Vec3       dragStart = {};
    Vec3       dragCurrent = {};
    Vec3       initialPosition = {};
    Vec3       initialRotation = {};  // Euler
    Vec3       initialScale = {};
};

// ============================================================================
// TRANSFORM (for objects in the viewport)
// ============================================================================
struct Transform3D {
    Vec3 position = {};
    Vec3 rotation = {};  // Euler degrees
    Vec3 scale    = {1, 1, 1};

    Mat4 GetMatrix() const;
};

// ============================================================================
// VIEWPORT WIDGET
// ============================================================================
class Viewport : public Widget {
public:
    explicit Viewport(std::string_view name = "Viewport");

    // Render backend reference (needed to create render targets)
    void Initialize(IRenderBackend* backend, u32 width, u32 height);
    void Resize(u32 width, u32 height);

    // Camera
    Camera&       GetCamera()       { return _camera; }
    const Camera& GetCamera() const { return _camera; }
    void SetCameraMode(CameraMode mode) { _cameraMode = mode; }
    CameraMode GetCameraMode() const { return _cameraMode; }

    // Gizmo
    void SetGizmoMode(GizmoMode mode) { _gizmo.mode = mode; }
    GizmoMode GetGizmoMode() const    { return _gizmo.mode; }
    void SetGizmoSpace(GizmoSpace s)  { _gizmo.space = s; }
    void SetSelectedTransform(Transform3D* t) { _selectedTransform = t; }

    // Grid
    void SetShowGrid(bool v) { _showGrid = v; }
    void SetGridSize(f32 s)  { _gridSize = s; }

    // Overlay drawing callback (for custom scene rendering)
    // Called each frame to render scene content into the viewport texture
    std::function<void(DrawList&, const Camera&, Vec2 viewportSize)> onRenderScene;

    // Callbacks
    std::function<void(const Transform3D&)>  onTransformChanged;
    std::function<void(Vec3 rayOrigin, Vec3 rayDir)> onViewportClick;

    // Picking
    Vec3 ScreenToWorldRay(Vec2 screenPos) const;

    // Allow ViewportManager to call protected methods
    friend class ViewportManager;

protected:
    void OnUpdate(f32 dt) override;
    void OnDraw(DrawList& drawList, const ThemeEngine& theme) override;
    bool OnMouseDown(Vec2 pos, MouseButtonEvent& e) override;
    bool OnMouseUp(Vec2 pos, MouseButtonEvent& e) override;
    bool OnMouseMove(Vec2 pos, MouseMoveEvent& e) override;
    bool OnMouseScroll(f32 delta) override;
    bool OnKeyDown(KeyEvent& e) override;

private:
    IRenderBackend*  _backend = nullptr;
    RenderTargetHandle _renderTarget = 0;
    u32 _rtWidth = 0, _rtHeight = 0;

    Camera      _camera;
    CameraMode  _cameraMode = CameraMode::Orbit;
    GizmoState  _gizmo;
    Transform3D* _selectedTransform = nullptr;

    bool _showGrid   = true;
    f32  _gridSize   = 1.0f;
    f32  _gridExtent = 50.0f;

    // Camera orbit state
    bool _isOrbiting = false;
    bool _isPanning  = false;
    Vec2 _lastMousePos = {};
    f32  _orbitYaw   = -45.0f;
    f32  _orbitPitch = 30.0f;
    f32  _orbitDistance = 15.0f;

    // Drawing helpers
    void DrawToolbar(DrawList& dl, const ThemeEngine& theme);
    void DrawGrid3D(DrawList& dl);
    void DrawGizmo(DrawList& dl, const ThemeEngine& theme);
    void DrawGizmoTranslate(DrawList& dl, Vec2 center);
    void DrawGizmoRotate(DrawList& dl, Vec2 center);
    void DrawGizmoScale(DrawList& dl, Vec2 center);
    void DrawOrientationCube(DrawList& dl);
    void DrawViewInfo(DrawList& dl, const ThemeEngine& theme);

    // Camera helpers
    void UpdateOrbitCamera();
    Vec2 WorldToScreen(Vec3 worldPos) const;

    // Gizmo interaction
    GizmoAxis HitTestGizmo(Vec2 pos);
};

// ============================================================================
// MULTI-VIEWPORT MANAGER
// ============================================================================
enum class ViewportLayout : u8 {
    Single,
    SplitH,     // Left | Right
    SplitV,     // Top / Bottom
    Quad,       // 2x2
    ThreeLeft,  // Large left + 2 stacked right
    ThreeTop    // Large top + 2 side by side bottom
};

class ViewportManager {
public:
    ViewportManager() = default;

    void Initialize(IRenderBackend* backend);

    Viewport* AddViewport(std::string_view name);
    void RemoveViewport(std::string_view name);
    Viewport* GetViewport(std::string_view name);
    Viewport* GetActiveViewport() { return _active; }

    void SetLayout(ViewportLayout layout) { _layout = layout; }
    ViewportLayout GetLayout() const { return _layout; }

    void SetBounds(Rect bounds);
    void Update(f32 dt);
    void Draw(DrawList& dl, const ThemeEngine& theme);

    bool HandleMouseDown(Vec2 pos, MouseButtonEvent& e);
    bool HandleMouseUp(Vec2 pos, MouseButtonEvent& e);
    bool HandleMouseMove(Vec2 pos, MouseMoveEvent& e);

private:
    std::vector<std::unique_ptr<Viewport>> _viewports;
    Viewport* _active = nullptr;
    ViewportLayout _layout = ViewportLayout::Single;
    IRenderBackend* _backend = nullptr;
    Rect _bounds = {};

    void LayoutViewports();
};

} // namespace ace
