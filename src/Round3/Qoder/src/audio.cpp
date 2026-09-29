#include "audio.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <string>

#include "settings.h"

namespace {
std::wstring g_fireMusic;
std::wstring g_boomMusic;

std::wstring Utf8ToWide(const char* s) {
    if (!s) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    std::wstring w(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, w.data(), len);
    return w;
}
}  // namespace

void Audio::Init() {
    g_fireMusic = Utf8ToWide(Settings::FIRE_MUSIC);
    g_boomMusic = Utf8ToWide(Settings::BOOM_MUSIC);
}

void Audio::PlayFire() {
    PlaySoundW(g_fireMusic.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
}

void Audio::PlayBoom() {
    PlaySoundW(g_boomMusic.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
}
