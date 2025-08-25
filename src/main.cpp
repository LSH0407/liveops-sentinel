#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#ifdef _WIN32
#include <windows.h>
#endif

// 핵심 모듈
#include "core/Config.h"
#include "core/SystemMetrics.h"
#include "net/Probe.h"

using namespace core;

// 전역 변수
std::atomic<bool> running(true);

// 시그널 핸들러
void signalHandler(int signal) {
    running = false;
    std::cout << "\n종료 신호를 받았습니다. 정리 중..." << std::endl;
}

// 로거 초기화
void initializeLogger() {
    auto& config = core::Config::getInstance();
    std::cout << "LiveOps Sentinel 시작" << std::endl;
}

// 메트릭 수집 및 출력
void collectAndOutputMetrics() {
    auto& system_metrics = core::SystemMetrics::getInstance();
    auto& network_probe = net::Probe::getInstance();
    
    // 시스템 메트릭 수집
    auto sys_metrics = system_metrics.getMetrics();
    
    // 네트워크 메트릭 수집
    auto net_metrics = network_probe.getMetrics();
    
    // 타임스탬프 생성
    auto now = std::chrono::system_clock::now();
    auto ts = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    
    // JSONL 형식으로 출력 (GUI 호환) - 줄단위로 보장
    nlohmann::json snapshot = {
        {"event", "metrics"},
        {"ts", ts},
        {"cpu_pct", sys_metrics["cpu_pct"]},
        {"memory_pct", sys_metrics["memory_pct"]},
        {"mem_mb", sys_metrics["memory_mb"]},
        {"gpu_pct", sys_metrics["gpu_pct"]},  // GPU 메트릭 추가
        {"rtt_ms", net_metrics["rtt_ms"]},
        {"loss_pct", net_metrics["loss_pct"]},
        {"uplink_kbps", net_metrics["uplink_kbps"]}
    };
    
    std::string line = snapshot.dump();
    std::cout << line << "\n";  // 개행으로 라인 경계 보장
}

// 메인 루프
void runMonitoringLoop() {
    auto& config = core::Config::getInstance();
    
    int interval_ms = config.getProbeIntervalMs();
    std::cout << "모니터링 루프 시작 (간격: " << interval_ms << "ms)" << std::endl;
    
    while (running) {
        try {
            collectAndOutputMetrics();
        } catch (const std::exception& e) {
            std::cerr << "메트릭 수집 중 오류: " << e.what() << std::endl;
        }
        
        // 지정된 간격만큼 대기
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }
}

// 진단 모드 실행
void runDiagnosticMode(int duration_seconds, const std::string& platform) {
    auto& config = core::Config::getInstance();
    
    std::cout << "진단 모드 시작 - 플랫폼: " << platform << ", 지속시간: " << duration_seconds << "초" << std::endl;
    
    // 플랫폼 설정
    config.setPlatform(platform);
    
    // 진단 시작 이벤트 출력
    auto start_ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    std::cout << "EVENT:diagnose_start PLATFORM:" << platform 
              << " DURATION:" << duration_seconds << " TS:" << start_ts << std::endl;
    
    // 진단 기간 동안 메트릭 수집
    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::seconds(duration_seconds);
    
    while (running && std::chrono::steady_clock::now() < end_time) {
        try {
            collectAndOutputMetrics();
        } catch (const std::exception& e) {
            std::cerr << "진단 중 메트릭 수집 오류: " << e.what() << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1초 간격
    }
    
    // 진단 완료 이벤트 출력
    auto end_ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    std::cout << "EVENT:diagnose_done PLATFORM:" << platform 
              << " DURATION:" << duration_seconds << " TS:" << end_ts << std::endl;
    
    std::cout << "진단 모드 완료" << std::endl;
}

// 명령 처리
void processCommand(const std::string& command_line) {
    try {
        // 간단한 명령 파싱 (예: "diagnose 60 soop")
        std::istringstream iss(command_line);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "diagnose") {
            int duration_sec = 60;
            std::string platform = "soop";
            
            if (iss >> duration_sec) {
                iss >> platform;
            }
            
            runDiagnosticMode(duration_sec, platform);
        } else {
            std::cerr << "알 수 없는 명령: " << cmd << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "명령 처리 오류: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // 표준출력 즉시 플러시 설정
    std::ios::sync_with_stdio(false);
    std::cout.setf(std::ios::unitbuf);            // << 할 때마다 자동 flush
    setvbuf(stdout, nullptr, _IONBF, 0);          // C stdout도 무버퍼(Windows에서도 OK)
    
    // 레디 배너 출력
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    std::cout << "BACKEND_READY pid=" << GetCurrentProcessId() << "\n";
#else
    std::cout << "BACKEND_READY pid=" << getpid() << "\n";
#endif
    
    // 시그널 핸들러 설정
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // 설정 로드
        auto& config = core::Config::getInstance();
        
        // 로거 초기화
        initializeLogger();
        
        std::cout << "LiveOps Sentinel 시작" << std::endl;
        std::cout << "설정 파일: " << config.getConfigPath() << std::endl;
        
        // 명령행 인수 처리
        if (argc > 1) {
            std::string arg = argv[1];
            if (arg == "--help" || arg == "-h") {
                std::cout << "LiveOps Sentinel - 기본 모니터링 시스템\n";
                std::cout << "사용법:\n";
                std::cout << "  " << argv[0] << "                    # 실시간 모니터링\n";
                std::cout << "  " << argv[0] << " --diagnose <sec>   # 진단 모드\n";
                std::cout << "  " << argv[0] << " --help             # 도움말\n";
                return 0;
            } else if (arg == "--diagnose" && argc > 2) {
                int duration = std::stoi(argv[2]);
                std::string platform = (argc > 3) ? argv[3] : "soop";
                runDiagnosticMode(duration, platform);
                return 0;
            }
        }
        
        // 표준 입력에서 명령 읽기 (비동기)
        std::thread command_thread([&]() {
            std::string line;
            while (running && std::getline(std::cin, line)) {
                if (!line.empty()) {
                    processCommand(line);
                }
            }
        });
        
        // 메인 모니터링 루프 실행
        runMonitoringLoop();
        
        // 명령 스레드 종료 대기
        if (command_thread.joinable()) {
            command_thread.join();
        }
        
        std::cout << "LiveOps Sentinel 종료" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "치명적 오류: " << e.what() << std::endl;
        return 1;
    }
}

