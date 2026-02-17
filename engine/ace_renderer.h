/*
 * ACE Renderer — Custom DX11 Rendering Engine
 * Zero ImGui dependency. Own draw list, font system, shader pipeline.
 *
 * Vertex format: float2 pos, float2 uv, uint32 col (ABGR, R8G8B8A8_UNORM)
 * Font: stb_truetype → alpha bitmap atlas with white pixel for solid primitives
 * Shaders: HLSL compiled at runtime via D3DCompile
 */

#pragma once

#include <cstdint>
#include <cmath>
#include <cstring>
#include <vector>
#include <string>
#include <algorithm>

// ============================================================================
// COLOR MACRO
// ============================================================================
#define ACE_COL32(R,G,B,A) \
    (((uint32_t)(A)<<24) | ((uint32_t)(B)<<16) | ((uint32_t)(G)<<8) | (uint32_t)(R))

#define ACE_COL32_R(C) ((C)       & 0xFF)
#define ACE_COL32_G(C) (((C)>>8)  & 0xFF)
#define ACE_COL32_B(C) (((C)>>16) & 0xFF)
#define ACE_COL32_A(C) (((C)>>24) & 0xFF)

#define ACE_COL32_WHITE  ACE_COL32(255,255,255,255)
#define ACE_COL32_BLACK  ACE_COL32(0,0,0,255)

// ============================================================================
// MATH TYPES
// ============================================================================
struct ACEVec2 {
    float x, y;
    ACEVec2() : x(0), y(0) {}
    ACEVec2(float x, float y) : x(x), y(y) {}
    ACEVec2 operator+(const ACEVec2& b) const { return {x+b.x, y+b.y}; }
    ACEVec2 operator-(const ACEVec2& b) const { return {x-b.x, y-b.y}; }
    ACEVec2 operator*(float s) const { return {x*s, y*s}; }
};

struct ACEVec4 {
    float x, y, z, w;
    ACEVec4() : x(0), y(0), z(0), w(0) {}
    ACEVec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

struct ACERect {
    float x1, y1, x2, y2;
    ACERect() : x1(0), y1(0), x2(0), y2(0) {}
    ACERect(float x1, float y1, float x2, float y2) : x1(x1), y1(y1), x2(x2), y2(y2) {}
    bool Contains(float px, float py) const { return px>=x1 && py>=y1 && px<x2 && py<y2; }
    float Width()  const { return x2 - x1; }
    float Height() const { return y2 - y1; }
};

// ============================================================================
// VERTEX & DRAW COMMAND
// ============================================================================
struct ACEVertex {
    float pos[2];   // screen position
    float uv[2];    // texture coordinate
    uint32_t col;   // ABGR color
};

struct ACEDrawCmd {
    uint32_t elemCount;   // number of indices
    ACERect  clipRect;    // scissor rectangle
    uint32_t vtxOffset;   // vertex buffer offset
    uint32_t idxOffset;   // index buffer offset
};

// ============================================================================
// FONT GLYPH
// ============================================================================
struct ACEGlyphInfo {
    float u0, v0, u1, v1;     // atlas UV
    float xoff, yoff;         // render offset from cursor
    float xoff2, yoff2;       // bottom-right offset
    float xadvance;           // horizontal advance
};

// ============================================================================
// FONT (loaded from TTF via stb_truetype)
// ============================================================================
class ACEFont {
public:
    float fontSize = 0;
    float lineHeight = 0;
    float ascent = 0;
    float descent = 0;
    int   atlasWidth = 0;
    int   atlasHeight = 0;
    unsigned char* atlasData = nullptr;     // single-channel alpha
    float whitePixelU = 0, whitePixelV = 0; // UV of white pixel

    ACEGlyphInfo glyphs[128] = {};          // ASCII range
    bool         glyphValid[128] = {};

    bool LoadFromFile(const char* path, float size);
    bool LoadFromMemory(const unsigned char* data, int dataSize, float size);
    void Free();

    const ACEGlyphInfo* GetGlyph(int c) const;
    ACEVec2 CalcTextSize(const char* text, const char* textEnd = nullptr) const;
};

// ============================================================================
// DRAW LIST
// ============================================================================
class ACEDrawList {
public:
    std::vector<ACEVertex> vtxBuffer;
    std::vector<uint32_t>  idxBuffer;
    std::vector<ACEDrawCmd> cmdBuffer;

