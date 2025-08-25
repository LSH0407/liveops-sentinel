#requires -Version 5.1
param(
    [switch]$Build,
    [switch]$Run,
    [switch]$All,
    [switch]$Help
)

if ($Help) {
    Write-Host @"
LiveOps Sentinel Test Script

Usage:
    .\scripts\test.ps1 [options]

Options:
    -Build      Build tests only
    -Run        Run tests only
    -All        Build then run tests (default)
    -Help       Show help

Examples:
    .\scripts\test.ps1                    # Build then run tests
    .\scripts\test.ps1 -Build            # Build tests only
    .\scripts\test.ps1 -Run              # Run tests only
"@
    exit 0
}

$ErrorActionPreference = 'Stop'

# Test executable path
$TestExePath = "build\Release\performance_test.exe"

# Build
if ($Build -or $All) {
    Write-Host "Building tests..."
    & "$PSScriptRoot\build.ps1" -Release -Tests
    
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Test build failed."
        exit 1
    }
}

# Run tests
if ($Run -or $All) {
    if (-not (Test-Path $TestExePath)) {
        Write-Error "Test executable not found: $TestExePath"
        Write-Host "Please build tests first: .\scripts\test.ps1 -Build"
        exit 1
    }
    
    Write-Host "Running tests..."
    Write-Host "Executable: $TestExePath"
    
    & $TestExePath
    
    $ExitCode = $LASTEXITCODE
    if ($ExitCode -eq 0) {
        Write-Host "All tests completed successfully."
    } else {
        Write-Host "Some tests failed. (Exit code: $ExitCode)"
    }
    
    exit $ExitCode
}

Write-Host "Tests completed!"
