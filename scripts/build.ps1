# LiveOps Sentinel 빌드 스크립트
param(
    [string]$BuildType = "Release",
    [string]$Architecture = "x64",
    [switch]$Clean,
    [switch]$Help
)

if ($Help) {
    Write-Host "LiveOps Sentinel 빌드 스크립트"
    Write-Host "사용법: .\scripts\build.ps1 [옵션]"
    Write-Host ""
    Write-Host "옵션:"
    Write-Host "  -BuildType <type>     빌드 타입 (Debug/Release, 기본값: Release)"
    Write-Host "  -Architecture <arch>   아키텍처 (x64/x86, 기본값: x64)"
    Write-Host "  -Clean                빌드 디렉토리 정리"
    Write-Host "  -Help                 도움말 표시"
    exit 0
}

# vcpkg 환경 확인
if (-not $env:VCPKG_ROOT) {
    Write-Error "VCPKG_ROOT 환경 변수가 설정되지 않았습니다."
    Write-Host "vcpkg를 설치하고 VCPKG_ROOT를 설정해주세요."
    exit 1
}

# 빌드 디렉토리 설정
$BuildDir = "build"
$BuildPath = Join-Path (Split-Path $PSScriptRoot -Parent) $BuildDir

Write-Host "LiveOps Sentinel 빌드 시작..."
Write-Host "빌드 타입: $BuildType"
Write-Host "아키텍처: $Architecture"
Write-Host "빌드 디렉토리: $BuildPath"

# 빌드 디렉토리 정리
if ($Clean -or (Test-Path $BuildPath)) {
    Write-Host "빌드 디렉토리 정리 중..."
    if (Test-Path $BuildPath) {
        Remove-Item -Path $BuildPath -Recurse -Force
    }
}

# 빌드 디렉토리 생성
New-Item -ItemType Directory -Path $BuildPath -Force | Out-Null

# CMake 구성
Write-Host "CMake 구성 중..."
$CmakeArgs = @(
    "-B", $BuildPath,
    "-S", (Split-Path $PSScriptRoot -Parent),
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
)

if ($Architecture -eq "x86") {
    $CmakeArgs += "-A", "Win32"
} else {
    $CmakeArgs += "-A", "x64"
}

$CmakeResult = & cmake @CmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake 구성 실패"
    exit 1
}

# 빌드 실행
Write-Host "빌드 실행 중..."
$BuildResult = & cmake --build $BuildPath --config $BuildType --parallel
if ($LASTEXITCODE -ne 0) {
    Write-Error "빌드 실패"
    exit 1
}

Write-Host "빌드 완료!"
Write-Host "실행 파일 위치: $BuildPath\$BuildType\liveops_backend.exe"

# 실행 파일 확인
$ExePath = Join-Path $BuildPath $BuildType "liveops_backend.exe"
if (Test-Path $ExePath) {
    Write-Host "빌드 성공! 실행 파일이 생성되었습니다: $ExePath"
} else {
    Write-Error "실행 파일을 찾을 수 없습니다: $ExePath"
    exit 1
}
