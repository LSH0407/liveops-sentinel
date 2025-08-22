@echo off
echo ========================================
echo LiveOps Sentinel Local Build Script
echo ========================================
echo.

echo [1/4] Installing vcpkg dependencies...
C:\vcpkg\vcpkg install
if %errorlevel% neq 0 (
    echo ERROR: vcpkg install failed
    pause
    exit /b 1
)

echo.
echo [2/4] Running PowerShell build script...
powershell -ExecutionPolicy Bypass -File scripts\build_release_win.ps1 -SkipTests -SkipPackage
if %errorlevel% neq 0 (
    echo ERROR: Build script failed
    pause
    exit /b 1
)

echo.
echo [3/4] Checking build results...
if exist "dist\liveops_sentinel.exe" (
    echo SUCCESS: Executable created at dist\liveops_sentinel.exe
    dir dist\liveops_sentinel.exe
) else (
    echo ERROR: Executable not found
    pause
    exit /b 1
)

echo.
echo [4/4] Listing dist directory contents...
dir dist

echo.
echo ========================================
echo LOCAL BUILD COMPLETED SUCCESSFULLY!
echo ========================================
echo.
echo You can now run: dist\liveops_sentinel.exe
echo.
pause
