/**
 * ACE Engine — Render Backend Abstraction
 * GPU-agnostic interface for rendering backends (DX11, DX12, Vulkan, OpenGL).
 * All UI rendering flows through this abstraction.
 */

#pragma once

#include "../core/types.h"
#include <memory>
#include <string>
#include <vector>

namespace ace {

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
struct FontAtlas;

// ============================================================================
// GPU RESOURCE HANDLES
// ============================================================================
using TextureHandle = u64;
using ShaderHandle  = u64;
using BufferHandle  = u64;

constexpr TextureHandle INVALID_TEXTURE = 0;
constexpr ShaderHandle  INVALID_SHADER  = 0;
constexpr BufferHandle  INVALID_BUFFER  = 0;

// ============================================================================
// VERTEX FORMAT — UI vertex with position, UV, color
// ============================================================================
struct UIVertex {
    Vec2  pos;
    Vec2  uv;
    u32   color; // ABGR packed

    UIVertex() = default;
    UIVertex(Vec2 p, Vec2 uv, Color c) : pos(p), uv(uv), color(c.PackABGR()) {}
    UIVertex(Vec2 p, Vec2 uv, u32 col) : pos(p), uv(uv), color(col) {}
};

// ============================================================================
// DRAW COMMAND — Single batched draw call
// ============================================================================
struct DrawCmd {
    u32           indexOffset{0};
    u32           indexCount{0};
    Rect          clipRect;
    TextureHandle texture{INVALID_TEXTURE};
    u8            blendMode{0}; // 0 = alpha blend, 1 = additive, 2 = opaque
};

// ============================================================================
// DRAW LIST — Accumulated geometry for one layer/viewport
// ============================================================================
class DrawList {
public:
    std::vector<UIVertex>  vertices;
    std::vector<u32>       indices;
    std::vector<DrawCmd>   commands;

    void Clear() {
        vertices.clear();
        indices.clear();
        commands.clear();
        _currentTexture = INVALID_TEXTURE;
    }

    void PushClipRect(Rect r) { _clipStack.push_back(r); }
    void PopClipRect() { if (!_clipStack.empty()) _clipStack.pop_back(); }
    Rect CurrentClipRect() const {
        return _clipStack.empty() ? Rect{0, 0, 99999, 99999} : _clipStack.back();
    }

    // ---- Primitives --------------------------------------------------------

    void AddFilledRect(Rect r, Color c) {
        EnsureCommand(INVALID_TEXTURE);
        u32 base = static_cast<u32>(vertices.size());
        u32 col = c.PackABGR();
        vertices.push_back({{r.x, r.y}, {0, 0}, col});
        vertices.push_back({{r.Right(), r.y}, {1, 0}, col});
        vertices.push_back({{r.Right(), r.Bottom()}, {1, 1}, col});
        vertices.push_back({{r.x, r.Bottom()}, {0, 1}, col});
        indices.insert(indices.end(), {base, base+1, base+2, base, base+2, base+3});
        commands.back().indexCount += 6;
    }

    void AddFilledRoundRect(Rect r, Color c, f32 radius, i32 segments = 8) {
        EnsureCommand(INVALID_TEXTURE);
        u32 col = c.PackABGR();
        radius = std::min(radius, std::min(r.w, r.h) * 0.5f);

        // Center vertex
        u32 centerIdx = static_cast<u32>(vertices.size());
        Vec2 center = r.Center();
        vertices.push_back({center, {0.5f, 0.5f}, col});

        // Generate rounded corners
        Vec2 corners[4] = {
            {r.x + radius, r.y + radius},
            {r.Right() - radius, r.y + radius},
            {r.Right() - radius, r.Bottom() - radius},
            {r.x + radius, r.Bottom() - radius}
        };

        f32 startAngles[4] = {math::PI, math::PI * 1.5f, 0.0f, math::PI * 0.5f};
        u32 totalVerts = 0;

        for (int corner = 0; corner < 4; ++corner) {
            for (int s = 0; s <= segments; ++s) {
                f32 angle = startAngles[corner] + (math::PI * 0.5f) * (f32(s) / f32(segments));
                f32 cx = corners[corner].x + std::cos(angle) * radius;
                f32 cy = corners[corner].y + std::sin(angle) * radius;
                vertices.push_back({{cx, cy}, {0, 0}, col});
                totalVerts++;
            }
        }

        u32 firstOuter = centerIdx + 1;
        for (u32 i = 0; i < totalVerts; ++i) {
            u32 next = (i + 1) % totalVerts;
            indices.push_back(centerIdx);
            indices.push_back(firstOuter + i);
            indices.push_back(firstOuter + next);
        }
        commands.back().indexCount += totalVerts * 3;
    }

    void AddRect(Rect r, Color c, f32 thickness = 1.0f) {
        AddFilledRect({r.x, r.y, r.w, thickness}, c);             // top
        AddFilledRect({r.x, r.Bottom() - thickness, r.w, thickness}, c); // bottom
        AddFilledRect({r.x, r.y, thickness, r.h}, c);             // left
        AddFilledRect({r.Right() - thickness, r.y, thickness, r.h}, c); // right
    }

    void AddLine(Vec2 a, Vec2 b, Color c, f32 thickness = 1.0f) {
        Vec2 dir = (b - a).Normalized();
        Vec2 perp = dir.Perpendicular() * (thickness * 0.5f);
        u32 col = c.PackABGR();

        EnsureCommand(INVALID_TEXTURE);
        u32 base = static_cast<u32>(vertices.size());
        vertices.push_back({a + perp, {0, 0}, col});
        vertices.push_back({b + perp, {1, 0}, col});
        vertices.push_back({b - perp, {1, 1}, col});
        vertices.push_back({a - perp, {0, 1}, col});
        indices.insert(indices.end(), {base, base+1, base+2, base, base+2, base+3});
        commands.back().indexCount += 6;
    }

