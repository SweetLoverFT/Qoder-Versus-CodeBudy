#include "Renderer.h"

#include <d3dcompiler.h>
#include <cstdint>
#include <cstring>

// ---------------- 内嵌 HLSL 着色器 ----------------

// 顶点着色器：像素坐标 + 正交缩放偏移，转换为裁剪空间
static const char* g_vsSource = R"(
cbuffer PerFrame : register(b0)
{
    float2 gScale;
    float2 gOffset;
};

struct VSInput
{
    float2 position : POSITION;
    float2 texcoord : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

PSInput main(VSInput input)
{
    PSInput output;
    output.position = float4(input.position * gScale + gOffset, 0.0f, 1.0f);
    output.texcoord = input.texcoord;
    return output;
}
)";

// 像素着色器：采样纹理
static const char* g_psSource = R"(
Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    return gTexture.Sample(gSampler, input.texcoord);
}
)";

static void ShowError(const wchar_t* msg)
{
    MessageBoxW(nullptr, msg, L"坦克大战", MB_ICONERROR | MB_OK);
}

Renderer::~Renderer()
{
    Shutdown();
}

void Renderer::Shutdown()
{
    if (m_whiteTexture) { m_whiteTexture->Release(); m_whiteTexture = nullptr; }
    if (m_ib) { m_ib->Release(); m_ib = nullptr; }
    if (m_vb) { m_vb->Release(); m_vb = nullptr; }
    if (m_cbPerFrame) { m_cbPerFrame->Release(); m_cbPerFrame = nullptr; }
    if (m_sampler) { m_sampler->Release(); m_sampler = nullptr; }
    if (m_rasterState) { m_rasterState->Release(); m_rasterState = nullptr; }
    if (m_blendState) { m_blendState->Release(); m_blendState = nullptr; }
    if (m_inputLayout) { m_inputLayout->Release(); m_inputLayout = nullptr; }
    if (m_ps) { m_ps->Release(); m_ps = nullptr; }
    if (m_vs) { m_vs->Release(); m_vs = nullptr; }
    if (m_rtv) { m_rtv->Release(); m_rtv = nullptr; }
    if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
    if (m_context) { m_context->Release(); m_context = nullptr; }
    if (m_device) { m_device->Release(); m_device = nullptr; }
}

bool Renderer::Init(HWND hwnd, int width, int height)
{
    m_hwnd = hwnd;
    m_width = width;
    m_height = height;

    if (!CreateDeviceAndSwapChain(hwnd, width, height)) return false;
    if (!CreateRenderTarget()) return false;
    if (!CreateShadersAndState()) return false;
    if (!CreateSpriteBuffers()) return false;

    m_quads.reserve(kMaxSprites);
    return true;
}

bool Renderer::CreateDeviceAndSwapChain(HWND hwnd, int width, int height)
{
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = 1;
    scd.OutputWindow = hwnd;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    scd.Flags = 0;

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL chosenLevel = D3D_FEATURE_LEVEL_11_0;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        featureLevels,
        1,
        D3D11_SDK_VERSION,
        &scd,
        &m_swapChain,
        &m_device,
        &chosenLevel,
        &m_context);

    // 硬件设备失败时回退到 WARP（软件光栅化），保证无独显环境也能运行
    if (FAILED(hr))
    {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            featureLevels,
            1,
            D3D11_SDK_VERSION,
            &scd,
            &m_swapChain,
            &m_device,
            &chosenLevel,
            &m_context);
    }

    if (FAILED(hr))
    {
        ShowError(L"初始化 Direct3D 11 设备失败（硬件与 WARP 均不可用）。");
        return false;
    }
    return true;
}

bool Renderer::CreateRenderTarget()
{
    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                        reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr))
    {
        ShowError(L"获取交换链后台缓冲区失败。");
        return false;
    }

    hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_rtv);
    backBuffer->Release();
    if (FAILED(hr))
    {
        ShowError(L"创建渲染目标视图失败。");
        return false;
    }
    return true;
}

