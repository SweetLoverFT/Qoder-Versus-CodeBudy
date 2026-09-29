# TankWar Python → C++ DirectX 11 转写计划

## Context

用户要求把 `TankGame\TankWar`（pygame 坦克大战，作者 Zack 的 GitHub 项目 TankWar）1:1 忠实转写为 C++ 游戏源码，全部产物（源码 + 功能分析文档）输出到 `TankGame-Qoder\`。已与用户确认两点：

1. **图形方案 = DirectX 11**：本机无老版 DirectX SDK (June 2010)，但 VS2026 自带的 Windows SDK 10.0.26100.0 含完整 DX11（`um\d3d11.h`、`um\d3dcompiler.h`、`shared\dxgi.h` + d3d11/d3dcompiler/dxgi/windowscodecs/winmm x64 库），即现代 DirectX SDK。无 Vulkan SDK、无 MinGW。
2. **忠实复刻原版行为**，包括怪癖（5 个敌方坦克全部出生在 (0,0) 重叠、子弹命中国家(boss)墙直接游戏结束、敌人互不碰撞、草墙可穿越且绘制在坦克上方等）。

工具链：cl 19.51（`C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat`），仅用 Windows SDK，不引入第三方库。

## 交付物结构（全部在 TankGame-Qoder 下新建）

```
TankGame-Qoder/
├─ build.bat                # vcvars64 + cl 一键编译（x64）
├─ README.md                # 构建/运行/操作说明
├─ src/
│  ├─ main.cpp              # 入口 ← main.py
│  ├─ settings.h            # 常量配置 ← settings.py
│  ├─ sprites.h / sprites.cpp   # 精灵类 ← sprites.py
│  ├─ game.h / game.cpp     # 游戏主体 ← tank_war.py
│  ├─ renderer.h / renderer.cpp # D3D11 窗口/交换链/精灵绘制/WIC 纹理
│  ├─ audio.h / audio.cpp   # winmm PlaySound 封装
│  └─ input.h / input.cpp   # 按键状态表（消息驱动）
├─ resources/               # 从 TankWar\resources 原样拷贝（images + musics）
└─ docs/
   ├─ python-analysis.md    # Python 源码逐文件功能分析（含怪癖标注）
   ├─ mapping.md            # Python→C++ 类/函数映射表
   └─ differences.md        # 与 pygame 可观察行为的差异说明
