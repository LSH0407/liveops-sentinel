@echo off
echo ========================================
echo Fixing Release v0.2.1
echo ========================================
echo.

echo [1/6] Deleting existing tag locally...
git tag -d v0.2.1
if %errorlevel% neq 0 (
    echo WARNING: Tag deletion failed (might not exist locally)
)

echo.
echo [2/6] Deleting existing tag on GitHub...
git push origin --delete v0.2.1
if %errorlevel% neq 0 (
    echo WARNING: Remote tag deletion failed (might not exist remotely)
)

echo.
echo [3/6] Creating new tag locally...
git tag v0.2.1
if %errorlevel% neq 0 (
    echo ERROR: Failed to create tag
    pause
    exit /b 1
)

echo.
echo [4/6] Pushing new tag to GitHub...
git push origin v0.2.1
if %errorlevel% neq 0 (
    echo ERROR: Failed to push tag
    pause
    exit /b 1
)

echo.
echo [5/6] Checking GitHub Actions...
echo Please check: https://github.com/LSH0407/liveops-sentinel/actions
echo.

echo [6/6] Checking Releases...
echo Please check: https://github.com/LSH0407/liveops-sentinel/releases
echo.

echo ========================================
echo RELEASE FIX COMPLETED!
echo ========================================
echo.
echo GitHub Actions will now automatically:
echo - Build the project
echo - Create packages (ZIP, TGZ)
echo - Generate GitHub Release with Assets
echo.
echo Check progress at the links above.
echo.
pause
