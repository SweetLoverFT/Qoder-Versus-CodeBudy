@echo off
rem TankWar C++ DirectX 11 build script (MSVC x64)
rem Run from the TankGame-Qoder directory: resource paths are relative
setlocal
cd /d "%~dp0"

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 (
    echo [ERROR] vcvars64.bat failed. Check the Visual Studio installation path.
    exit /b 1
)

cl /nologo /EHsc /std:c++17 /utf-8 /O2 /W4 /DUNICODE /D_UNICODE ^
   src\*.cpp ^
   /Fe:TankWar.exe ^
   /link d3d11.lib d3dcompiler.lib dxgi.lib windowscodecs.lib winmm.lib user32.lib ole32.lib
if errorlevel 1 (
    echo [ERROR] Compilation failed
    exit /b 1
)

echo.
echo [OK] Build complete: TankWar.exe
echo Run it from this directory (double-click or command line).
endlocal
