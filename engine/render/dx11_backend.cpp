/**
 * ACE Engine — DX11 Render Backend Implementation
 */

#include "dx11_backend.h"

#ifdef _WIN32

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

namespace ace {

// ============================================================================
// HLSL SHADERS (embedded)
// ============================================================================
static const char* g_vertexShaderSrc = R"(
cbuffer CB : register(b0) {
    float4x4 projection;
};

struct VS_INPUT {
    float2 pos   : POSITION;
    float2 uv    : TEXCOORD0;
    uint   color : COLOR0;
};

struct PS_INPUT {
    float4 pos   : SV_POSITION;
    float2 uv    : TEXCOORD0;
    float4 color : COLOR0;
};

PS_INPUT main(VS_INPUT input) {
    PS_INPUT output;
    output.pos = mul(float4(input.pos, 0.0f, 1.0f), projection);
    output.uv = input.uv;

    // Unpack ABGR u32 → float4 RGBA
    output.color.r = float((input.color >>  0) & 0xFF) / 255.0f;
    output.color.g = float((input.color >>  8) & 0xFF) / 255.0f;
    output.color.b = float((input.color >> 16) & 0xFF) / 255.0f;
    output.color.a = float((input.color >> 24) & 0xFF) / 255.0f;

    return output;
}
)";

static const char* g_pixelShaderSrc = R"(
Texture2D    tex0      : register(t0);
SamplerState sampler0  : register(s0);

struct PS_INPUT {
    float4 pos   : SV_POSITION;
    float2 uv    : TEXCOORD0;
    float4 color : COLOR0;
};

float4 main(PS_INPUT input) : SV_TARGET {
    float4 texColor = tex0.Sample(sampler0, input.uv);
    return input.color * texColor;
}
)";

// ============================================================================
// INITIALIZATION
// ============================================================================
bool DX11Backend::Initialize(void* windowHandle, u32 width, u32 height) {
    _hwnd = static_cast<HWND>(windowHandle);
    _width = width;
    _height = height;

    if (!CreateDeviceAndSwapChain(_hwnd, width, height)) return false;
    if (!CreateRenderTargetView()) return false;
    if (!CreateShaders()) return false;
    if (!CreateBuffers()) return false;
    if (!CreateBlendState()) return false;
    if (!CreateRasterizerState()) return false;
    if (!CreateSamplerState()) return false;

    // Create 1x1 white pixel as default texture
    u32 white = 0xFFFFFFFF;
    TextureDesc desc{};
    desc.width = 1;
    desc.height = 1;
    desc.channels = 4;
    desc.data = reinterpret_cast<const u8*>(&white);
    _whitePixel = CreateTexture(desc);

    return true;
}

void DX11Backend::Shutdown() {
    _textures.clear();
    _renderTargets.clear();
    _samplerState.Reset();
    _rasterizerState.Reset();
    _blendState.Reset();
    _constantBuffer.Reset();
    _indexBuffer.Reset();
    _vertexBuffer.Reset();
    _inputLayout.Reset();
    _pixelShader.Reset();
    _vertexShader.Reset();
    _rtv.Reset();
    _swapChain.Reset();
    _context.Reset();
    _device.Reset();
}

void DX11Backend::Resize(u32 width, u32 height) {
    if (width == 0 || height == 0) return;
    _width = width;
    _height = height;

    _rtv.Reset();
    _swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    CreateRenderTargetView();
}

// ============================================================================
// FRAME MANAGEMENT
// ============================================================================
void DX11Backend::BeginFrame() {
    auto cv = _clearColor.ToFloat();
    float clear[4] = {cv.x, cv.y, cv.z, cv.w};
    _context->ClearRenderTargetView(_rtv.Get(), clear);

    _context->OMSetRenderTargets(1, _rtv.GetAddressOf(), nullptr);

    D3D11_VIEWPORT vp{};
    vp.Width = (FLOAT)_width;
    vp.Height = (FLOAT)_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    _context->RSSetViewports(1, &vp);
}

void DX11Backend::EndFrame() {
    // Nothing needed between render and present
}

void DX11Backend::Present() {
    _swapChain->Present(1, 0); // VSync ON
}

