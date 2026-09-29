#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <cwchar>

#include "Game.h"

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Game* game = reinterpret_cast<Game*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_KEYDOWN:
        if (game) game->OnKeyDown(wParam, (lParam & 0x40000000) != 0);
        return 0;
    case WM_KEYUP:
        if (game) game->OnKeyUp(wParam);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    // WIC 需要 COM
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // 将工作目录设置为可执行文件所在目录，保证 resources 相对路径正确
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    wchar_t* sep = std::wcsrchr(exePath, L'\\');
    if (sep) *sep = L'\0';
    SetCurrentDirectoryW(exePath);

    const int GAME_W = 950;
    const int GAME_H = 650;

    const wchar_t* kClassName = L"TankGameWindowClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kClassName;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    // 固定尺寸窗口（不可缩放），与 pygame 窗口一致
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT rc = { 0, 0, GAME_W, GAME_H };
    AdjustWindowRect(&rc, style, FALSE);

    HWND hwnd = CreateWindowExW(
        0, kClassName, L"坦克大战 (DirectX 11)",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
    {
        MessageBoxW(nullptr, L"创建窗口失败。", L"坦克大战", MB_ICONERROR);
        CoUninitialize();
        return 1;
    }

    Game game;
    if (!game.Init(hwnd))
    {
        DestroyWindow(hwnd);
        CoUninitialize();
        return 1;
    }

    // 将 Game 指针存入窗口，供窗口过程转发键盘消息
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&game));

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    game.Run();

    game.Shutdown();
    DestroyWindow(hwnd);
    CoUninitialize();
    return 0;
}
