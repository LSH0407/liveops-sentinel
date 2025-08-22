@echo off
echo ========================================
echo Creating Release v0.2.1
echo ========================================
echo.

echo [1/4] Creating tag locally...
git tag v0.2.1
if %errorlevel% neq 0 (
    echo ERROR: Failed to create tag
    pause
    exit /b 1
)

echo.
echo [2/4] Pushing tag to GitHub...
git push origin v0.2.1
if %errorlevel% neq 0 (
    echo ERROR: Failed to push tag
    pause
    exit /b 1
)

echo.
echo [3/4] Checking GitHub Actions...
echo Please check: https://github.com/LSH0407/liveops-sentinel/actions
echo.

echo [4/4] Checking Releases...
echo Please check: https://github.com/LSH0407/liveops-sentinel/releases
echo.

echo ========================================
echo RELEASE CREATION COMPLETED!
echo ========================================
echo.
echo GitHub Actions will now automatically:
echo - Build the project
echo - Create packages
echo - Generate GitHub Release
echo.
echo Check progress at the links above.
echo.
pause
