# LiveOps Sentinel

UE/OBS 라이브 송출 품질 모니터링 도구

## 개요

라이브 스트리밍 환경에서 네트워크 품질과 OBS Studio 상태를 실시간으로 모니터링하여 안정적인 송출을 보장하는 도구입니다.

## 빌드 요구사항

- **C++20** 지원 컴파일러
- **CMake 3.20+**
- **vcpkg** (패키지 관리자)
- **Visual Studio 2022** (Windows) 또는 **GCC/Clang** (Linux)

## Console Mode (no GUI)

### 필수 패키지
- asio
- nlohmann-json  
- spdlog

### vcpkg 설정

1. vcpkg 설치 및 환경변수 설정:
```powershell
# Windows PowerShell
$env:VCPKG_ROOT="C:\vcpkg"
# 또는 영구 설정
setx VCPKG_ROOT C:\vcpkg
```

2. 의존성 설치:
```bash
vcpkg install asio nlohmann-json spdlog
```

### 빌드 방법

#### Windows (권장)
```cmd
# 자동 빌드 스크립트 사용 (PowerShell 정책 무관)
scripts\build_console_win.cmd

# 또는 PowerShell 직접 실행
.\scripts\build_console_win.ps1

# 또는 수동 빌드
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
  -DDISABLE_GUI=ON -DENABLE_OBS=OFF -DCMAKE_BUILD_TYPE=Release

cmake --build build --config Release --parallel
```

#### Linux
```bash
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DDISABLE_GUI=ON -DENABLE_OBS=OFF -DCMAKE_BUILD_TYPE=Release

cmake --build build --config Release --parallel
```

### 실행
```cmd
# Windows (권장)
scripts\build_console_win.cmd
scripts\run_latest_console.cmd   # 항상 out\liveops_sentinel.exe 실행

# 또는 직접 실행
.\out\liveops_sentinel.exe

# Linux  
./build/liveops_sentinel
```

## GUI Mode (선택사항)

GUI 모드를 사용하려면 추가 패키지가 필요합니다:
- SDL2
- ImGui

```powershell
# GUI 모드 빌드
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake `
  -DDISABLE_GUI=OFF -DENABLE_OBS=OFF -DCMAKE_BUILD_TYPE=Release
```

## 빌드 옵션

- `DISABLE_GUI=ON` (기본값): GUI 없이 콘솔 모드로 빌드
- `ENABLE_OBS=OFF` (기본값): OBS WebSocket 기능 비활성화, 콘솔 스텁 사용
- `CMAKE_BUILD_TYPE=Release`: Release 모드 빌드

## Common Build Issues

### vcpkg 툴체인 미전달 시 오류
```
CMake Error: Could not find a package configuration file provided by "asio"
```
**해결책**: `-DCMAKE_TOOLCHAIN_FILE` 옵션을 추가하세요.

### 경고를 오류로 취급하지 않음
이 프로젝트는 경고를 오류로 취급하지 않습니다 (`/WX`, `-Werror` 미사용).

### Windows 시스템 라이브러리
Windows에서 `ws2_32`, `iphlpapi` 라이브러리가 자동으로 링크됩니다.

## 라이선스

MIT License

<!--REPO_URL_START-->
<!--REPO_URL_END--> 
