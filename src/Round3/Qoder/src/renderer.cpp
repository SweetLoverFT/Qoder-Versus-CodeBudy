#include "renderer.h"

#include <wincodec.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <cstring>
#include <vector>

#include "input.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace {

constexpr const wchar_t* kWindowClass = L"TankWarCppWnd";

struct Vertex {
    float x, y;    // 屏幕像素坐标
    float u, v;    // 贴图 UV
    float r, g, b, a;  // 顶点颜色
};

constexpr D3D11_INPUT_ELEMENT_DESC kLayout[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
};
constexpr UINT kVertexStride = sizeof(Vertex);

const char* kVsSource = R"(
cbuffer CBuffer : register(b0) { float4x4 transform; };
struct VSIn { float2 pos : POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };
struct PSIn { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };
PSIn VS(VSIn i) {
    PSIn o;
    o.pos = mul(transform, float4(i.pos, 0.0, 1.0));
    o.uv = i.uv;
    o.color = i.color;
    return o;
}
)";

const char* kPsSource = R"(
Texture2D tex : register(t0);
SamplerState samp : register(s0);
struct PSIn { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };
float4 PS(PSIn i) : SV_TARGET { return tex.Sample(samp, i.uv) * i.color; }
)";

std::wstring Utf8ToWide(const char* s) {
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    std::wstring w(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, w.data(), len);
    return w;
}

void ShowError(const wchar_t* msg) {
    MessageBoxW(nullptr, msg, L"TankWar", MB_OK | MB_ICONERROR);
}

}  // namespace

LRESULT CALLBACK Renderer::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_KEYDOWN:
            // 过滤自动重复（bit30），等价 pygame 的 KEYDOWN 无 repeat 语义
            if (!(lParam & (1u << 30))) InputQueue::Push({true, (unsigned)wParam});
            return 0;
        case WM_KEYUP:
            InputQueue::Push({false, (unsigned)wParam});
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

bool Renderer::Init(HINSTANCE hInstance, int width, int height, const wchar_t* title) {
    width_ = width;
    height_ = height;

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kWindowClass;
    if (!RegisterClassExW(&wc)) {
        ShowError(L"注册窗口类失败");
        return false;
    }

    // 与 pygame 一致：窗口不可拉伸，客户区精确为 950x650
    RECT rc{0, 0, width, height};
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
    hwnd_ = CreateWindowExW(0, kWindowClass, title,
                            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            rc.right - rc.left, rc.bottom - rc.top,
                            nullptr, nullptr, hInstance, nullptr);
    if (!hwnd_) {
        ShowError(L"创建窗口失败");
        return false;
    }

    if (!CreateGpuResources(width, height)) return false;

    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&wicFactory_)))) {
        ShowError(L"创建 WIC 工厂失败");
        return false;
    }

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    return true;
}

bool Renderer::CreateGpuResources(int width, int height) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferDesc.Width = (UINT)width;
    sd.BufferDesc.Height = (UINT)height;
    sd.BufferDesc.RefreshRate = {0, 1};
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 1;
    sd.OutputWindow = hwnd_;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
                                  D3D_FEATURE_LEVEL_10_0};
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        levels, 3, D3D11_SDK_VERSION, &sd, &swapChain_, &device_, nullptr, &context_);
    if (FAILED(hr)) {
        // 硬件设备不可用时退回 WARP 软渲染
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            levels, 3, D3D11_SDK_VERSION, &sd, &swapChain_, &device_, nullptr, &context_);
    }
    if (FAILED(hr)) {
        ShowError(L"创建 D3D11 设备失败");
        return false;
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    if (FAILED(swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) {
        ShowError(L"获取后台缓冲失败");
        return false;
    }
    backBuffer_ = backBuffer;
    if (FAILED(device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &rtv_))) {
        ShowError(L"创建渲染目标失败");
        return false;
    }

    // 着色器：运行时编译（HLSL 源码内嵌）
    ComPtr<ID3DBlob> vsBlob, errBlob;
    hr = D3DCompile(kVsSource, strlen(kVsSource), nullptr, nullptr, nullptr, "VS", "vs_4_0",
                    D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vsBlob, &errBlob);
    if (FAILED(hr)) {
        ShowError(L"编译顶点着色器失败");
        return false;
    }
    device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr,
                                &vs_);

    ComPtr<ID3DBlob> psBlob;
    hr = D3DCompile(kPsSource, strlen(kPsSource), nullptr, nullptr, nullptr, "PS", "ps_4_0",
                    D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &psBlob, &errBlob);
    if (FAILED(hr)) {
        ShowError(L"编译像素着色器失败");
        return false;
    }
    device_->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr,
                               &ps_);

    device_->CreateInputLayout(kLayout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                               &inputLayout_);

    // 顶点/索引/常量缓冲
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = kVertexStride * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device_->CreateBuffer(&bd, nullptr, &vertexBuffer_);

    const unsigned short indices[] = {0, 1, 2, 2, 1, 3};
    D3D11_BUFFER_DESC ibd{};
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(indices);
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA isd{};
    isd.pSysMem = indices;
    device_->CreateBuffer(&ibd, &isd, &indexBuffer_);

    D3D11_BUFFER_DESC cbd{};
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.ByteWidth = sizeof(XMMATRIX);  // 16 个 float，与 HLSL float4x4 对齐
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    device_->CreateBuffer(&cbd, nullptr, &constantBuffer_);

    // 混合：premultiplied alpha（WIC 输出 32bppPBGRA）
    D3D11_BLEND_DESC blend{};
    blend.RenderTarget[0].BlendEnable = TRUE;
    blend.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device_->CreateBlendState(&blend, &blendState_);

    D3D11_RASTERIZER_DESC raster{};
    raster.FillMode = D3D11_FILL_SOLID;
    raster.CullMode = D3D11_CULL_NONE;
    raster.DepthClipEnable = TRUE;
    device_->CreateRasterizerState(&raster, &rasterState_);

    D3D11_SAMPLER_DESC samp{};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    device_->CreateSamplerState(&samp, &sampler_);

    return true;
}

