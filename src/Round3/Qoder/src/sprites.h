#pragma once

#include <vector>

#include "settings.h"

struct Texture;

// 矩形（等价 pygame.Rect 的常用语义，坐标为整数、原点左上）
struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    int left() const { return x; }
    int right() const { return x + w; }
    int top() const { return y; }
    int bottom() const { return y + h; }
    int centerx() const { return x + w / 2; }
    int centery() const { return y + h / 2; }

    // 等价 pygame.sprite.collide_rect：两矩形相交（边界接触不算）
    bool Collides(const Rect& o) const {
        return left() < o.right() && right() > o.left() && top() < o.bottom() &&
               bottom() > o.top();
    }
};

// 全局随机数（等价 Python random.randint / random.choice 的分布语义）
int RandomInt(int lo, int hi);  // 闭区间 [lo, hi] 均匀分布

// 对应 sprites.py 的 BaseSprite
class Sprite {
public:
    virtual ~Sprite() = default;

    const Texture* image = nullptr;
    Rect rect;
    int direction = -1;  // Settings::LEFT/RIGHT/UP/DOWN
    int speed = 0;

    // 按当前方向移动 speed 像素
    virtual void Update();
};

// 对应 sprites.py 的 Bullet
class Bullet : public Sprite {
public:
    explicit Bullet(const Texture* img);
};

// 对应 sprites.py 的 Wall
class Wall : public Sprite {
public:
    int type = 0;   // Settings::RED_WALL/IRON_WALL/WEED_WALL/SEA_WALL/BOSS_WALL
    int life = 2;   // 剩余生命（红墙默认 2，中两发子弹摧毁；boss 墙创建时覆写为 1）
};

// 对应 sprites.py 的 TankSprite
class Tank : public Sprite {
public:
    int type = -1;              // Settings::HERO / Settings::ENEMY
    bool is_alive = true;
    bool is_moving = false;
    std::vector<Bullet> bullets;
    const Texture* bulletImage = nullptr;

    void Shot();                        // 发射子弹（先清理出屏子弹，最多 3 发）
    void MoveOutWall(const Wall& wall); // 被墙挡住时把坦克推出墙外
    void Update() override;             // 存活时按方向移动
};

// 对应 sprites.py 的 Hero
class Hero : public Tank {
public:
    bool is_hit_wall = false;
    const Texture* dirImages[4] = {};

    void HitWall();  // 仅检测屏幕四边越界
    void Update() override;
};

// 对应 sprites.py 的 Enemy
class Enemy : public Tank {
public:
    float terminal = 0.0f;
    const Texture* dirImages[4] = {};

    void RandomTurn();    // 从除当前方向外的 3 个方向随机选一个
    void RandomShot();    // 每帧 1/60 概率开火
    void HitWallTurn();   // 出屏时钳制回边内并随机转向
    void Update() override;
};

// 爆炸动画实体（替代 Python 版 Thread + sleep 的异步爆炸呈现）
struct Effect {
    Rect rect;
    const Texture* frames[8] = {};
    double startTime = 0.0;
    double frameDur = 0.0;  // 坦克 0.05s/帧，墙 0.07s/帧

    int FrameIndex(double now) const;
    bool Finished(double now) const { return now - startTime >= frameDur * 8.0; }
};
