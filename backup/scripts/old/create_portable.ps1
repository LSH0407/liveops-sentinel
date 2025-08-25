# LiveOps Sentinel 포터블 버전 생성 스크립트
param(
    [string]$Version = "1.0.0",
    [string]$BuildType = "Release",
    [string]$OutputDir = "release"
)

Write-Host "=== LiveOps Sentinel 포터블 버전 생성 ===" -ForegroundColor Green

# 출력 디렉토리 생성
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force
}

# 빌드 디렉토리 확인
$buildDir = "build_backend"
if (-not (Test-Path $buildDir)) {
    Write-Host "빌드 디렉토리가 없습니다. 먼저 빌드를 실행하세요." -ForegroundColor Red
    exit 1
}

# 실행 파일 확인
$exePath = "$buildDir\$BuildType\liveops_sentinel.exe"
if (-not (Test-Path $exePath)) {
    Write-Host "실행 파일을 찾을 수 없습니다: $exePath" -ForegroundColor Red
    exit 1
}

# 포터블 디렉토리 생성
$portableDir = "$OutputDir\LiveOpsSentinel-Portable-$Version"
if (Test-Path $portableDir) {
    Remove-Item -Path $portableDir -Recurse -Force
}
New-Item -ItemType Directory -Path $portableDir -Force

Write-Host "포터블 디렉토리 생성: $portableDir" -ForegroundColor Green

# 파일 복사
$filesToCopy = @(
    @{Source = $exePath; Dest = "liveops_sentinel.exe"},
    @{Source = "assets\config.example.json"; Dest = "config.json"},
    @{Source = "LICENSE"; Dest = "LICENSE"},
    @{Source = "README.md"; Dest = "README.md"},
    @{Source = "QUICK_START.md"; Dest = "QUICK_START.md"}
)

foreach ($file in $filesToCopy) {
    if (Test-Path $file.Source) {
        Copy-Item -Path $file.Source -Destination "$portableDir\$($file.Dest)" -Force
        Write-Host "복사됨: $($file.Source) -> $($file.Dest)" -ForegroundColor Cyan
    } else {
        Write-Host "경고: 파일을 찾을 수 없습니다: $($file.Source)" -ForegroundColor Yellow
    }
}

# 포터블 설정 파일 생성
$portableConfig = @"
# LiveOps Sentinel 포터블 설정
# 이 파일은 포터블 버전에서 사용됩니다.

[Portable]
# 포터블 모드 활성화
enabled = true

# 설정 파일 경로 (상대 경로)
config_file = config.json

# 로그 파일 경로 (상대 경로)
log_file = logs/liveops_sentinel.log

# 데이터 디렉토리 (상대 경로)
data_dir = data

[Paths]
# 실행 파일 경로
executable = liveops_sentinel.exe

# 설정 파일 경로
config = config.json

# 로그 디렉토리
logs = logs

# 데이터 디렉토리
data = data
"@

$portableConfig | Out-File -FilePath "$portableDir\portable.ini" -Encoding UTF8

# 하위 디렉토리 생성
$subDirs = @("logs", "data", "backups")
foreach ($dir in $subDirs) {
    New-Item -ItemType Directory -Path "$portableDir\$dir" -Force
    Write-Host "디렉토리 생성: $dir" -ForegroundColor Cyan
}

# 배치 파일 생성 (Windows용)
$batchFile = @"
@echo off
echo LiveOps Sentinel 포터블 버전 시작...
echo.

REM 현재 디렉토리를 스크립트 위치로 설정
cd /d "%~dp0"

REM 로그 디렉토리 생성
if not exist "logs" mkdir logs

REM 데이터 디렉토리 생성
if not exist "data" mkdir data

REM 백업 디렉토리 생성
if not exist "backups" mkdir backups

REM 설정 파일 확인
if not exist "config.json" (
    echo 설정 파일이 없습니다. config.example.json을 config.json으로 복사합니다.
    copy "config.example.json" "config.json" >nul
)

echo 애플리케이션을 시작합니다...
start "" "liveops_sentinel.exe"

echo.
echo LiveOps Sentinel이 백그라운드에서 실행 중입니다.
echo 종료하려면 작업 관리자에서 liveops_sentinel.exe를 종료하세요.
pause
"@

$batchFile | Out-File -FilePath "$portableDir\start_liveops_sentinel.bat" -Encoding Default

# PowerShell 스크립트 생성
$psScript = @"
# LiveOps Sentinel 포터블 버전 시작 스크립트
Write-Host "LiveOps Sentinel 포터블 버전 시작..." -ForegroundColor Green

# 현재 디렉토리를 스크립트 위치로 설정
Set-Location $PSScriptRoot

# 필요한 디렉토리 생성
@("logs", "data", "backups") | ForEach-Object {
    if (-not (Test-Path $_)) {
        New-Item -ItemType Directory -Path $_ -Force
        Write-Host "디렉토리 생성: $_" -ForegroundColor Cyan
    }
}

# 설정 파일 확인
if (-not (Test-Path "config.json")) {
    if (Test-Path "config.example.json") {
        Copy-Item "config.example.json" "config.json"
        Write-Host "설정 파일을 생성했습니다: config.json" -ForegroundColor Yellow
    }
}

Write-Host "애플리케이션을 시작합니다..." -ForegroundColor Green
Start-Process -FilePath "liveops_sentinel.exe" -NoNewWindow

Write-Host "LiveOps Sentinel이 백그라운드에서 실행 중입니다." -ForegroundColor Cyan
Write-Host "종료하려면 작업 관리자에서 liveops_sentinel.exe를 종료하세요." -ForegroundColor Yellow
"@

$psScript | Out-File -FilePath "$portableDir\start_liveops_sentinel.ps1" -Encoding UTF8

# ZIP 파일 생성
$zipPath = "$OutputDir\LiveOpsSentinel-Portable-$Version.zip"
if (Test-Path $zipPath) {
    Remove-Item -Path $zipPath -Force
}

Write-Host "ZIP 파일 생성 중..." -ForegroundColor Yellow
Compress-Archive -Path "$portableDir\*" -DestinationPath $zipPath -Force

if (Test-Path $zipPath) {
    Write-Host "포터블 버전 생성 완료!" -ForegroundColor Green
    Write-Host "ZIP 파일: $zipPath" -ForegroundColor Cyan
    
    # 파일 크기 정보
    $zipSize = (Get-Item $zipPath).Length
    $zipSizeMB = [math]::Round($zipSize / 1MB, 2)
    Write-Host "파일 크기: $zipSizeMB MB" -ForegroundColor Cyan
} else {
    Write-Host "ZIP 파일 생성 실패!" -ForegroundColor Red
    exit 1
}

# 정리
Remove-Item -Path $portableDir -Recurse -Force
Write-Host "임시 디렉토리 정리 완료" -ForegroundColor Green
