#include "sprites.h"

#include <algorithm>
#include <random>

#include "audio.h"
#include "renderer.h"  // Texture 完整定义（Bullet 构造需要取贴图尺寸）

namespace {
std::mt19937& Rng() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}
}  // namespace

int RandomInt(int lo, int hi) {
    return std::uniform_int_distribution<int>(lo, hi)(Rng());
}

void Sprite::Update() {
    if (direction == Settings::LEFT) {
        rect.x -= speed;
    } else if (direction == Settings::RIGHT) {
        rect.x += speed;
    } else if (direction == Settings::UP) {
        rect.y -= speed;
    } else if (direction == Settings::DOWN) {
        rect.y += speed;
    }
}

Bullet::Bullet(const Texture* img) {
    image = img;
    speed = Settings::BULLET_SPEED;
    if (img) {
        rect.w = img->width;
        rect.h = img->height;
    }
}

void Tank::Shot() {
    // 把飞出屏幕的子弹移除（等价 __remove_sprites）
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) {
                      return b.rect.bottom() <= 0 || b.rect.top() >= Settings::SCREEN_HEIGHT ||
                             b.rect.right() <= 0 || b.rect.left() >= Settings::SCREEN_WIDTH;
                  }),
                  bullets.end());

    if (!is_alive) return;
    if (bullets.size() >= Settings::MAX_BULLETS) return;
    if (type == Settings::HERO) Audio::PlayFire();

    Bullet bullet(bulletImage);
    bullet.direction = direction;
    switch (direction) {
        case Settings::LEFT:
            bullet.rect.x = rect.left() - bullet.rect.w;
            bullet.rect.y = rect.centery() - bullet.rect.h / 2;
            break;
        case Settings::RIGHT:
            bullet.rect.x = rect.right();
            bullet.rect.y = rect.centery() - bullet.rect.h / 2;
            break;
        case Settings::UP:
            bullet.rect.y = rect.top() - bullet.rect.h;
            bullet.rect.x = rect.centerx() - bullet.rect.w / 2;
            break;
        case Settings::DOWN:
            bullet.rect.y = rect.bottom();
            bullet.rect.x = rect.centerx() - bullet.rect.w / 2;
            break;
        default:
            break;
    }
    bullets.push_back(std::move(bullet));
}

void Tank::MoveOutWall(const Wall& wall) {
    switch (direction) {
        case Settings::LEFT:
            rect.x = wall.rect.right() + 2;
            break;
        case Settings::RIGHT:
            rect.x = wall.rect.left() - rect.w - 2;
            break;
        case Settings::UP:
            rect.y = wall.rect.bottom() + 2;
            break;
        case Settings::DOWN:
            rect.y = wall.rect.top() - rect.h - 2;
            break;
        default:
            break;
    }
}

void Tank::Update() {
    if (!is_alive) return;
    Sprite::Update();
}

void Hero::HitWall() {
    if ((direction == Settings::LEFT && rect.left() <= 0) ||
        (direction == Settings::RIGHT && rect.right() >= Settings::SCREEN_WIDTH) ||
        (direction == Settings::UP && rect.top() <= 0) ||
        (direction == Settings::DOWN && rect.bottom() >= Settings::SCREEN_HEIGHT)) {
        is_hit_wall = true;
    }
}

void Hero::Update() {
    if (!is_hit_wall) {
        Tank::Update();
        image = dirImages[direction];  // 换方向贴图（等价 __turn）
    }
}

void Enemy::RandomTurn() {
    int candidates[3];
    int n = 0;
    for (int d = 0; d < 4; ++d) {
        if (d != direction) candidates[n++] = d;
    }
    direction = candidates[RandomInt(0, 2)];
    terminal = (float)RandomInt(80, 320);
    image = dirImages[direction];
}

void Enemy::RandomShot() {
    // 等价 random.choice([True] + [False]*59)：1/60 概率
    if (RandomInt(0, 59) == 0) Shot();
}

void Enemy::HitWallTurn() {
    bool turn = false;
    if (direction == Settings::LEFT && rect.left() <= 0) {
        turn = true;
        rect.x = 2;
    } else if (direction == Settings::RIGHT && rect.right() >= Settings::SCREEN_WIDTH - 1) {
        turn = true;
        rect.x = Settings::SCREEN_WIDTH - 2 - rect.w;
    } else if (direction == Settings::UP && rect.top() <= 0) {
        turn = true;
        rect.y = 2;
    } else if (direction == Settings::DOWN && rect.bottom() >= Settings::SCREEN_HEIGHT - 1) {
        turn = true;
        rect.y = Settings::SCREEN_HEIGHT - 2 - rect.h;
    }
    if (turn) RandomTurn();
}

void Enemy::Update() {
    RandomShot();
    if (terminal <= 0) {
        RandomTurn();
    } else {
        Tank::Update();
        terminal -= (float)speed;  // 碰墙掉头倒计时
    }
}

int Effect::FrameIndex(double now) const {
    int idx = (int)((now - startTime) / frameDur);
    if (idx < 0) idx = 0;
    if (idx > 7) idx = 7;
    return idx;
}
