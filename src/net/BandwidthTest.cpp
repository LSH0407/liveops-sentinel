#include "BandwidthTest.h"
#include <spdlog/spdlog.h>
#include <thread>
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

BandwidthTest::BandwidthTest() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

BandwidthTest::~BandwidthTest() {
    stopTest();
#ifdef _WIN32
    WSACleanup();
#endif
}

BandwidthResult BandwidthTest::runTest(const std::string& server, int duration_seconds) {
    BandwidthResult result;
    
    spdlog::info("대역폭 테스트 시작: {} ({}초)", server, duration_seconds);
    
    try {
        // 지연시간 측정
        result.latency_ms = measureLatency(server);
        
        // 업로드 대역폭 측정
        auto upload_result = measureUploadBandwidth(server, duration_seconds);
        result.upload_mbps = upload_result.upload_mbps;
        
        // 다운로드 대역폭 측정
        auto download_result = measureDownloadBandwidth(server, duration_seconds);
        result.download_mbps = download_result.download_mbps;
        
        result.success = true;
        spdlog::info("대역폭 테스트 완료: 업로드={:.1f}Mbps, 다운로드={:.1f}Mbps, 지연={:.1f}ms", 
                    result.upload_mbps, result.download_mbps, result.latency_ms);
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
        spdlog::error("대역폭 테스트 실패: {}", e.what());
    }
    
    return result;
}

void BandwidthTest::runTestAsync(const std::string& server, int duration_seconds, 
                                std::function<void(const BandwidthResult&)> callback) {
    if (running_) {
        spdlog::warn("이미 테스트가 실행 중입니다");
        return;
    }
    
    running_ = true;
    progress_ = 0.0;
    
    std::thread([this, server, duration_seconds, callback]() {
        auto result = runTest(server, duration_seconds);
        running_ = false;
        progress_ = 100.0;
        callback(result);
    }).detach();
}

void BandwidthTest::stopTest() {
    running_ = false;
}

double BandwidthTest::getProgress() const {
    return progress_;
}

BandwidthResult BandwidthTest::measureUploadBandwidth(const std::string& server, int duration_seconds) {
    BandwidthResult result;
    
    // 간단한 HTTP POST 기반 업로드 측정
    // 실제로는 더 정교한 측정이 필요하지만, 여기서는 기본 구현
    
    auto start_time = std::chrono::steady_clock::now();
    size_t total_bytes = 0;
    
    // 1MB 테스트 데이터 생성
    std::string test_data = generateTestData(1024 * 1024);
    
    while (running_ && std::chrono::steady_clock::now() - start_time < std::chrono::seconds(duration_seconds)) {
        // 실제 구현에서는 HTTP POST 요청
        total_bytes += test_data.size();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();
        progress_ = (elapsed / 1000.0 / duration_seconds) * 50.0; // 업로드는 50%까지
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    if (duration_ms > 0) {
        result.upload_mbps = (total_bytes * 8.0 / 1000000.0) / (duration_ms / 1000.0);
    }
    
    return result;
}

BandwidthResult BandwidthTest::measureDownloadBandwidth(const std::string& server, int duration_seconds) {
    BandwidthResult result;
    
    auto start_time = std::chrono::steady_clock::now();
    size_t total_bytes = 0;
    
    while (running_ && std::chrono::steady_clock::now() - start_time < std::chrono::seconds(duration_seconds)) {
        // 실제 구현에서는 HTTP GET 요청
        total_bytes += 1024 * 1024; // 1MB씩 시뮬레이션
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();
        progress_ = 50.0 + (elapsed / 1000.0 / duration_seconds) * 50.0; // 다운로드는 50-100%
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    if (duration_ms > 0) {
        result.download_mbps = (total_bytes * 8.0 / 1000000.0) / (duration_ms / 1000.0);
    }
    
    return result;
}

double BandwidthTest::measureLatency(const std::string& server) {
    // 간단한 ping 시뮬레이션
    auto start_time = std::chrono::steady_clock::now();
    
    // 실제 구현에서는 ICMP ping 또는 TCP 연결 시간 측정
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 시뮬레이션
    
    auto end_time = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000.0;
}

std::string BandwidthTest::generateTestData(size_t size_bytes) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::string data;
    data.reserve(size_bytes);
    
    for (size_t i = 0; i < size_bytes; ++i) {
        data.push_back(static_cast<char>(dis(gen)));
    }
    
    return data;
}

