#include "Game.h"

#include <algorithm>

// ---------------- 初始化 ----------------
bool Game::Init(HWND hwnd)
{
    if (!m_renderer.Init(hwnd, Settings::SCREEN_WIDTH, Settings::SCREEN_HEIGHT))
    {
        MessageBoxW(nullptr, L"初始化 DirectX 11 渲染器失败。", L"坦克大战", MB_ICONERROR);
        return false;
    }

    if (!loadTextures())
    {
        MessageBoxW(nullptr, L"加载游戏资源失败，请确认 resources 目录与可执行文件位于同一目录。", L"坦克大战", MB_ICONERROR);
        return false;
    }

    createSprites();
    return true;
}

void Game::Shutdown()
{
    for (int i = 0; i < 4; ++i) m_heroTex[i].Reset();
    for (int i = 0; i < 4; ++i) m_enemyTex[i].Reset();
    m_bulletTex.Reset();
    for (int i = 0; i < 6; ++i) m_wallTex[i].Reset();
    for (int i = 0; i < 8; ++i) m_boomTex[i].Reset();

    m_renderer.Shutdown();
}

bool Game::loadTextures()
{
    ID3D11Device* device = m_renderer.GetDevice();

    for (int d = 0; d < 4; ++d)
        if (!LoadTextureFromFile(device, Settings::HERO_IMAGES[d], m_heroTex[d])) return false;
    for (int d = 0; d < 4; ++d)
        if (!LoadTextureFromFile(device, Settings::ENEMY_IMAGES[d], m_enemyTex[d])) return false;
    if (!LoadTextureFromFile(device, Settings::BULLET_IMAGE, m_bulletTex)) return false;
    for (int i = 0; i < 6; ++i)
        if (!LoadTextureFromFile(device, Settings::WALLS[i], m_wallTex[i])) return false;
    for (int i = 0; i < 8; ++i)
    {
        if (!LoadTextureFromFile(device, Settings::BOOMS[i], m_boomTex[i])) return false;
        m_boomSrvs[i] = m_boomTex[i].srv;
    }
    return true;
}

void Game::createSprites()
{
    // 我方坦克
    m_hero.bulletTexture = m_bulletTex.srv;
    for (int d = 0; d < 4; ++d) m_hero.textures[d] = m_heroTex[d].srv;
    m_hero.turn();

    // 敌方坦克（原版未设置位置，转写时按索引均匀分配到屏幕顶部）
    for (int i = 0; i < Settings::ENEMY_COUNT; ++i)
    {
        auto enemy = std::make_unique<Enemy>();
        enemy->bulletTexture = m_bulletTex.srv;
        for (int d = 0; d < 4; ++d) enemy->textures[d] = m_enemyTex[d].srv;
        enemy->turn();

        float slot = Settings::SCREEN_WIDTH / (float)Settings::ENEMY_COUNT;
        enemy->rect.setLeft(i * slot + (slot - Settings::TANK_SIZE) * 0.5f);
        enemy->rect.setTop(0.0f);

        m_enemies.push_back(std::move(enemy));
    }

    // 地图墙体
    drawMap();
}

void Game::drawMap()
{
    for (int y = 0; y < Settings::MAP_ROWS; ++y)
    {
        for (int x = 0; x < Settings::MAP_COLS; ++x)
        {
            int v = Settings::MAP_ONE[y][x];
            if (v == 0) continue;

            auto wall = std::make_unique<Wall>();
            wall->type = v; // 1红墙/2铁墙/3草/4海/5老巢
            wall->setTexture(m_wallTex[v].srv, (float)m_wallTex[v].width, (float)m_wallTex[v].height);
            wall->rect.x = x * Settings::BOX_SIZE;
            wall->rect.y = y * Settings::BOX_SIZE;
            wall->rect.w = Settings::BOX_SIZE;
            wall->rect.h = Settings::BOX_SIZE;
            if (v == Settings::BOSS_WALL) wall->life = 1;

            m_walls.push_back(std::move(wall));
        }
    }
}

