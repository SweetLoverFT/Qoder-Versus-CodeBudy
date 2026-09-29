#pragma once

#include <vector>

// 键盘事件（等价 pygame 的 KEYDOWN/KEYUP 事件，无自动重复）

struct KeyEvent {
    bool down;   // true=按下 false=松开
    unsigned vk; // 虚拟键码
};

namespace InputQueue {
void Push(const KeyEvent& e);
void Drain(std::vector<KeyEvent>& out);
}  // namespace InputQueue
