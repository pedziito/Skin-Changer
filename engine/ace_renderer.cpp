/*
 * ACE Renderer — Implementation
 * DrawList primitive building, stb_truetype font loading, DX11 rendering backend.
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler")

#include "ace_renderer.h"

// stb_truetype implementation
#define STB_TRUETYPE_IMPLEMENTATION
#include "../vendor/stb/stb_truetype.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// FONT IMPLEMENTATION
// ============================================================================
bool ACEFont::LoadFromFile(const char* path, float size) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* buf = new unsigned char[len];
    fread(buf, 1, len, f);
    fclose(f);
    bool ok = LoadFromMemory(buf, (int)len, size);
    delete[] buf;
    return ok;
}

bool ACEFont::LoadFromMemory(const unsigned char* data, int dataSize, float size) {
    fontSize = size;

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, data, 0)) return false;

    // Calculate metrics
    float scale = stbtt_ScaleForPixelHeight(&info, size);
    int iAscent, iDescent, iLineGap;
    stbtt_GetFontVMetrics(&info, &iAscent, &iDescent, &iLineGap);
    ascent = iAscent * scale;
    descent = iDescent * scale;
    lineHeight = (iAscent - iDescent + iLineGap) * scale;

    // Bake atlas — allocate generous size
    atlasWidth = 512;
    atlasHeight = 512;
    atlasData = new unsigned char[atlasWidth * atlasHeight];
    memset(atlasData, 0, atlasWidth * atlasHeight);

    stbtt_bakedchar baked[96]; // ASCII 32-127
    int result = stbtt_BakeFontBitmap(data, 0, size, atlasData, atlasWidth, atlasHeight, 32, 96, baked);
    if (result <= 0) {
        // Try larger atlas
        delete[] atlasData;
        atlasWidth = 1024;
        atlasHeight = 1024;
        atlasData = new unsigned char[atlasWidth * atlasHeight];
        memset(atlasData, 0, atlasWidth * atlasHeight);
        result = stbtt_BakeFontBitmap(data, 0, size, atlasData, atlasWidth, atlasHeight, 32, 96, baked);
        if (result <= 0) {
            delete[] atlasData;
            atlasData = nullptr;
            return false;
        }
    }

    // Store glyph info
    float invW = 1.0f / atlasWidth;
    float invH = 1.0f / atlasHeight;
    memset(glyphValid, 0, sizeof(glyphValid));

    for (int i = 0; i < 96; i++) {
        int c = 32 + i;
        auto& b = baked[i];
        auto& g = glyphs[c];
        g.u0 = b.x0 * invW;
        g.v0 = b.y0 * invH;
        g.u1 = b.x1 * invW;
        g.v1 = b.y1 * invH;
        g.xoff = b.xoff;
        g.yoff = b.yoff;
        g.xoff2 = b.xoff + (b.x1 - b.x0);
        g.yoff2 = b.yoff + (b.y1 - b.y0);
        g.xadvance = b.xadvance;
        glyphValid[c] = true;
    }

    // Insert white pixel at bottom-right corner of atlas
    int wpX = atlasWidth - 2;
    int wpY = atlasHeight - 2;
    for (int dy = 0; dy < 2; dy++)
        for (int dx = 0; dx < 2; dx++)
            atlasData[(wpY+dy) * atlasWidth + (wpX+dx)] = 255;
    whitePixelU = (wpX + 0.5f) * invW;
    whitePixelV = (wpY + 0.5f) * invH;

    return true;
}

void ACEFont::Free() {
    delete[] atlasData;
    atlasData = nullptr;
}

const ACEGlyphInfo* ACEFont::GetGlyph(int c) const {
    if (c >= 0 && c < 128 && glyphValid[c]) return &glyphs[c];
    if (glyphValid['?']) return &glyphs['?'];
    return nullptr;
}

ACEVec2 ACEFont::CalcTextSize(const char* text, const char* textEnd) const {
    float w = 0, maxW = 0;
    float h = lineHeight;
    if (!text) return { 0, 0 };
    const char* s = text;
    while (s != textEnd && *s) {
        if (*s == '\n') {
            if (w > maxW) maxW = w;
            w = 0;
            h += lineHeight;
        } else {
            auto* g = GetGlyph(*s);
            if (g) w += g->xadvance;
        }
        s++;
    }
    if (w > maxW) maxW = w;
    return { maxW, h };
}

// ============================================================================
// DRAW LIST IMPLEMENTATION
// ============================================================================
void ACEDrawList::Clear() {
    vtxBuffer.clear();
    idxBuffer.clear();
    cmdBuffer.clear();
    _path.clear();
    _clipStack.clear();
}

ACEVec2 ACEDrawList::_WhiteUV() const {
    if (font) return { font->whitePixelU, font->whitePixelV };
    return { 0, 0 };
}

void ACEDrawList::_AddDrawCmd() {
    ACEDrawCmd cmd;
    cmd.elemCount = 0;
    cmd.vtxOffset = 0;
    cmd.idxOffset = (uint32_t)idxBuffer.size();
    if (!_clipStack.empty())
        cmd.clipRect = _clipStack.back();
    else
        cmd.clipRect = { -8192, -8192, 8192, 8192 };
    cmdBuffer.push_back(cmd);
}

void ACEDrawList::_UpdateClipRect() {
    ACERect clip;
    if (!_clipStack.empty())
        clip = _clipStack.back();
    else
        clip = { -8192, -8192, 8192, 8192 };

    if (cmdBuffer.empty() || cmdBuffer.back().elemCount != 0) {
        _AddDrawCmd();
    } else {
        cmdBuffer.back().clipRect = clip;
    }
}

void ACEDrawList::PushClipRect(float x1, float y1, float x2, float y2) {
    _clipStack.push_back({ x1, y1, x2, y2 });
    _UpdateClipRect();
}

void ACEDrawList::PopClipRect() {
    if (!_clipStack.empty()) _clipStack.pop_back();
    _UpdateClipRect();
}

void ACEDrawList::_PrimReserve(int vtxCount, int idxCount) {
    if (cmdBuffer.empty()) _AddDrawCmd();
    cmdBuffer.back().elemCount += idxCount;
}

void ACEDrawList::_PrimRect(float x1, float y1, float x2, float y2,
                             float u1, float v1, float u2, float v2, uint32_t col) {
    uint32_t idx = (uint32_t)vtxBuffer.size();
    _PrimReserve(4, 6);

    idxBuffer.push_back(idx);     idxBuffer.push_back(idx + 1);
    idxBuffer.push_back(idx + 2); idxBuffer.push_back(idx);
    idxBuffer.push_back(idx + 2); idxBuffer.push_back(idx + 3);

    vtxBuffer.push_back({{ x1, y1 }, { u1, v1 }, col});
    vtxBuffer.push_back({{ x2, y1 }, { u2, v1 }, col});
    vtxBuffer.push_back({{ x2, y2 }, { u2, v2 }, col});
    vtxBuffer.push_back({{ x1, y2 }, { u1, v2 }, col});
}

void ACEDrawList::_PrimQuad(ACEVec2 a, ACEVec2 b, ACEVec2 c, ACEVec2 d,
                             ACEVec2 uvA, ACEVec2 uvB, ACEVec2 uvC, ACEVec2 uvD, uint32_t col) {
    uint32_t idx = (uint32_t)vtxBuffer.size();
    _PrimReserve(4, 6);

    idxBuffer.push_back(idx);     idxBuffer.push_back(idx + 1);
    idxBuffer.push_back(idx + 2); idxBuffer.push_back(idx);
    idxBuffer.push_back(idx + 2); idxBuffer.push_back(idx + 3);

    vtxBuffer.push_back({{ a.x, a.y }, { uvA.x, uvA.y }, col});
    vtxBuffer.push_back({{ b.x, b.y }, { uvB.x, uvB.y }, col});
    vtxBuffer.push_back({{ c.x, c.y }, { uvC.x, uvC.y }, col});
    vtxBuffer.push_back({{ d.x, d.y }, { uvD.x, uvD.y }, col});
}

// ---- BASIC SHAPES ----

void ACEDrawList::AddRectFilled(float x1, float y1, float x2, float y2, uint32_t col, float rounding) {
    if ((col & 0xFF000000) == 0) return;
    if (rounding > 0.0f) {
        PathRect(x1, y1, x2, y2, rounding);
        PathFillConvex(col);
    } else {
        ACEVec2 uv = _WhiteUV();
        _PrimRect(x1, y1, x2, y2, uv.x, uv.y, uv.x, uv.y, col);
    }
}

void ACEDrawList::AddRect(float x1, float y1, float x2, float y2, uint32_t col, float rounding, float thickness) {
    if ((col & 0xFF000000) == 0) return;
    if (rounding > 0.0f) {
        PathRect(x1, y1, x2, y2, rounding);
        PathStroke(col, true, thickness);
    } else {
        // Four line segments
        AddLine(x1, y1, x2, y1, col, thickness);
        AddLine(x2, y1, x2, y2, col, thickness);
        AddLine(x2, y2, x1, y2, col, thickness);
        AddLine(x1, y2, x1, y1, col, thickness);
    }
}

void ACEDrawList::AddRectFilledMultiColor(float x1, float y1, float x2, float y2,
                                           uint32_t colTL, uint32_t colTR, uint32_t colBR, uint32_t colBL) {
    ACEVec2 uv = _WhiteUV();
    uint32_t idx = (uint32_t)vtxBuffer.size();
    _PrimReserve(4, 6);

    idxBuffer.push_back(idx);     idxBuffer.push_back(idx + 1);
    idxBuffer.push_back(idx + 2); idxBuffer.push_back(idx);
    idxBuffer.push_back(idx + 2); idxBuffer.push_back(idx + 3);

    vtxBuffer.push_back({{ x1, y1 }, { uv.x, uv.y }, colTL});
    vtxBuffer.push_back({{ x2, y1 }, { uv.x, uv.y }, colTR});
    vtxBuffer.push_back({{ x2, y2 }, { uv.x, uv.y }, colBR});
    vtxBuffer.push_back({{ x1, y2 }, { uv.x, uv.y }, colBL});
}

void ACEDrawList::AddLine(float x1, float y1, float x2, float y2, uint32_t col, float thickness) {
    if ((col & 0xFF000000) == 0) return;
    ACEVec2 uv = _WhiteUV();

    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 0.001f) return;
    float nx = -dy / len * thickness * 0.5f;
    float ny =  dx / len * thickness * 0.5f;

    ACEVec2 a = { x1 + nx, y1 + ny };
    ACEVec2 b = { x2 + nx, y2 + ny };
    ACEVec2 c = { x2 - nx, y2 - ny };
    ACEVec2 d = { x1 - nx, y1 - ny };
    _PrimQuad(a, b, c, d, uv, uv, uv, uv, col);
}

void ACEDrawList::AddCircleFilled(float cx, float cy, float radius, uint32_t col, int segments) {
    if ((col & 0xFF000000) == 0 || radius <= 0) return;
    if (segments <= 0) segments = std::max(24, (int)(radius * 1.5f));
    ACEVec2 uv = _WhiteUV();

    uint32_t centerIdx = (uint32_t)vtxBuffer.size();
    _PrimReserve(segments + 1, segments * 3);

    vtxBuffer.push_back({{ cx, cy }, { uv.x, uv.y }, col});

    for (int i = 0; i < segments; i++) {
        float a = (float)i / segments * 2.0f * (float)M_PI;
        vtxBuffer.push_back({{ cx + cosf(a) * radius, cy + sinf(a) * radius }, { uv.x, uv.y }, col});
    }

    for (int i = 0; i < segments; i++) {
        idxBuffer.push_back(centerIdx);
        idxBuffer.push_back(centerIdx + 1 + i);
        idxBuffer.push_back(centerIdx + 1 + ((i + 1) % segments));
    }
}

void ACEDrawList::AddCircle(float cx, float cy, float radius, uint32_t col, int segments, float thickness) {
    if ((col & 0xFF000000) == 0 || radius <= 0) return;
    if (segments <= 0) segments = std::max(24, (int)(radius * 1.5f));

    PathClear();
    for (int i = 0; i <= segments; i++) {
        float a = (float)i / segments * 2.0f * (float)M_PI;
        PathLineTo(cx + cosf(a) * radius, cy + sinf(a) * radius);
    }
    PathStroke(col, true, thickness);
}

void ACEDrawList::AddTriangleFilled(float x1, float y1, float x2, float y2, float x3, float y3, uint32_t col) {
    if ((col & 0xFF000000) == 0) return;
    ACEVec2 uv = _WhiteUV();
    uint32_t idx = (uint32_t)vtxBuffer.size();
    _PrimReserve(3, 3);

    idxBuffer.push_back(idx);
    idxBuffer.push_back(idx + 1);
    idxBuffer.push_back(idx + 2);

    vtxBuffer.push_back({{ x1, y1 }, { uv.x, uv.y }, col});
    vtxBuffer.push_back({{ x2, y2 }, { uv.x, uv.y }, col});
    vtxBuffer.push_back({{ x3, y3 }, { uv.x, uv.y }, col});
}

// ---- TEXT ----

void ACEDrawList::AddText(float x, float y, uint32_t col, const char* text, const char* textEnd) {
    if ((col & 0xFF000000) == 0 || !text || !font) return;

    float startX = x;
    const char* s = text;
    while (s != textEnd && *s) {
        if (*s == '\n') {
            x = startX;
            y += font->lineHeight;
            s++;
            continue;
        }
        auto* g = font->GetGlyph(*s);
        if (g) {
            float px0 = x + g->xoff;
            float py0 = y + g->yoff + font->ascent;
            float px1 = x + g->xoff2;
            float py1 = y + g->yoff2 + font->ascent;

            _PrimRect(px0, py0, px1, py1, g->u0, g->v0, g->u1, g->v1, col);
            x += g->xadvance;
        }
        s++;
    }
}

void ACEDrawList::AddTextShadow(float x, float y, uint32_t col, const char* text) {
    AddText(x + 1, y + 1, ACE_COL32(0, 0, 0, 180), text);
    AddText(x, y, col, text);
}

void ACEDrawList::AddTextCentered(float x1, float y1, float x2, float y2, uint32_t col, const char* text) {
    if (!font || !text) return;
    ACEVec2 ts = font->CalcTextSize(text);
    float x = x1 + (x2 - x1 - ts.x) * 0.5f;
    float y = y1 + (y2 - y1 - ts.y) * 0.5f;
    AddText(x, y, col, text);
}

// ---- PATH BUILDING ----

void ACEDrawList::PathClear() { _path.clear(); }
void ACEDrawList::PathLineTo(float x, float y) { _path.push_back({x, y}); }

void ACEDrawList::PathArcTo(float cx, float cy, float radius, float aMin, float aMax, int segments) {
    if (segments <= 0) segments = std::max(12, (int)(radius * 0.8f));
    for (int i = 0; i <= segments; i++) {
        float a = aMin + (float)i / segments * (aMax - aMin);
        _path.push_back({ cx + cosf(a) * radius, cy + sinf(a) * radius });
    }
}

void ACEDrawList::PathRect(float x1, float y1, float x2, float y2, float rounding) {
    PathClear();
    if (rounding <= 0.5f) {
        PathLineTo(x1, y1); PathLineTo(x2, y1);
        PathLineTo(x2, y2); PathLineTo(x1, y2);
    } else {
        float r = std::min(rounding, std::min((x2-x1)*0.5f, (y2-y1)*0.5f));
        // Higher segment count for clean rounded corners
        int segs = std::max(16, (int)(r * 1.2f));
        PathArcTo(x1+r, y1+r, r, (float)M_PI,     (float)M_PI*1.5f, segs);
        PathArcTo(x2-r, y1+r, r, (float)M_PI*1.5f, (float)M_PI*2.0f, segs);
        PathArcTo(x2-r, y2-r, r, 0.0f,             (float)M_PI*0.5f, segs);
        PathArcTo(x1+r, y2-r, r, (float)M_PI*0.5f, (float)M_PI,      segs);
    }
}

void ACEDrawList::PathFillConvex(uint32_t col) {
    if (_path.size() < 3) { PathClear(); return; }
    ACEVec2 uv = _WhiteUV();

    uint32_t baseIdx = (uint32_t)vtxBuffer.size();
    int count = (int)_path.size();
    _PrimReserve(count, (count - 2) * 3);

    for (auto& p : _path)
        vtxBuffer.push_back({{ p.x, p.y }, { uv.x, uv.y }, col});

    for (int i = 2; i < count; i++) {
        idxBuffer.push_back(baseIdx);
        idxBuffer.push_back(baseIdx + i - 1);
        idxBuffer.push_back(baseIdx + i);
    }
    PathClear();
}

void ACEDrawList::PathStroke(uint32_t col, bool closed, float thickness) {
    if (_path.size() < 2) { PathClear(); return; }
    ACEVec2 uv = _WhiteUV();

    int count = (int)_path.size();
    int segCount = closed ? count : count - 1;

    for (int i = 0; i < segCount; i++) {
        int j = (i + 1) % count;
        float dx = _path[j].x - _path[i].x;
        float dy = _path[j].y - _path[i].y;
        float len = sqrtf(dx*dx + dy*dy);
        if (len < 0.001f) continue;
        float nx = -dy / len * thickness * 0.5f;
        float ny =  dx / len * thickness * 0.5f;

        ACEVec2 a = { _path[i].x + nx, _path[i].y + ny };
        ACEVec2 b = { _path[j].x + nx, _path[j].y + ny };
        ACEVec2 c = { _path[j].x - nx, _path[j].y - ny };
        ACEVec2 d = { _path[i].x - nx, _path[i].y - ny };
        _PrimQuad(a, b, c, d, uv, uv, uv, uv, col);
    }
    PathClear();
}

// ============================================================================
// DX11 RENDERER IMPLEMENTATION
// ============================================================================

// HLSL source strings
static const char* g_aceVertexShaderSrc =
    "cbuffer ConstBuf : register(b0) { float4x4 ProjectionMatrix; };\n"
    "struct VS_INPUT  { float2 pos : POSITION; float2 uv : TEXCOORD0; float4 col : COLOR0; };\n"
    "struct PS_INPUT  { float4 pos : SV_POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };\n"
    "PS_INPUT main(VS_INPUT i) {\n"
    "  PS_INPUT o;\n"
    "  o.pos = mul(ProjectionMatrix, float4(i.pos.xy, 0.f, 1.f));\n"
    "  o.col = i.col;\n"
    "  o.uv  = i.uv;\n"
    "  return o;\n"
    "}\n";

static const char* g_acePixelShaderSrc =
    "struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };\n"
    "Texture2D tex0 : register(t0);\n"
    "SamplerState smp0 : register(s0);\n"
    "float4 main(PS_INPUT i) : SV_Target {\n"
    "  float alpha = tex0.Sample(smp0, i.uv).r;\n"
    "  return float4(i.col.rgb, i.col.a * alpha);\n"
    "}\n";

struct ACE_CONSTANT_BUFFER { float mvp[4][4]; };

bool ACERenderer::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
    _device = device;
    _context = context;
    return CreateDeviceObjects();
}

bool ACERenderer::CreateDeviceObjects() {
    if (!_device) return false;

    // ---- Compile Vertex Shader ----
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errBlob = nullptr;
    if (FAILED(D3DCompile(g_aceVertexShaderSrc, strlen(g_aceVertexShaderSrc),
                           nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsBlob, &errBlob))) {
        if (errBlob) errBlob->Release();
        return false;
    }
    _device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &_vertexShader);

    // ---- Input Layout ----
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, offsetof(ACEVertex, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, offsetof(ACEVertex, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, offsetof(ACEVertex, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    _device->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &_inputLayout);
    vsBlob->Release();

    // ---- Compile Pixel Shader ----
    ID3DBlob* psBlob = nullptr;
    if (FAILED(D3DCompile(g_acePixelShaderSrc, strlen(g_acePixelShaderSrc),
                           nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &psBlob, &errBlob))) {
        if (errBlob) errBlob->Release();
        return false;
    }
    _device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &_pixelShader);
    psBlob->Release();

    // ---- Constant Buffer ----
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(ACE_CONSTANT_BUFFER);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        _device->CreateBuffer(&desc, nullptr, &_constantBuffer);
    }

    // ---- Blend State (alpha blending) ----
    {
        D3D11_BLEND_DESC desc = {};
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        _device->CreateBlendState(&desc, &_blendState);
    }

    // ---- Rasterizer (no culling, scissor) ----
    {
        D3D11_RASTERIZER_DESC desc = {};
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.ScissorEnable = TRUE;
        desc.DepthClipEnable = TRUE;
        _device->CreateRasterizerState(&desc, &_rasterizerState);
    }

    // ---- Depth Stencil (disabled) ----
    {
        D3D11_DEPTH_STENCIL_DESC desc = {};
        desc.DepthEnable = FALSE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        _device->CreateDepthStencilState(&desc, &_depthStencilState);
    }

    // ---- Sampler (bilinear, clamp) ----
    {
        D3D11_SAMPLER_DESC desc = {};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        _device->CreateSamplerState(&desc, &_sampler);
    }

    return true;
}

bool ACERenderer::CreateFontTexture(ACEFont& font) {
    if (!_device || !font.atlasData) return false;

    // Create R8_UNORM texture from atlas bitmap
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = font.atlasWidth;
    texDesc.Height = font.atlasHeight;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subData = {};
    subData.pSysMem = font.atlasData;
    subData.SysMemPitch = font.atlasWidth;

    ID3D11Texture2D* tex = nullptr;
    if (FAILED(_device->CreateTexture2D(&texDesc, &subData, &tex))) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    _device->CreateShaderResourceView(tex, &srvDesc, &_fontTextureSRV);
    tex->Release();

    return true;
}

void ACERenderer::DestroyDeviceObjects() {
    if (_fontTextureSRV)    { _fontTextureSRV->Release();   _fontTextureSRV = nullptr; }
    if (_sampler)           { _sampler->Release();          _sampler = nullptr; }
    if (_depthStencilState) { _depthStencilState->Release();_depthStencilState = nullptr; }
    if (_rasterizerState)   { _rasterizerState->Release();  _rasterizerState = nullptr; }
    if (_blendState)        { _blendState->Release();       _blendState = nullptr; }
    if (_constantBuffer)    { _constantBuffer->Release();   _constantBuffer = nullptr; }
    if (_inputLayout)       { _inputLayout->Release();      _inputLayout = nullptr; }
    if (_pixelShader)       { _pixelShader->Release();      _pixelShader = nullptr; }
    if (_vertexShader)      { _vertexShader->Release();     _vertexShader = nullptr; }
    if (_vertexBuffer)      { _vertexBuffer->Release();     _vertexBuffer = nullptr; }
    if (_indexBuffer)       { _indexBuffer->Release();      _indexBuffer = nullptr; }
}

void ACERenderer::Shutdown() { DestroyDeviceObjects(); }

void ACERenderer::SetupRenderState(float displayW, float displayH) {
    // Orthographic projection matrix
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (_context->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped) == S_OK) {
        ACE_CONSTANT_BUFFER* cb = (ACE_CONSTANT_BUFFER*)mapped.pData;
        float L = 0, R = displayW, T = 0, B = displayH;
        float mvp[4][4] = {
            { 2.0f/(R-L),   0.0f,       0.0f, 0.0f },
            { 0.0f,         2.0f/(T-B), 0.0f, 0.0f },
            { 0.0f,         0.0f,       0.5f, 0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),0.5f, 1.0f },
        };
        memcpy(cb->mvp, mvp, sizeof(mvp));
        _context->Unmap(_constantBuffer, 0);
    }

    D3D11_VIEWPORT vp = {};
    vp.Width = displayW;
    vp.Height = displayH;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    _context->RSSetViewports(1, &vp);

    unsigned int stride = sizeof(ACEVertex);
    unsigned int offset = 0;
    _context->IASetInputLayout(_inputLayout);
    _context->IASetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
    _context->IASetIndexBuffer(_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _context->VSSetShader(_vertexShader, nullptr, 0);
    _context->VSSetConstantBuffers(0, 1, &_constantBuffer);
    _context->PSSetShader(_pixelShader, nullptr, 0);
    _context->PSSetSamplers(0, 1, &_sampler);
    _context->GSSetShader(nullptr, nullptr, 0);
    _context->HSSetShader(nullptr, nullptr, 0);
    _context->DSSetShader(nullptr, nullptr, 0);
    _context->CSSetShader(nullptr, nullptr, 0);

    const float blend_factor[4] = { 0, 0, 0, 0 };
    _context->OMSetBlendState(_blendState, blend_factor, 0xffffffff);
    _context->OMSetDepthStencilState(_depthStencilState, 0);
    _context->RSSetState(_rasterizerState);
}

void ACERenderer::RenderDrawList(const ACEDrawList& drawList, float displayW, float displayH) {
    if (drawList.vtxBuffer.empty() || drawList.idxBuffer.empty()) return;

    // Grow vertex buffer if needed
    int totalVtx = (int)drawList.vtxBuffer.size();
    int totalIdx = (int)drawList.idxBuffer.size();

    if (!_vertexBuffer || _vtxBufSize < totalVtx) {
        if (_vertexBuffer) _vertexBuffer->Release();
        _vtxBufSize = totalVtx + 5000;
        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = _vtxBufSize * sizeof(ACEVertex);
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(_device->CreateBuffer(&desc, nullptr, &_vertexBuffer))) return;
    }

    if (!_indexBuffer || _idxBufSize < totalIdx) {
        if (_indexBuffer) _indexBuffer->Release();
        _idxBufSize = totalIdx + 10000;
        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = _idxBufSize * sizeof(uint32_t);
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(_device->CreateBuffer(&desc, nullptr, &_indexBuffer))) return;
    }

    // Upload vertex/index data
    D3D11_MAPPED_SUBRESOURCE vtxRes, idxRes;
    if (_context->Map(_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &vtxRes) != S_OK) return;
    if (_context->Map(_indexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &idxRes) != S_OK) return;
    memcpy(vtxRes.pData, drawList.vtxBuffer.data(), totalVtx * sizeof(ACEVertex));
    memcpy(idxRes.pData, drawList.idxBuffer.data(), totalIdx * sizeof(uint32_t));
    _context->Unmap(_vertexBuffer, 0);
    _context->Unmap(_indexBuffer, 0);

    // Backup DX11 state
    struct {
        UINT scissorCount, viewportCount;
        D3D11_RECT scissors[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        ID3D11RasterizerState* rs;
        ID3D11BlendState* bs;    FLOAT bf[4];    UINT sm;
        ID3D11DepthStencilState* dss; UINT sr;
        ID3D11PixelShader* ps;   ID3D11VertexShader* vs;
        ID3D11InputLayout* il;
        ID3D11Buffer* ib, *vb, *cb;
        UINT ibOff, vbStride, vbOff;
        DXGI_FORMAT ibFmt;
        D3D11_PRIMITIVE_TOPOLOGY topo;
        ID3D11ShaderResourceView* psSRV;
        ID3D11SamplerState* psSampler;
    } old = {};
    old.scissorCount = old.viewportCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    _context->RSGetScissorRects(&old.scissorCount, old.scissors);
    _context->RSGetViewports(&old.viewportCount, old.viewports);
    _context->RSGetState(&old.rs);
    _context->OMGetBlendState(&old.bs, old.bf, &old.sm);
    _context->OMGetDepthStencilState(&old.dss, &old.sr);
    _context->PSGetShaderResources(0, 1, &old.psSRV);
    _context->PSGetSamplers(0, 1, &old.psSampler);
    _context->PSGetShader(&old.ps, nullptr, nullptr);
    _context->VSGetShader(&old.vs, nullptr, nullptr);
    _context->VSGetConstantBuffers(0, 1, &old.cb);
    _context->IAGetPrimitiveTopology(&old.topo);
    _context->IAGetIndexBuffer(&old.ib, &old.ibFmt, &old.ibOff);
    _context->IAGetVertexBuffers(0, 1, &old.vb, &old.vbStride, &old.vbOff);
    _context->IAGetInputLayout(&old.il);

    // Setup our render state
    SetupRenderState(displayW, displayH);

    // Bind font texture
    if (_fontTextureSRV)
        _context->PSSetShaderResources(0, 1, &_fontTextureSRV);

    // Execute draw commands
    for (auto& cmd : drawList.cmdBuffer) {
        if (cmd.elemCount == 0) continue;

        D3D11_RECT r = {
            (LONG)cmd.clipRect.x1, (LONG)cmd.clipRect.y1,
            (LONG)cmd.clipRect.x2, (LONG)cmd.clipRect.y2
        };
        _context->RSSetScissorRects(1, &r);
        _context->DrawIndexed(cmd.elemCount, cmd.idxOffset, 0);
    }

    // Restore DX11 state
    _context->RSSetScissorRects(old.scissorCount, old.scissors);
    _context->RSSetViewports(old.viewportCount, old.viewports);
    _context->RSSetState(old.rs);            if (old.rs) old.rs->Release();
    _context->OMSetBlendState(old.bs, old.bf, old.sm); if (old.bs) old.bs->Release();
    _context->OMSetDepthStencilState(old.dss, old.sr); if (old.dss) old.dss->Release();
    _context->PSSetShaderResources(0, 1, &old.psSRV);  if (old.psSRV) old.psSRV->Release();
    _context->PSSetSamplers(0, 1, &old.psSampler);      if (old.psSampler) old.psSampler->Release();
    _context->PSSetShader(old.ps, nullptr, 0);           if (old.ps) old.ps->Release();
    _context->VSSetShader(old.vs, nullptr, 0);           if (old.vs) old.vs->Release();
    _context->VSSetConstantBuffers(0, 1, &old.cb);       if (old.cb) old.cb->Release();
    _context->IASetPrimitiveTopology(old.topo);
    _context->IASetIndexBuffer(old.ib, old.ibFmt, old.ibOff); if (old.ib) old.ib->Release();
    _context->IASetVertexBuffers(0, 1, &old.vb, &old.vbStride, &old.vbOff); if (old.vb) old.vb->Release();
    _context->IASetInputLayout(old.il);                  if (old.il) old.il->Release();
}
