#pragma once

#include <windows.h>

#include <memory>
#include <vector>

#include "renderer.h"
#include "sprites.h"

// 对应 tank_war.py 的 TankWar 类：游戏主体（初始化、事件、碰撞、更新、绘制、主循环）
class Game {
public:
    int Run(HINSTANCE hInstance);

private:
    void CreateSprites();  // 对应 __create_sprite：hero + 5 个敌人 + 地图墙
    void DrawMap();        // 对应 __draw_map
    void HandleEvents();   // 对应 __event_handler + __check_keydown/__check_keyup
    void CheckCollide();   // 对应 __check_collide
    void UpdateSprites();  // 对应 __update_sprites 的更新部分
    void Draw();           // 对应 __update_sprites 的绘制部分 + display.update

    void HitWallLife(Wall& wall);  // 红墙被子弹击中：生命 -1，归零时爆炸移除
    void KillEnemy(Enemy& enemy);
    void KillHero();

    Renderer renderer_;
    std::unique_ptr<Hero> hero_;
    std::vector<Enemy> enemies_;
    std::vector<Wall> walls_;
    std::vector<Effect> effects_;

    bool game_still_ = true;   // boss 墙中弹时置 false，结束游戏
    double heroDeadAt_ = 0.0;  // hero 死亡时刻（冻结 0.4s 后退出）
    bool heroDead_ = false;
    bool screenshotRequested_ = false;  // F12 请求截图（本帧绘制完成后执行）

    // 贴图（由 Renderer 持有并缓存，这里只存指针）
    const Texture* heroImages_[4] = {};
    const Texture* enemyImages_[4] = {};
    const Texture* bulletImage_ = nullptr;
    const Texture* wallImages_[6] = {};
    const Texture* boomImages_[8] = {};
};

// 高精度计时（秒）
double NowSeconds();
