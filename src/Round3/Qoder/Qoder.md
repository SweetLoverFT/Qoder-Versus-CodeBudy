# Qoder 复杂任务能力测试

## 问题 1

你可以访问的工作目录为：TankGame，你的任务是把 Python 写的 TankGame 转为 C++ DirectX 写的游戏源码，转写过程中的代码功能分析、源码等，都输出到 TankGame-Qoder 下面

注意：请使用当前机器里面自带的 C++ 版本和工具，DirectX SDK 也是一样，如果没有 DirectX SDK，那么请按照顺序从 OpenGL、Vulkan、GDI、GDI+ 里面选择一个实现

## 回答 1

我先了解 Python 版 TankGame 的代码结构，并检查本机的 C++ 编译环境和图形 SDK 情况。
发现 TankGame 下是一个叫 TankWar 的 Pygame 项目。我先读取主程序源码和 README，同时检查本机编译环境与图形 SDK。
README 提到还有 tank_war.py、sprites.py、settings.py，但工作区里没看到，我检查一下 git 仓库状态，并探测本机编译器和图形 SDK。
仓库里其实有完整源码（settings.py / sprites.py / tank_war.py + tools）。环境方面：本机有 VS2026 + Windows SDK 10.0.26100（含 DirectX 11 头文件，需再确认路径）。我继续读 Python 源码和确认 SDK 头文件。
DirectX 11 全套（头文件+库）在 Windows SDK 10.0.26100 里是齐的，MSVC 19.51 可用。继续读剩余 Python 源码。

Python 源码全部读完，游戏逻辑已清楚。补一个环境细节确认（dxgi.h 位置），然后我需要和你确认图形方案的选择。
环境和源码都摸清了。有两个影响方案走向的问题需要确认。

## 交互 1

本机没有老版 DirectX SDK (June 2010)，但 VS2026 自带的 Windows SDK 10.0.26100 包含完整的 DirectX 11（d3d11.h/dxgi.h/d3dcompiler + 库文件）。图形方案选哪个？
DirectX 11（推荐）

原版 Python 代码有些明显的小瑕疵，转写时如何处理？例如：5 个敌方坦克全部出生在左上角 (0,0) 重叠、敌方子弹命中国家(boss)墙直接游戏结束等。
忠实复刻原版行为（推荐）

## 回答 2

两项都确认了：DirectX 11 + 忠实复刻。现在让 Plan 代理基于我掌握的源码分析和环境信息设计实现方案。
Plan 代理的方案已出，我审阅后修正了一个细节（Python 中墙/坦克死亡瞬间就从碰撞组移除，爆炸期不再参与碰撞），现在写最终计划。
计划已完成并写入[**计划文件**](vivid-stream-dory.md)：DirectX 11（Windows SDK 自带）+ WIC 贴图 + winmm 音频，1:1 忠实复刻原版行为，源码与三份分析文档全部输出到 TankGame-Qoder。

计划已批准。我创建任务清单并开始实现。
资源已就位。现在开始写源码，先是 settings.h、input、audio 基础模块。
基础模块完成，接下来是 D3D11 渲染器（窗口、交换链、着色器、WIC 贴图）。
渲染器完成。接下来是精灵类 sprites.h/cpp。
精灵类完成。现在写游戏主体 game.h/cpp 和 main.cpp。
游戏主体完成。写 build.bat 和 README，然后开始编译。

现在编译。
build.bat 的中文注释在 GBK 代码页下乱码导致解析失败。改成纯 ASCII 注释重写。
两处修复：renderer.h 缺少 wincodec.h 头文件，sprites.cpp 需要 Texture 完整定义。
编译通过了，只是缺 user32.lib 和 ole32.lib。补上链接库。
编译成功，零警告。现在启动游戏验证画面。

## 提问者说明此处实际效果

