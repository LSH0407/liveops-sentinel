# LiveOps Sentinel - 빌드 아키텍처 문서

## 프로젝트 개요
LiveOps Sentinel은 OBS 스트리밍 환경을 위한 모니터링 및 진단 도구입니다. GUI/콘솔 모드를 지원하며, 모듈화된 구조로 설계되었습니다.

## 빌드 옵션

### CMake 옵션
- `ENABLE_GUI=ON` (기본값): SDL2+ImGui GUI 모드로 빌드
- `ENABLE_GUI=OFF`: 콘솔 모드로 빌드
- `ENABLE_OBS=OFF` (기본값): OBS WebSocket 기능 비활성화 (스텁 사용)
- `ENABLE_OBS=ON`: OBS WebSocket 기능 활성화 (실제 구현 사용)

### 빌드 명령어

#### GUI 모드 (기본)
```bash
# Visual Studio 2022
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DENABLE_GUI=ON -DENABLE_OBS=OFF
cmake --build build --config Release --parallel

# Ninja
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DENABLE_GUI=ON -DENABLE_OBS=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

#### 콘솔 모드
```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DENABLE_GUI=OFF -DENABLE_OBS=OFF
cmake --build build --config Release
```

## 의존성 관리

### vcpkg.json
```json
{
    "name": "liveops-sentinel",
    "version-string": "0.2.0",
    "dependencies": [
        "asio",
        "nlohmann-json", 
        "spdlog",
        "sdl2",
        { "name": "imgui", "default-features": false, "features": ["sdl2-binding", "opengl3-binding"], "platform": "windows" },
        "glad"
    ],
    "builtin-baseline": "6ecbbbdf31cba47aafa7cf6189b1e73e10ac61f8"
}
```

### 라이브러리 링크
- **공통**: asio, nlohmann-json, spdlog
- **GUI 모드**: SDL2, ImGui, Glad
- **Windows**: ws2_32, iphlpapi

## 소스 코드 구조

### 핵심 모듈
```
src/
├── main.cpp                    # 엔트리포인트 (GUI/콘솔 분기)
├── core/                       # 핵심 기능
│   ├── Config.cpp/.h          # 설정 관리
│   ├── Metrics.cpp/.h         # 메트릭 수집
│   ├── ReportWriter.cpp/.h    # 리포트 생성
│   └── SystemMetrics.h        # 시스템 메트릭
├── diag/                       # 진단 기능
│   ├── BandwidthBench.cpp/.h  # 대역폭 벤치마크
│   └── Recommendation.cpp/.h  # 인코딩 추천 엔진
├── net/                        # 네트워크 기능
│   └── Probe.cpp/.h           # 네트워크 프로브
└── sys/                        # 시스템 기능
    └── ProcessMon.cpp/.h      # 프로세스 모니터링
```

### 인터페이스 모듈 (헤더 + 구현 2종)
```
src/
├── obs/                        # OBS 연동
│   ├── ObsClient.h            # 단일 헤더 (공개 API)
│   ├── ObsClientStub.cpp      # 스텁 구현 (ENABLE_OBS=OFF)
│   └── ObsClient.cpp          # 실제 구현 (ENABLE_OBS=ON)
└── alert/                      # 알림 기능
    ├── Notifier.h             # 단일 헤더 (공개 API)
    ├── NotifierStub.cpp       # 스텁 구현 (ENABLE_OBS=OFF)
    └── Notifier.cpp           # 실제 구현 (ENABLE_OBS=ON)
