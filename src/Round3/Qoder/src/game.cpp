#include "game.h"

#include <algorithm>

#include "audio.h"
#include "input.h"

namespace {

// 墙类型是否阻挡坦克通行（红墙/铁墙/boss 墙；草可穿越）
bool IsSolidWall(int type) {
    return type == Settings::RED_WALL || type == Settings::IRON_WALL ||
           type == Settings::BOSS_WALL;
}

// 帧率限制（等价 pygame clock.tick(60)：只减速不补帧）
void CapFrameRate(double frameStart, double frameDur) {
    while (true) {
        double remain = frameStart + frameDur - NowSeconds();
        if (remain <= 0) break;
        if (remain > 0.001) Sleep((DWORD)(remain * 1000.0) > 1 ? (DWORD)(remain * 1000.0) : 1);
        else Sleep(0);
    }
}

}  // namespace

double NowSeconds() {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
}

int Game::Run(HINSTANCE hInstance) {
    if (!renderer_.Init(hInstance, Settings::SCREEN_WIDTH, Settings::SCREEN_HEIGHT,
                        Settings::GAME_NAME))
        return 1;
    Audio::Init();

    for (int d = 0; d < 4; ++d) {
        heroImages_[d] = renderer_.LoadTexture(Settings::HERO_IMAGES[d]);
        enemyImages_[d] = renderer_.LoadTexture(Settings::ENEMY_IMAGES[d]);
    }
    bulletImage_ = renderer_.LoadTexture(Settings::BULLET_IMAGE_NAME);
    for (int i = 0; i < 6; ++i) wallImages_[i] = renderer_.LoadTexture(Settings::WALL_IMAGES[i]);
    for (int i = 0; i < 8; ++i) boomImages_[i] = renderer_.LoadTexture(Settings::BOOM_IMAGES[i]);

    CreateSprites();
    DrawMap();

    const double frameDur = 1.0 / Settings::FPS;
    // 对应 Python: while True and hero.is_alive and game_still:
    while (!renderer_.WantsQuit() && game_still_ &&
           !(heroDead_ && NowSeconds() - heroDeadAt_ >= 0.4)) {
        double frameStart = NowSeconds();

        // 1、事件监听（含键盘按下/松开）
        renderer_.PumpMessages();
        if (!heroDead_) HandleEvents();

        // 2、碰撞检测；3、更新精灵
        if (!heroDead_) {
            CheckCollide();
            UpdateSprites();
        }

        // 4、绘制 + 更新显示
        Draw();
        if (screenshotRequested_) {
            renderer_.SaveScreenshot(L"screenshot.png");
            screenshotRequested_ = false;
        }
        renderer_.EndFrame();

        // 5、帧率限制 60FPS
        CapFrameRate(frameStart, frameDur);
    }

    renderer_.Shutdown();
    return 0;
}

void Game::CreateSprites() {
    // 我方英雄（初始贴图 hero1U.gif，出生在底部中央偏左）
    hero_ = std::make_unique<Hero>();
    hero_->type = Settings::HERO;
    hero_->speed = Settings::HERO_SPEED;
    hero_->direction = Settings::UP;
    hero_->bulletImage = bulletImage_;
    for (int d = 0; d < 4; ++d) hero_->dirImages[d] = heroImages_[d];
    hero_->image = heroImages_[Settings::UP];
    hero_->rect.w = hero_->image->width;
    hero_->rect.h = hero_->image->height;
    // centerx = 屏幕中心 - 2 格；bottom = 屏幕底部
    hero_->rect.x = Settings::SCREEN_WIDTH / 2 - Settings::BOX_SIZE * 2 - hero_->rect.w / 2;
    hero_->rect.y = Settings::SCREEN_HEIGHT - hero_->rect.h;

    // 5 个敌方坦克（原版怪癖：构造后不设置位置，全部出生在 (0,0) 重叠）
    for (int i = 0; i < Settings::ENEMY_COUNT; ++i) {
        Enemy enemy;
        enemy.type = Settings::ENEMY;
        enemy.speed = Settings::ENEMY_SPEED;
        enemy.bulletImage = bulletImage_;
        enemy.direction = RandomInt(0, 3);
        enemy.terminal = (float)RandomInt(80, 320);
        for (int d = 0; d < 4; ++d) enemy.dirImages[d] = enemyImages_[d];
        enemy.image = enemyImages_[enemy.direction];
        enemy.rect.w = enemy.image->width;
        enemy.rect.h = enemy.image->height;
        enemies_.push_back(std::move(enemy));
    }
}