void Renderer::Shutdown() {
    if (context_) context_->ClearState();
    if (hwnd_) DestroyWindow(hwnd_);
    hwnd_ = nullptr;
    CoUninitialize();
}

void Renderer::PumpMessages() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) quit_ = true;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void Renderer::BeginFrame() {
    context_->OMSetRenderTargets(1, rtv_.GetAddressOf(), nullptr);

    // 屏幕颜色：黑色（对应 Settings.SCREEN_COLOR）
    const float clear[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    context_->ClearRenderTargetView(rtv_.Get(), clear);

    D3D11_VIEWPORT vp{0.0f, 0.0f, (float)width_, (float)height_, 0.0f, 1.0f};
    context_->RSSetViewports(1, &vp);

    context_->OMSetBlendState(blendState_.Get(), nullptr, 0xFFFFFFFF);
    context_->RSSetState(rasterState_.Get());
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->IASetInputLayout(inputLayout_.Get());

    UINT stride = kVertexStride;
    UINT offset = 0;
    context_->IASetVertexBuffers(0, 1, vertexBuffer_.GetAddressOf(), &stride, &offset);
    context_->IASetIndexBuffer(indexBuffer_.Get(), DXGI_FORMAT_R16_UINT, 0);

    context_->VSSetShader(vs_.Get(), nullptr, 0);
    context_->PSSetShader(ps_.Get(), nullptr, 0);
    context_->VSSetConstantBuffers(0, 1, constantBuffer_.GetAddressOf());
    context_->PSSetSamplers(0, 1, sampler_.GetAddressOf());

    // 正交投影：左上为原点、Y 向下，与 pygame 屏幕坐标系一致。
    // 列主序写入 cbuffer，供 HLSL mul(transform, pos) 使用，
    // 数值等价 XMMatrixOrthographicOffCenterLH(0, w, h, 0, 0, 1) 的转置。
    const float ortho[16] = {
        2.0f / (float)width_, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / (float)height_, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    context_->UpdateSubresource(constantBuffer_.Get(), 0, nullptr, ortho, 0, 0);
}

void Renderer::DrawQuad(const Texture* tex, int x, int y, int w, int h) {
    if (!tex || !tex->srv) return;

    Vertex verts[4] = {
        {(float)x, (float)y, 0.0f, 0.0f, 1, 1, 1, 1},
        {(float)(x + w), (float)y, 1.0f, 0.0f, 1, 1, 1, 1},
        {(float)x, (float)(y + h), 0.0f, 1.0f, 1, 1, 1, 1},
        {(float)(x + w), (float)(y + h), 1.0f, 1.0f, 1, 1, 1, 1},
    };

    D3D11_MAPPED_SUBRESOURCE ms{};
    if (FAILED(context_->Map(vertexBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) return;
    memcpy(ms.pData, verts, sizeof(verts));
    context_->Unmap(vertexBuffer_.Get(), 0);

    context_->PSSetShaderResources(0, 1, tex->srv.GetAddressOf());
    context_->DrawIndexed(6, 0, 0);
}

void Renderer::DrawSprite(const Texture* tex, int x, int y) {
    if (!tex) return;
    DrawQuad(tex, x, y, tex->width, tex->height);
}

void Renderer::EndFrame() { swapChain_->Present(0, 0); }

Texture* Renderer::LoadTexture(const char* utf8Path) {
    auto it = textures_.find(utf8Path);
    if (it != textures_.end()) return &it->second;

    std::wstring wpath = Utf8ToWide(utf8Path);

    // WIC 解码：只取第 0 帧（等价 pygame.image.load 对 GIF 只取首帧的行为）
    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(wicFactory_->CreateDecoderFromFilename(wpath.c_str(), nullptr, GENERIC_READ,
                                                      WICDecodeMetadataCacheOnLoad, &decoder)))
        return nullptr;

    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame))) return nullptr;

    // 统一转 premultiplied BGRA，GIF 透明索引与 PNG alpha 走同一路径
    ComPtr<IWICBitmapSource> conv;
    if (FAILED(WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, frame.Get(), &conv)))
        return nullptr;

    UINT w = 0, h = 0;
    conv->GetSize(&w, &h);
    std::vector<BYTE> pixels((size_t)w * h * 4);
    if (FAILED(conv->CopyPixels(nullptr, w * 4, (UINT)pixels.size(), pixels.data())))
        return nullptr;

    D3D11_TEXTURE2D_DESC td{};
    td.Width = w;
    td.Height = h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = pixels.data();
    sd.SysMemPitch = w * 4;

    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(device_->CreateTexture2D(&td, &sd, &tex))) return nullptr;

    Texture texture;
    if (FAILED(device_->CreateShaderResourceView(tex.Get(), nullptr, &texture.srv)))
        return nullptr;
    texture.width = (int)w;
    texture.height = (int)h;

    return &textures_.emplace(utf8Path, std::move(texture)).first->second;
}

