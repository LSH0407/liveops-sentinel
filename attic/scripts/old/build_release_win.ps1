#!/usr/bin/env pwsh
# LiveOps Sentinel Windows Release Build Script
# Usage: .\scripts\build_release_win.ps1

param(
    [string]$VcpkgRoot = $env:VCPKG_ROOT,
    [string]$BuildType = "Release",
    [switch]$SkipTests,
    [switch]$SkipPackage,
    [switch]$Clean
)

Write-Host "=== LiveOps Sentinel Windows Release Build ===" -ForegroundColor Green

# Check if we're in the right directory
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Error "CMakeLists.txt not found. Please run this script from the project root."
    exit 1
}

# Detect vcpkg root
if (-not $VcpkgRoot) {
    $possiblePaths = @(
        "C:\vcpkg",
        "$env:USERPROFILE\vcpkg",
        "$env:LOCALAPPDATA\vcpkg"
    )
    
    foreach ($path in $possiblePaths) {
        if (Test-Path "$path\scripts\buildsystems\vcpkg.cmake") {
            $VcpkgRoot = $path
            break
        }
    }
    
    if (-not $VcpkgRoot) {
        Write-Error "vcpkg not found. Please set VCPKG_ROOT environment variable or install vcpkg."
        Write-Host "Install vcpkg: git clone https://github.com/Microsoft/vcpkg.git && .\vcpkg\bootstrap-vcpkg.bat" -ForegroundColor Yellow
        exit 1
    }
}

Write-Host "Using vcpkg root: $VcpkgRoot" -ForegroundColor Cyan

# Clean build directory if requested
if ($Clean -and (Test-Path "build")) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "build"
}

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Configure CMake
Write-Host "Configuring CMake..." -ForegroundColor Cyan
$cmakeArgs = @(
    "-B", "build",
    "-S", ".",
    "-DCMAKE_TOOLCHAIN_FILE=$VcpkgRoot\scripts\buildsystems\vcpkg.cmake",
    "-DCMAKE_BUILD_TYPE=$BuildType"
)

$cmakeResult = & cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed"
    exit 1
}

# Build
Write-Host "Building project..." -ForegroundColor Cyan
$buildArgs = @(
    "--build", "build",
    "--config", $BuildType,
    "--parallel"
)

$buildResult = & cmake @buildArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed"
    exit 1
}

# Run tests (if not skipped)
if (-not $SkipTests) {
    Write-Host "Running tests..." -ForegroundColor Cyan
    $testArgs = @(
        "--test-dir", "build",
        "--output-on-failure"
    )
    
    $testResult = & ctest @testArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Some tests failed, but continuing with build..."
    }
}

# Create dist directory
if (-not (Test-Path "dist")) {
    New-Item -ItemType Directory -Path "dist" | Out-Null
}

# Install to dist directory
Write-Host "Installing to dist directory..." -ForegroundColor Cyan
$installArgs = @(
    "--install", "build",
    "--config", $BuildType,
    "--prefix", "dist"
)

$installResult = & cmake @installArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "Install failed"
    exit 1
}

# Create package (if not skipped)
if (-not $SkipPackage) {
    Write-Host "Creating package..." -ForegroundColor Cyan
    
    # Inject repository URL into README for packaging
    if (Test-Path "scripts\inject_repo_url.py") {
        Write-Host "Injecting repository URL..." -ForegroundColor Cyan
        & python scripts\inject_repo_url.py
    }
    
    $cpackArgs = @(
        "--config", "build\CPackConfig.cmake"
    )
    
    $cpackResult = & cpack @cpackArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Package creation failed"
        exit 1
    }
    
    # List created packages
    Write-Host "Created packages:" -ForegroundColor Green
    Get-ChildItem -Path "." -Filter "*.zip" | ForEach-Object {
        Write-Host "  $($_.Name) ($([math]::Round($_.Length / 1MB, 2)) MB)" -ForegroundColor White
    }
}

# Verify installation
Write-Host "Verifying installation..." -ForegroundColor Cyan
$exePath = "dist\liveops_sentinel.exe"
if (Test-Path $exePath) {
    $fileInfo = Get-Item $exePath
    Write-Host "✓ Executable created: $exePath" -ForegroundColor Green
    Write-Host "  Size: $([math]::Round($fileInfo.Length / 1MB, 2)) MB" -ForegroundColor White
    Write-Host "  Created: $($fileInfo.CreationTime)" -ForegroundColor White
} else {
    Write-Error "Executable not found at $exePath"
    exit 1
}

# Check for required files
$requiredFiles = @(
    "dist\config.example.json",
    "dist\LICENSE",
    "dist\README.md"
)

foreach ($file in $requiredFiles) {
    if (Test-Path $file) {
        Write-Host "✓ $file" -ForegroundColor Green
    } else {
        Write-Warning "Missing: $file"
    }
}

# Check for DLLs
$dllCount = (Get-ChildItem -Path "dist" -Filter "*.dll").Count
Write-Host "✓ Runtime DLLs: $dllCount files" -ForegroundColor Green

Write-Host "=== Build completed successfully! ===" -ForegroundColor Green
Write-Host "Installation directory: dist\" -ForegroundColor Cyan
Write-Host "Run: .\dist\liveops_sentinel.exe" -ForegroundColor Yellow