bool Renderer::CreateShadersAndState()
{
    // 编译顶点着色器
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    HRESULT hr = D3DCompile(g_vsSource, strlen(g_vsSource), "vs.hlsl", nullptr, nullptr,
                            "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) errorBlob->Release();
        ShowError(L"编译顶点着色器失败。");
        return false;
    }

    hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);
    if (FAILED(hr)) { vsBlob->Release(); ShowError(L"创建顶点着色器失败。"); return false; }

    // 编译像素着色器
    ID3DBlob* psBlob = nullptr;
    hr = D3DCompile(g_psSource, strlen(g_psSource), "ps.hlsl", nullptr, nullptr,
                    "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) errorBlob->Release();
        vsBlob->Release();
        ShowError(L"编译像素着色器失败。");
        return false;
    }

    hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps);
    if (FAILED(hr)) { psBlob->Release(); vsBlob->Release(); ShowError(L"创建像素着色器失败。"); return false; }

    // 输入布局
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = m_device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
    vsBlob->Release();
    psBlob->Release();
    if (FAILED(hr)) { ShowError(L"创建输入布局失败。"); return false; }

    // 常量缓冲区：float2 scale + float2 offset（共 16 字节）
    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth = 16;
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = m_device->CreateBuffer(&cbd, nullptr, &m_cbPerFrame);
    if (FAILED(hr)) { ShowError(L"创建常量缓冲区失败。"); return false; }

    // 混合状态：标准 alpha 混合
    D3D11_BLEND_DESC bd = {};
    bd.AlphaToCoverageEnable = FALSE;
    bd.IndependentBlendEnable = FALSE;
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = m_device->CreateBlendState(&bd, &m_blendState);
    if (FAILED(hr)) { ShowError(L"创建混合状态失败。"); return false; }

    // 光栅化状态：不剔除背面
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.FrontCounterClockwise = FALSE;
    rd.DepthClipEnable = TRUE;
    rd.ScissorEnable = FALSE;
    hr = m_device->CreateRasterizerState(&rd, &m_rasterState);
    if (FAILED(hr)) { ShowError(L"创建光栅化状态失败。"); return false; }

    // 采样器：点采样（像素风，避免边缘插值产生杂色）
    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    hr = m_device->CreateSamplerState(&sd, &m_sampler);
    if (FAILED(hr)) { ShowError(L"创建采样器状态失败。"); return false; }

    return true;
}

bool Renderer::CreateSpriteBuffers()
{
    // 动态顶点缓冲区：每个精灵 4 个顶点
    D3D11_BUFFER_DESC vbd = {};
    vbd.ByteWidth = sizeof(SpriteVertex) * 4 * (UINT)kMaxSprites;
    vbd.Usage = D3D11_USAGE_DYNAMIC;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = m_device->CreateBuffer(&vbd, nullptr, &m_vb);
    if (FAILED(hr)) { ShowError(L"创建顶点缓冲区失败。"); return false; }

    // 静态索引缓冲区：每个精灵 6 个索引
    std::vector<uint32_t> indices;
    indices.reserve(kMaxSprites * 6);
    for (uint32_t i = 0; i < (uint32_t)kMaxSprites; ++i)
    {
        uint32_t base = i * 4;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    D3D11_BUFFER_DESC ibd = {};
    ibd.ByteWidth = (UINT)(indices.size() * sizeof(uint32_t));
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = indices.data();
    init.SysMemPitch = 0;
    init.SysMemSlicePitch = 0;

    hr = m_device->CreateBuffer(&ibd, &init, &m_ib);
    if (FAILED(hr)) { ShowError(L"创建索引缓冲区失败。"); return false; }

    return true;
}

void Renderer::Resize(int width, int height)
{
    if (width <= 0 || height <= 0) return;
    m_width = width;
    m_height = height;

    m_context->OMSetRenderTargets(0, nullptr, nullptr);
    if (m_rtv) { m_rtv->Release(); m_rtv = nullptr; }

    m_swapChain->ResizeBuffers(0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, 0);
    CreateRenderTarget();
}

void Renderer::BeginFrame(float r, float g, float b, float a)
{
    m_quads.clear();

    m_context->OMSetRenderTargets(1, &m_rtv, nullptr);
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = (float)m_width;
    vp.Height = (float)m_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);

    float clearColor[4] = { r, g, b, a };
    m_context->ClearRenderTargetView(m_rtv, clearColor);
}

