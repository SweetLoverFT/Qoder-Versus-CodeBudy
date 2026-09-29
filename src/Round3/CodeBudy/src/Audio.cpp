#include "Audio.h"

#include <windows.h>
#include <mmsystem.h>

namespace Audio
{
    void Play(const wchar_t* path)
    {
        // SND_ASYNC：异步播放；SND_NODEFAULT：文件不存在时不播放系统默认提示音
        PlaySoundW(path, nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
    }
}
