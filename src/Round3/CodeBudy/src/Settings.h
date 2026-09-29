#pragma once

// 游戏常量（对应 settings.py）
namespace Settings
{
    // 游戏设置
    inline constexpr int FPS = 60;                          // 游戏帧率
    inline constexpr int BOX_SIZE = 50;                     // 单位格子大小
    inline constexpr int SCREEN_WIDTH = BOX_SIZE * 19;      // 950
    inline constexpr int SCREEN_HEIGHT = BOX_SIZE * 13;     // 650

    // 通用方向
    inline constexpr int LEFT = 0;
    inline constexpr int RIGHT = 1;
    inline constexpr int UP = 2;
    inline constexpr int DOWN = 3;

    // 坦克类型
    inline constexpr int HERO = 0;
    inline constexpr int ENEMY = 1;

    // 墙体类型（0空白、1红墙、2铁墙、3草、4海、5鸟/老巢）
    inline constexpr int RED_WALL = 1;
    inline constexpr int IRON_WALL = 2;
    inline constexpr int WEED_WALL = 3;
    inline constexpr int WATER = 4;
    inline constexpr int BOSS_WALL = 5;

    // 速度
    inline constexpr float HERO_SPEED = 2.0f;
    inline constexpr float ENEMY_SPEED = 1.0f;
    inline constexpr float BULLET_SPEED = 5.0f;

    // 精灵尺寸（与资源实际像素尺寸一致）
    inline constexpr float TANK_SIZE = 40.0f;    // 坦克贴图 40x40
    inline constexpr float BULLET_SIZE = 3.0f;   // 子弹贴图 3x3
    inline constexpr float BOOM_SIZE = 40.0f;    // 爆炸贴图 40x40

    // 敌方数量
    inline constexpr int ENEMY_COUNT = 5;

    // 资源路径
    inline constexpr const wchar_t* HERO_IMAGES[4] = {
        L"resources/images/hero/hero1L.gif",
        L"resources/images/hero/hero1R.gif",
        L"resources/images/hero/hero1U.gif",
        L"resources/images/hero/hero1D.gif",
    };

    inline constexpr const wchar_t* ENEMY_IMAGES[4] = {
        L"resources/images/enemy/enemy2L.gif",
        L"resources/images/enemy/enemy2R.gif",
        L"resources/images/enemy/enemy2U.gif",
        L"resources/images/enemy/enemy2D.gif",
    };

    inline constexpr const wchar_t* BULLET_IMAGE = L"resources/images/bullet/bullet.png";

    // 墙体图片（下标即地图值：0..5）
    inline constexpr const wchar_t* WALLS[6] = {
        L"resources/images/walls/0.png",
        L"resources/images/walls/1.png",
        L"resources/images/walls/2.png",
        L"resources/images/walls/3.png",
        L"resources/images/walls/4.png",
        L"resources/images/walls/5.png",
    };

    // 爆炸帧
    inline constexpr const wchar_t* BOOMS[8] = {
        L"resources/images/boom/blast1.gif",
        L"resources/images/boom/blast2.gif",
        L"resources/images/boom/blast3.gif",
        L"resources/images/boom/blast4.gif",
        L"resources/images/boom/blast5.gif",
        L"resources/images/boom/blast6.gif",
        L"resources/images/boom/blast7.gif",
        L"resources/images/boom/blast8.gif",
    };

    // 音频
    inline constexpr const wchar_t* BOOM_MUSIC = L"resources/musics/boom.wav";
    inline constexpr const wchar_t* FIRE_MUSIC = L"resources/musics/fire.wav";
    inline constexpr const wchar_t* HIT_MUSIC = L"resources/musics/hit.wav";

    // 地图（13 行 × 19 列）
    inline constexpr int MAP_ROWS = 13;
    inline constexpr int MAP_COLS = 19;
    inline constexpr int MAP_ONE[MAP_ROWS][MAP_COLS] = {
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0 },
        { 0, 1, 0, 0, 1, 3, 3, 1, 1, 2, 1, 1, 3, 3, 1, 0, 0, 1, 0 },
        { 0, 1, 0, 0, 1, 3, 3, 1, 1, 2, 1, 1, 3, 3, 1, 0, 0, 1, 0 },
        { 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0 },
        { 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0 },
        { 0, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 0 },
        { 0, 1, 3, 3, 3, 1, 0, 0, 1, 1, 1, 0, 0, 1, 3, 3, 3, 1, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 1, 5, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    };
}