void Game::DrawMap() {
    for (int y = 0; y < Settings::SCREEN_ROWS; ++y) {
        for (int x = 0; x < Settings::SCREEN_COLS; ++x) {
            int cell = Settings::MAP_ONE[y][x];
            if (cell == 0) continue;
            Wall wall;
            wall.image = wallImages_[cell];
            wall.rect.x = x * Settings::BOX_SIZE;
            wall.rect.y = y * Settings::BOX_SIZE;
            wall.rect.w = wall.image->width;
            wall.rect.h = wall.image->height;
            wall.type = cell;
            if (cell == Settings::BOSS_WALL) wall.life = 1;
            walls_.push_back(std::move(wall));
        }
    }
}

void Game::HandleEvents() {
    std::vector<KeyEvent> events;
    InputQueue::Drain(events);
    for (const KeyEvent& e : events) {
        if (!e.down) {  // KEYUP：对应 __check_keyup
            switch (e.vk) {
                case VK_LEFT:
                    hero_->direction = Settings::LEFT;
                    hero_->is_moving = false;
                    break;
                case VK_RIGHT:
                    hero_->direction = Settings::RIGHT;
                    hero_->is_moving = false;
                    break;
                case VK_UP:
                    hero_->direction = Settings::UP;
                    hero_->is_moving = false;
                    break;
                case VK_DOWN:
                    hero_->direction = Settings::DOWN;
                    hero_->is_moving = false;
                    break;
                default:
                    break;
            }
        } else {  // KEYDOWN：对应 __check_keydown
            switch (e.vk) {
                case VK_LEFT:
                    hero_->direction = Settings::LEFT;
                    hero_->is_moving = true;
                    hero_->is_hit_wall = false;
                    break;
                case VK_RIGHT:
                    hero_->direction = Settings::RIGHT;
                    hero_->is_moving = true;
                    hero_->is_hit_wall = false;
                    break;
                case VK_UP:
                    hero_->direction = Settings::UP;
                    hero_->is_moving = true;
                    hero_->is_hit_wall = false;
                    break;
                case VK_DOWN:
                    hero_->direction = Settings::DOWN;
                    hero_->is_moving = true;
                    hero_->is_hit_wall = false;
                    break;
                case VK_SPACE:
                    hero_->Shot();
                    break;
                case VK_F12:
                    // 调试截图：本帧绘制完成后保存（见 Run 主循环）
                    screenshotRequested_ = true;
                    break;
                default:
                    break;
            }
        }
    }
}

void Game::HitWallLife(Wall& wall) {
    wall.life -= 1;
    if (wall.life <= 0) {
        // 墙被摧毁：爆炸动画（0.07s/帧，共 8 帧），期间不再参与碰撞
        Effect fx;
        fx.rect = wall.rect;
        for (int i = 0; i < 8; ++i) fx.frames[i] = boomImages_[i];
        fx.startTime = NowSeconds();
        fx.frameDur = 0.07;
        effects_.push_back(std::move(fx));
        Audio::PlayBoom();
    }
}

void Game::KillEnemy(Enemy& enemy) {
    enemy.is_alive = false;
    Effect fx;
    fx.rect = enemy.rect;
    for (int i = 0; i < 8; ++i) fx.frames[i] = boomImages_[i];
    fx.startTime = NowSeconds();
    fx.frameDur = 0.05;  // 坦克爆炸帧时长
    effects_.push_back(std::move(fx));
    Audio::PlayBoom();
}

void Game::KillHero() {
    hero_->is_alive = false;
    // Python 版 Hero.kill() 主线程同步播放爆炸音效并定格约 0.4s 后退出
    Audio::PlayBoom();
    heroDead_ = true;
    heroDeadAt_ = NowSeconds();
}

