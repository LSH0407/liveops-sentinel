@echo off
echo ========================================
echo Convert to Console Application
echo ========================================
echo.

echo [1/6] Checking current status...
git status
echo.

echo [2/6] Adding Dashboard.cpp changes...
git add src/ui/Dashboard.cpp
if %errorlevel% neq 0 (
    echo ERROR: Failed to add Dashboard.cpp
    pause
    exit /b 1
)

echo.
echo [3/6] Adding .gitignore changes...
git add .gitignore
if %errorlevel% neq 0 (
    echo ERROR: Failed to add .gitignore
    pause
    exit /b 1
)

echo.
echo [4/6] Committing console application conversion...
git commit -m "Convert to console application: remove GUI dependencies"
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
echo [6/6] Creating new tag v0.2.2...
git tag v0.2.2
git push origin v0.2.2
if %errorlevel% neq 0 (
    echo ERROR: Failed to create/push tag
    pause
    exit /b 1
)

echo.
echo ========================================
echo CONSOLE APPLICATION CONVERSION COMPLETED!
echo ========================================
echo.
echo Changes made:
echo - Converted Dashboard to console output
echo - Removed all GUI dependencies
echo - Fixed compilation errors
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
echo - LiveOpsSentinel-0.2.2-win-x64.exe (Windows installer)
echo - LiveOpsSentinel-0.2.2-win-x64.zip (Windows ZIP)
echo - LiveOpsSentinel-0.2.2-linux-x64.tar.gz (Linux)
echo.
pause


