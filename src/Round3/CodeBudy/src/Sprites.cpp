#include "Sprites.h"
#include "Audio.h"

#include <cstdlib>
#include <ctime>
#include <algorithm>

// ---------------- 随机数工具 ----------------
static void EnsureRandomSeeded()
{
    static bool seeded = false;
    if (!seeded)
    {
        srand((unsigned)time(nullptr));
        seeded = true;
    }
}

static int RandomInt(int min, int max)
{
    EnsureRandomSeeded();
    return min + rand() % (max - min + 1);
}

// ---------------- BaseSprite ----------------
void BaseSprite::startExplosion()
{
    exploding = true;
    explosionFrame = 0;
    explosionTimer = 0.0f;
    Audio::Play(Settings::BOOM_MUSIC); // 爆炸音效
}

bool BaseSprite::updateExplosion(ID3D11ShaderResourceView* const boomTextures[], int boomCount, float dt)
{
    if (!exploding) return false;

    const float frameTime = 0.05f; // 每帧 0.05 秒（对应 Python time.sleep(0.05)）
    explosionTimer += dt;
    while (explosionTimer >= frameTime)
    {
        explosionTimer -= frameTime;
        explosionFrame++;
        if (explosionFrame >= boomCount)
        {
            exploding = false;
            texture = nullptr;
            return true; // 爆炸结束
        }
    }

    if (explosionFrame < boomCount)
    {
        setTexture(boomTextures[explosionFrame], Settings::BOOM_SIZE, Settings::BOOM_SIZE);
    }
    return false;
}

// ---------------- TankSprite ----------------
void TankSprite::shot()
{
    // 移除飞出屏幕的子弹
    removeOutOfScreenBullets();
    if (!is_alive) return;
    if (bullets.size() >= 3) return;
    if (type == Settings::HERO)
    {
        Audio::Play(Settings::FIRE_MUSIC); // 我方射击音效
    }

    auto bullet = std::make_unique<Bullet>();
    bullet->setTexture(bulletTexture, Settings::BULLET_SIZE, Settings::BULLET_SIZE);
    bullet->direction = direction;

    // 按方向在坦克边缘生成子弹
    if (direction == Settings::LEFT)
    {
        bullet->rect.setRight(rect.left());
        bullet->rect.setCentery(rect.centery());
    }
    else if (direction == Settings::RIGHT)
    {
        bullet->rect.setLeft(rect.right());
        bullet->rect.setCentery(rect.centery());
    }
    else if (direction == Settings::UP)
    {
        bullet->rect.setBottom(rect.top());
        bullet->rect.setCenterx(rect.centerx());
    }
    else if (direction == Settings::DOWN)
    {
        bullet->rect.setTop(rect.bottom());
        bullet->rect.setCenterx(rect.centerx());
    }

    bullets.push_back(std::move(bullet));
}

void TankSprite::moveOutWall(const Rect& wallRect)
{
    if (direction == Settings::LEFT) rect.setLeft(wallRect.right() + 2.0f);
    else if (direction == Settings::RIGHT) rect.setRight(wallRect.left() - 2.0f);
    else if (direction == Settings::UP) rect.setTop(wallRect.bottom() + 2.0f);
    else if (direction == Settings::DOWN) rect.setBottom(wallRect.top() - 2.0f);
}

void TankSprite::removeOutOfScreenBullets()
{
    bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(),
            [](const std::unique_ptr<Bullet>& b)
            {
                return b->rect.bottom() <= 0 ||
                       b->rect.top() >= Settings::SCREEN_HEIGHT ||
                       b->rect.right() <= 0 ||
                       b->rect.left() >= Settings::SCREEN_WIDTH;
            }),
        bullets.end());
}

void TankSprite::removeDeadBullets()
{
    bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(),
            [](const std::unique_ptr<Bullet>& b) { return !b->is_alive; }),
        bullets.end());
}

// ---------------- Hero ----------------
Hero::Hero()
{
    type = Settings::HERO;
    speed = Settings::HERO_SPEED;
    direction = Settings::UP;
    is_hit_wall = false;

    rect.w = Settings::TANK_SIZE;
    rect.h = Settings::TANK_SIZE;
    // 初始位置：屏幕水平中心偏左 2 格，底部贴屏幕下缘
    rect.setCenterx(Settings::SCREEN_WIDTH * 0.5f - Settings::BOX_SIZE * 2);
    rect.setBottom(Settings::SCREEN_HEIGHT);
}

void Hero::turn()
{
    setTexture(textures[direction], Settings::TANK_SIZE, Settings::TANK_SIZE);
}

void Hero::hitWall()
{
    if ((direction == Settings::LEFT && rect.left() <= 0) ||
        (direction == Settings::RIGHT && rect.right() >= Settings::SCREEN_WIDTH) ||
        (direction == Settings::UP && rect.top() <= 0) ||
        (direction == Settings::DOWN && rect.bottom() >= Settings::SCREEN_HEIGHT))
    {
        is_hit_wall = true;
    }
}

void Hero::update()
{
    if (!is_hit_wall)
    {
        TankSprite::update();
        turn();
    }
}

// ---------------- Enemy ----------------
Enemy::Enemy()
{
    is_hit_wall = false;
    type = Settings::ENEMY;
    speed = Settings::ENEMY_SPEED;
    direction = RandomInt(0, 3);
    terminal = (float)RandomInt(40 * 2, 40 * 8);

    rect.w = Settings::TANK_SIZE;
    rect.h = Settings::TANK_SIZE;
}

void Enemy::turn()
{
    setTexture(textures[direction], Settings::TANK_SIZE, Settings::TANK_SIZE);
}

void Enemy::randomTurn()
{
    is_hit_wall = false;

    // 从除当前方向外的 3 个方向中随机选一个
    int dirs[3];
    int n = 0;
    for (int d = 0; d < 4; ++d)
    {
        if (d != direction) dirs[n++] = d;
    }
    direction = dirs[RandomInt(0, 2)];
    terminal = (float)RandomInt(40 * 2, 40 * 8);
    turn();
}

void Enemy::randomShot()
{
    // 1/60 概率射击
    if (RandomInt(0, 59) == 0)
    {
        shot();
    }
}

void Enemy::hitWallTurn()
{
    bool turnFlag = false;
    if (direction == Settings::LEFT && rect.left() <= 0)
    {
        turnFlag = true;
        rect.setLeft(2.0f);
    }
    else if (direction == Settings::RIGHT && rect.right() >= Settings::SCREEN_WIDTH - 1)
    {
        turnFlag = true;
        rect.setRight(Settings::SCREEN_WIDTH - 2.0f);
    }
    else if (direction == Settings::UP && rect.top() <= 0)
    {
        turnFlag = true;
        rect.setTop(2.0f);
    }
    else if (direction == Settings::DOWN && rect.bottom() >= Settings::SCREEN_HEIGHT - 1)
    {
        turnFlag = true;
        rect.setBottom(Settings::SCREEN_HEIGHT - 2.0f);
    }
    if (turnFlag) randomTurn();
}

void Enemy::update()
{
    randomShot();
    if (terminal <= 0)
    {
        randomTurn();
    }
    else
    {
        TankSprite::update();
        terminal -= speed;
    }
}
