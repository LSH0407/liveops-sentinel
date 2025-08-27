@echo off
echo ========================================
echo Remove SDL2 and Convert to Console App
echo ========================================
echo.

echo [1/7] Checking current status...
git status
echo.

echo [2/7] Adding CMakeLists.txt changes...
git add CMakeLists.txt
if %errorlevel% neq 0 (
    echo ERROR: Failed to add CMakeLists.txt
    pause
    exit /b 1
)

echo.
echo [3/7] Adding GitHub Actions changes...
git add .github/workflows/release.yml
if %errorlevel% neq 0 (
    echo ERROR: Failed to add workflow file
    pause
    exit /b 1
)

echo.
echo [4/7] Committing SDL2 removal...
git commit -m "Remove SDL2 and imgui dependencies - convert to console application"
if %errorlevel% neq 0 (
    echo ERROR: Failed to commit changes
    pause
    exit /b 1
)

echo.
echo [5/7] Pushing to GitHub...
git push origin main
if %errorlevel% neq 0 (
    echo ERROR: Failed to push changes
    pause
    exit /b 1
)

echo.
echo [6/7] Deleting existing tag...
git tag -d v0.2.1
git push origin --delete v0.2.1
if %errorlevel% neq 0 (
    echo WARNING: Tag deletion failed (might not exist)
)

echo.
echo [7/7] Creating new tag v0.2.1...
git tag v0.2.1
git push origin v0.2.1
if %errorlevel% neq 0 (
    echo ERROR: Failed to create/push tag
    pause
    exit /b 1
)

echo.
echo ========================================
echo SDL2 REMOVAL COMPLETED!
echo ========================================
echo.
echo Changes made:
echo - Removed SDL2 and imgui dependencies
echo - Simplified system dependencies
echo - Converted to console application
echo - Fixed autoreconf errors by removing problematic dependencies
echo.
echo GitHub Actions will now:
echo - Install only stable dependencies (asio, nlohmann-json, spdlog)
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







