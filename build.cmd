@echo off
rem One-click build of OSP2-BE (MSYS2 MinGW64 + Ninja).
rem   build.cmd        release: build-mingw\Release\osp\zz-osp-pak8be.pk3
rem   build.cmd debug  with cg_debugServer (CG_DEBUG_SERVER):
rem                    build-mingw-debug\Release\osp\zz-osp-pak8be.pk3
setlocal
cd /d "%~dp0"
set "PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%"

set "BUILD_DIR=build-mingw"
set "DEBUG_OPT=OFF"
if /i "%~1"=="debug" (
    set "BUILD_DIR=build-mingw-debug"
    set "DEBUG_OPT=ON"
)

if not exist "%BUILD_DIR%\build.ninja" (
    cmake -S . -B %BUILD_DIR% -G Ninja -DCMAKE_BUILD_TYPE=Release -DOSP2BE_DEBUG=%DEBUG_OPT%
    if errorlevel 1 goto :fail
)

ninja -C %BUILD_DIR%
if errorlevel 1 goto :fail

echo.
echo OK: %BUILD_DIR%\Release\osp\zz-osp-pak8be.pk3
pause
exit /b 0

:fail
echo.
echo BUILD FAILED
pause
exit /b 1