Qoder 反复运行了很多次，渲染都是错的：游戏导出的画面几乎全黑，只有一条橙色斜线（后来修改几次出现白色三角形，反正各种渲染错误；中途修改对一次，但地图背景色是白色的，和目标不符合；再后来是莫名其妙崩溃退出、子弹发射方向与炮管方向不一致，直接把红鹰干掉了）；此外，如何抓取 DirectX 渲染的平面，软件没有经验，常规方法是不可能抓取到的（后续修改了才抓到），此处已经暴露阿里系的 Qoder 在游戏引擎等较为复杂项目上的严重不足

## 回答 2 继续，这部分就是抓取验证游戏运行画面的部分

进程存活。截图核对画面。
PowerShell 5.1 按 GBK 读无 BOM 脚本导致中文标题匹配失败。改为按进程 PID 找窗口，不依赖标题字符串。
截到了 950x650 客户区。查看画面内容。
截图里游戏窗口被其他窗口遮挡了（拍到的是重叠的 IDE 界面）。改进脚本：先把游戏窗口置前再截。
窗口被 IDE 遮挡，改用 PrintWindow 直接从窗口渲染内容抓帧（不受遮挡影响），并输出标题 Unicode 码点核对。
游戏进程退出了——很可能被随机 AI 的流弹命中 boss 墙或英雄而正常结束（这正是游戏规则的退出条件），但也可能有问题。重新启动游戏并立刻抓帧验证。
标题码点其实是 UTF-16 字节被脚本按 ANSI 误读——真实标题就是「坦克大战」，没问题。看抓到的画面。
窗口标题正确，但客户区全黑——可能是 PrintWindow 抓不到 D3D 内容，也可能贴图真没加载。用 TOPMOST 把游戏窗口置顶后再用 CopyFromScreen 抓屏来区分。
第二次运行的进程也退出了。我写一个一体化脚本：启动游戏 → 立即置顶截屏 → 统计非黑像素 → 记录退出码，一次弄清「是否正常渲染」和「退出原因」。
有内容了——采样到 3641 个非黑像素（约 9.4%），游戏确实在渲染。查看截图核对画面。
截图描述不可靠，改用像素级抽查：直接读取地图关键位置（红墙/铁墙/boss墙/英雄/敌人出生点）的颜色。
像素抽查不 conclusive。我直接给游戏加一个 F12 截图功能（把 D3D 后备缓冲经 WIC 存成 PNG——这是游戏自身渲染内容的绝对证据），重建后再验证，同时记录退出码确认「退出」是正常游戏结束还是崩溃。
启动游戏，用 PostMessage 直接给游戏窗口发 F12，读取游戏自己导出的渲染画面。
screenshot.png 已生成（游戏自身导出的渲染画面）。查看它。
游戏导出的画面几乎全黑，只有一条橙色斜线——渲染有问题。先做像素级分析定位。
渲染内容异常：只有右下角一块橙色斜线内容。先用 GDI+ 解码源图片，确认图片本身的尺寸和颜色，再决定是图片问题还是管线问题。

## 回答 2 继续，此时上下文等出现问题，回复都是英语了

Back-buffer dump analysis is my ground truth, so let me re-read the game draw path and sprites to correlate the artifact.

The draw code looks right, but the F12 dump is taken before `Draw()` — i.e. after `Present` with `DXGI_SWAP_EFFECT_DISCARD`, where back-buffer contents are officially undefined. That alone could explain garbage. Let me check the remaining headers, then instrument: capture after Draw, and add a raw-D3D white-quad debug test to isolate the pipeline from the WIC path.

Code review of the game/sprites is clean — the one glaring flaw is in my instrumentation: F12 saves the back buffer before `Draw()`, i.e. after `Present` with `DXGI_SWAP_EFFECT_DISCARD`, where contents are officially undefined. That artifact may be stale garbage, not the live frame. I'll fix the capture timing and add a raw-D3D white/red quad test to isolate pipeline vs WIC, all in one build cycle.