void Renderer::DrawSprite(ID3D11ShaderResourceView* texture, float x, float y, float w, float h)
{
    if (!texture || m_quads.size() >= kMaxSprites) return;

    SpriteQuad quad;
    quad.texture = texture;
    float x0 = x, y0 = y, x1 = x + w, y1 = y + h;
    quad.v[0] = { x0, y0, 0.0f, 0.0f };
    quad.v[1] = { x1, y0, 1.0f, 0.0f };
    quad.v[2] = { x1, y1, 1.0f, 1.0f };
    quad.v[3] = { x0, y1, 0.0f, 1.0f };
    m_quads.push_back(quad);
}

void Renderer::EndFrame()
{
    FlushSprites();
    m_swapChain->Present(1, 0);
}

void Renderer::FlushSprites()
{
    if (m_quads.empty()) return;

    // 上传顶点
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(m_context->Map(m_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        SpriteVertex* dst = static_cast<SpriteVertex*>(mapped.pData);
        for (size_t i = 0; i < m_quads.size(); ++i)
        {
            std::memcpy(dst + i * 4, m_quads[i].v, sizeof(SpriteVertex) * 4);
        }
        m_context->Unmap(m_vb, 0);
    }

    // 管线状态
    UINT stride = sizeof(SpriteVertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_vb, &stride, &offset);
    m_context->IASetIndexBuffer(m_ib, DXGI_FORMAT_R32_UINT, 0);
    m_context->IASetInputLayout(m_inputLayout);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->VSSetShader(m_vs, nullptr, 0);
    m_context->PSSetShader(m_ps, nullptr, 0);
    m_context->PSSetSamplers(0, 1, &m_sampler);
    m_context->OMSetBlendState(m_blendState, nullptr, 0xffffffff);
    m_context->RSSetState(m_rasterState);

    // 正交投影常量：scale=(2/w, -2/h)，offset=(-1, 1)，使左上角为原点
    float cbData[4] = { 2.0f / (float)m_width, -2.0f / (float)m_height, -1.0f, 1.0f };
    if (SUCCEEDED(m_context->Map(m_cbPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        std::memcpy(mapped.pData, cbData, sizeof(cbData));
        m_context->Unmap(m_cbPerFrame, 0);
    }
    m_context->VSSetConstantBuffers(0, 1, &m_cbPerFrame);

    // 按提交顺序绘制，保持叠放次序（如草盖在坦克之上）
    for (size_t i = 0; i < m_quads.size(); ++i)
    {
        ID3D11ShaderResourceView* srv = m_quads[i].texture;
        m_context->PSSetShaderResources(0, 1, &srv);
        m_context->DrawIndexed(6, 0, (UINT)(i * 4));
    }
}

ID3D11ShaderResourceView* Renderer::GetWhiteTexture()
{
    if (!m_whiteTexture) CreateWhiteTexture();
    return m_whiteTexture;
}

bool Renderer::CreateWhiteTexture()
{
    uint32_t white = 0xFFFFFFFF; // BGRA 白色

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = 1;
    td.Height = 1;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = &white;
    init.SysMemPitch = sizeof(uint32_t);

    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = m_device->CreateTexture2D(&td, &init, &tex);
    if (FAILED(hr)) return false;

    hr = m_device->CreateShaderResourceView(tex, nullptr, &m_whiteTexture);
    tex->Release();
    return SUCCEEDED(hr);
}
