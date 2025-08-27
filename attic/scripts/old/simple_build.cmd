@echo off
setlocal

echo === Simple Build Script ===

REM Set vcpkg path - prefer local vcpkg
set VCPKG_ROOT=%~dp0..\.tools\vcpkg
if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo Local vcpkg not found, trying global...
    set VCPKG_ROOT=C:\vcpkg
    if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
        echo No vcpkg found. Please run setup first.
        exit /b 1
    )
)

echo VCPKG_ROOT=%VCPKG_ROOT%

REM Clean and create build directory
if exist build rmdir /s /q build
mkdir build
cd build

REM Configure
echo Configuring CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DDISABLE_GUI=ON -DENABLE_OBS=OFF -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo CMake configuration failed
    echo Checking vcpkg log...
    if exist vcpkg-manifest-install.log type vcpkg-manifest-install.log
    exit /b 1
)

REM Build
echo Building...
cmake --build . --config Release --parallel

if errorlevel 1 (
    echo Build failed
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
    exit /b 1
)

echo === Build Complete ===
