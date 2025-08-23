@echo off
echo ========================================
echo Fix Source Code - Remove SDL2/WebSocket
echo ========================================
echo.

echo [1/7] Checking current status...
git status
echo.

echo [2/7] Adding App.h changes...
git add src/app/App.h
if %errorlevel% neq 0 (
    echo ERROR: Failed to add App.h
    pause
    exit /b 1
)

echo.
echo [3/7] Adding ObsClient.h changes...
git add src/obs/ObsClient.h
if %errorlevel% neq 0 (
    echo ERROR: Failed to add ObsClient.h
    pause
    exit /b 1
)

echo.
echo [4/7] Adding ReportWriter.cpp changes...
git add src/core/ReportWriter.cpp
if %errorlevel% neq 0 (
    echo ERROR: Failed to add ReportWriter.cpp
    pause
    exit /b 1
)

echo.
echo [5/7] Committing source code fixes...
git commit -m "Fix source code: remove SDL2 and websocketpp references"
if %errorlevel% neq 0 (
    echo ERROR: Failed to commit changes
    pause
    exit /b 1
)

echo.
echo [6/7] Pushing to GitHub...
git push origin main
if %errorlevel% neq 0 (
    echo ERROR: Failed to push changes
    pause
    exit /b 1
)

echo.
echo [7/7] Creating new tag v0.2.1...
git tag -d v0.2.1
git push origin --delete v0.2.1
git tag v0.2.1
git push origin v0.2.1
if %errorlevel% neq 0 (
    echo ERROR: Failed to create/push tag
    pause
    exit /b 1
)

echo.
echo ========================================
echo SOURCE CODE FIXES COMPLETED!
echo ========================================
echo.
echo Changes made:
echo - Removed SDL2 includes from App.h
echo - Removed websocketpp includes from ObsClient.h
echo - Added missing regex include to ReportWriter.cpp
echo - Converted to console application
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


