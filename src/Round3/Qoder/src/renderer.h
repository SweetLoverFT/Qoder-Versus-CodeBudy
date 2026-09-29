#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <string>
#include <unordered_map>

// 2D 贴图（对应 pygame 加载的 Surface 图像）
struct Texture {
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    int width = 0;
    int height = 0;
};

// 自研最小 D3D11 2D 渲染器：窗口/交换链/正交投影/逐精灵绘制/WIC 贴图加载
class Renderer {
public:
    bool Init(HINSTANCE hInstance, int width, int height, const wchar_t* title);
    void Shutdown();

    HWND GetHwnd() const { return hwnd_; }
    void PumpMessages();
    bool WantsQuit() const { return quit_; }

    void BeginFrame();                       // 清屏 + 设置管线状态
    void DrawSprite(const Texture* tex, int x, int y);
    void EndFrame();                         // Present

    Texture* LoadTexture(const char* utf8Path);  // 带缓存，失败返回 nullptr

    // 调试：把当前后备缓冲保存为 PNG（F12 触发，验证渲染内容用）
    void SaveScreenshot(const wchar_t* utf16Path);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool CreateGpuResources(int width, int height);
    bool WritePng(const wchar_t* path, int w, int h, int stride, const BYTE* pixels);
    void DrawQuad(const Texture* tex, int x, int y, int w, int h);  // 指定尺寸画矩形

    HWND hwnd_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    bool quit_ = false;

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer_;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vs_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> ps_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer_;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blendState_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterState_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_;
    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory_;

    std::unordered_map<std::string, Texture> textures_;
};