// ---------------- 输入 ----------------
void Game::OnKeyDown(WPARAM key, bool isRepeat)
{
    switch (key)
    {
    case VK_LEFT:
        m_hero.direction = Settings::LEFT;
        m_hero.is_moving = true;
        m_hero.is_hit_wall = false;
        break;
    case VK_RIGHT:
        m_hero.direction = Settings::RIGHT;
        m_hero.is_moving = true;
        m_hero.is_hit_wall = false;
        break;
    case VK_UP:
        m_hero.direction = Settings::UP;
        m_hero.is_moving = true;
        m_hero.is_hit_wall = false;
        break;
    case VK_DOWN:
        m_hero.direction = Settings::DOWN;
        m_hero.is_moving = true;
        m_hero.is_hit_wall = false;
        break;
    case VK_SPACE:
        if (!isRepeat) m_hero.shot(); // 仅在首次按下时发射，避免按住连发
        break;
    }
}

void Game::OnKeyUp(WPARAM key)
{
    switch (key)
    {
    case VK_LEFT:
        m_hero.direction = Settings::LEFT;
        m_hero.is_moving = false;
        break;
    case VK_RIGHT:
        m_hero.direction = Settings::RIGHT;
        m_hero.is_moving = false;
        break;
    case VK_UP:
        m_hero.direction = Settings::UP;
        m_hero.is_moving = false;
        break;
    case VK_DOWN:
        m_hero.direction = Settings::DOWN;
        m_hero.is_moving = false;
        break;
    }
}

// ---------------- 碰撞检测（对应 __check_collide） ----------------
bool Game::isBlockingWall(int wallType)
{
    return wallType == Settings::RED_WALL ||
           wallType == Settings::IRON_WALL ||
           wallType == Settings::BOSS_WALL;
}

void Game::handleBulletWallCollision(Wall* wall, Bullet* bullet)
{
    if (wall->type == Settings::RED_WALL)
    {
        wall->kill();
        bullet->kill();
    }
    else if (wall->type == Settings::BOSS_WALL)
    {
        m_gameStill = false; // 老巢被毁，游戏结束
    }
    else if (wall->type == Settings::IRON_WALL)
    {
        bullet->kill(); // 铁墙不毁，仅子弹消失
    }
    // WEED_WALL / WATER：子弹穿过，无处理
}

void Game::checkCollide()
{
    // 1、保证坦克不移出屏幕
    m_hero.hitWall();
    for (auto& e : m_enemies)
        if (e->is_alive) e->hitWallTurn();

    // 2、子弹与墙、坦克与墙
    for (auto& w : m_walls)
    {
        if (!w->is_alive) continue;

        // 我方子弹击中墙
        for (auto& b : m_hero.bullets)
        {
            if (!b->is_alive) continue;
            if (Intersects(w->rect, b->rect)) handleBulletWallCollision(w.get(), b.get());
        }
        // 敌方子弹击中墙
        for (auto& e : m_enemies)
        {
            for (auto& b : e->bullets)
            {
                if (!b->is_alive) continue;
                if (Intersects(w->rect, b->rect)) handleBulletWallCollision(w.get(), b.get());
            }
        }
        // 我方坦克撞墙
        if (m_hero.is_alive && Intersects(m_hero.rect, w->rect))
        {
            if (isBlockingWall(w->type))
            {
                m_hero.is_hit_wall = true;
                m_hero.moveOutWall(w->rect);
            }
        }
        // 敌方坦克撞墙
        for (auto& e : m_enemies)
        {
            if (!e->is_alive) continue;
            if (Intersects(w->rect, e->rect))
            {
                if (isBlockingWall(w->type))
                {
                    e->moveOutWall(w->rect);
                    e->randomTurn();
                }
            }
        }
    }

    // 3、我方子弹击中敌方坦克
    for (auto& e : m_enemies)
    {
        if (!e->is_alive) continue;
        for (auto& b : m_hero.bullets)
        {
            if (!b->is_alive) continue;
            if (Intersects(b->rect, e->rect))
            {
                b->kill();
                e->kill();
            }
        }
    }

    // 4、敌方子弹击中我方坦克
    if (m_hero.is_alive)
    {
        for (auto& e : m_enemies)
        {
            for (auto& b : e->bullets)
            {
                if (!b->is_alive) continue;
                if (Intersects(b->rect, m_hero.rect))
                {
                    b->kill();
                    m_hero.kill();
                }
            }
        }
    }

    // 5、清理已标记死亡的子弹
    m_hero.removeDeadBullets();
    for (auto& e : m_enemies) e->removeDeadBullets();
}

