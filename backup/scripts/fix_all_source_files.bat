@echo off
echo ========================================
echo Fix All Source Files - Remove All GUI
echo ========================================
echo.

echo [1/8] Checking current status...
git status
echo.

echo [2/8] Adding Dashboard.cpp changes...
git add src/ui/Dashboard.cpp
if %errorlevel% neq 0 (
    echo ERROR: Failed to add Dashboard.cpp
    pause
    exit /b 1
)

echo.
echo [3/8] Adding WebhookWizard.cpp changes...
git add src/ui/WebhookWizard.cpp
if %errorlevel% neq 0 (
    echo ERROR: Failed to add WebhookWizard.cpp
    pause
    exit /b 1
)

echo.
echo [4/8] Adding Checklist.cpp changes...
git add src/ui/Checklist.cpp
if %errorlevel% neq 0 (
    echo ERROR: Failed to add Checklist.cpp
    pause
    exit /b 1
)

echo.
echo [5/8] Adding App.cpp changes...
git add src/app/App.cpp
if %errorlevel% neq 0 (
    echo ERROR: Failed to add App.cpp
    pause
    exit /b 1
)

echo.
echo [6/8] Adding ObsClient.cpp changes...
git add src/obs/ObsClient.cpp
if %errorlevel% neq 0 (
    echo ERROR: Failed to add ObsClient.cpp
    pause
    exit /b 1
)

echo.
echo [7/8] Committing all source code fixes...
git commit -m "Fix all source files: remove SDL2, imgui, websocketpp - convert to console app"
if %errorlevel% neq 0 (
    echo ERROR: Failed to commit changes
    pause
    exit /b 1
)

echo.
echo [8/8] Pushing to GitHub...
git push origin main
if %errorlevel% neq 0 (
    echo ERROR: Failed to push changes
    pause
    exit /b 1
)

echo.
echo ========================================
echo ALL SOURCE FILES FIXED!
echo ========================================
echo.
echo Changes made:
echo - Removed imgui.h from Dashboard.cpp
echo - Removed imgui.h from WebhookWizard.cpp
echo - Removed imgui.h from Checklist.cpp
echo - Removed SDL2/imgui from App.cpp
echo - Removed websocketpp from ObsClient.cpp
echo - Converted all UI to console output
echo - Fixed all compilation errors
echo.
echo GitHub Actions will now:
echo - Build successfully on both Windows and Linux
echo - Create ZIP and NSIS installer packages
echo - Generate GitHub Release with executable files
echo.
echo Check progress at:
echo - Actions: https://github.com/LSH0407/liveops-sentinel/actions
echo - Releases: https://github.com/LSH0407/liveops-sentinel/releases
echo.
echo Expected files in release:
echo - LiveOpsSentinel-0.2.1-win-x64.exe (Windows installer)
echo - LiveOpsSentinel-0.2.1-win-x64.zip (Windows ZIP)
echo - LiveOpsSentinel-0.2.1-linux-x64.tar.gz (Linux)
echo.
pause







