@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

set "GAME_DIR=%~dp0"
set "OSP2_URL=https://konstalker.github.io/assets/zz-osp-pak8be.pk3"
set "OSP2_FILE=!GAME_DIR!osp\zz-osp-pak8be.pk3"
set "VERSION_URL=https://konstalker.github.io/assets/version.txt"
set "CHANGELOG_URL=https://konstalker.github.io/assets/version_changelog.txt"
set "TEMP_VERSION=%TEMP%\osp2_version.txt"
set "TEMP_CHANGELOG=%TEMP%\osp2_changelog.txt"
set "EXIT_CODE=0"

title OSP2-BE updater

echo ==========================================
echo            OSP2-BE update check
echo ==========================================
echo.

echo Fetching remote version...
curl -L --retry 3 --fail --silent -o "!TEMP_VERSION!" "!VERSION_URL!"
if errorlevel 1 (
    echo [ERROR] Failed to download version.txt from the server.
    echo         Check your connection: !VERSION_URL!
    set "EXIT_CODE=1"
    goto cleanup
)
if not exist "!TEMP_VERSION!" (
    echo [ERROR] Version file was not saved after download.
    set "EXIT_CODE=1"
    goto cleanup
)

set "REMOTE_VER="
for /f "usebackq tokens=* delims=" %%a in ("!TEMP_VERSION!") do set "REMOTE_VER=%%a"
set "REMOTE_VER=!REMOTE_VER: =!"
if not defined REMOTE_VER (
    echo [ERROR] Remote version file is empty or unreadable.
    set "EXIT_CODE=1"
    goto cleanup
)

echo Remote version: !REMOTE_VER!
echo.

curl -L --retry 3 --silent -o "!TEMP_CHANGELOG!" "!CHANGELOG_URL!"
if errorlevel 1 (
    echo [WARNING] Failed to download changelog.
) else (
    if exist "!TEMP_CHANGELOG!" (
        echo Changelog:
        type "!TEMP_CHANGELOG!"
        echo.
    )
)

set "LOCAL_VER="
call :ReadVersionFromPk3

if exist "!OSP2_FILE!" (
    if defined LOCAL_VER (
        echo Installed version in pk3: !LOCAL_VER!
    ) else (
        echo [WARNING] Package found but version.txt inside pk3 could not be read.
        echo           Will download the latest package.
    )
) else (
    echo Local package not found:
    echo !OSP2_FILE!
    echo First-time download.
)
echo.

if defined LOCAL_VER (
    if "!LOCAL_VER!"=="!REMOTE_VER!" (
        echo [OK] You already have the latest version: !LOCAL_VER!
        echo      No update needed.
        goto cleanup
    )
    echo New version available: !REMOTE_VER!  yours: !LOCAL_VER!
) else (
    echo Will download version: !REMOTE_VER!
)
echo.

if not exist "!GAME_DIR!osp\" (
    mkdir "!GAME_DIR!osp\" 2>nul
    if errorlevel 1 (
        echo [ERROR] Failed to create the osp folder.
        set "EXIT_CODE=1"
        goto cleanup
    )
)

echo Downloading OSP2-BE package...
curl -L --retry 3 --fail --progress-bar -o "!OSP2_FILE!" "!OSP2_URL!"
if errorlevel 1 (
    echo.
    echo [ERROR] Failed to download zz-osp-pak8be.pk3
    echo         URL: !OSP2_URL!
    set "EXIT_CODE=1"
    goto cleanup
)

echo.
echo [OK] OSP2-BE updated successfully to version !REMOTE_VER!
goto cleanup

:ReadVersionFromPk3
set "LOCAL_VER="
if not exist "!OSP2_FILE!" exit /b 0
set "OSP2_PK3=!OSP2_FILE!"
for /f "usebackq delims=" %%V in (`powershell -NoProfile -Command "try { Add-Type -AssemblyName System.IO.Compression.FileSystem; $p=[IO.Path]::GetFullPath($env:OSP2_PK3); $z=[IO.Compression.ZipFile]::OpenRead($p); $e=$z.GetEntry('version.txt'); if ($null -ne $e) { $sr=New-Object IO.StreamReader($e.Open()); $line=$sr.ReadLine(); $sr.Close(); if ($line) { $line.Trim() } }; $z.Dispose() } catch { '' }"`) do set "LOCAL_VER=%%V"
set "OSP2_PK3="
exit /b 0

:cleanup
del "!TEMP_VERSION!" 2>nul
del "!TEMP_CHANGELOG!" 2>nul
echo.
echo Done. Exit code: !EXIT_CODE!
pause
exit /b !EXIT_CODE!
