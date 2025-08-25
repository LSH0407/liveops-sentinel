# LiveOps Sentinel - 스크립트 가이드

이 디렉토리에는 LiveOps Sentinel 프로젝트의 개발 및 빌드를 위한 PowerShell 스크립트들이 포함되어 있습니다.

## 📁 스크립트 목록

### 🔨 빌드 스크립트
- **`build.ps1`** - 통합 빌드 스크립트
  - 다양한 빌드 옵션 지원 (Release/Debug, GUI/OBS, Tests)
  - vcpkg 자동 설정
  - CMake 빌드 자동화

### 🚀 실행 스크립트
- **`run.ps1`** - 프로그램 실행 스크립트
  - 콘솔/GUI 모드 지원
  - 설정 파일 지정 가능
  - 자동 실행 파일 검색

### 🧪 테스트 스크립트
- **`test.ps1`** - 테스트 실행 스크립트
  - 성능 테스트 빌드 및 실행
  - 테스트 결과 확인

### 🛠️ 유틸리티 스크립트
- **`clean_repo.ps1`** - 프로젝트 정리 스크립트
  - 빌드 캐시 및 임시 파일 정리
  - `-WhatIfOnly` 옵션으로 미리보기 가능

- **`setup_vcpkg.ps1`** - vcpkg 설정 스크립트
  - vcpkg 자동 설치 및 설정
  - 환경변수 자동 설정

- **`update_vcpkg_baseline.ps1`** - vcpkg 베이스라인 업데이트
  - vcpkg.json의 builtin-baseline 자동 업데이트

## 🚀 빠른 시작

### 1. 기본 빌드 및 실행
```powershell
# 빌드
.\scripts\build.ps1

# 실행
.\scripts\run.ps1
```

### 2. GUI 모드 빌드 및 실행
```powershell
# GUI 모드 빌드
.\scripts\build.ps1 -Gui

# GUI 모드 실행
.\scripts\run.ps1 -Gui
```

### 3. OBS 연동 빌드
```powershell
# OBS 연동 빌드
.\scripts\build.ps1 -Obs

# 실행
.\scripts\run.ps1
```

### 4. 테스트 실행
```powershell
# 테스트 빌드 및 실행
.\scripts\test.ps1

# 테스트 빌드만
.\scripts\test.ps1 -Build

# 테스트 실행만
.\scripts\test.ps1 -Run
```

### 5. 프로젝트 정리
```powershell
# 정리 (미리보기)
.\scripts\clean_repo.ps1 -WhatIfOnly

# 정리 실행
.\scripts\clean_repo.ps1
```

## ⚙️ 고급 사용법

### 빌드 옵션 조합
```powershell
# Debug + GUI + OBS + Tests
.\scripts\build.ps1 -Debug -Gui -Obs -Tests

# Clean + Release
.\scripts\build.ps1 -Clean -Release
```

### vcpkg 설정
```powershell
# vcpkg 설정
.\scripts\setup_vcpkg.ps1

# 베이스라인 업데이트
.\scripts\update_vcpkg_baseline.ps1
```

## 🔧 스크립트 옵션

### build.ps1
- `-Clean`: 빌드 전 정리
- `-Release`: Release 모드 (기본값)
- `-Debug`: Debug 모드
- `-Gui`: GUI 모드 활성화
- `-Obs`: OBS 연동 활성화
- `-Tests`: 테스트 빌드 포함
- `-Help`: 도움말 표시

### run.ps1
- `-Gui`: GUI 모드로 실행
- `-Config <path>`: 설정 파일 경로 지정
- `-Help`: 도움말 표시

### test.ps1
- `-Build`: 테스트 빌드만 실행
- `-Run`: 테스트 실행만 실행
- `-All`: 빌드 후 테스트 실행 (기본값)
- `-Help`: 도움말 표시

### clean_repo.ps1
- `-WhatIfOnly`: 실제 삭제하지 않고 미리보기만

## 📝 주의사항

1. **PowerShell 실행 정책**: 스크립트 실행을 위해 다음 명령을 실행하세요:
   ```powershell
   Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
   ```

2. **vcpkg 요구사항**: 빌드 전에 vcpkg가 설정되어 있어야 합니다.

3. **Visual Studio**: Visual Studio 2022가 설치되어 있어야 합니다.

4. **관리자 권한**: 일부 작업은 관리자 권한이 필요할 수 있습니다.

## 🐛 문제 해결

### 빌드 실패
1. vcpkg 설정 확인: `.\scripts\setup_vcpkg.ps1`
2. 프로젝트 정리: `.\scripts\clean_repo.ps1`
3. 다시 빌드: `.\scripts\build.ps1 -Clean`

### 실행 파일을 찾을 수 없음
1. 빌드가 완료되었는지 확인
2. 올바른 경로에서 실행하고 있는지 확인
3. 빌드 옵션을 확인

### vcpkg 오류
1. vcpkg 재설정: `.\scripts\setup_vcpkg.ps1`
2. 환경변수 확인: `echo $env:VCPKG_ROOT`
3. vcpkg 업데이트: `git pull` (vcpkg 디렉토리에서)

## 📚 추가 정보

- **프로젝트 문서**: [../README.md](../README.md)
- **빌드 설정**: [../CMakeLists.txt](../CMakeLists.txt)
- **의존성**: [../vcpkg.json](../vcpkg.json)
