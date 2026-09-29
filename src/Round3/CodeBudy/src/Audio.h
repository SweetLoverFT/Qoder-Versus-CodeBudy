#pragma once

// 音效播放封装（对应 pygame.mixer.music 播放 WAV 的行为）
namespace Audio
{
    // 异步播放 WAV 音效（非阻塞）
    void Play(const wchar_t* path);
}
