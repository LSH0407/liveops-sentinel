#requires -Version 5.1
param(
    [switch]$Gui,
    [switch]$Help,
    [string]$Config = ""
)

if ($Help) {
    Write-Host @"
LiveOps Sentinel Run Script

Usage:
    .\scripts\run.ps1 [options]

Options:
    -Gui        Run in GUI mode
    -Config     Specify config file path
    -Help       Show help

Examples:
    .\scripts\run.ps1                    # Default console mode
    .\scripts\run.ps1 -Gui              # GUI mode
    .\scripts\run.ps1 -Config config.json # Use specific config file
"@
    exit 0
}

$ErrorActionPreference = 'Stop'

# Check executable paths
$ExePaths = @(
    "build\Release\liveops_sentinel.exe",
    "build\Debug\liveops_sentinel.exe",
    "out\liveops_sentinel.exe",
    "liveops_sentinel.exe"
)

$ExePath = $null
foreach ($path in $ExePaths) {
    if (Test-Path $path) {
        $ExePath = $path
        break
    }
}

if (-not $ExePath) {
    Write-Error "Executable not found. Please build first: .\scripts\build.ps1"
    exit 1
}

Write-Host "Executable: $ExePath"

# Run options
$RunOptions = @()

if ($Gui) {
    $RunOptions += "--gui"
    Write-Host "Running in GUI mode"
}

if ($Config) {
    if (Test-Path $Config) {
        $RunOptions += "--config", $Config
        Write-Host "Config file: $Config"
    } else {
        Write-Warning "Config file not found: $Config"
    }
}

# Run
Write-Host "Starting LiveOps Sentinel..."
Write-Host "$ExePath $($RunOptions -join ' ')"
& $ExePath @RunOptions

$ExitCode = $LASTEXITCODE
if ($ExitCode -eq 0) {
    Write-Host "Program exited normally."
} else {
    Write-Host "Program exited with error code $ExitCode."
}

exit $ExitCode
