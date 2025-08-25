Param(
  [string]$Version = "0.1.0",
  [string]$VcpkgRoot = $env:VCPKG_ROOT
)

$ErrorActionPreference = "Stop"
Write-Host "=== Make Release v$Version ==="

# 1) C++ 백엔드 Release 빌드
if (-not $VcpkgRoot) { throw "VCPKG_ROOT not set" }
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot\scripts\buildsystems\vcpkg.cmake" `
  -DENABLE_GUI=OFF -DENABLE_OBS=OFF
cmake --build build --config Release --parallel

# 2) Python 가상환경 + PyInstaller
pushd ui_py
python -m venv .venv
.\.venv\Scripts\python -m pip install --upgrade pip
.\.venv\Scripts\python -m pip install -r requirements.txt pyinstaller
.\.venv\Scripts\pyinstaller packaging\liveops_gui.spec
popd

# 3) Portable ZIP 구성
$dst = "release"
if (Test-Path $dst) { Remove-Item $dst -Recurse -Force }
New-Item -ItemType Directory -Path $dst | Out-Null
Copy-Item -Recurse "ui_py\dist\LiveOpsSentinel" "$dst\LiveOpsSentinel"
Copy-Item README.md "$dst\LiveOpsSentinel\" -ErrorAction SilentlyContinue
Compress-Archive -Path "$dst\LiveOpsSentinel\*" -DestinationPath "LiveOpsSentinel_Portable_v$Version.zip" -Force

# 4) Inno Setup(선택)
$ISCC = (Get-Command iscc.exe -ErrorAction SilentlyContinue)
if ($ISCC) {
  pushd ui_py\packaging
  & $ISCC.Path "installer.iss"
  popd
} else {
  Write-Warning "Inno Setup not found (iscc.exe). Skip installer build."
}
Write-Host "=== Done ==="
