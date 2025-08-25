Param(
  [string]$Version = "0.1.0",
  [string]$VcpkgRoot = $env:VCPKG_ROOT
)

$ErrorActionPreference = "Stop"
Write-Host "=== Make Portable ZIP v$Version ==="

# 1) 백엔드 Release 빌드 (이미 빌드되어도 재사용)
if (-not $VcpkgRoot) { throw "VCPKG_ROOT not set" }
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot\scripts\buildsystems\vcpkg.cmake" `
  -DENABLE_GUI=OFF -DENABLE_OBS=OFF
cmake --build build --config Release --parallel

# 2) PyInstaller로 GUI 빌드
pushd ui_py
if (-not (Test-Path .venv)) {
  try { py -3.11 -m venv .venv } catch { python -m venv .venv }
}
.\.venv\Scripts\python -m pip install --upgrade pip
.\.venv\Scripts\python -m pip install -r requirements.txt pyinstaller
.\.venv\Scripts\python -m PyInstaller packaging\liveops_gui.spec --noconfirm --clean
popd

# 3) 포터블 ZIP 생성
$dst = "release"
if (Test-Path $dst) { Remove-Item $dst -Recurse -Force }
New-Item -ItemType Directory -Path $dst | Out-Null
Copy-Item -Recurse "ui_py\dist\LiveOpsSentinel" "$dst\LiveOpsSentinel"
Compress-Archive -Path "$dst\LiveOpsSentinel\*" -DestinationPath "LiveOpsSentinel_Portable_v$Version.zip" -Force

Write-Host "OUT: LiveOpsSentinel_Portable_v$Version.zip"
Write-Host "=== Done ==="
