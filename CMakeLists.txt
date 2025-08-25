cmake_minimum_required(VERSION 3.22)
project(LiveOpsSentinel VERSION 1.0.0 LANGUAGES CXX)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 컴파일러 옵션
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /Zc:__cplusplus)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    # /WX는 비활성화 (경고를 오류로 처리하지 않음)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 소스 파일
set(SOURCES
    src/main.cpp
    src/core/Config.cpp
    src/core/SystemMetrics.cpp
    src/net/Probe.cpp
)

# vcpkg 패키지 찾기
find_package(nlohmann_json CONFIG REQUIRED)

# 실행 파일 생성
add_executable(liveops_backend ${SOURCES})

# 헤더 파일 경로
target_include_directories(liveops_backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 라이브러리 링크
target_link_libraries(liveops_backend PRIVATE
    nlohmann_json::nlohmann_json
)

# Windows 특정 설정
if(WIN32)
    target_link_libraries(liveops_backend PRIVATE
        pdh.lib
        iphlpapi.lib
    )
endif()

# 설치 설정
install(TARGETS liveops_backend
    RUNTIME DESTINATION bin
)

# 패키징 설정 (선택적)
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_NAME "LiveOpsSentinel")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    include(CPack)
endif()
