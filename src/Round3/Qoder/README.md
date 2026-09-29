# 坦克大战（TankWar）C++ DirectX 11 版

将 Python pygame 版《坦克大战》（`TankGame\TankWar`，原作者 Zack / IronSpiderMan）1:1 忠实转写的 C++ 版本，使用 DirectX 11（Windows SDK 自带）+ WIC 贴图解码 + winmm 音效，仅依赖本机 MSVC 与 Windows SDK，无任何第三方库。

## 构建

要求：Visual Studio（本机为 VS2026 Community，cl 19.51）+ Windows SDK 10（含 d3d11/d3dcompiler/windowscodecs/winmm）。

双击 `build.bat` 即可编译出 `TankWar.exe`；或在 Git Bash 中执行：

```bash
MSYS_NO_PATHCONV=1 cmd.exe /c build.bat
```

## 运行

**必须在 `TankGame-Qoder` 目录下运行**（资源使用相对路径）：

```bash
./TankWar.exe
```

或直接双击 `TankWar.exe`（explorer 双击时工作目录即 exe 所在目录，资源可正常加载）。

## 操作

| 按键 | 作用 |
| --- | --- |
| ← → ↑ ↓ | 移动我方坦克（按住持续移动，松开停止） |
| 空格 | 开火（最多同时 3 发子弹） |
| 关闭窗口 | 退出游戏 |

## 游戏规则（与原版一致）

- 消灭场上 5 辆敌方坦克；敌方坦克会自动移动并随机开火。
- 红墙被子弹击中 2 次摧毁；铁墙无法摧毁；草墙可穿越（子弹也可穿过），且绘制在坦克上方；地图底部中央的「鸟」为国家墙，**被任何子弹击中即游戏结束**。
- 我方坦克被敌方子弹击中即游戏结束（画面定格约 0.4 秒后自动退出）。
- 原版怪癖均已保留：5 辆敌方坦克全部出生在地图左上角 (0,0)、坦克之间无碰撞、敌方子弹随其坦克被摧毁而消失。

## 目录结构

```
TankGame-Qoder/
├─ build.bat          # 一键编译
├─ TankWar.exe        # 编译产物
├─ src/               # C++ 源码
│  ├─ main.cpp        # 入口（对应 main.py）
│  ├─ settings.h      # 配置与地图（对应 settings.py）
│  ├─ sprites.h/.cpp  # 精灵类（对应 sprites.py）
│  ├─ game.h/.cpp     # 游戏主体（对应 tank_war.py）
│  ├─ renderer.h/.cpp # D3D11 渲染器（窗口/交换链/着色器/WIC 贴图）
│  ├─ input.h/.cpp    # 键盘事件队列
│  └─ audio.h/.cpp    # winmm 音效播放
├─ resources/         # 图片与音效资源（自原项目拷贝）
└─ docs/              # 转写分析文档
```

## 技术要点

- **渲染**：DirectX 11，正交投影（左上原点、Y 向下，与 pygame 坐标系一致），动态顶点缓冲逐精灵绘制，premultiplied alpha 混合（WIC 统一把 GIF 透明索引 / PNG alpha 转成 32bppPBGRA）。
- **贴图**：WIC 只解码 GIF 第 0 帧，与 `pygame.image.load` 行为一致。
- **爆炸动画**：单线程时间轴动画（坦克 0.05s/帧、墙 0.07s/帧，共 8 帧），替代原版 `Thread + time.sleep` 的实现。
- **音频**：`PlaySound`（winmm）异步播放 fire.wav / boom.wav，与 `pygame.mixer.music` 的单通道替换语义一致。
- **键盘**：WM_KEYDOWN/WM_KEYUP 事件队列并过滤自动重复，等价 pygame 的无 repeat KEYDOWN/KEYUP 事件。

详细的功能分析与转写对照见 `docs/` 目录。
