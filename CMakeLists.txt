cmake_minimum_required(VERSION 3.22)
project(liveops_sentinel VERSION 0.2.0 LANGUAGES CXX)
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

# Install rules
install(TARGETS liveops_sentinel RUNTIME DESTINATION .)

# Install resources
install(FILES assets/config.example.json DESTINATION .)
install(FILES LICENSE README.md DESTINATION . OPTIONAL)
install(DIRECTORY scripts/ DESTINATION scripts OPTIONAL)

# Windows runtime dependencies
if(WIN32)
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    
    # Install runtime dependencies (CMake 3.21+)
    install(TARGETS liveops_sentinel
        RUNTIME_DEPENDENCY_SET deps
        RUNTIME DESTINATION .
    )
    
    # Get runtime dependencies and install them
    file(GET_RUNTIME_DEPENDENCIES
        TARGETS liveops_sentinel
        RESOLVED_DEPENDENCIES_VAR resolved_deps
        UNRESOLVED_DEPENDENCIES_VAR unresolved_deps
        CONFLICTING_DEPENDENCIES_PREFIX conflicting_deps
        EXECUTABLES
            $<TARGET_FILE:liveops_sentinel>
        LIBRARIES
            $<TARGET_FILE:liveops_sentinel>
        DIRECTORIES
            $<TARGET_FILE_DIR:liveops_sentinel>
    )
    
    # Install resolved dependencies, excluding system DLLs
    foreach(dep ${resolved_deps})
        get_filename_component(dep_name "${dep}" NAME)
        string(TOLOWER "${dep_name}" dep_name_lower)
        
        # Skip system DLLs
        if(NOT dep_name_lower MATCHES "^(api-ms-win|vcruntime|msvcp|ucrt|kernel32|user32|gdi32|shell32|ole32|oleaut32|uuid|advapi32|ws2_32|iphlpapi|winmm|version|psapi|oleacc|uxtheme|dwmapi|d3d|dxgi|dinput|dwrite|d2d1|d3dcompiler|d3dx|system32|syswow64)")
            install(FILES "${dep}" DESTINATION .)
        endif()
    endforeach()
    
    if(unresolved_deps)
        message(WARNING "Unresolved dependencies: ${unresolved_deps}")
    endif()
endif()

# CPack configuration
include(CPack)
set(CPACK_PACKAGE_NAME "LiveOps Sentinel")
set(CPACK_PACKAGE_VENDOR "LSH0407")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "UE/OBS 라이브 송출 품질 모니터링 도구")
set(CPACK_PACKAGE_DESCRIPTION "라이브 스트리밍 환경에서 네트워크 품질과 OBS Studio 상태를 실시간으로 모니터링하여 안정적인 송출을 보장하는 도구입니다.")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# ZIP generator (primary)
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "LiveOpsSentinel-${PROJECT_VERSION}-win-x64")

# NSIS generator (optional, for Windows installer)
if(WIN32)
    set(CPACK_GENERATOR "${CPACK_GENERATOR};NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "LiveOps Sentinel")
    set(CPACK_NSIS_DISPLAY_NAME "LiveOps Sentinel ${PROJECT_VERSION}")
    set(CPACK_NSIS_CONTACT "https://github.com/LSH0407/liveops-sentinel")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/LSH0407/liveops-sentinel")
    set(CPACK_NSIS_ENABLE_ICON_EXTRA "https://github.com/LSH0407/liveops-sentinel")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/assets/icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/assets/icon.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "liveops_sentinel.exe")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    set(CPACK_NSIS_MENU_LINKS
        "https://github.com/LSH0407/liveops-sentinel" "GitHub Repository"
        "https://github.com/LSH0407/liveops-sentinel/releases" "Latest Releases"
    )
endif()
