cmake_minimum_required(VERSION 3.22)
project(liveops_sentinel LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg toolchain file
if(EXISTS "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
    message(STATUS "Using vcpkg toolchain file")
endif()

# Required packages
find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Optional packages (with fallbacks)
find_package(websocketpp CONFIG QUIET)
if(NOT websocketpp_FOUND)
    message(WARNING "websocketpp not found - OBS WebSocket features will be disabled")
    add_compile_definitions(DISABLE_OBS_WEBSOCKET)
endif()

find_package(doctest CONFIG QUIET)
if(NOT doctest_FOUND)
    message(WARNING "doctest not found - unit tests will be disabled")
    add_compile_definitions(DISABLE_UNIT_TESTS)
endif()

add_executable(liveops_sentinel
  src/main.cpp
  src/app/App.cpp src/app/App.h
  src/ui/Dashboard.cpp src/ui/Dashboard.h
  src/net/Probe.cpp src/net/Probe.h
  src/obs/ObsClient.cpp src/obs/ObsClient.h
  src/obs/EventLog.cpp src/obs/EventLog.h
  src/core/Metrics.cpp src/core/Metrics.h
  src/core/Config.cpp src/core/Config.h
  src/core/ReportWriter.cpp src/core/ReportWriter.h
  src/alert/Notifier.cpp src/alert/Notifier.h
  src/sys/ProcessMon.cpp src/sys/ProcessMon.h
  src/ui/Checklist.cpp src/ui/Checklist.h
  src/ui/WebhookWizard.cpp src/ui/WebhookWizard.h
  src/diag/BandwidthBench.cpp src/diag/BandwidthBench.h
  src/diag/Recommendation.cpp src/diag/Recommendation.h
)

target_include_directories(liveops_sentinel PRIVATE src)

target_link_libraries(liveops_sentinel PRIVATE
  SDL2::SDL2 SDL2::SDL2main
  imgui::imgui
  asio::asio
  nlohmann_json::nlohmann_json
  spdlog::spdlog
)

# Optional libraries
if(websocketpp_FOUND)
    target_link_libraries(liveops_sentinel PRIVATE websocketpp::websocketpp)
endif()

if (WIN32)
  target_link_libraries(liveops_sentinel PRIVATE opengl32)
  target_compile_definitions(liveops_sentinel PRIVATE _WIN32_WINNT=0x0601)
else()
  target_link_libraries(liveops_sentinel PRIVATE pthread)
endif()

# 컴파일러 경고 설정
target_compile_options(liveops_sentinel PRIVATE 
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror=return-type>
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)

# clang-tidy 설정
set(CMAKE_CXX_CLANG_TIDY 
  clang-tidy;
  -checks=-*,readability-*,performance-*,modernize-*,bugprone-*;
  -header-filter=src/*
)

# 테스트 추가 (doctest가 있을 때만)
if(doctest_FOUND)
    add_subdirectory(tests)
endif()

# Repository URL injection target
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    add_custom_target(inject_repo_url
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/inject_repo_url.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Injecting repository URL into README.md"
        VERBATIM
    )
else()
    message(WARNING "Python3 not found - inject_repo_url target not available")
endif()
