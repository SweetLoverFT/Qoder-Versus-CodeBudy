#pragma once

// 音频播放（winmm PlaySound，等价 pygame.mixer.music 的单通道替换语义）

namespace Audio {
void Init();     // 构建资源宽字符路径
void PlayFire(); // 我方坦克开火音效
void PlayBoom(); // 爆炸音效
}  // namespace Audio