Header is clean. Now the renderer.cpp changes: refactor DrawSprite into DrawQuad, add CreateSolidTexture, add a WIC-pixel debug dump, and refactor SaveScreenshot to share a WritePng helper.

Now add the WIC-pixel debug dump in LoadTexture and a shared WritePng helper.

Now wire up the flag-based screenshot timing and debug quads in game.h/game.cpp.

Now rebuild with the instrumentation.

One const-correctness fix — WIC wants a non-const pointer.

Build is clean. Now the drive script: launch, post F12 via PostMessage, then dump analysis.

Both dumps captured. Let me look at them directly.

The texture dump is correct — WIC decodes the GIF fine. But the screenshot shows the debug full-screen white quad clipped to a bottom-right triangle — every quad is being clipped by a diagonal. That's the root cause. Let me extract the exact clip boundary line from the dump to identify the transform error.

Exact data at last: white exists for columns 475–949 at y≥325 — the scene is being compressed into the bottom-right quadrant (the colored pixels are brick/iron/eagle wall colors at half scale). The transform is halving and dropping the translation. Definitive bisection: hardcode the NDC math in the vertex shader, bypassing the matrix/cbuffer entirely.

Now upgrade the analyzer to print an ASCII map of the frame.

Breakthrough. With the hardcoded VS the scene renders perfectly — the ASCII map matches MAP_ONE exactly: walls at correct cells, hero bottom-center, enemies wandering near the top, bullets in flight. So the vertex/WIC/pipeline path is fully exonerated; the bug is specifically in the matrix/cbuffer path. Now restore the matrix shader but with a manually-built column-major float[16] (no XMMatrix calls), plus a dump of the exact floats for verification.

The matrix values are correct, but the window wasn't found this run — the process may have exited early. Let me improve the drive script diagnostics and retry.

The manual matrix works — scene renders correctly. Now let me confirm the root cause by dumping what the XMMatrix path actually produced, to decide between keeping the manual matrix or understanding a deeper issue.

Root cause found. The matrix dump proves the XMMatrix data was correct all along — the manual and XMMatrix floats are transposes of each other, both valid upload orders. The real bug was the F12 screenshot timing: it captured the back buffer before `Draw()`, i.e. post-`Present` with `DXGI_SWAP_EFFECT_DISCARD`, where contents are undefined garbage — not the live frame. The rendering was never broken; my instrumentation was lying.

Now I'll restore the XMMatrix path and strip all debug scaffolding.

Now strip the debug quads from Game::Draw and the texture dump from LoadTexture.

Clean build and final visual verification with the debug code stripped.

Something's off — the XMMatrix data was proven correct, yet the render collapsed to a rank-1 line. Let me inspect the current file state carefully.

The XMMatrix data was proven identical-by-symmetry, yet the render differs — this needs a definitive look at the actual DirectXMath implementation on this machine.

## 回答 2 继续，此时恢复成中文了

