#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <vector>

// 精灵顶点：像素坐标 + 纹理坐标
struct SpriteVertex
{
    float x, y;
    float u, v;
};

// DirectX 11 渲染器：负责设备/交换链初始化、着色器编译、精灵批量绘制
class Renderer
{
public:
    Renderer() = default;
    ~Renderer();

    bool Init(HWND hwnd, int width, int height);
    void Shutdown();

    void Resize(int width, int height);

    void BeginFrame(float r, float g, float b, float a);
    void DrawSprite(ID3D11ShaderResourceView* texture, float x, float y, float w, float h);
    void EndFrame();

    ID3D11Device* GetDevice() const { return m_device; }
    ID3D11DeviceContext* GetContext() const { return m_context; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    // 1x1 白色纹理，用于纯色方块 / 调试绘制
    ID3D11ShaderResourceView* GetWhiteTexture();

private:
    bool CreateDeviceAndSwapChain(HWND hwnd, int width, int height);
    bool CreateRenderTarget();
    bool CreateShadersAndState();
    bool CreateSpriteBuffers();
    bool CreateWhiteTexture();
    void FlushSprites();

    struct SpriteQuad
    {
        ID3D11ShaderResourceView* texture = nullptr;
        SpriteVertex v[4];
    };

    HWND m_hwnd = nullptr;
    int m_width = 0;
    int m_height = 0;

    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_rtv = nullptr;

    ID3D11VertexShader* m_vs = nullptr;
    ID3D11PixelShader* m_ps = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;
    ID3D11Buffer* m_cbPerFrame = nullptr;
    ID3D11Buffer* m_vb = nullptr;
    ID3D11Buffer* m_ib = nullptr;

    ID3D11BlendState* m_blendState = nullptr;
    ID3D11RasterizerState* m_rasterState = nullptr;
    ID3D11SamplerState* m_sampler = nullptr;

    ID3D11ShaderResourceView* m_whiteTexture = nullptr;

    static constexpr size_t kMaxSprites = 4096;
    std::vector<SpriteQuad> m_quads;
};
