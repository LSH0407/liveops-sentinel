@echo off
setlocal

echo === Quick Build Script ===

REM Change to project root
cd /d "%~dp0.."

REM Set vcpkg path
set VCPKG_ROOT=%CD%\.tools\vcpkg

echo VCPKG_ROOT=%VCPKG_ROOT%

REM Clean and create build directory
if exist build rmdir /s /q build
mkdir build
cd build

REM Configure
echo Configuring CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DDISABLE_GUI=ON -DENABLE_OBS=OFF -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo CMake configuration failed
    pause
    exit /b 1
)

REM Build
echo Building...
cmake --build . --config Release

if errorlevel 1 (
    echo Build failed
    pause
    exit /b 1
)

REM Check result
if exist Release\liveops_sentinel.exe (
    echo Build successful!
    echo Executable: %CD%\Release\liveops_sentinel.exe
    
    REM Copy to out directory
    if not exist ..\out mkdir ..\out
    copy Release\liveops_sentinel.exe ..\out\
    echo Copied to: ..\out\liveops_sentinel.exe
    
    REM Test run
    echo Testing executable...
    Release\liveops_sentinel.exe
) else (
    echo Executable not found
    pause
    exit /b 1
)

echo === Build Complete ===
pause
