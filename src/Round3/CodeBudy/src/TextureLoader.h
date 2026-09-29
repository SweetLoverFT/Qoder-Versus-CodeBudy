#pragma once

#include <d3d11.h>

// 纹理：着色器资源视图 + 原始尺寸
struct Texture
{
    ID3D11ShaderResourceView* srv = nullptr;
    int width = 0;
    int height = 0;

    void Reset()
    {
        if (srv) srv->Release();
        srv = nullptr;
        width = 0;
        height = 0;
    }
};

// 用 WIC 解码图片（PNG / GIF / JPEG 等），GIF 取首帧
// 成功返回 true，并填充 out；失败返回 false
bool LoadTextureFromFile(ID3D11Device* device, const wchar_t* path, Texture& out);
