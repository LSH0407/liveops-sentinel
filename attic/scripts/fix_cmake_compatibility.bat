@echo off
echo ========================================
echo Fix CMake Compatibility and Create Release
echo ========================================
echo.

echo [1/6] Checking current status...
git status
echo.

echo [2/6] Adding changes...
git add CMakeLists.txt
if %errorlevel% neq 0 (
    echo ERROR: Failed to add changes
    pause
    exit /b 1
)

echo.
echo [3/6] Committing CMake compatibility fix...
git commit -m "Fix CMake compatibility - remove duplicate install rules"
if %errorlevel% neq 0 (
    echo ERROR: Failed to commit changes
    pause
    exit /b 1
)

echo.
echo [4/6] Pushing to GitHub...
git push origin main
if %errorlevel% neq 0 (
    echo ERROR: Failed to push changes
    pause
    exit /b 1
)

echo.
echo [5/6] Deleting existing tag...
git tag -d v0.2.1
git push origin --delete v0.2.1
if %errorlevel% neq 0 (
    echo WARNING: Tag deletion failed (might not exist)
)

echo.
echo [6/6] Creating new tag v0.2.1...
git tag v0.2.1
git push origin v0.2.1
if %errorlevel% neq 0 (
    echo ERROR: Failed to create/push tag
    pause
    exit /b 1
)

echo.
echo ========================================
echo RELEASE CREATION COMPLETED!
echo ========================================
echo.
echo Changes made:
echo - Removed duplicate install(TARGETS) in CMakeLists.txt
echo - Fixed CMake compatibility issues
echo.
echo GitHub Actions will now:
echo - Build the project on both Windows and Linux
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

