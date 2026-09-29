#pragma once

// 矩形结构（像素坐标，左上角为原点，与 pygame Rect 对应）
struct Rect
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    Rect() = default;
    Rect(float x_, float y_, float w_, float h_) : x(x_), y(y_), w(w_), h(h_) {}

    float left() const { return x; }
    float right() const { return x + w; }
    float top() const { return y; }
    float bottom() const { return y + h; }
    float centerx() const { return x + w * 0.5f; }
    float centery() const { return y + h * 0.5f; }

    void setLeft(float v) { x = v; }
    void setRight(float v) { x = v - w; }
    void setTop(float v) { y = v; }
    void setBottom(float v) { y = v - h; }
    void setCenterx(float v) { x = v - w * 0.5f; }
    void setCentery(float v) { y = v - h * 0.5f; }
};

// 矩形相交检测（与 pygame collide_rect 一致：严格比较，恰好相邻不算碰撞）
inline bool Intersects(const Rect& a, const Rect& b)
{
    return a.right() > b.x && a.x < b.right() &&
           a.bottom() > b.y && a.y < b.bottom();
}
