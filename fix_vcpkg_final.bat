@echo off
echo ========================================
echo Fix vcpkg Git Commit ID and Create Release
echo ========================================
echo.

echo [1/7] Adding all changes...
git add .
if %errorlevel% neq 0 (
    echo ERROR: Git add failed
    pause
    exit /b 1
)

echo.
echo [2/7] Committing vcpkg fix...
git commit -m "Fix vcpkg Git commit ID to use proper SHA1 hash"
if %errorlevel% neq 0 (
    echo ERROR: Git commit failed
    pause
    exit /b 1
)

echo.
echo [3/7] Pushing changes...
git push origin main
if %errorlevel% neq 0 (
    echo ERROR: Git push failed
    pause
    exit /b 1
)

echo.
echo [4/7] Deleting existing tag locally...
git tag -d v0.2.1
if %errorlevel% neq 0 (
    echo WARNING: Tag deletion failed (might not exist locally)
)

echo.
echo [5/7] Deleting existing tag on GitHub...
git push origin --delete v0.2.1
if %errorlevel% neq 0 (
    echo WARNING: Remote tag deletion failed (might not exist remotely)
)

echo.
echo [6/7] Creating new tag...
git tag v0.2.1
if %errorlevel% neq 0 (
    echo ERROR: Failed to create tag
    pause
    exit /b 1
)

echo.
echo [7/7] Pushing new tag...
git push origin v0.2.1
if %errorlevel% neq 0 (
    echo ERROR: Failed to push tag
    pause
    exit /b 1
)

echo.
echo ========================================
echo VCPKG FIX COMPLETED!
echo ========================================
echo.
echo Changes made:
echo - Fixed vcpkg Git commit ID from '2024.01.12' to proper SHA1 hash
echo - Fixed YAML indentation issues
echo - Both Windows and Linux builds should now work
echo.
echo GitHub Actions will now:
echo - Successfully setup vcpkg with correct commit ID
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
