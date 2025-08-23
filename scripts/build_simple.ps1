$ErrorActionPreference='Stop'
$RepoRoot = Resolve-Path "$PSScriptRoot\.."
Set-Location $RepoRoot

Write-Host "=== LiveOps Sentinel Simple Build ===" -ForegroundColor Green
Write-Host "Repository: $RepoRoot"

try {
  # vcpkg setup
  $localVcpkg = Join-Path $RepoRoot ".tools\vcpkg"
  if (!(Test-Path "$localVcpkg\.git")) {
    Write-Host "Cloning vcpkg..." -ForegroundColor Yellow
    git clone https://github.com/microsoft/vcpkg $localVcpkg
    if (Test-Path "$localVcpkg\bootstrap-vcpkg.bat") {
      Write-Host "Bootstrapping vcpkg..." -ForegroundColor Yellow
      & "$localVcpkg\bootstrap-vcpkg.bat" -disableMetrics | Out-Null
    }
  }

  $env:VCPKG_ROOT = $localVcpkg
  $toolchain = "$localVcpkg\scripts\buildsystems\vcpkg.cmake"
  Write-Host "VCPKG_ROOT: $localVcpkg" -ForegroundColor Cyan

  # baseline update
  if (Test-Path "$localVcpkg\.git") {
    $sha = (git -C $localVcpkg rev-parse HEAD).Trim()
    $json = Get-Content .\vcpkg.json -Raw | ConvertFrom-Json
    $json | Add-Member -Name 'builtin-baseline' -Value $sha -MemberType NoteProperty -Force
    ($json | ConvertTo-Json -Depth 10) | Set-Content .\vcpkg.json -Encoding utf8
    Write-Host "Updated baseline: $sha" -ForegroundColor Cyan
  }

  # Clean build directory
  $BuildDir = "build"
  if (Test-Path $BuildDir) { 
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
  }
  New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
  Write-Host "Build directory: $BuildDir" -ForegroundColor Cyan

  # vcpkg install
  if (Test-Path "$localVcpkg\vcpkg.exe") {
    Write-Host "Installing dependencies..." -ForegroundColor Yellow
    & "$localVcpkg\vcpkg.exe" install --triplet x64-windows
    if ($LASTEXITCODE -ne 0) {
      throw "vcpkg install failed with exit code $LASTEXITCODE"
    }
  } else {
    throw "vcpkg.exe not found at $localVcpkg\vcpkg.exe"
  }

  # Configure
  Write-Host "Configuring CMake..." -ForegroundColor Yellow
  $cfgArgs = @(
    "-S", ".",
    "-B", $BuildDir,
    "-G", "Visual Studio 17 2022",
    "-A", "x64",
    "-DCMAKE_TOOLCHAIN_FILE=`"$toolchain`"",
    "-DDISABLE_GUI=ON",
    "-DENABLE_OBS=OFF",
    "-DCMAKE_BUILD_TYPE=Release"
  )
  & cmake @cfgArgs
  if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed with exit code $LASTEXITCODE"
  }

  # Build
  Write-Host "Building..." -ForegroundColor Yellow
  & cmake --build $BuildDir --config Release --parallel
  if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE"
  }

  # Find and copy exe
  $exe = Get-ChildItem $BuildDir -Recurse -Filter "liveops_sentinel.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
  if (!$exe) {
    throw "liveops_sentinel.exe not found after build"
  }

  $outDir = Join-Path $RepoRoot "out"
  New-Item -ItemType Directory -Force -Path $outDir | Out-Null
  Copy-Item -Force $exe.FullName (Join-Path $outDir "liveops_sentinel.exe")
  Write-Host "SUCCESS: Executable copied to out\liveops_sentinel.exe" -ForegroundColor Green

  Write-Host "BUILD SUCCESSFUL!" -ForegroundColor Green
  exit 0

} catch {
  Write-Host "BUILD FAILED: $($_.Exception.Message)" -ForegroundColor Red
  Write-Host "Error details: $($_.Exception)" -ForegroundColor Red
  exit 1
}
