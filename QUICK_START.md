# LiveOps Sentinel - 빠른 시작 가이드

## 필수 요구사항

### 개발 환경
- Windows 10/11
- Visual Studio 2022 또는 Ninja
- CMake 3.20+
- Git

### vcpkg 설정
```bash
# vcpkg 설치 (이미 설치되어 있다면 생략)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat

# 환경변수 설정
set VCPKG_ROOT=C:\path\to\vcpkg
```

## 빠른 빌드

### 1. 프로젝트 클론
```bash
git clone <repository-url>
cd liveops-sentinel
```

### 2. GUI 모드 빌드 (권장)
```bash
# Visual Studio 2022
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DENABLE_GUI=ON -DENABLE_OBS=OFF
cmake --build build --config Release

# 또는 Ninja (더 빠름)
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DENABLE_GUI=ON -DENABLE_OBS=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### 3. 콘솔 모드 빌드
```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DENABLE_GUI=OFF -DENABLE_OBS=OFF
cmake --build build --config Release
```

## 실행

### GUI 모드
```bash
.\build\Release\liveops_sentinel.exe
```
- ImGui 창이 열리고 "GUI build OK (SDL2 + ImGui)" 메시지 표시
- 창을 닫으면 프로그램 종료

### 콘솔 모드
```bash
.\build\Release\liveops_sentinel.exe
```
- 콘솔에 "LiveOps Sentinel Console Mode" 메시지 출력

## 빌드 옵션

### CMake 옵션
| 옵션 | 기본값 | 설명 |
|------|--------|------|
| `ENABLE_GUI` | `ON` | GUI 모드 활성화 |
| `ENABLE_OBS` | `OFF` | OBS WebSocket 기능 활성화 |

### 예시
```bash
# GUI + OBS 기능 모두 활성화
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DENABLE_GUI=ON -DENABLE_OBS=ON

# 콘솔 모드만
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DENABLE_GUI=OFF -DENABLE_OBS=OFF
```

## 문제 해결

### 일반적인 오류

#### 1. vcpkg 경로 오류
```
CMake Error: Could not find toolchain file
```
**해결**: `VCPKG_ROOT` 환경변수 확인
```bash
echo %VCPKG_ROOT%
```

#### 2. 의존성 패키지 오류
```
Could not find a package configuration file provided by "SDL2"
```
**해결**: vcpkg 재설치
```bash
vcpkg install --triplet=x64-windows
```

#### 3. 컴파일러 오류
```
error C1083: 포함 파일을 열 수 없습니다
```
**해결**: Visual Studio 2022 설치 확인, C++20 지원 확인

### 디버깅

#### 빌드 로그 확인
```bash
cmake --build build --config Release --verbose
```

#### CMake 캐시 확인
```bash
cat build/CMakeCache.txt | findstr "ENABLE_GUI\|ENABLE_OBS"
```

## 개발 팁

### 1. 빠른 재빌드
```bash
# 캐시 유지하면서 재구성
cmake --build build --config Release --clean-first
```

### 2. 병렬 빌드
```bash
# CPU 코어 수만큼 병렬 빌드
cmake --build build --config Release --parallel
```

### 3. 디버그 빌드
```bash
cmake --build build --config Debug
```

## 다음 단계

### 1. 기능 확장
- OBS WebSocket 실제 구현
- 알림 시스템 구현
- UI 컴포넌트 추가

### 2. 문서 확인
- `BUILD_ARCHITECTURE.md`: 상세한 빌드 아키텍처
- `CURRENT_STATUS.md`: 현재 프로젝트 상태

### 3. 개발 참여
- 이슈 리포트
- 기능 요청
- 코드 기여

## 지원

문제가 발생하면 다음을 확인하세요:
1. 이 문서의 문제 해결 섹션
2. `BUILD_ARCHITECTURE.md`의 상세 설명
3. GitHub 이슈 트래커