// ============================================================================
// DRAW LIST RENDERING
// ============================================================================
void DX11Backend::RenderDrawList(const DrawList& drawList, u32 viewportW, u32 viewportH) {
    if (drawList.vertices.empty() || drawList.indices.empty()) return;

    // Grow vertex buffer if needed
    u32 vtxSize = static_cast<u32>(drawList.vertices.size() * sizeof(UIVertex));
    if (vtxSize > _vbSize) {
        _vbSize = vtxSize + 4096;
        D3D11_BUFFER_DESC bd{};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = _vbSize;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        _device->CreateBuffer(&bd, nullptr, _vertexBuffer.ReleaseAndGetAddressOf());
    }

    // Grow index buffer if needed
    u32 idxSize = static_cast<u32>(drawList.indices.size() * sizeof(u32));
    if (idxSize > _ibSize) {
        _ibSize = idxSize + 4096;
        D3D11_BUFFER_DESC bd{};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = _ibSize;
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        _device->CreateBuffer(&bd, nullptr, _indexBuffer.ReleaseAndGetAddressOf());
    }

    // Upload vertex data
    D3D11_MAPPED_SUBRESOURCE mapped{};
    _context->Map(_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, drawList.vertices.data(), drawList.vertices.size() * sizeof(UIVertex));
    _context->Unmap(_vertexBuffer.Get(), 0);

    // Upload index data
    _context->Map(_indexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, drawList.indices.data(), drawList.indices.size() * sizeof(u32));
    _context->Unmap(_indexBuffer.Get(), 0);

    // Update projection matrix
    UpdateConstantBuffer(viewportW, viewportH);

    // Bind pipeline
    UINT stride = sizeof(UIVertex);
    UINT offset = 0;
    _context->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
    _context->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _context->IASetInputLayout(_inputLayout.Get());

    _context->VSSetShader(_vertexShader.Get(), nullptr, 0);
    _context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
    _context->PSSetShader(_pixelShader.Get(), nullptr, 0);
    _context->PSSetSamplers(0, 1, _samplerState.GetAddressOf());

    _context->OMSetBlendState(_blendState.Get(), nullptr, 0xFFFFFFFF);
    _context->RSSetState(_rasterizerState.Get());

    // Issue draw calls
    for (auto& cmd : drawList.commands) {
        if (cmd.indexCount == 0) continue;

        // Set scissor rect
        D3D11_RECT scissor;
        scissor.left   = (LONG)cmd.clipRect.x;
        scissor.top    = (LONG)cmd.clipRect.y;
        scissor.right  = (LONG)cmd.clipRect.Right();
        scissor.bottom = (LONG)cmd.clipRect.Bottom();
        _context->RSSetScissorRects(1, &scissor);

        // Bind texture
        TextureHandle tex = cmd.texture != INVALID_TEXTURE ? cmd.texture : _whitePixel;
        auto it = _textures.find(tex);
        if (it != _textures.end()) {
            _context->PSSetShaderResources(0, 1, it->second.srv.GetAddressOf());
        }

        _context->DrawIndexed(cmd.indexCount, cmd.indexOffset, 0);
    }
}

// ============================================================================
// TEXTURE MANAGEMENT
// ============================================================================
TextureHandle DX11Backend::CreateTexture(const TextureDesc& desc) {
    TextureData td{};
    td.width = desc.width;
    td.height = desc.height;

    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = desc.width;
    texDesc.Height = desc.height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = desc.data;
    initData.SysMemPitch = desc.width * 4;

    HRESULT hr = _device->CreateTexture2D(&texDesc, desc.data ? &initData : nullptr,
                                           td.texture.GetAddressOf());
    if (FAILED(hr)) return INVALID_TEXTURE;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    hr = _device->CreateShaderResourceView(td.texture.Get(), &srvDesc, td.srv.GetAddressOf());
    if (FAILED(hr)) return INVALID_TEXTURE;

    TextureHandle handle = _nextTextureHandle++;
    _textures[handle] = std::move(td);
    return handle;
}

void DX11Backend::DestroyTexture(TextureHandle handle) {
    _textures.erase(handle);
}

// ============================================================================
// RENDER TARGET
// ============================================================================
RenderTargetHandle DX11Backend::CreateRenderTarget(u32 width, u32 height) {
    return CreateRenderTarget(RenderTargetDesc{width, height});
}

RenderTargetHandle DX11Backend::CreateRenderTarget(const RenderTargetDesc& desc) {
    RenderTarget rt{};
    rt.width = desc.width;
    rt.height = desc.height;

    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = desc.width;
    texDesc.Height = desc.height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(_device->CreateTexture2D(&texDesc, nullptr, rt.texture.GetAddressOf())))
        return INVALID_RT;

    if (FAILED(_device->CreateRenderTargetView(rt.texture.Get(), nullptr, rt.rtv.GetAddressOf())))
        return INVALID_RT;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    if (FAILED(_device->CreateShaderResourceView(rt.texture.Get(), &srvDesc, rt.srv.GetAddressOf())))
        return INVALID_RT;

    // Create a TextureHandle for this RT so it can be sampled
    TextureData td{};
    td.texture = rt.texture;
    td.srv = rt.srv;
    td.width = desc.width;
    td.height = desc.height;
    TextureHandle texH = _nextTextureHandle++;
    _textures[texH] = std::move(td);
    rt.texHandle = texH;

    RenderTargetHandle handle = _nextRTHandle++;
    _renderTargets[handle] = std::move(rt);
    return handle;
}

void DX11Backend::DestroyRenderTarget(RenderTargetHandle handle) {
    auto it = _renderTargets.find(handle);
    if (it != _renderTargets.end()) {
        _textures.erase(it->second.texHandle);
        _renderTargets.erase(it);
    }
}