    ACEFont* font = nullptr;

    // Clear all buffers
    void Clear();

    // Clip rect stack
    void PushClipRect(float x1, float y1, float x2, float y2);
    void PopClipRect();

    // ---- PRIMITIVES ----
    void AddRectFilled(float x1, float y1, float x2, float y2, uint32_t col, float rounding = 0);
    void AddRect(float x1, float y1, float x2, float y2, uint32_t col, float rounding = 0, float thickness = 1.0f);
    void AddRectFilledMultiColor(float x1, float y1, float x2, float y2,
                                  uint32_t colTL, uint32_t colTR, uint32_t colBR, uint32_t colBL);
    void AddLine(float x1, float y1, float x2, float y2, uint32_t col, float thickness = 1.0f);
    void AddCircleFilled(float cx, float cy, float radius, uint32_t col, int segments = 0);
    void AddCircle(float cx, float cy, float radius, uint32_t col, int segments = 0, float thickness = 1.0f);
    void AddTriangleFilled(float x1, float y1, float x2, float y2, float x3, float y3, uint32_t col);

    // ---- TEXT ----
    void AddText(float x, float y, uint32_t col, const char* text, const char* textEnd = nullptr);
    void AddTextShadow(float x, float y, uint32_t col, const char* text);
    void AddTextCentered(float x1, float y1, float x2, float y2, uint32_t col, const char* text);

    // ---- PATH BUILDING (for rounded shapes) ----
    void PathClear();
    void PathLineTo(float x, float y);
    void PathArcTo(float cx, float cy, float radius, float aMin, float aMax, int segments = 0);
    void PathRect(float x1, float y1, float x2, float y2, float rounding = 0);
    void PathFillConvex(uint32_t col);
    void PathStroke(uint32_t col, bool closed, float thickness = 1.0f);

private:
    std::vector<ACEVec2> _path;
    std::vector<ACERect> _clipStack;

    void _AddDrawCmd();
    void _UpdateClipRect();
    void _PrimReserve(int vtxCount, int idxCount);
    void _PrimRect(float x1, float y1, float x2, float y2,
                   float u1, float v1, float u2, float v2, uint32_t col);
    void _PrimQuad(ACEVec2 a, ACEVec2 b, ACEVec2 c, ACEVec2 d,
                   ACEVec2 uvA, ACEVec2 uvB, ACEVec2 uvC, ACEVec2 uvD, uint32_t col);
    ACEVec2 _WhiteUV() const;
};

// ============================================================================
// DX11 RENDERER
// ============================================================================
// Forward declarations (avoid pulling in d3d11.h here)
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D11Buffer;
struct ID3D11BlendState;
struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
struct ID3D11SamplerState;
struct ID3D11ShaderResourceView;

class ACERenderer {
public:
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    // Build font atlas as DX11 texture
    bool CreateFontTexture(ACEFont& font);

    // Render a completed draw list
    void RenderDrawList(const ACEDrawList& drawList, float displayW, float displayH);

private:
    bool CreateDeviceObjects();
    void DestroyDeviceObjects();
    void SetupRenderState(float displayW, float displayH);

    ID3D11Device*               _device = nullptr;
    ID3D11DeviceContext*        _context = nullptr;
    ID3D11VertexShader*         _vertexShader = nullptr;
    ID3D11PixelShader*          _pixelShader = nullptr;
    ID3D11InputLayout*          _inputLayout = nullptr;
    ID3D11Buffer*               _vertexBuffer = nullptr;
    ID3D11Buffer*               _indexBuffer = nullptr;
    ID3D11Buffer*               _constantBuffer = nullptr;
    ID3D11BlendState*           _blendState = nullptr;
    ID3D11RasterizerState*      _rasterizerState = nullptr;
    ID3D11DepthStencilState*    _depthStencilState = nullptr;
    ID3D11SamplerState*         _sampler = nullptr;
    ID3D11ShaderResourceView*   _fontTextureSRV = nullptr;
    int _vtxBufSize = 5000;
    int _idxBufSize = 10000;
};
