#pragma once

// 对应 Python 版 settings.py：集中存放游戏配置、地图与资源路径

namespace Settings {

// 游戏设置
inline constexpr int FPS = 60;              // 游戏帧率
inline constexpr const wchar_t* GAME_NAME = L"坦克大战";  // 游戏标题
inline constexpr int BOX_SIZE = 50;         // 单位格子大小
inline constexpr int SCREEN_COLS = 19;      // 屏幕横向格子数
inline constexpr int SCREEN_ROWS = 13;      // 屏幕纵向格子数
inline constexpr int SCREEN_WIDTH = BOX_SIZE * SCREEN_COLS;   // 950
inline constexpr int SCREEN_HEIGHT = BOX_SIZE * SCREEN_ROWS;  // 650

// 通用变量（方向常量）
inline constexpr int LEFT = 0;
inline constexpr int RIGHT = 1;
inline constexpr int UP = 2;
inline constexpr int DOWN = 3;

// 地图（0=空白 1=红墙 2=铁墙 3=草 4=海 5=国家/boss 墙）
inline constexpr int MAP_ONE[SCREEN_ROWS][SCREEN_COLS] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0},
    {0, 1, 0, 0, 1, 3, 3, 1, 1, 2, 1, 1, 3, 3, 1, 0, 0, 1, 0},
    {0, 1, 0, 0, 1, 3, 3, 1, 1, 2, 1, 1, 3, 3, 1, 0, 0, 1, 0},
    {0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0},
    {0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0},
    {0, 1, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 0},
    {0, 1, 3, 3, 3, 1, 0, 0, 1, 1, 1, 0, 0, 1, 3, 3, 3, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 5, 1, 0, 0, 0, 0, 0, 0, 0, 0},
};

// 音频
inline constexpr const char* BOOM_MUSIC = "resources/musics/boom.wav";
inline constexpr const char* FIRE_MUSIC = "resources/musics/fire.wav";

// 坦克类型
inline constexpr int HERO = 0;
inline constexpr int ENEMY = 1;

// 我方坦克
inline constexpr const char* HERO_IMAGE_NAME = "resources/images/hero/hero1U.gif";
inline constexpr const char* HERO_IMAGES[4] = {
    "resources/images/hero/hero1L.gif",
    "resources/images/hero/hero1R.gif",
    "resources/images/hero/hero1U.gif",
    "resources/images/hero/hero1D.gif",
};
inline constexpr int HERO_SPEED = 2;

// 敌方坦克
inline constexpr const char* ENEMY_IMAGES[4] = {
    "resources/images/enemy/enemy2L.gif",
    "resources/images/enemy/enemy2R.gif",
    "resources/images/enemy/enemy2U.gif",
    "resources/images/enemy/enemy2D.gif",
};
inline constexpr int ENEMY_COUNT = 5;
inline constexpr int ENEMY_SPEED = 1;

// 子弹
inline constexpr const char* BULLET_IMAGE_NAME = "resources/images/bullet/bullet.png";
inline constexpr int BULLET_SPEED = 5;

// 墙类型
inline constexpr int RED_WALL = 1;
inline constexpr int IRON_WALL = 2;
inline constexpr int WEED_WALL = 3;
inline constexpr int SEA_WALL = 4;
inline constexpr int BOSS_WALL = 5;
// 墙贴图与地图数值一一对应（0.png 为空白，地图中不生成）
inline constexpr const char* WALL_IMAGES[6] = {
    "resources/images/walls/0.png",
    "resources/images/walls/1.png",
    "resources/images/walls/2.png",
    "resources/images/walls/3.png",
    "resources/images/walls/4.png",
    "resources/images/walls/5.png",
};

// 爆炸贴图（8 帧）
inline constexpr const char* BOOM_IMAGES[8] = {
    "resources/images/boom/blast1.gif",
    "resources/images/boom/blast2.gif",
    "resources/images/boom/blast3.gif",
    "resources/images/boom/blast4.gif",
    "resources/images/boom/blast5.gif",
    "resources/images/boom/blast6.gif",
    "resources/images/boom/blast7.gif",
    "resources/images/boom/blast8.gif",
};

// 每辆坦克同时最多存活子弹数
inline constexpr int MAX_BULLETS = 3;

}  // namespace Settings
