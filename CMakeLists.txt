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
# Note: websocketpp was removed from vcpkg registry in 2025
# OBS WebSocket features will be disabled for now
add_compile_definitions(DISABLE_OBS_WEBSOCKET)

# Note: doctest was removed from vcpkg registry in 2025
# Unit tests will be disabled for now
add_compile_definitions(DISABLE_UNIT_TESTS)

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
# Note: websocketpp was removed from vcpkg registry in 2025
# OBS WebSocket features are disabled

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

# 테스트 추가 (doctest was removed from vcpkg registry in 2025)
# Tests are disabled for now
# if(doctest_FOUND)
#     add_subdirectory(tests)
# endif()

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
    
    # Install runtime dependencies (simplified for compatibility)
    install(TARGETS liveops_sentinel RUNTIME DESTINATION .)
    
    # Note: Runtime dependencies will be handled by CPack
    # For manual DLL copying, use windeployqt or similar tools
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
    set(CPACK_NSIS_URL_UPDATE_INFO "https://github.com/LSH0407/liveops-sentinel/releases")
    set(CPACK_NSIS_HELP_LINK "https://github.com/LSH0407/liveops-sentinel")
    set(CPACK_NSIS_ENABLE_ICON_EXTRA "https://github.com/LSH0407/liveops-sentinel")
    
    # Icon settings (if icon exists)
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets/icon.ico")
        set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/assets/icon.ico")
        set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/assets/icon.ico")
        set(CPACK_NSIS_INSTALLED_ICON_NAME "liveops_sentinel.exe")
    endif()
    
    # Installation settings
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_ICON_EXTRA ON)
    set(CPACK_NSIS_DISPLAY_NAME "LiveOps Sentinel ${PROJECT_VERSION}")
    set(CPACK_NSIS_PACKAGE_NAME "LiveOps Sentinel")
    
    # Start menu shortcuts
    set(CPACK_NSIS_MENU_LINKS
        "liveops_sentinel.exe" "LiveOps Sentinel"
        "https://github.com/LSH0407/liveops-sentinel" "GitHub Repository"
        "https://github.com/LSH0407/liveops-sentinel/releases" "Latest Releases"
    )
    
    # NSIS script customization
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
        CreateDirectory \\\"$SMPROGRAMS\\\\LiveOps Sentinel\\\"
        CreateShortCut \\\"$SMPROGRAMS\\\\LiveOps Sentinel\\\\LiveOps Sentinel.lnk\\\" \\\"$INSTDIR\\\\liveops_sentinel.exe\\\"
        CreateShortCut \\\"$SMPROGRAMS\\\\LiveOps Sentinel\\\\Uninstall.lnk\\\" \\\"$INSTDIR\\\\uninstall.exe\\\"
        CreateShortCut \\\"$DESKTOP\\\\LiveOps Sentinel.lnk\\\" \\\"$INSTDIR\\\\liveops_sentinel.exe\\\"
    ")
    
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        Delete \\\"$SMPROGRAMS\\\\LiveOps Sentinel\\\\LiveOps Sentinel.lnk\\\"
        Delete \\\"$SMPROGRAMS\\\\LiveOps Sentinel\\\\Uninstall.lnk\\\"
        Delete \\\"$DESKTOP\\\\LiveOps Sentinel.lnk\\\"
        RMDir \\\"$SMPROGRAMS\\\\LiveOps Sentinel\\\"
    ")
endif()