void DX11Backend::BindRenderTarget(RenderTargetHandle handle) {
    auto it = _renderTargets.find(handle);
    if (it == _renderTargets.end()) return;

    auto& rt = it->second;
    float clear[4] = {0, 0, 0, 0};
    _context->ClearRenderTargetView(rt.rtv.Get(), clear);
    _context->OMSetRenderTargets(1, rt.rtv.GetAddressOf(), nullptr);

    D3D11_VIEWPORT vp{};
    vp.Width = (FLOAT)rt.width;
    vp.Height = (FLOAT)rt.height;
    vp.MaxDepth = 1.0f;
    _context->RSSetViewports(1, &vp);
}

void DX11Backend::UnbindRenderTarget() {
    _context->OMSetRenderTargets(1, _rtv.GetAddressOf(), nullptr);

    D3D11_VIEWPORT vp{};
    vp.Width = (FLOAT)_width;
    vp.Height = (FLOAT)_height;
    vp.MaxDepth = 1.0f;
    _context->RSSetViewports(1, &vp);
}

TextureHandle DX11Backend::GetRenderTargetTexture(RenderTargetHandle handle) {
    auto it = _renderTargets.find(handle);
    return it != _renderTargets.end() ? it->second.texHandle : INVALID_TEXTURE;
}

// ============================================================================
// INTERNAL: Device and swap chain creation
// ============================================================================
bool DX11Backend::CreateDeviceAndSwapChain(HWND hwnd, u32 w, u32 h) {
    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferCount = 2;
    scd.BufferDesc.Width = w;
    scd.BufferDesc.Height = h;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        nullptr, 0, D3D11_SDK_VERSION,
        &scd, _swapChain.GetAddressOf(),
        _device.GetAddressOf(), &featureLevel,
        _context.GetAddressOf()
    );

    return SUCCEEDED(hr);
}

bool DX11Backend::CreateRenderTargetView() {
    ComPtr<ID3D11Texture2D> backBuffer;
    _swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    return SUCCEEDED(_device->CreateRenderTargetView(
        backBuffer.Get(), nullptr, _rtv.GetAddressOf()));
}

bool DX11Backend::CreateShaders() {
    // Compile vertex shader
    ComPtr<ID3DBlob> vsBlob, errorBlob;
    HRESULT hr = D3DCompile(g_vertexShaderSrc, strlen(g_vertexShaderSrc),
        "VS", nullptr, nullptr, "main", "vs_5_0", 0, 0,
        vsBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = _device->CreateVertexShader(vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(), nullptr, _vertexShader.GetAddressOf());
    if (FAILED(hr)) return false;

    // Input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(UIVertex, pos),   D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(UIVertex, uv),    D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32_UINT,         0, offsetof(UIVertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    hr = _device->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(), _inputLayout.GetAddressOf());
    if (FAILED(hr)) return false;

    // Compile pixel shader
    ComPtr<ID3DBlob> psBlob;
    hr = D3DCompile(g_pixelShaderSrc, strlen(g_pixelShaderSrc),
        "PS", nullptr, nullptr, "main", "ps_5_0", 0, 0,
        psBlob.GetAddressOf(), errorBlob.ReleaseAndGetAddressOf());
    if (FAILED(hr)) return false;

    hr = _device->CreatePixelShader(psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(), nullptr, _pixelShader.GetAddressOf());
    return SUCCEEDED(hr);
}

bool DX11Backend::CreateBuffers() {
    // Initial vertex buffer
    _vbSize = 65536 * sizeof(UIVertex);
    D3D11_BUFFER_DESC vbd{};
    vbd.Usage = D3D11_USAGE_DYNAMIC;
    vbd.ByteWidth = _vbSize;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(_device->CreateBuffer(&vbd, nullptr, _vertexBuffer.GetAddressOf())))
        return false;

    // Initial index buffer
    _ibSize = 65536 * sizeof(u32);
    D3D11_BUFFER_DESC ibd{};
    ibd.Usage = D3D11_USAGE_DYNAMIC;
    ibd.ByteWidth = _ibSize;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(_device->CreateBuffer(&ibd, nullptr, _indexBuffer.GetAddressOf())))
        return false;

    // Constant buffer (projection matrix)
    D3D11_BUFFER_DESC cbd{};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(Mat4);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    return SUCCEEDED(_device->CreateBuffer(&cbd, nullptr, _constantBuffer.GetAddressOf()));
}

bool DX11Backend::CreateBlendState() {
    D3D11_BLEND_DESC bd{};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    return SUCCEEDED(_device->CreateBlendState(&bd, _blendState.GetAddressOf()));
}

bool DX11Backend::CreateRasterizerState() {
    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.ScissorEnable = TRUE;
    rd.DepthClipEnable = TRUE;
    return SUCCEEDED(_device->CreateRasterizerState(&rd, _rasterizerState.GetAddressOf()));
}

bool DX11Backend::CreateSamplerState() {
    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    return SUCCEEDED(_device->CreateSamplerState(&sd, _samplerState.GetAddressOf()));
}

void DX11Backend::UpdateConstantBuffer(u32 viewportW, u32 viewportH) {
    Mat4 proj = Mat4::Ortho(0, (f32)viewportW, 0, (f32)viewportH);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    _context->Map(_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, &proj, sizeof(Mat4));
    _context->Unmap(_constantBuffer.Get(), 0);
}

} // namespace ace

#endif // _WIN32
