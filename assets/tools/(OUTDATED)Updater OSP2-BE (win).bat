@echo off
setlocal

:: Define URLs and paths
set "OSP2_URL=https://q3a.space/osp2/assets/zz-osp-pak8be.pk3"
set "OSP2_FILE=%~dp0osp\zz-osp-pak8be.pk3"
set "VERSION_URL=https://q3a.space/osp2/assets/version.txt"
set "CHANGELOG_URL=https://q3a.space/osp2/assets/version_changelog.txt"
set "TEMP_VERSION=%TEMP%\osp2_version.txt"
set "TEMP_CHANGELOG=%TEMP%\osp2_changelog.txt"

cls
echo ==========================================
echo             UPDATING OSP2
echo ==========================================
echo.

:: Download version and changelog
echo Downloading version information...
curl -L --retry 3 --silent -o "%TEMP_VERSION%" "%VERSION_URL%"
curl -L --retry 3 --silent -o "%TEMP_CHANGELOG%" "%CHANGELOG_URL%"

:: Display version and changelog
if exist "%TEMP_VERSION%" (
    echo.
    echo Current version:
    type "%TEMP_VERSION%"
    echo.
)

if exist "%TEMP_CHANGELOG%" (
    echo Changelog:
    type "%TEMP_CHANGELOG%"
    echo.
)

:: Ensure the target directory exists
if not exist "%~dp0osp" (
    echo Creating directory "%~dp0osp"... 
    mkdir "%~dp0osp"
)

:: Download the OSP2 file
echo Downloading OSP2 package...
curl -L --retry 3 --progress-bar -o "%OSP2_FILE%" "%OSP2_URL%"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: OSP2 download failed!
    del "%TEMP_VERSION%" 2>nul
    del "%TEMP_CHANGELOG%" 2>nul
    pause
    exit /b %ERRORLEVEL%
)

:: Clean up temporary files
del "%TEMP_VERSION%" 2>nul
del "%TEMP_CHANGELOG%" 2>nul

echo.
echo OSP2 updated successfully!
pause
exit /b 0
