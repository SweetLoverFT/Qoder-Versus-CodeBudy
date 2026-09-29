#pragma once

#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <memory>

#include "Renderer.h"
#include "TextureLoader.h"
#include "Sprites.h"

// 游戏主类（对应 tank_war.py: TankWar）
class Game
{
public:
    bool Init(HWND hwnd);
    void Run();
    void Shutdown();

    // 键盘事件（由窗口过程转发）
    void OnKeyDown(WPARAM key, bool isRepeat);
    void OnKeyUp(WPARAM key);

private:
    bool loadTextures();
    void createSprites();
    void drawMap();
    void checkCollide();
    void updateSprites(float dt);
    void updateExplosions(float dt);
    void cleanup();
    void render();
    void drawSprite(BaseSprite* s);
    void handleBulletWallCollision(Wall* wall, Bullet* bullet);
    static bool isBlockingWall(int wallType);

    Renderer m_renderer;

    // 纹理
    Texture m_heroTex[4];
    Texture m_enemyTex[4];
    Texture m_bulletTex;
    Texture m_wallTex[6];
    Texture m_boomTex[8];
    ID3D11ShaderResourceView* m_boomSrvs[8] = { nullptr };

    // 精灵
    Hero m_hero;
    std::vector<std::unique_ptr<Enemy>> m_enemies;
    std::vector<std::unique_ptr<Wall>> m_walls;

    // 状态
    bool m_gameStill = true;   // 老巢是否存活

    // 计时
    LARGE_INTEGER m_freq = {};
};