    void AddCircle(Vec2 center, f32 radius, Color c, i32 segments = 32) {
        EnsureCommand(INVALID_TEXTURE);
        u32 col = c.PackABGR();
        u32 centerIdx = static_cast<u32>(vertices.size());
        vertices.push_back({center, {0.5f, 0.5f}, col});

        for (int i = 0; i <= segments; ++i) {
            f32 angle = math::TAU * (f32(i) / f32(segments));
            Vec2 p = center + Vec2{std::cos(angle), std::sin(angle)} * radius;
            vertices.push_back({p, {0, 0}, col});
        }

        for (int i = 0; i < segments; ++i) {
            indices.push_back(centerIdx);
            indices.push_back(centerIdx + 1 + i);
            indices.push_back(centerIdx + 2 + i);
        }
        commands.back().indexCount += segments * 3;
    }

    void AddTriangle(Vec2 a, Vec2 b, Vec2 c, Color col) {
        EnsureCommand(INVALID_TEXTURE);
        u32 packed = col.PackABGR();
        u32 base = static_cast<u32>(vertices.size());
        vertices.push_back({a, {0, 0}, packed});
        vertices.push_back({b, {0.5f, 1}, packed});
        vertices.push_back({c, {1, 0}, packed});
        indices.insert(indices.end(), {base, base+1, base+2});
        commands.back().indexCount += 3;
    }

    void AddTexturedRect(Rect r, TextureHandle tex, Color tint = Color::White(),
                         Vec2 uvMin = {0,0}, Vec2 uvMax = {1,1}) {
        EnsureCommand(tex);
        u32 col = tint.PackABGR();
        u32 base = static_cast<u32>(vertices.size());
        vertices.push_back({{r.x, r.y}, uvMin, col});
        vertices.push_back({{r.Right(), r.y}, {uvMax.x, uvMin.y}, col});
        vertices.push_back({{r.Right(), r.Bottom()}, uvMax, col});
        vertices.push_back({{r.x, r.Bottom()}, {uvMin.x, uvMax.y}, col});
        indices.insert(indices.end(), {base, base+1, base+2, base, base+2, base+3});
        commands.back().indexCount += 6;
    }

    // Gradient (vertical by default)
    void AddGradientRect(Rect r, Color topLeft, Color topRight, Color bottomRight, Color bottomLeft) {
        EnsureCommand(INVALID_TEXTURE);
        u32 base = static_cast<u32>(vertices.size());
        vertices.push_back({{r.x, r.y}, {0, 0}, topLeft.PackABGR()});
        vertices.push_back({{r.Right(), r.y}, {1, 0}, topRight.PackABGR()});
        vertices.push_back({{r.Right(), r.Bottom()}, {1, 1}, bottomRight.PackABGR()});
        vertices.push_back({{r.x, r.Bottom()}, {0, 1}, bottomLeft.PackABGR()});
        indices.insert(indices.end(), {base, base+1, base+2, base, base+2, base+3});
        commands.back().indexCount += 6;
    }

    // Bezier curve
    void AddBezierCurve(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, Color c, f32 thickness = 2.0f, i32 segments = 24) {
        Vec2 prev = p0;
        for (int i = 1; i <= segments; ++i) {
            f32 t = f32(i) / f32(segments);
            f32 u = 1.0f - t;
            Vec2 pt = p0 * (u*u*u) + p1 * (3*u*u*t) + p2 * (3*u*t*t) + p3 * (t*t*t);
            AddLine(prev, pt, c, thickness);
            prev = pt;
        }
    }

private:
    void EnsureCommand(TextureHandle tex) {
        if (commands.empty() || _currentTexture != tex) {
            DrawCmd cmd;
            cmd.indexOffset = static_cast<u32>(indices.size());
            cmd.indexCount = 0;
            cmd.clipRect = CurrentClipRect();
            cmd.texture = tex;
            commands.push_back(cmd);
            _currentTexture = tex;
        }
    }

    TextureHandle       _currentTexture{INVALID_TEXTURE};
    std::vector<Rect>   _clipStack;
};

// ============================================================================
// TEXTURE DESCRIPTOR
// ============================================================================
struct TextureDesc {
    u32   width{0};
    u32   height{0};
    u8    channels{4}; // RGBA
    bool  generateMips{false};
    const u8* data{nullptr};
};

// ============================================================================
// RENDER BACKEND INTERFACE — Implemented per graphics API
// ============================================================================
class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    // Lifecycle
    virtual bool Initialize(void* windowHandle, u32 width, u32 height) = 0;
    virtual void Shutdown() = 0;
    virtual void Resize(u32 width, u32 height) = 0;

    // Frame
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Present() = 0;

    // Draw
    virtual void RenderDrawList(const DrawList& drawList, u32 viewportW, u32 viewportH) = 0;

    // Textures
    virtual TextureHandle CreateTexture(const TextureDesc& desc) = 0;
    virtual void DestroyTexture(TextureHandle handle) = 0;

    // Clear
    virtual void SetClearColor(Color c) = 0;

    // Info
    virtual std::string GetBackendName() const = 0;
    virtual Vec2 GetViewportSize() const = 0;
};

// ============================================================================
// RENDER TARGET (for multi-viewport / offscreen rendering)
// ============================================================================
struct RenderTargetDesc {
    u32  width;
    u32  height;
    bool depthBuffer{false};
};

using RenderTargetHandle = u64;
constexpr RenderTargetHandle INVALID_RT = 0;

} // namespace ace
