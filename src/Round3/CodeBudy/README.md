# 坦克大战（TankGame）— C++ / DirectX 11 版

将 Python/pygame 实现的经典《坦克大战》完整转写为 **C++ + DirectX 11** 桌面游戏，行为与原版 1:1 等价。

## 游戏说明

- **我方坦克**：方向键（↑↓←→）控制移动，空格键发射子弹（最多同时 3 发）
- **敌方坦克**：5 辆，随机移动、随机转向、随机射击（约 1/60 概率）
- **地图**：13×19 网格（每格 50px，屏幕 950×650）
  - 红砖墙：可摧毁，需命中 2 次
  - 铁墙：不可摧毁
  - 草丛：可穿越，且绘制在坦克之上形成隐蔽
  - 老巢（鸟）：被击中即游戏结束
- **胜负条件**：我方坦克被击毁 或 老巢被摧毁 即结束
- **特效**：射击/爆炸 WAV 音效，8 帧爆炸动画

## 环境要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Windows 10 / 11 |
| 编译器 | MSVC（Visual Studio 2026，含「使用 C++ 的桌面开发」工作负载） |
| 构建工具 | CMake 3.16+ |
| 图形 API | DirectX 11（Windows SDK 内置，无需额外安装旧版 DirectX SDK） |

## 目录结构

```
TankGame-CodeBuddy/
├── CMakeLists.txt              # 构建脚本（链接 d3d11/d3dcompiler/dxgi/winmm/windowscodecs）
├── README.md                   # 本文档
├── docs/
│   ├── 代码功能分析.md           # Python → C++ 的功能映射分析
│   └── 转写说明.md               # 转写决策、与原版差异、资源说明
├── resources/                  # 游戏资源（图片 + 音效，从 Python 项目复制）
└── src/
    ├── main.cpp                # 入口：Win32 窗口 + 工作目录定位 + 主循环
    ├── Settings.h              # 常量、方向、地图（对应 settings.py）
    ├── Rect.h                  # 矩形结构 + 相交判定（对应 pygame.Rect）
    ├── Renderer.h/.cpp         # D3D11 渲染器：设备/交换链/着色器/精灵绘制
    ├── TextureLoader.h/.cpp    # WIC 图片解码（PNG/GIF，对应 pygame.image.load）
    ├── Audio.h/.cpp            # PlaySound 音效封装（对应 pygame.mixer.music）
    ├── Sprites.h/.cpp          # 精灵体系：BaseSprite/Bullet/Tank/Hero/Enemy/Wall
    └── Game.h/.cpp             # 游戏主类：地图/输入/碰撞/更新/渲染（对应 tank_war.py）
```

## 构建与运行

### 1. 配置工程

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
```

### 2. 编译

```powershell
# Release 版（推荐）
cmake --build build --config Release

# 或 Debug 版
cmake --build build --config Debug
```

构建完成后，`resources` 目录会自动复制到 exe 所在目录。

### 3. 运行

```powershell
# Release
.\build\Release\TankGame.exe

# Debug
.\build\Debug\TankGame.exe
```

> 程序启动时会自动将工作目录定位到 exe 所在目录，因此从任意目录运行都能正确加载资源。

## 操作说明

| 按键 | 功能 |
|------|------|
| ↑ ↓ ← → | 移动我方坦克 |
| 空格 | 发射子弹 |
| 关闭窗口 | 退出游戏 |

## 相关文档

- 详细的代码功能映射见 [`docs/代码功能分析.md`](docs/代码功能分析.md)
- 转写决策与原版差异见 [`docs/转写说明.md`](docs/转写说明.md)