把 BeginFrame 里的 XMMatrix 路径替换为已验证正确的列主序手动矩阵，然后重新编译验证。
已替换为手动矩阵。现在重新编译。
编译通过。运行游戏发 F12 截图验证画面。
画面已恢复完整渲染。直接查看截图确认细节。
截图已生成，画面完整。现在把 ASCII 图与 MAP_ONE 逐格核对。
截图内容已恢复完整画面。现在写一个逐格校验脚本，把地图每个非空格的中心像素与对应墙贴图逐一比对，并确认 hero/enemy 位置。
贴图尺寸确认：坦克 40×40、墙 50×50。写逐格校验脚本（地图非空格中心像素 vs 墙贴图中心像素精确比对 + 敌人/hero 色块扫描）。
墙布局 100/100 格与贴图逐像素完全一致。坦克像素未精确匹配是线性采样的 ±1 纹素混合所致，把检测改为容差匹配再扫。
PowerShell 5.1 按 ANSI 读取无 BOM 脚本，中文注释导致解析错误。把脚本改为纯 ASCII。
墙体 100/100 全部一致。但坦克色块扫描没找到——直接探测坦克位置的像素实际颜色来定位差异。
hero 与贴图逐像素完全一致（(375,630)=(249,224,93)=tex(20,20)）。用模板匹配确认左上角敌人与 hero 位置。
GetPixel 逐点比较太慢，改用 LockBits 直接字节比对。先停掉后台任务。
PowerShell 字节循环仍太慢，改为 Add-Type 内嵌 C# 做模板匹配。
找到两个敌人（DOWN 完美匹配 1102/1102 于 (2,12)，RIGHT 部分匹配于 (11,0)）。hero 扫描区域越界导致崩溃，修正区域并扩大敌人扫描范围。
hero 完美匹配 (355,610)，找到 2 个敌人，还差 3 个。读一下 game.cpp 的敌人更新顺序确认逻辑。
敌人初始方向是随机 0-3（game.cpp:105），game loop 里每帧先 HitWallTurn 再 Update。为把 5 个敌人散开后确认，改用一个延迟 10 秒再截图的驱动脚本。
10 秒截图验证通过：hero 100% 在 (355,610)，5 个敌人全部找到且像素级一致（含一个被草半遮住的）。接下来做交互与退出路径测试。先看爆炸特效用的贴图。
写 boss 墙命中测试：方向键移动 → 两发摧毁红墙 → 第三发命中 boss → 验证退出码 0。先确认按键处理。
写 boss 墙命中自动化测试脚本（方向键移动 + 三次开火 + 分阶段截图）。
退出码 0 ✓。写截图验证脚本核对 A/B/C 三张截图的状态变化。
发现问题：PostMessage 的按下+抬起在同一帧被处理，hero 根本没动（这是测试驱动的问题，不是游戏 bug——真实键盘输入两事件至少隔几帧）。先看 blast 贴图尺寸，再修测试脚本。
blast 是 40×40。先核对 Python 原版的 is_moving 语义（关系到同帧按键行为是否忠实），再修测试驱动。
Python 里 hero 仅在 `is_moving` 时更新——同帧按下+抬起不移动是忠实行为。确认 C++ 侧实现一致。
C++ 与 Python 一致（game.cpp:331 仅在 is_moving 时更新 hero）。修测试驱动：方向键按住 300ms 再抬起。
这次游戏没退出，说明第三发子弹没打到 boss。分析截图看实际状态。
hero 在奇怪地上移且仍朝上开火——方向键事件疑似没按预期处理。读 HandleEvents 完整代码。
直接查看 shot-C 截图，看 hero 和墙的实际状态。
截图描述显示左侧有爆炸、子弹横飞——需要精确像素数据。用 ASCII 图分析 shot-C。
需要看 WndProc 的完整按键处理代码，怀疑事件管道有问题。
WndProc 没问题。写一个细粒度按键探针测试，逐步截图观察 SPACE 和方向键的真实效果。
SPACE 开火正常（子弹可见）、UP 移动正常。看 RIGHT 的实际效果——加测 hero 的 x 范围。
MoveOutWall 的行为可疑，对照 Python 原版实现。

## 此处处发了压缩上下文

先重读 game.cpp 剩余部分和 sprites.cpp 关键逻辑，确认方向处理与子弹生成。
代码逻辑核对无误：`dirImages` 映射正确、keydown/keyup 方向处理正确、子弹按当前方向生成。直接看 run 2 留下的截图和脚本，确认当时实际发生了什么。
脚本逻辑正确。直接目视检查 run 2 的三张截图。
图片描述不够精确，用放大裁剪的方式直接检查 hero 区域。先看现有匹配脚本可复用的部分。
现在写一个决定性诊断脚本：对 shot-A/B/C 用 4 个 hero 模板逐一匹配（判定朝向），同时检查墙状态、子弹和爆炸动画。
扫描区域越界，修正三处边界。

## 问题 3

结束吧，快下班了，确认 6700s 你也完不成，保持这个状态吧