// ---------------- 更新（对应 __update_sprites） ----------------
void Game::updateSprites(float dt)
{
    // 按原版顺序更新
    if (m_hero.is_moving)
        m_hero.update();

    // walls.update() 为空操作，跳过

    for (auto& b : m_hero.bullets) b->update();
    for (auto& e : m_enemies) e->update();
    for (auto& e : m_enemies)
        for (auto& b : e->bullets) b->update();

    updateExplosions(dt);
    cleanup();
}

void Game::updateExplosions(float dt)
{
    if (m_hero.exploding) m_hero.updateExplosion(m_boomSrvs, 8, dt);
    for (auto& e : m_enemies)
        if (e->exploding) e->updateExplosion(m_boomSrvs, 8, dt);
    for (auto& w : m_walls)
        if (w->exploding) w->updateExplosion(m_boomSrvs, 8, dt);
}

void Game::cleanup()
{
    // 移除爆炸结束的敌方坦克
    m_enemies.erase(
        std::remove_if(m_enemies.begin(), m_enemies.end(),
            [](const std::unique_ptr<Enemy>& e) { return !e->is_alive && !e->exploding; }),
        m_enemies.end());

    // 移除爆炸结束的墙
    m_walls.erase(
        std::remove_if(m_walls.begin(), m_walls.end(),
            [](const std::unique_ptr<Wall>& w) { return !w->is_alive && !w->exploding; }),
        m_walls.end());
}

// ---------------- 渲染 ----------------
void Game::drawSprite(BaseSprite* s)
{
    if (!s->texture) return;
    m_renderer.DrawSprite(s->texture, s->rect.x, s->rect.y, s->textureW, s->textureH);
}

void Game::render()
{
    m_renderer.BeginFrame(0.0f, 0.0f, 0.0f, 1.0f); // 黑底（SCREEN_COLOR）

    // 叠放顺序：敌弹 → 敌坦克 → 我弹 → 我坦克 → 墙（草盖在坦克上形成隐蔽）
    for (auto& e : m_enemies)
        for (auto& b : e->bullets) drawSprite(b.get());
    for (auto& e : m_enemies) drawSprite(e.get());
    for (auto& b : m_hero.bullets) drawSprite(b.get());
    drawSprite(&m_hero);
    for (auto& w : m_walls) drawSprite(w.get());

    m_renderer.EndFrame();
}

// ---------------- 主循环（对应 run_game） ----------------
void Game::Run()
{
    QueryPerformanceFrequency(&m_freq);
    LARGE_INTEGER last;
    QueryPerformanceCounter(&last);
    const double frameDuration = 1.0 / Settings::FPS;

    MSG msg = {};
    bool running = true;
    while (running)
    {
        // 1、消息泵（窗口事件与键盘消息）
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!running) break;

        // 2、帧间隔
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        float dt = (float)((double)(now.QuadPart - last.QuadPart) / m_freq.QuadPart);
        last = now;

        // 3、游戏逻辑
        if (m_gameStill && m_hero.is_alive)
        {
            checkCollide();
            updateSprites(dt);
        }
        else
        {
            // 游戏结束阶段：仅推进爆炸动画（我方坦克爆炸）
            updateExplosions(dt);
            cleanup();
        }

        // 4、渲染
        render();

        // 5、帧限流（60 FPS）
        if (dt < frameDuration)
            Sleep((DWORD)((frameDuration - dt) * 1000.0));

        // 6、游戏结束判定：老巢被毁立即结束；我方阵亡则等爆炸动画播完
        if ((!m_gameStill || !m_hero.is_alive) && !m_hero.exploding)
            running = false;
    }
}
