cmake_minimum_required(VERSION 3.20)
project(liveops_sentinel VERSION 0.2.0 LANGUAGES CXX)

# C++20 강제 설정
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 빌드 옵션
option(ENABLE_GUI "Build with ImGui GUI" OFF)
option(ENABLE_OBS "Enable OBS WebSocket integration" OFF)

if(ENABLE_GUI)
  add_compile_definitions(ENABLE_GUI IMGUI_IMPL_OPENGL_LOADER_GLAD ASIO_STANDALONE)
else()
  add_compile_definitions(ASIO_STANDALONE)
endif()

# 컴파일러 경고 설정 (경고는 오류 아님)
if(MSVC)
    add_compile_options(/W4 /permissive- /EHsc)
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# vcpkg 툴체인 파일 확인
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR "vcpkg toolchain file not specified. Use: -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake")
endif()

# 필수 패키지 (콘솔 모드 기준)
find_package(asio CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# GUI 패키지 (조건부)
if(ENABLE_GUI)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(imgui CONFIG REQUIRED)
    find_package(glad CONFIG REQUIRED)
    add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    message(STATUS "GUI enabled - SDL2, ImGui, and Glad will be linked")
else()
    message(STATUS "GUI disabled - running in console mode")
endif()

# OBS WebSocket 기능
if(ENABLE_OBS)
    message(STATUS "OBS WebSocket features enabled")
    add_compile_definitions(ENABLE_OBS)
else()
    message(STATUS "OBS WebSocket features disabled - using stubs")
endif()

# 공통 소스(항상 포함)
set(SRCS
  src/main.cpp
  src/core/Config.cpp
  src/core/ReportWriter.cpp
  src/net/Probe.cpp
  src/sys/ProcessMon.cpp
  src/diag/BandwidthBench.cpp
  src/diag/Recommendation.cpp
  src/alert/Notifier.h
  src/obs/ObsClient.h
)

if(ENABLE_GUI)
  list(APPEND SRCS src/ui/AppGLFW.cpp src/ui/AppGLFW.h)
  find_package(glad CONFIG REQUIRED)
  find_package(glfw3 CONFIG REQUIRED)
  find_package(imgui CONFIG REQUIRED)
endif()

if(ENABLE_OBS)
  list(APPEND SRCS src/obs/ObsClient.cpp src/alert/Notifier.cpp)
else()
  list(APPEND SRCS src/obs/ObsClientStub.cpp src/alert/NotifierStub.cpp)
endif()

add_executable(liveops_sentinel ${SRCS})
target_include_directories(liveops_sentinel PRIVATE src)

# spdlog 소비자 매크로는 제거(있다면)
# target_compile_definitions(liveops_sentinel PRIVATE SPDLOG_COMPILED_LIB)  # 제거

# 공통 링크(필요 시 유지)
# target_link_libraries(liveops_sentinel PRIVATE asio nlohmann_json spdlog)

if(ENABLE_GUI)
  target_link_libraries(liveops_sentinel PRIVATE glad::glad glfw imgui::imgui)
endif()

if(WIN32)
  target_link_libraries(liveops_sentinel PRIVATE ws2_32 iphlpapi)
endif()

# 설치 규칙
install(TARGETS liveops_sentinel
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
