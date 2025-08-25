# LiveOps Sentinel 개발자 가이드

## 목차
1. [개발 환경 설정](#개발-환경-설정)
2. [프로젝트 구조](#프로젝트-구조)
3. [아키텍처 개요](#아키텍처-개요)
4. [코딩 표준](#코딩-표준)
5. [빌드 시스템](#빌드-시스템)
6. [테스트](#테스트)
7. [배포](#배포)
8. [기여 가이드라인](#기여-가이드라인)

## 개발 환경 설정

### 필수 도구
- **Visual Studio 2022**: Community Edition 이상
- **CMake**: 3.20 이상
- **Git**: 최신 버전
- **vcpkg**: 패키지 관리자
- **PowerShell**: 스크립트 실행

### 개발 환경 구축

#### 1. 저장소 클론
```bash
git clone https://github.com/liveops-sentinel/liveops-sentinel.git
cd liveops-sentinel
```

#### 2. vcpkg 설정
```powershell
# vcpkg 설치
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 필요한 패키지 설치
.\vcpkg install nlohmann-json:x64-windows
.\vcpkg install spdlog:x64-windows
.\vcpkg install curl:x64-windows
.\vcpkg install glfw3:x64-windows
.\vcpkg install imgui:x64-windows
.\vcpkg install websocketpp:x64-windows
.\vcpkg install asio:x64-windows
```

#### 3. CMake 설정
```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
```

## 프로젝트 구조

```
liveops-sentinel/
├── src/                    # 소스 코드
│   ├── core/              # 핵심 기능
│   │   ├── Config.cpp     # 설정 관리
│   │   ├── Logger.cpp     # 로깅 시스템
│   │   ├── SystemMetrics.cpp # 시스템 메트릭
│   │   ├── MemoryManager.cpp # 메모리 관리
│   │   ├── PerformanceOptimizer.cpp # 성능 최적화
│   │   └── UpdateManager.cpp # 업데이트 관리
│   ├── net/               # 네트워크 모듈
│   │   ├── Probe.cpp      # 네트워크 프로브
│   │   └── NetworkDiagnostics.cpp # 네트워크 진단
│   ├── obs/               # OBS 통합
│   │   ├── ObsClient.cpp  # OBS 클라이언트
│   │   └── ObsClientStub.cpp # 스텁 구현
│   ├── alert/             # 알림 시스템
│   │   └── Notifier.cpp   # 알림 발송
│   ├── notify/            # 알림 관리
│   │   ├── AlertManager.cpp # 알림 관리자
│   │   └── AlertScheduler.cpp # 알림 스케줄러
│   ├── ui/                # 사용자 인터페이스
│   │   └── AppGLFW.cpp    # GLFW 기반 GUI
│   └── main.cpp           # 메인 진입점
├── tests/                 # 테스트 코드
├── scripts/               # 빌드 및 배포 스크립트
├── docs/                  # 문서
├── assets/                # 리소스 파일
└── CMakeLists.txt         # CMake 설정
```

## 아키텍처 개요

### 모듈 구조
LiveOps Sentinel은 모듈화된 아키텍처를 사용합니다:

```
┌─────────────────┐
│   Main App      │
├─────────────────┤
│   UI Layer      │
├─────────────────┤
│  Core Modules   │
├─────────────────┤
│  System Layer   │
└─────────────────┘
```

### 핵심 컴포넌트

#### 1. Core Module
- **Config**: 설정 파일 관리
- **Logger**: 구조화된 로깅
- **SystemMetrics**: 시스템 리소스 모니터링
- **MemoryManager**: 메모리 관리 및 최적화
- **PerformanceOptimizer**: 성능 최적화
- **UpdateManager**: 자동 업데이트

#### 2. Network Module
- **Probe**: 네트워크 상태 모니터링
- **NetworkDiagnostics**: 고급 네트워크 진단

#### 3. OBS Module
- **ObsClient**: OBS WebSocket 클라이언트
- **ObsClientStub**: 테스트용 스텁 구현

#### 4. Alert Module
- **Notifier**: 알림 발송 (Discord, Slack, Email)
- **AlertManager**: 알림 관리 및 임계값 처리
- **AlertScheduler**: 알림 스케줄링

#### 5. UI Module
- **AppGLFW**: GLFW 기반 GUI 애플리케이션

### 디자인 패턴

#### PIMPL (Pointer to Implementation)
```cpp
// 헤더 파일
class MyClass {
public:
    MyClass();
    ~MyClass();
    void doSomething();
private:
    class MyClassImpl;
    std::unique_ptr<MyClassImpl> impl_;
};

// 구현 파일
class MyClass::MyClassImpl {
public:
    void doSomething() {
        // 실제 구현
    }
};

MyClass::MyClass() : impl_(std::make_unique<MyClassImpl>()) {}
MyClass::~MyClass() = default;
void MyClass::doSomething() { impl_->doSomething(); }
```

#### 싱글톤 패턴
```cpp
class Singleton {
public:
    static Singleton& getInstance() {
        static Singleton instance;
        return instance;
    }
private:
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};
```

#### 콜백 패턴
```cpp
class EventHandler {
public:
    using Callback = std::function<void(const Event&)>;
    
    void setCallback(Callback callback) {
        callback_ = callback;
    }
    
    void triggerEvent(const Event& event) {
        if (callback_) {
            callback_(event);
        }
    }
private:
    Callback callback_;
};
```

## 코딩 표준

### 명명 규칙

#### 클래스 및 구조체
```cpp
class NetworkProbe;           // PascalCase
struct AlertThresholds;       // PascalCase
```

#### 함수 및 메서드
```cpp
void startMonitoring();       // camelCase
bool isConnected() const;     // camelCase
```

#### 변수
```cpp
int connection_count;         // snake_case
std::string webhook_url;      // snake_case
```

#### 상수
```cpp
const int MAX_RETRY_COUNT = 3;    // UPPER_SNAKE_CASE
const std::string DEFAULT_URL = "ws://localhost:4444";
```

#### 네임스페이스
```cpp
namespace core { }            // 소문자
namespace net { }             // 소문자
namespace ui { }              // 소문자
```

### 코드 스타일

#### 들여쓰기
- 4칸 공백 사용 (탭 사용 금지)
- 일관된 들여쓰기 유지

#### 중괄호 스타일
```cpp
// K&R 스타일 사용
if (condition) {
    // 코드
} else {
    // 코드
}

class MyClass {
public:
    MyClass() {
        // 생성자
    }
};
```

#### 주석
```cpp
/**
 * @brief 클래스 설명
 * 
 * 이 클래스는 네트워크 모니터링을 담당합니다.
 * 
 * @param url 연결할 URL
 * @return 연결 성공 여부
 */
class NetworkMonitor {
public:
    // 한 줄 주석
    bool connect(const std::string& url);
};
```

### 에러 처리

#### 예외 처리
```cpp
try {
    auto result = riskyOperation();
    if (!result) {
        throw std::runtime_error("Operation failed");
    }
} catch (const std::exception& e) {
    logger.error("Error: {}", e.what());
    return false;
}
```

#### 반환값 처리
```cpp
std::expected<Result, Error> performOperation() {
    if (condition) {
        return Result{};
    } else {
        return std::unexpected(Error::INVALID_STATE);
    }
}
```

## 빌드 시스템

### CMake 설정

#### 기본 CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.20)
project(LiveOpsSentinel VERSION 1.0.0 LANGUAGES CXX)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 컴파일러 옵션
if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 옵션 설정
option(ENABLE_GUI "Enable GUI mode" ON)
option(ENABLE_OBS "Enable OBS integration" ON)
option(BUILD_TESTS "Build tests" OFF)

# vcpkg 패키지 찾기
find_package(nlohmann-json REQUIRED)
find_package(spdlog REQUIRED)
find_package(CURL REQUIRED)

if(ENABLE_GUI)
    find_package(glfw3 REQUIRED)
    find_package(imgui REQUIRED)
endif()

if(ENABLE_OBS)
    find_package(websocketpp REQUIRED)
endif()

# 소스 파일 설정
set(SOURCES
    src/main.cpp
    src/core/Config.cpp
    src/core/Logger.cpp
    # ... 기타 소스 파일
)

# 실행 파일 생성
add_executable(liveops_sentinel ${SOURCES})

# 라이브러리 링크
target_link_libraries(liveops_sentinel
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    CURL::libcurl
)

if(ENABLE_GUI)
    target_link_libraries(liveops_sentinel
        glfw
        imgui::imgui
    )
endif()
```

### 빌드 스크립트

#### Windows 빌드
```powershell
# Release 빌드
cmake --build build --config Release

# Debug 빌드
cmake --build build --config Debug

# 특정 타겟 빌드
cmake --build build --target liveops_sentinel
```

#### Linux 빌드
```bash
# Release 빌드
cmake --build build --config Release

# 병렬 빌드
cmake --build build --config Release --parallel 8
```

## 테스트

### 테스트 프레임워크
Google Test를 사용하여 단위 테스트를 작성합니다.

#### 테스트 구조
```cpp
#include <gtest/gtest.h>
#include "core/Config.h"

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 테스트 설정
    }
    
    void TearDown() override {
        // 테스트 정리
    }
};

TEST_F(ConfigTest, LoadConfig) {
    core::Config config;
    EXPECT_TRUE(config.loadConfig("test_config.json"));
    EXPECT_EQ(config.getLogLevel(), "info");
}

TEST_F(ConfigTest, SaveConfig) {
    core::Config config;
    config.setLogLevel("debug");
    EXPECT_TRUE(config.saveConfig("test_output.json"));
}
```

#### 테스트 실행
```bash
# 모든 테스트 실행
ctest --output-on-failure

# 특정 테스트 실행
./tests/test_config --gtest_filter=ConfigTest.*
```

### 통합 테스트
```cpp
class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 통합 테스트 환경 설정
    }
};

TEST_F(IntegrationTest, FullWorkflow) {
    // 전체 워크플로우 테스트
    auto app = std::make_unique<LiveOpsApp>();
    EXPECT_TRUE(app->initialize());
    EXPECT_TRUE(app->startMonitoring());
    // ... 테스트 로직
    app->shutdown();
}
```

## 배포

### 릴리스 빌드
```powershell
# 릴리스 빌드 스크립트 실행
.\scripts\build_release.ps1

# 설치 프로그램 생성
.\scripts\create_installer.ps1 -Version 1.0.0

# 포터블 버전 생성
.\scripts\create_portable.ps1 -Version 1.0.0
```

### 패키징
```powershell
# ZIP 패키지 생성
Compress-Archive -Path "build/Release/*" -DestinationPath "LiveOpsSentinel-1.0.0.zip"

# MSI 패키지 생성 (WiX Toolset 사용)
candle LiveOpsSentinel.wxs
light LiveOpsSentinel.wixobj
```

### 배포 체크리스트
- [ ] 모든 테스트 통과
- [ ] 릴리스 빌드 성공
- [ ] 설치 프로그램 테스트
- [ ] 문서 업데이트
- [ ] 릴리스 노트 작성
- [ ] GitHub 릴리스 생성

## 기여 가이드라인

### 개발 워크플로우

#### 1. 이슈 생성
- 버그 리포트 또는 기능 요청
- 명확한 설명과 재현 단계 포함

#### 2. 브랜치 생성
```bash
git checkout -b feature/new-feature
git checkout -b fix/bug-fix
```

#### 3. 개발 및 테스트
- 기능 구현
- 단위 테스트 작성
- 통합 테스트 실행

#### 4. 커밋 메시지
```bash
# 기능 추가
feat: add network diagnostics feature

# 버그 수정
fix: resolve memory leak in Config class

# 문서 업데이트
docs: update user manual

# 리팩토링
refactor: improve error handling in Logger
```

#### 5. Pull Request 생성
- 명확한 제목과 설명
- 관련 이슈 링크
- 테스트 결과 포함

### 코드 리뷰

#### 리뷰 체크리스트
- [ ] 코드 스타일 준수
- [ ] 기능 요구사항 충족
- [ ] 테스트 커버리지
- [ ] 성능 영향
- [ ] 보안 고려사항
- [ ] 문서화

#### 리뷰 코멘트 예시
```cpp
// 좋은 예
// TODO: 이 부분을 별도 함수로 분리하는 것을 고려해보세요
// SUGGESTION: std::optional을 사용하여 더 안전하게 만들 수 있습니다
// QUESTION: 이 예외 처리가 충분한가요?

// 피해야 할 예
// 이 코드는 잘못되었습니다
// 이렇게 하면 안 됩니다
```

### 문서화

#### 코드 문서화
```cpp
/**
 * @brief 네트워크 연결을 모니터링하는 클래스
 * 
 * 이 클래스는 네트워크 연결의 상태를 실시간으로 모니터링하고
 * 문제가 발생하면 알림을 발송합니다.
 * 
 * @example
 * ```cpp
 * NetworkMonitor monitor;
 * monitor.startMonitoring();
 * monitor.setCallback([](const Alert& alert) {
 *     std::cout << "Alert: " << alert.message << std::endl;
 * });
 * ```
 */
class NetworkMonitor {
public:
    /**
     * @brief 모니터링을 시작합니다
     * 
     * @param interval_ms 모니터링 간격 (밀리초)
     * @return 성공 여부
     * 
     * @throws std::runtime_error 네트워크 인터페이스 접근 실패 시
     */
    bool startMonitoring(int interval_ms = 1000);
};
```

#### API 문서화
- 모든 공개 API에 대한 문서 작성
- 예제 코드 포함
- 매개변수 및 반환값 설명

---

## 추가 리소스

### 참고 자료
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [CMake Documentation](https://cmake.org/documentation/)
- [Google Test Documentation](https://google.github.io/googletest/)

### 도구
- **Clang-Format**: 코드 포맷팅
- **Clang-Tidy**: 정적 분석
- **Valgrind**: 메모리 검사 (Linux)
- **AddressSanitizer**: 메모리 검사

### 커뮤니티
- **GitHub Discussions**: 기능 논의
- **Discord**: 실시간 개발자 채팅
- **Code Review**: 코드 리뷰 및 피드백

---

**버전**: 1.0.0  
**최종 업데이트**: 2024년 12월  
**라이선스**: MIT License
