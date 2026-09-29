// 对应 Python 版 main.py：程序入口
#include "game.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    Game game;
    return game.Run(hInstance);
}
