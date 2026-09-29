@echo off
rem Load MSVC x64 environment, then run the build script in Git Bash.
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1

cd /d "%~dp0"

rem 定位 Git Bash：先查 PATH，再查常见安装位置（不硬编码用户目录）
set "BASH="
for %%I in (bash.exe) do set "BASH=%%~$PATH:I"
if not defined BASH if exist "%ProgramFiles%\Git\bin\bash.exe" set "BASH=%ProgramFiles%\Git\bin\bash.exe"
if not defined BASH if exist "%LocalAppData%\Programs\Git\bin\bash.exe" set "BASH=%LocalAppData%\Programs\Git\bin\bash.exe"
if not defined BASH if exist "C:\msys64\usr\bin\bash.exe" set "BASH=C:\msys64\usr\bin\bash.exe"
if not defined BASH (
    echo [ERROR] Git Bash not found. Install Git for Windows or add bash.exe to PATH.
    exit /b 1
)

rem 用脚本所在目录（相对当前工程）驱动构建，路径随工程位置自适应
set "HERE=%~dp0"
set "HERE=%HERE:\=/%"

"%BASH%" -lc "cd '%HERE%' && ./build-x64-msvc.sh"
exit /b %errorlevel%