bool Renderer::WritePng(const wchar_t* path, int w, int h, int stride, const BYTE* pixels) {
    ComPtr<IWICBitmap> bitmap;
    if (FAILED(wicFactory_->CreateBitmapFromMemory(
            (UINT)w, (UINT)h, GUID_WICPixelFormat32bppBGRA, (UINT)stride,
            (UINT)(size_t)stride * (UINT)h, const_cast<BYTE*>(pixels), &bitmap)))
        return false;

    ComPtr<IWICStream> stream;
    if (FAILED(wicFactory_->CreateStream(&stream))) return false;
    if (FAILED(stream->InitializeFromFilename(path, GENERIC_WRITE))) return false;

    ComPtr<IWICBitmapEncoder> encoder;
    if (FAILED(wicFactory_->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)))
        return false;
    if (FAILED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache))) return false;

    ComPtr<IWICBitmapFrameEncode> frameEncode;
    if (FAILED(encoder->CreateNewFrame(&frameEncode, nullptr))) return false;
    if (FAILED(frameEncode->Initialize(nullptr))) return false;
    if (FAILED(frameEncode->SetSize((UINT)w, (UINT)h))) return false;
    if (FAILED(frameEncode->WriteSource(bitmap.Get(), nullptr))) return false;
    if (FAILED(frameEncode->Commit())) return false;
    return SUCCEEDED(encoder->Commit());
}

void Renderer::SaveScreenshot(const wchar_t* path) {
    if (!backBuffer_ || !device_ || !context_) return;

    // 把后备缓冲拷贝到可读的 staging 纹理
    D3D11_TEXTURE2D_DESC td{};
    td.Width = (UINT)width_;
    td.Height = (UINT)height_;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_STAGING;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(device_->CreateTexture2D(&td, nullptr, &staging))) return;
    context_->CopyResource(staging.Get(), backBuffer_.Get());

    D3D11_MAPPED_SUBRESOURCE ms{};
    if (FAILED(context_->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &ms))) return;
    std::vector<BYTE> pixels((size_t)width_ * height_ * 4);
    for (int row = 0; row < height_; ++row) {
        memcpy(pixels.data() + (size_t)row * width_ * 4,
               (const BYTE*)ms.pData + (size_t)row * ms.RowPitch, (size_t)width_ * 4);
    }
    context_->Unmap(staging.Get(), 0);

    WritePng(path, width_, height_, width_ * 4, pixels.data());
}