void Game::CheckCollide() {
    // 保证坦克不移出屏幕
    hero_->HitWall();
    for (auto& enemy : enemies_) enemy.HitWallTurn();

    // 逐墙处理：子弹击中墙、坦克撞墙
    for (size_t wi = 0; wi < walls_.size(); ++wi) {
        Wall& wall = walls_[wi];
        bool wallDead = false;

        // 我方英雄子弹击中墙
        for (size_t bi = 0; bi < hero_->bullets.size(); ++bi) {
            Bullet& bullet = hero_->bullets[bi];
            if (!wall.rect.Collides(bullet.rect)) continue;
            if (wall.type == Settings::RED_WALL) {
                HitWallLife(wall);
                if (wall.life <= 0) wallDead = true;
                hero_->bullets.erase(hero_->bullets.begin() + bi);
                --bi;
            } else if (wall.type == Settings::BOSS_WALL) {
                game_still_ = false;  // 国家墙被击中，游戏结束（子弹不消失，原版如此）
            } else if (wall.type == Settings::IRON_WALL) {
                hero_->bullets.erase(hero_->bullets.begin() + bi);
                --bi;
            }
        }
        if (wallDead) {
            walls_.erase(walls_.begin() + wi);
            --wi;
            continue;
        }

        // 敌方子弹击中墙
        for (auto& enemy : enemies_) {
            for (size_t bi = 0; bi < enemy.bullets.size(); ++bi) {
                Bullet& bullet = enemy.bullets[bi];
                if (!wall.rect.Collides(bullet.rect)) continue;
                if (wall.type == Settings::RED_WALL) {
                    HitWallLife(wall);
                    if (wall.life <= 0) wallDead = true;
                    enemy.bullets.erase(enemy.bullets.begin() + bi);
                    --bi;
                } else if (wall.type == Settings::BOSS_WALL) {
                    game_still_ = false;
                } else if (wall.type == Settings::IRON_WALL) {
                    enemy.bullets.erase(enemy.bullets.begin() + bi);
                    --bi;
                }
            }
            if (wallDead) break;
        }
        if (wallDead) {
            walls_.erase(walls_.begin() + wi);
            --wi;
            continue;
        }

        // 我方坦克撞墙（不可穿越墙）
        if (hero_->rect.Collides(wall.rect) && IsSolidWall(wall.type)) {
            hero_->is_hit_wall = true;
            hero_->MoveOutWall(wall);
        }

        // 敌方坦克撞墙
        for (auto& enemy : enemies_) {
            if (enemy.rect.Collides(wall.rect) && IsSolidWall(wall.type)) {
                enemy.MoveOutWall(wall);
                enemy.RandomTurn();
            }
        }
    }

    // 我方子弹击中敌方坦克（等价 groupcollide：子弹与坦克同归于尽）
    for (size_t bi = 0; bi < hero_->bullets.size(); ++bi) {
        Bullet& bullet = hero_->bullets[bi];
        bool bulletDead = false;
        for (size_t ei = 0; ei < enemies_.size(); ++ei) {
            if (!bullet.rect.Collides(enemies_[ei].rect)) continue;
            KillEnemy(enemies_[ei]);
            enemies_.erase(enemies_.begin() + ei);
            bulletDead = true;
            break;  // 子弹命中第一个敌人后消失
        }
        if (bulletDead) {
            hero_->bullets.erase(hero_->bullets.begin() + bi);
            --bi;
        }
    }

    // 敌方子弹击中我方英雄
    for (auto& enemy : enemies_) {
        for (size_t bi = 0; bi < enemy.bullets.size(); ++bi) {
            if (!enemy.bullets[bi].rect.Collides(hero_->rect)) continue;
            enemy.bullets.erase(enemy.bullets.begin() + bi);
            --bi;
            if (!hero_->is_alive) break;
            KillHero();
            break;  // hero 死亡后本帧不再处理其余子弹
        }
    }
}

void Game::UpdateSprites() {
    // 对应 __update_sprites 的更新部分（绘制顺序见 Draw）
    if (hero_->is_moving) hero_->Update();
    for (auto& wall : walls_) wall.Update();  // Wall::update 为空操作
    for (auto& bullet : hero_->bullets) bullet.Update();
    for (auto& enemy : enemies_) enemy.Update();
    for (auto& enemy : enemies_) {
        for (auto& bullet : enemy.bullets) bullet.Update();
    }

    // 移除播放完毕的爆炸动画
    double now = NowSeconds();
    effects_.erase(std::remove_if(effects_.begin(), effects_.end(),
                                  [now](const Effect& fx) { return fx.Finished(now); }),
                   effects_.end());
}

void Game::Draw() {
    renderer_.BeginFrame();

    // 绘制顺序与 Python blit 顺序一致：敌弹 → 敌人 → 我弹 → 英雄 → 墙（草墙盖住坦克）
    for (auto& enemy : enemies_) {
        for (auto& bullet : enemy.bullets) {
            renderer_.DrawSprite(bullet.image, bullet.rect.x, bullet.rect.y);
        }
    }
    for (auto& enemy : enemies_) {
        renderer_.DrawSprite(enemy.image, enemy.rect.x, enemy.rect.y);
    }
    for (auto& bullet : hero_->bullets) {
        renderer_.DrawSprite(bullet.image, bullet.rect.x, bullet.rect.y);
    }
    renderer_.DrawSprite(hero_->image, hero_->rect.x, hero_->rect.y);
    for (auto& wall : walls_) {
        renderer_.DrawSprite(wall.image, wall.rect.x, wall.rect.y);
    }

    // 爆炸动画最后绘制（Python 版由后台线程 blit 呈现，绘制在最上层）
    double now = NowSeconds();
    for (auto& fx : effects_) {
        renderer_.DrawSprite(fx.frames[fx.FrameIndex(now)], fx.rect.x, fx.rect.y);
    }
}
