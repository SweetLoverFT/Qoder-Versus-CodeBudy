#include "TextureLoader.h"

#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <combaseapi.h>
#include <cstdint>
#include <vector>

using Microsoft::WRL::ComPtr;

bool LoadTextureFromFile(ID3D11Device* device, const wchar_t* path, Texture& out)
{
    if (!device || !path) return false;

    // 创建 WIC 工厂
    ComPtr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&factory));
    if (FAILED(hr)) return false;

    // 从文件创建解码器（GIF 取首帧）
    ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ,
                                            WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) return false;

    UINT width = 0, height = 0;
    frame->GetSize(&width, &height);

    // 转换为 32 位 BGRA
    ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr)) return false;

    hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA,
                               WICBitmapDitherTypeNone, nullptr, 0.0f,
                               WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return false;

    const UINT rowPitch = width * 4;
    std::vector<uint8_t> pixels((size_t)rowPitch * height);
    hr = converter->CopyPixels(nullptr, rowPitch, (UINT)pixels.size(), pixels.data());
    if (FAILED(hr)) return false;

    // 创建 D3D11 纹理
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = width;
    td.Height = height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = pixels.data();
    init.SysMemPitch = rowPitch;

    ComPtr<ID3D11Texture2D> tex;
    hr = device->CreateTexture2D(&td, &init, &tex);
    if (FAILED(hr)) return false;

    hr = device->CreateShaderResourceView(tex.Get(), nullptr, &out.srv);
    if (FAILED(hr)) return false;

    out.width = (int)width;
    out.height = (int)height;
    return true;
}
