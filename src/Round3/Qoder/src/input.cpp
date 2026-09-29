#include "input.h"

namespace {
std::vector<KeyEvent> g_events;
}

void InputQueue::Push(const KeyEvent& e) { g_events.push_back(e); }

void InputQueue::Drain(std::vector<KeyEvent>& out) {
    out.swap(g_events);
    g_events.clear();
}