```

## 核心设计

### 类映射（sprites.py → sprites.h/cpp）

- `Sprite`：`Texture* img; Rect rect(图片原始尺寸); int direction; int speed;` + `Move()`（按方向移动 speed 像素）← `BaseSprite`
- `Bullet : Sprite`（speed=5）
- `Tank : Sprite`：`bool is_alive=true, is_moving=false; int type; std::vector<Bullet> bullets;` + `Shot()`（先清屏外子弹、死亡或 ≥3 发不发、hero 发时播 fire.wav、子弹生成在炮口方向紧贴处）、`MoveOutWall(Wall*)`（推出墙外 2px）← `TankSprite`
- `Hero : Tank`：`bool is_hit_wall;` + `HitWall()`（仅屏幕四边越界检测）、`Update()`（未碰墙才移动并换 `hero1{dir}.gif` 方向贴图）← `Hero`。出生 centerx=375、bottom=650、方向 UP
- `Enemy : Tank`：`float terminal;` + `RandomTurn()`（从其余 3 方向随机挑、terminal 重置 80..320、换 `enemy2{dir}.gif`）、`RandomShot()`（1/60 概率）、`HitWallTurn()`（出屏钳制 + RandomTurn）、`Update()`（RandomShot → terminal<=0 则转，否则移动并 terminal-=speed）。**构造后不设位置，5 个全在 (0,0)**（怪癖保留）
- `Wall : Sprite`：`int type; int life=2;`（boss 创建时 life=1 覆写）

### 游戏主体（tank_war.py → game.h/cpp）

`Game` 类：hero、`vector<Enemy>`、`vector<Wall>`、`vector<Bullet>`（hero/enemy 子弹）、`vector<Effect>`（爆炸动画实体）、`game_still`。方法对应 `__create_sprite/__draw_map/__check_collide/__update_sprites/run_game`。

- 地图：`MAP_ONE` 13×19 硬编码于 settings.h；0=空、1=红墙、2=铁墙、3=草、5=boss，贴图 `walls/{1,2,3,5}.png`
- 碰撞（忠实复刻）：
  - 子弹 vs 墙：红墙→墙 life-1 + 弹死（2 发摧毁）；铁墙→弹死；boss→`game_still=false`；草→无碰撞（穿过）
  - 坦克 vs 墙（红/铁/boss）：hero 置 is_hit_wall + MoveOutWall；enemy MoveOutWall + RandomTurn
  - hero 子弹 vs 敌人 → 同归于尽；敌人子弹 vs hero → hero 死亡
  - **无坦克互撞、无 hero-敌人身体碰撞、无子弹互撞**（Python 本无）
- 绘制顺序（= pygame blit 序，逐个 DrawIndexed 提交）：enemy bullets → enemies → hero bullets → hero → walls（草墙最后画、盖住坦克）
- 死亡与爆炸：死亡瞬间从碰撞/更新中移除，转为 `Effect` 实体保留在绘制列表——坦克 8 帧×0.05s、墙 8 帧×0.07s 时间轴动画（单线程，替代 Python 的 Thread+sleep），播完删除；**Effect 不参与碰撞**（Python 中 kill() 立即移出 sprite 组，线程只做视觉 blit）。死亡坦克的子弹立即消失（Python 中随坦克移出组而不再绘制）
- hero 死亡：Python 中 `Hero.kill()` 主线程同步跑 boom（blit 无 display.update，不可见）→ 画面定格约 0.4s → 退出进程。C++ 复刻：世界冻结、继续 Present 约 0.4s 后正常 return 0，**不播可见爆炸**（写入 differences.md）
- 主循环：消息泵 → 碰撞 → 更新 → 绘制 → Present → QPC 帧帽 60FPS（等价 tick(60) 只减速不补帧）；每帧末检查 `hero.is_alive && game_still`，退出返回 0

### 渲染（renderer.h/cpp，自研最小实现）

- `RegisterClassW`+`CreateWindowW`，客户区 950×650，标题 `L"坦克大战"`（源文件 /utf-8）
- D3D11 交换链 BufferCount=1、Present(0,0)（无 vsync）；`XMMatrixOrthographicOffCenterLH(0,950,650,0,0,1)` 像素左上原点 Y 向下
- 顶点 `{float2 pos; float2 uv; float4 color;}` + 6 索引/精灵；`D3D11_USAGE_DYNAMIC` + `Map(DISCARD)` 每帧重建；**逐精灵一次 DrawIndexed**（每帧 ~50 call，规模可接受），提交序即 blit 序
- 混合：WIC 转 `32bppPBGRA`（premultiplied alpha）→ `SrcBlend=ONE, DestBlend=INV_SRC_ALPHA`（GIF 透明与 PNG alpha 统一处理）
- 着色器：运行时 `D3DCompile`（VS：pos/uv/color 乘矩阵；PS：`tex.Sample × color`），无需 fxc 离线步骤
- WIC 加载：`CreateDecoderFromFilename` → `GetFrame(0)`（等价 pygame 只取 GIF 首帧）→ 转 32bppPBGRA → `CreateTexture2D(IMMUTABLE)`。风险：GIF 透明索引转 alpha 需实测，若不正确则补 colorkey→alpha 后处理

### 输入 / 音频

- `input.cpp`：窗口过程记录 `bool keyDown[256]`；方向键 keydown → direction + is_moving=true + 清 is_hit_wall，keyup → is_moving=false；空格仅非重复 keydown（过滤 lParam bit30）触发 Shot —— 等价 pygame KEYDOWN/KEYUP 无自动重复语义
- `audio.cpp`：`PlaySound(L"resources\\musics\\fire.wav", 0, SND_FILENAME|SND_ASYNC)`；再播自动替换 ≈ mixer.music 单通道语义；动画结束 `SND_PURGE`
- 随机数：`std::mt19937`（random_device 种子）+ `uniform_int_distribution(0,3)` / `(80,320)` / `(0,59)==0`，分布等价 Python randint（序列不可复现，写入 differences.md）

### build.bat

```bat
@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cl /nologo /EHsc /std:c++17 /utf-8 /O2 /W4 /DUNICODE /D_UNICODE src\*.cpp /Fe:TankWar.exe /link d3d11.lib d3dcompiler.lib dxgi.lib windowscodecs.lib winmm.lib
```

Git Bash 下运行需 `MSYS_NO_PATHCONV=1 cmd.exe /c build.bat`（README 注明）。vcvars 后 INCLUDE 自动含 um/shared，无需手工 -I。

### docs（中文）

- `python-analysis.md`：4 个 .py 逐文件逐方法行为分析，怪癖明确标注（敌人 (0,0) 出生、boss 墙中弹即败、墙/坦克爆炸为线程 blit 的视觉呈现、敌弹随死亡坦克消失、草盖坦克、无坦克互撞）
- `mapping.md`：Python 类/函数 → C++ 文件/类/函数对照表
- `differences.md`：随机序列不可复现、定时精度（sleep vs 时间轴）、hero 死亡不可见爆炸复刻为 0.4s 定格、音频 SND_PURGE 语义

## 实施步骤

1. 创建目录结构，拷贝 resources（images + musics 原样）
2. `settings.h`（常量 + MAP_ONE + 贴图/音频路径表）
3. `renderer.*`（窗口/D3D11/WIC/着色器/DrawSprite）+ `audio.*` + `input.*`
4. `sprites.*`（Sprite/Bullet/Tank/Hero/Enemy/Wall）
5. `game.*`（地图构建、碰撞、更新、绘制、主循环）+ `main.cpp`
6. `build.bat` + README
7. 编译至零错误零警告，跑验证清单
8. 写 docs 三份分析文档

## 验证

1. `build.bat` 编译通过（/W4 零警告）
2. 启动 `TankWar.exe`：进程存活、窗口标题"坦克大战"
3. PowerShell `CopyFromScreen` 截图核对：黑底、地图墙布局与 MAP_ONE 逐格一致、hero 底部居中、5 敌重叠左上角、草墙盖住其下坦克
4. 模拟按键（PowerShell SendKeys 或人工）：方向键移动、空格开火 + fire.wav、红墙两发摧毁 + boom.wav、boss 墙中弹 → 进程退出且 exit code 0、hero 中弹 → 定格约 0.4s 后退出、窗口关闭退出码 0
5. 无法完全自动化的部分（音效听感、手感）在 README 验证清单中留人工项

## 风险决策备忘

- dxgi.h 在 SDK `shared\` 目录：依赖 vcvars 环境变量，不手工 -I，不引用旧 SDK
- WIC GIF 透明索引：实现后读一帧像素校验透明处 alpha==0，否则补 colorkey 转换
- premultiplied 全链路一致（PBGRA + ONE/INV_SRC_ALPHA），勿混 straight alpha
- 绘制严格按 blit 顺序逐个 draw，不做按纹理合并重排（草盖坦克依赖提交序）
- 不"顺手修复"任何原版怪癖——任何改动点只在 differences.md 里说明
