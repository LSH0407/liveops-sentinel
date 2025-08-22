@echo off
echo ========================================
echo LiveOps Sentinel Deployment Script
echo ========================================
echo.

echo [1/5] Checking Git status...
git status
if %errorlevel% neq 0 (
    echo ERROR: Git command failed
    pause
    exit /b 1
)

echo.
echo [2/5] Adding all changes...
git add .
if %errorlevel% neq 0 (
    echo ERROR: Git add failed
    pause
    exit /b 1
)

echo.
echo [3/5] Committing changes...
git commit -m "Update release pipeline and dependencies"
if %errorlevel% neq 0 (
    echo ERROR: Git commit failed
    pause
    exit /b 1
)

echo.
echo [4/5] Pushing to GitHub...
git push origin main
if %errorlevel% neq 0 (
    echo ERROR: Git push failed
    pause
    exit /b 1
)

echo.
echo [5/5] Creating and pushing tag...
git tag v0.2.1
if %errorlevel% neq 0 (
    echo ERROR: Git tag failed
    pause
    exit /b 1
)

git push origin v0.2.1
if %errorlevel% neq 0 (
    echo ERROR: Git push tag failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo DEPLOYMENT COMPLETED SUCCESSFULLY!
echo ========================================
echo.
echo GitHub Actions will now automatically:
echo - Build the project for Windows and Linux
echo - Create release packages
echo - Generate GitHub Release with downloads
echo.
echo Check progress at: https://github.com/LSH0407/liveops-sentinel/actions
echo.
pause
