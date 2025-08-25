# LiveOps Sentinel - 빠른 시작 가이드

## 🚀 5분 만에 시작하기

### 1. 저장소 클론
```bash
git clone https://github.com/your-username/liveops-sentinel.git
cd liveops-sentinel
```

### 2. vcpkg 설정
```powershell
# PowerShell에서 실행
.\scripts\setup_vcpkg.ps1
```

### 3. 빌드 및 실행
```powershell
# 기본 빌드
.\scripts\build.ps1

# 실행
.\scripts\run.ps1
```

## 📋 요구사항

### 필수 소프트웨어
- **Windows 10/11** (현재 지원)
- **Visual Studio 2022** (Community 버전도 가능)
- **Git** (최신 버전)
- **PowerShell 5.1+**

### 자동 설치되는 것
- **vcpkg** (패키지 관리자)
- **CMake** (빌드 시스템)
- **필요한 라이브러리들** (asio, nlohmann-json, spdlog 등)

## 🔧 빌드 옵션

### 기본 빌드 (콘솔 모드)
```powershell
.\scripts\build.ps1
```

### GUI 모드 빌드
```powershell
.\scripts\build.ps1 -Gui
```

### OBS 연동 빌드
```powershell
.\scripts\build.ps1 -Obs
```

### 모든 기능 포함 빌드
```powershell
.\scripts\build.ps1 -Gui -Obs -Tests
```

### Debug 모드 빌드
```powershell
.\scripts\build.ps1 -Debug
```

## 🚀 실행 옵션

### 콘솔 모드 실행 (기본)
```powershell
.\scripts\run.ps1
```

### GUI 모드 실행
```powershell
.\scripts\run.ps1 -Gui
```

### 특정 설정 파일로 실행
```powershell
.\scripts\run.ps1 -Config my_config.json
```

## 🧪 테스트 실행

### 전체 테스트
```powershell
.\scripts\test.ps1
```

### 테스트 빌드만
```powershell
.\scripts\test.ps1 -Build
```

### 테스트 실행만
```powershell
.\scripts\test.ps1 -Run
```

## 🛠️ 유틸리티

### 프로젝트 정리
```powershell
# 미리보기
.\scripts\clean_repo.ps1 -WhatIfOnly

# 실제 정리
.\scripts\clean_repo.ps1
```

### vcpkg 베이스라인 업데이트
```powershell
.\scripts\update_vcpkg_baseline.ps1
```

## 📁 프로젝트 구조

```
liveops-sentinel/
├── src/                    # 소스 코드
│   ├── core/              # 핵심 시스템
│   ├── sys/               # 시스템 모니터링
│   ├── net/               # 네트워크 진단
│   ├── alert/             # 알림 시스템
│   ├── notify/            # 알림 채널
│   ├── obs/               # OBS 연동
│   └── main.cpp           # 메인 실행 파일
├── scripts/               # 빌드 및 실행 스크립트
├── tests/                 # 테스트 코드
├── docs/                  # 문서
├── backup/                # 백업 파일들
├── CMakeLists.txt         # 빌드 설정
└── README.md             # 프로젝트 문서
```

## ⚙️ 설정

### 기본 설정 파일 생성
프로그램 첫 실행 시 `config.json` 파일이 자동으로 생성됩니다.

### 주요 설정 옵션
```json
{
  "logging": {
    "level": "info",
    "console_enabled": true,
    "file_enabled": true
  },
  "monitoring": {
    "interval_ms": 1000,
    "cpu_warning_pct": 80.0,
    "memory_warning_pct": 75.0
  },
  "network": {
    "rtt_warning_ms": 50.0,
    "loss_warning_pct": 1.0
  },
  "notifications": {
    "discord_webhook": "",
    "slack_webhook": ""
  }
}
```

## 🐛 문제 해결

### 빌드 실패
1. **vcpkg 설정 확인**
   ```powershell
   .\scripts\setup_vcpkg.ps1
   ```

2. **프로젝트 정리 후 재빌드**
   ```powershell
   .\scripts\clean_repo.ps1
   .\scripts\build.ps1 -Clean
   ```

3. **Visual Studio 확인**
   - Visual Studio 2022가 설치되어 있는지 확인
   - C++ 개발 도구가 설치되어 있는지 확인

### 실행 파일을 찾을 수 없음
1. **빌드 완료 확인**
   ```powershell
   .\scripts\build.ps1
   ```

2. **실행 파일 경로 확인**
   - `build\Release\liveops_sentinel.exe`
   - `build\Debug\liveops_sentinel.exe`

### PowerShell 실행 정책 오류
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

### vcpkg 오류
1. **vcpkg 재설정**
   ```powershell
   .\scripts\setup_vcpkg.ps1
   ```

2. **환경변수 확인**
   ```powershell
   echo $env:VCPKG_ROOT
   ```

## 📚 다음 단계

### 기본 사용법
1. **모니터링 시작**: 프로그램 실행 후 시스템 모니터링이 자동으로 시작됩니다
2. **알림 설정**: Discord/Slack 웹훅을 설정하여 알림을 받을 수 있습니다
3. **성능 최적화**: 자동으로 메모리 및 CPU 최적화가 수행됩니다

### 고급 기능
- **OBS Studio 연동**: 스트리밍 상태 모니터링
- **네트워크 진단**: RTT, 패킷 손실, 대역폭 측정
- **성능 프로파일**: Performance/Balanced/Conservative 모드
- **자동 업데이트**: GitHub API를 통한 자동 업데이트

### 개발 참여
- **테스트 작성**: `tests/` 디렉토리에 테스트 추가
- **기능 개발**: `src/` 디렉토리에서 기능 개발
- **문서 작성**: `docs/` 디렉토리에 문서 추가

## 📞 지원

- **이슈 리포트**: [GitHub Issues](https://github.com/your-username/liveops-sentinel/issues)
- **문서**: [Wiki](https://github.com/your-username/liveops-sentinel/wiki)
- **릴리즈**: [Releases](https://github.com/your-username/liveops-sentinel/releases)

---

**LiveOps Sentinel** - 안정적인 실시간 환경을 위한 최고의 선택
