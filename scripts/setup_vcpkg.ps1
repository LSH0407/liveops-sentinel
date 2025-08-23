#requires -Version 5.1
param([string]$PreferredRoot=$env:VCPKG_ROOT)

$ErrorActionPreference = 'Stop'

function Ensure-Git() {
  $g = Get-Command git -ErrorAction SilentlyContinue
  if (-not $g) { throw "git not found in PATH. Please install Git." }
}

function Ensure-Vcpkg([string]$root) {
  Ensure-Git

  if ([string]::IsNullOrWhiteSpace($root)) { $root = "C:\vcpkg" }
  # 1) 글로벌 경로가 정상 git 리포인지 확인
  if (Test-Path "$root\.git") {
    $env:VCPKG_ROOT = $root
    Write-Host "[OK] Using global vcpkg: $root"
    return $root
  }

  if (Test-Path $root) {
    Write-Host "[WARN] $root exists but is not a git repo. Ignoring."
  }

  # 2) 리포 내부 .tools\vcpkg 사용
  $repoRoot = Resolve-Path "$PSScriptRoot\.."
  $local = Join-Path $repoRoot ".tools\vcpkg"
  if (Test-Path "$local\.git") {
    $env:VCPKG_ROOT = $local
    Write-Host "[OK] Using local vcpkg: $local"
    return $local
  }

  # 3) 없으면 클론+부트스트랩
  New-Item -ItemType Directory -Force -Path (Split-Path $local) | Out-Null
  Write-Host "[CLONE] git clone microsoft/vcpkg -> $local"
  git clone https://github.com/microsoft/vcpkg $local | Out-Null
  Write-Host "[BOOTSTRAP] bootstrap-vcpkg.bat"
  & "$local\bootstrap-vcpkg.bat" -disableMetrics | Out-Null

  $env:VCPKG_ROOT = $local
  Write-Host "[OK] Using local vcpkg: $local"
  return $local
}

$root = Ensure-Vcpkg -root $PreferredRoot
Write-Host "VCPKG_ROOT = $root"
