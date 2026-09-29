@echo off
rem Compile and run decoder availability check against the built MSVC libs.
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cd /d "%~dp0verify"
cl /nologo verify-codecs.c /I ..\ffmpeg-x64-msvc\include /Fe:verify-codecs.exe ..\ffmpeg-x64-msvc\bin\avcodec.lib ..\ffmpeg-x64-msvc\bin\avutil.lib
if errorlevel 1 exit /b 1
verify-codecs.exe
exit /b %errorlevel%