```

### GUI 모듈 (ENABLE_GUI=ON 시에만 포함)
```
src/ui/
└── AppSDL.cpp/.h              # 최소 GUI 루프 (SDL2 + ImGui + OpenGL3)
```

## API 인터페이스

### ObsClient API
```cpp
namespace obs {
struct Stats { 
    double droppedFramesRatio{0.0}; 
    double avgRenderMs{0.0}; 
    double cpuPct{0.0}; 
};

struct VideoSettings { 
    int baseWidth{1920}, baseHeight{1080}, outputWidth{1920}, outputHeight{1080}, fps{60}; 
};

class ObsClient {
public:
    bool connect(const std::string& host, int port, const std::string& password);
    bool isConnected() const;
    bool startStream(); bool stopStream();
    bool startRecord(); bool stopRecord();
    bool setCurrentProgramScene(const std::string& name);
    bool getSceneList(std::vector<std::string>& out);
    std::optional<Stats> getStats();
    std::optional<VideoSettings> getVideoSettings();
};
}
```

### Notifier API
```cpp
namespace alert {
class Notifier {
public:
    bool sendDiscord(const std::string& content);
    bool isReady() const;
};
}
```

### Recommendation API
```cpp
namespace diag {
enum class EncoderType { Auto=0, X264=1, NVENC=2, HEVC_NVENC=3 };
enum class PresetType { UltraFast, VeryFast, Fast, Quality, Performance };

struct ObsMetrics {
    int outputWidth{1920};
    int outputHeight{1080};
    int fps{60};
    double encodingLagMs{0.0};
    double renderLagMs{0.0};
};

struct Recommendation {
    EncoderType encoder{EncoderType::Auto};
    PresetType preset{PresetType::Quality};
    int minKbps{2500};
    int maxKbps{6000};
};

class RecommendationEngine {
public:
    EncoderType selectEncoder(int preferred);
    PresetType selectPreset(const ObsMetrics& m, EncoderType encoder);
    Recommendation recommend(const ObsMetrics& m, int preferredEncoder, int networkKbps, double headroom = 0.8);
};
}
```

## 빌드 분기 로직

### CMake 소스 분기
```cmake
# 공통 소스 (항상 포함)
set(SRCS
    src/main.cpp
    src/core/*.cpp
    src/diag/BandwidthBench.cpp
    src/diag/Recommendation.cpp
    src/alert/Notifier.h
    src/obs/ObsClient.h
)

# GUI 분기
if(ENABLE_GUI)
    list(APPEND SRCS src/ui/AppSDL.cpp)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(imgui CONFIG REQUIRED)
    find_package(glad CONFIG REQUIRED)
    add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLAD)
endif()

# OBS 분기 (실/스텁 구현 선택)
if(ENABLE_OBS)
    list(APPEND SRCS src/obs/ObsClient.cpp src/alert/Notifier.cpp)
else()
    list(APPEND SRCS src/obs/ObsClientStub.cpp src/alert/NotifierStub.cpp)
endif()
```

### 엔트리포인트 분기
```cpp
#include <iostream>
namespace ui { int RunApp(); }

int main() {
#ifdef ENABLE_GUI
    return ui::RunApp();  // GUI 모드
#else
    std::cout << "LiveOps Sentinel Console Mode\n";
    return 0;  // 콘솔 모드
#endif
}
```

## 최소 GUI 루프

### AppSDL.cpp
- SDL2 + ImGui + OpenGL3 기반
- 1280x720 창 크기
- "GUI build OK (SDL2 + ImGui)" 메시지 표시
- 이벤트 처리 및 렌더링 루프

## 컴파일 정의

### 공통
- `ASIO_STANDALONE`: ASIO 독립 실행
- `SPDLOG_COMPILED_LIB`: spdlog 컴파일된 라이브러리 사용

### GUI 모드
- `IMGUI_IMPL_OPENGL_LOADER_GLAD`: Glad를 OpenGL 로더로 사용

## 빌드 결과물

### GUI 모드
- 실행 파일: `build/Release/liveops_sentinel.exe`
- 기능: ImGui 창 표시, "GUI build OK" 메시지

### 콘솔 모드  
- 실행 파일: `build/Release/liveops_sentinel.exe`
- 기능: 콘솔 출력, "LiveOps Sentinel Console Mode" 메시지

## 개발 환경

### 필수 도구
- Visual Studio 2022 또는 Ninja
- CMake 3.20+
- vcpkg (의존성 관리)

### 권장 설정
- C++20 표준
- Windows x64 타겟
- Release 빌드 (성능 최적화)

## 문제 해결

### 일반적인 빌드 오류
1. **vcpkg 경로 문제**: `CMAKE_TOOLCHAIN_FILE` 환경변수 확인
2. **의존성 누락**: `vcpkg install` 재실행
3. **링커 오류**: 라이브러리 링크 순서 확인

### GUI 관련 문제
1. **OpenGL 컨텍스트 실패**: 그래픽 드라이버 업데이트
2. **SDL2 초기화 실패**: 시스템 라이브러리 확인
3. **ImGui 렌더링 문제**: OpenGL 버전 호환성 확인

## 향후 확장 계획

### 단기 목표
- [ ] OBS WebSocket 실제 구현 완성
- [ ] 알림 시스템 실제 구현 완성
- [ ] UI 컴포넌트 추가 (Dashboard, Checklist)

### 장기 목표
- [ ] 크로스 플랫폼 지원 (Linux, macOS)
- [ ] 플러그인 시스템
- [ ] 웹 인터페이스 추가
