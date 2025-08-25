# LiveOps Sentinel 설치 프로그램 생성 스크립트
param(
    [string]$Version = "1.0.0",
    [string]$BuildType = "Release",
    [string]$OutputDir = "release"
)

Write-Host "=== LiveOps Sentinel 설치 프로그램 생성 ===" -ForegroundColor Green

# 필요한 도구 확인
$innoSetup = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if (-not (Test-Path $innoSetup)) {
    Write-Host "Inno Setup이 설치되지 않았습니다. https://jrsoftware.org/isdl.php 에서 다운로드하세요." -ForegroundColor Red
    exit 1
}

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

# Inno Setup 스크립트 생성
$innoScript = @"
[Setup]
AppName=LiveOps Sentinel
AppVersion=$Version
AppPublisher=LiveOps Team
AppPublisherURL=https://github.com/liveops-sentinel
AppSupportURL=https://github.com/liveops-sentinel/issues
AppUpdatesURL=https://github.com/liveops-sentinel/releases
DefaultDirName={autopf}\LiveOps Sentinel
DefaultGroupName=LiveOps Sentinel
AllowNoIcons=yes
LicenseFile=LICENSE
OutputDir=$OutputDir
OutputBaseFilename=LiveOpsSentinel-Setup-$Version
SetupIconFile=assets\icon.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 6.1; Check: not IsAdminInstallMode

[Files]
Source: "$exePath"; DestDir: "{app}"; Flags: ignoreversion
Source: "assets\config.example.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\LiveOps Sentinel"; Filename: "{app}\liveops_sentinel.exe"
Name: "{group}\{cm:UninstallProgram,LiveOps Sentinel}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\LiveOps Sentinel"; Filename: "{app}\liveops_sentinel.exe"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\LiveOps Sentinel"; Filename: "{app}\liveops_sentinel.exe"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\liveops_sentinel.exe"; Description: "{cm:LaunchProgram,LiveOps Sentinel}"; Flags: nowait postinstall skipifsilent

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
end;
"@

# Inno Setup 스크립트 파일 생성
$innoScriptPath = "packaging\installer.iss"
if (-not (Test-Path "packaging")) {
    New-Item -ItemType Directory -Path "packaging" -Force
}
$innoScript | Out-File -FilePath $innoScriptPath -Encoding UTF8

Write-Host "Inno Setup 스크립트 생성 완료: $innoScriptPath" -ForegroundColor Green

# 설치 프로그램 생성
Write-Host "설치 프로그램 생성 중..." -ForegroundColor Yellow
& $innoSetup $innoScriptPath

if ($LASTEXITCODE -eq 0) {
    Write-Host "설치 프로그램 생성 완료!" -ForegroundColor Green
    Write-Host "출력 파일: $OutputDir\LiveOpsSentinel-Setup-$Version.exe" -ForegroundColor Cyan
} else {
    Write-Host "설치 프로그램 생성 실패!" -ForegroundColor Red
    exit 1
}
