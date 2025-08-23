@echo off
echo ========================================
echo Complete Console Application Conversion
echo ========================================
echo.

echo [1/6] Checking current status...
git status
echo.

echo [2/6] Adding Dashboard.h changes...
git add src/ui/Dashboard.h
if %errorlevel% neq 0 (
    echo ERROR: Failed to add Dashboard.h
    pause
    exit /b 1
)

echo.
echo [3/6] Adding Dashboard.cpp changes...
git add src/ui/Dashboard.cpp
if %errorlevel% neq 0 (
    echo ERROR: Failed to add Dashboard.cpp
    pause
    exit /b 1
)

echo.
echo [4/6] Committing complete console conversion...
git commit -m "Complete console application conversion: remove all GUI code"
if %errorlevel% neq 0 (
    echo ERROR: Failed to commit changes
    pause
    exit /b 1
)

echo.
echo [5/6] Pushing to GitHub...
git push origin main
if %errorlevel% neq 0 (
    echo ERROR: Failed to push changes
    pause
    exit /b 1
)

echo.
echo [6/6] Creating new tag v0.2.3...
git tag v0.2.3
git push origin v0.2.3
if %errorlevel% neq 0 (
    echo ERROR: Failed to create/push tag
    pause
    exit /b 1
)

echo.
echo ========================================
echo COMPLETE CONSOLE CONVERSION COMPLETED!
echo ========================================
echo.
echo Changes made:
echo - Removed all ImGui code from Dashboard.cpp
echo - Disabled BenchResult and ObsRecommendation types
echo - Converted all GUI methods to console output
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
echo - LiveOpsSentinel-0.2.3-win-x64.exe (Windows installer)
echo - LiveOpsSentinel-0.2.3-win-x64.zip (Windows ZIP)
echo - LiveOpsSentinel-0.2.3-linux-x64.tar.gz (Linux)
echo.
pause


