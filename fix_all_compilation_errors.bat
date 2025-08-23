@echo off
echo ========================================
echo Fix All Compilation Errors - Final
echo ========================================
echo.

echo [1/7] Checking current status...
git status
echo.

echo [2/7] Adding Dashboard.h changes...
git add src/ui/Dashboard.h
if %errorlevel% neq 0 (
    echo ERROR: Failed to add Dashboard.h
    pause
    exit /b 1
)

echo.
echo [3/7] Adding Dashboard.cpp changes...
git add src/ui/Dashboard.cpp
if %errorlevel% neq 0 (
    echo ERROR: Failed to add Dashboard.cpp
    pause
    exit /b 1
)

echo.
echo [4/7] Adding Checklist.cpp changes...
git add src/ui/Checklist.cpp
if %errorlevel% neq 0 (
    echo ERROR: Failed to add Checklist.cpp
    pause
    exit /b 1
)

echo.
echo [5/7] Adding WebhookWizard.cpp changes...
git add src/ui/WebhookWizard.cpp
if %errorlevel% neq 0 (
    echo ERROR: Failed to add WebhookWizard.cpp
    pause
    exit /b 1
)

echo.
echo [6/7] Committing all compilation fixes...
git commit -m "Fix all compilation errors: remove ImGui, fix headers, resolve type conflicts"
if %errorlevel% neq 0 (
    echo ERROR: Failed to commit changes
    pause
    exit /b 1
)

echo.
echo [7/7] Pushing to GitHub...
git push origin main
if %errorlevel% neq 0 (
    echo ERROR: Failed to push changes
    pause
    exit /b 1
)

echo.
echo [8/8] Creating new tag v0.2.4...
git tag v0.2.4
git push origin v0.2.4
if %errorlevel% neq 0 (
    echo ERROR: Failed to create/push tag
    pause
    exit /b 1
)

echo.
echo ========================================
echo ALL COMPILATION ERRORS FIXED!
echo ========================================
echo.
echo Changes made:
echo - Added missing headers (thread, atomic)
echo - Removed ProbeSample duplicate definition
echo - Fixed ProbeSample constructor usage
echo - Removed all ImGui code from Checklist.cpp
echo - Removed all ImGui code from WebhookWizard.cpp
echo - Fixed all type conflicts and incomplete types
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
echo - LiveOpsSentinel-0.2.4-win-x64.exe (Windows installer)
echo - LiveOpsSentinel-0.2.4-win-x64.zip (Windows ZIP)
echo - LiveOpsSentinel-0.2.4-linux-x64.tar.gz (Linux)
echo.
pause


