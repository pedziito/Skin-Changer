/**
 * ACE Engine — DX11 Render Backend
 * DirectX 11 implementation of IRenderBackend.
 * Handles device creation, shader compilation, buffer management, and draw call submission.
 */

#pragma once

#include "render_backend.h"

#ifdef _WIN32
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <unordered_map>

namespace ace {

using Microsoft::WRL::ComPtr;

class DX11Backend : public IRenderBackend {
public:
    DX11Backend() = default;
    ~DX11Backend() override { Shutdown(); }

    // IRenderBackend interface
    bool Initialize(void* windowHandle, u32 width, u32 height) override;
    void Shutdown() override;
    void Resize(u32 width, u32 height) override;

    void BeginFrame() override;
    void EndFrame() override;
    void Present() override;

    void RenderDrawList(const DrawList& drawList, u32 viewportW, u32 viewportH) override;

    TextureHandle CreateTexture(const TextureDesc& desc) override;
    void DestroyTexture(TextureHandle handle) override;

    void SetClearColor(Color c) override { _clearColor = c; }

    std::string GetBackendName() const override { return "DirectX 11"; }
    Vec2 GetViewportSize() const override { return {(f32)_width, (f32)_height}; }

    // DX11-specific
    ID3D11Device*        GetDevice()  const { return _device.Get(); }
    ID3D11DeviceContext* GetContext() const { return _context.Get(); }

    // Render target support (IRenderBackend interface)
    RenderTargetHandle CreateRenderTarget(u32 width, u32 height) override;
    void DestroyRenderTarget(RenderTargetHandle handle) override;

    // DX11-specific render target API
    RenderTargetHandle CreateRenderTarget(const RenderTargetDesc& desc);
    void BindRenderTarget(RenderTargetHandle handle);
    void UnbindRenderTarget();
    TextureHandle GetRenderTargetTexture(RenderTargetHandle handle);

private:
    bool CreateDeviceAndSwapChain(HWND hwnd, u32 w, u32 h);
    bool CreateRenderTargetView();
    bool CreateShaders();
    bool CreateBuffers();
    bool CreateBlendState();
    bool CreateRasterizerState();
    bool CreateSamplerState();
    void UpdateConstantBuffer(u32 viewportW, u32 viewportH);

    // Device
    ComPtr<ID3D11Device>           _device;
    ComPtr<ID3D11DeviceContext>    _context;
    ComPtr<IDXGISwapChain>         _swapChain;
    ComPtr<ID3D11RenderTargetView> _rtv;

    // Pipeline
    ComPtr<ID3D11VertexShader>   _vertexShader;
    ComPtr<ID3D11PixelShader>    _pixelShader;
    ComPtr<ID3D11InputLayout>    _inputLayout;
    ComPtr<ID3D11Buffer>         _vertexBuffer;
    ComPtr<ID3D11Buffer>         _indexBuffer;
    ComPtr<ID3D11Buffer>         _constantBuffer;
    ComPtr<ID3D11BlendState>     _blendState;
    ComPtr<ID3D11RasterizerState> _rasterizerState;
    ComPtr<ID3D11SamplerState>   _samplerState;

    // Textures
    struct TextureData {
        ComPtr<ID3D11Texture2D>          texture;
        ComPtr<ID3D11ShaderResourceView> srv;
        u32 width, height;
    };
    std::unordered_map<TextureHandle, TextureData> _textures;
    TextureHandle _nextTextureHandle{1};

    // Render targets
    struct RenderTarget {
        ComPtr<ID3D11Texture2D>          texture;
        ComPtr<ID3D11RenderTargetView>   rtv;
        ComPtr<ID3D11ShaderResourceView> srv;
        u32 width, height;
        TextureHandle texHandle;
    };
    std::unordered_map<RenderTargetHandle, RenderTarget> _renderTargets;
    RenderTargetHandle _nextRTHandle{1};

    // White pixel texture (default when no texture set)
    TextureHandle _whitePixel{INVALID_TEXTURE};

    // Buffer sizes
    u32 _vbSize{0};
    u32 _ibSize{0};

    // Viewport
    u32   _width{0};
    u32   _height{0};
    Color _clearColor{30, 30, 46, 255};

    HWND _hwnd{nullptr};
};

} // namespace ace

#endif // _WIN32
