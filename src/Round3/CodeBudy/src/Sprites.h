#pragma once

#include <d3d11.h>
#include <vector>
#include <memory>

#include "Rect.h"
#include "Settings.h"

// ==================== 基础精灵（对应 sprites.py: BaseSprite） ====================
// 游戏中所有可变物体的底层父类
class BaseSprite
{
public:
    Rect rect;                                        // 位置与碰撞尺寸
    int direction = Settings::UP;                     // 朝向
    float speed = 0.0f;                               // 移动速度
    ID3D11ShaderResourceView* texture = nullptr;      // 当前渲染纹理（对应 Python 的 image）
    float textureW = 0.0f;                            // 纹理原生宽（pygame blit 按原生尺寸绘制）
    float textureH = 0.0f;                            // 纹理原生高

    bool is_alive = true;                             // 存活状态

    // 爆炸动画状态（非阻塞，帧计时推进）
    bool exploding = false;
    int explosionFrame = 0;
    float explosionTimer = 0.0f;

    virtual ~BaseSprite() = default;

    void setTexture(ID3D11ShaderResourceView* srv, float w, float h)
    {
        texture = srv;
        textureW = w;
        textureH = h;
    }

    // 按方向移动
    virtual void update()
    {
        if (direction == Settings::LEFT) rect.x -= speed;
        else if (direction == Settings::RIGHT) rect.x += speed;
        else if (direction == Settings::UP) rect.y -= speed;
        else if (direction == Settings::DOWN) rect.y += speed;
    }

    // 默认死亡：仅标记（子弹等直接消失）
    virtual void kill()
    {
        is_alive = false;
    }

    // 开始爆炸（含爆炸音效）
    void startExplosion();

    // 推进爆炸动画，返回 true 表示爆炸结束
    bool updateExplosion(ID3D11ShaderResourceView* const boomTextures[], int boomCount, float dt);
};

// ==================== 子弹（对应 sprites.py: Bullet） ====================
class Bullet : public BaseSprite
{
public:
    Bullet()
    {
        speed = Settings::BULLET_SPEED;
        rect.w = Settings::BULLET_SIZE;
        rect.h = Settings::BULLET_SIZE;
    }
};

// ==================== 坦克基类（对应 sprites.py: TankSprite） ====================
class TankSprite : public BaseSprite
{
public:
    int type = Settings::ENEMY;                         // 坦克类型（我方/敌方）
    std::vector<std::unique_ptr<Bullet>> bullets;       // 该坦克发射的子弹
    bool is_moving = false;
    ID3D11ShaderResourceView* bulletTexture = nullptr;  // 子弹纹理（由游戏层统一加载后赋值）

    void shot();                                  // 发射子弹
    void moveOutWall(const Rect& wallRect);       // 碰撞后把坦克推出墙体
    void removeOutOfScreenBullets();              // 移除飞出屏幕的子弹
    void removeDeadBullets();                     // 移除已标记死亡的子弹

    // 坦克死亡：标记死亡并播放爆炸动画
    void kill() override
    {
        if (!is_alive) return;
        is_alive = false;
        startExplosion();
    }

    void update() override
    {
        if (!is_alive) return;
        BaseSprite::update();
    }
};

// ==================== 我方坦克（对应 sprites.py: Hero） ====================
class Hero : public TankSprite
{
public:
    bool is_hit_wall = false;
    ID3D11ShaderResourceView* textures[4] = { nullptr, nullptr, nullptr, nullptr };

    Hero();
    void turn();     // 按方向切换贴图
    void hitWall();  // 检测是否碰到屏幕边界
    void update() override;
};

// ==================== 敌方坦克（对应 sprites.py: Enemy） ====================
class Enemy : public TankSprite
{
public:
    bool is_hit_wall = false;
    float terminal = 0.0f;  // 距下一次转向的剩余"里程"
    ID3D11ShaderResourceView* textures[4] = { nullptr, nullptr, nullptr, nullptr };

    Enemy();
    void turn();        // 按方向切换贴图
    void randomTurn();  // 随机转向
    void randomShot();  // 随机射击
    void hitWallTurn(); // 碰屏幕边界掉头
    void update() override;
};

// ==================== 墙（对应 sprites.py: Wall） ====================
class Wall : public BaseSprite
{
public:
    int type = 0;   // 墙体类型
    int life = 2;   // 生命值（红砖墙需命中 2 次）

    void update() override { /* 墙静止不动 */ }

    // 被命中：life-1，life 归零时爆炸销毁
    void kill() override
    {
        if (!is_alive) return;
        life -= 1;
        if (life <= 0)
        {
            is_alive = false;
            startExplosion();
        }
    }
};
